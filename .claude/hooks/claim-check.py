#!/usr/bin/env python3
"""Claude Code hook: ask for evidence behind a done-claim (Stop event).

Mechanizes CLAUDE.md's "report outcomes faithfully" and the trust contract's
"self-review before handoff". A turn that EDITED files and then ends on a
completion claim ("done", "fixed", "verified", "all tests pass") without
having run any repo verification tool in between gets a warning naming the
matched phrase, the files touched, and the evidence that is missing.

Evidence is read from the transcript, not self-reported. Since the last human
prompt: was there a Bash/PowerShell tool call matching a verification command
(code-verify, claim-verify, sanitize-commit, documentation-verify,
layer-verify, registry-verify, pytest, ctest, --selftest, --benchmark-hotpath)
whose result was not an error? If yes, or if the turn made no edits, or if the
final message makes no claim, the hook stays silent.

Design choices specific to this repo:
  - Adapted from PrimeFoldTools/andon's claim_check_hook.py (MIT). That
    version accepts any self-written log line inside a time window; this one
    keys on tool calls the harness itself recorded, so nothing has to be
    logged and a stale entry cannot satisfy a fresh claim.
  - Edits are the trigger, not claims alone. A read-only analysis turn may
    legitimately say "confirmed"; an edit turn saying "done" without running
    the linter is the defect class (andon ledger 2026-04, "the false done").
    Shell writes count as edits through a heuristic (sed -i, tee, redirects,
    Set-Content, cp/mv, git apply); a miss there is a silent pass, never a
    false warning.
  - Warn only, never block: a block re-invokes the model, and canary-check.py
    already settled that a Stop hook here never spends tokens. The escape
    hatch is SS_CLAIM_CHECK=off.
  - Retry before deciding, same flush race as canary-check.py: the final
    assistant entry can land a few hundred ms after Stop fires.
  - Fail-open everywhere: any internal error exits 0 silently.
"""

import json
import os
import re
import sys
import time

RETRY_ATTEMPTS = 6
RETRY_DELAY_S = 0.4
MAX_LISTED = 5

EDIT_TOOLS = {"Edit", "Write", "MultiEdit", "NotebookEdit"}
SHELL_TOOLS = {"Bash", "PowerShell"}

VERIFY_RE = re.compile(
    r"code-verify\.py|claim-verify\.py|sanitize-commit\.py|documentation-verify\.py"
    r"|layer-verify\.py|registry-verify\.py|\bpytest\b|\bctest\b"
    r"|--selftest\b|--benchmark-hotpath\b"
)
SHELL_WRITE_RE = re.compile(
    r"\bsed\s+(?:-[a-zA-Z]*i|--in-place)\b|\btee\b|\bcp\s|\bmv\s|\bgit\s+apply\b"
    r"|\bpatch\s|\bSet-Content\b|\bOut-File\b|\bAdd-Content\b"
    r"|(?<![<>&|\d])>{1,2}\s*(?!&|/dev/null|\$null|nul\b)[\"'$\w./\\]"
    r"|\bopen\([^)]*[\"'][wa]b?[\"']"
)

_VERBS = r"done|complete|completed|fixed|verified|resolved|shipped|passing|green"
CLAIM_PATTERNS = (
    re.compile(
        r"(?im)(?:^|(?<=[.!?])\s+)\s*(?:done|complete|completed|fixed|verified|resolved"
        r"|shipped|all\s+set)\b\s*(?:[.!:;]|$)"
    ),
    re.compile(
        r"(?im)(?:^|(?<=[.!?])\s+)\s*(?:shipped|fixed|verified|resolved|completed"
        r"|implemented)\s+(?:the|this|that|these|those|all|both|every)\b[^\n.!?]{0,80}"
    ),
    re.compile(
        rf"\b(?:is|are|has\s+been|have\s+been|now|fully|already)"
        rf"(?:\s+(?:already|now|fully))?\s+(?:{_VERBS})\b",
        re.IGNORECASE,
    ),
    re.compile(r"\*\*(?:done|complete|fixed|verified|resolved|shipped)\*\*", re.IGNORECASE),
    re.compile(
        r"\b(?:all\s+)?(?:tests?|suite|lint(?:er)?|code-verify|claim-verify|ctest|pytest)"
        r"\s+(?:now\s+)?(?:pass(?:es|ed)?|green|clean)\b",
        re.IGNORECASE,
    ),
    re.compile(r"\bverified\s+end[\s-]to[\s-]end\b", re.IGNORECASE),
)
FENCE_RE = re.compile(r"```.*?```", re.DOTALL)


def load_entries(transcript_path: str) -> list[dict]:
    """Parse the JSONL transcript, skipping blank and malformed lines."""
    entries = []
    with open(transcript_path, encoding="utf-8") as handle:
        for raw in handle:
            raw = raw.strip()
            if not raw:
                continue
            try:
                entries.append(json.loads(raw))
            except ValueError:
                continue
    return entries


def blocks_of(entry: dict) -> list[dict]:
    """Return the content blocks of a transcript entry, or [] for string content."""
    content = (entry.get("message") or {}).get("content")
    if isinstance(content, list):
        return [block for block in content if isinstance(block, dict)]
    return []


def is_human_prompt(entry: dict) -> bool:
    """True for a typed user turn; tool results and injected meta turns are not."""
    if entry.get("type") != "user" or entry.get("isSidechain") or entry.get("isMeta"):
        return False
    content = (entry.get("message") or {}).get("content")
    if isinstance(content, str):
        return True
    return not any(block.get("type") == "tool_result" for block in blocks_of(entry))


def turn_entries(entries: list[dict]) -> list[dict]:
    """Slice the transcript from the last human prompt to the end."""
    for index in range(len(entries) - 1, -1, -1):
        if is_human_prompt(entries[index]):
            return entries[index:]
    return entries


def last_assistant_text(turn: list[dict]) -> str | None:
    """Return the visible text of the final main-thread assistant message."""
    for entry in reversed(turn):
        if entry.get("type") != "assistant" or entry.get("isSidechain"):
            continue
        content = (entry.get("message") or {}).get("content")
        if isinstance(content, str):
            text = content
        elif isinstance(content, list):
            text = "\n".join(
                block.get("text", "")
                for block in content
                if isinstance(block, dict) and block.get("type") == "text"
            )
        else:
            continue
        if text.strip():
            return text
    return None


def summarize_turn(turn: list[dict]) -> tuple[list[str], list[str], bool]:
    """Return (edited files, shell write commands, verification ran) for the turn."""
    errored = set()
    for entry in turn:
        if entry.get("type") != "user":
            continue
        for block in blocks_of(entry):
            if block.get("type") == "tool_result" and block.get("is_error"):
                errored.add(block.get("tool_use_id"))

    edited, shell_writes, verified = [], [], False
    for entry in turn:
        if entry.get("type") != "assistant" or entry.get("isSidechain"):
            continue
        for block in blocks_of(entry):
            if block.get("type") != "tool_use":
                continue
            name = block.get("name")
            inputs = block.get("input") or {}
            if name in EDIT_TOOLS:
                edited.append(str(inputs.get("file_path") or inputs.get("notebook_path") or "?"))
            elif name in SHELL_TOOLS:
                command = str(inputs.get("command") or "")
                if VERIFY_RE.search(command) and block.get("id") not in errored:
                    verified = True
                if SHELL_WRITE_RE.search(command):
                    shell_writes.append(command.strip().splitlines()[0][:80])
    return edited, shell_writes, verified


def _in_backticks(text: str, start: int, end: int) -> bool:
    open_tick = text.rfind("`", max(0, start - 200), start)
    if open_tick == -1:
        return False
    close_tick = text.find("`", end, min(len(text), end + 200))
    return close_tick != -1 and "`" not in text[open_tick + 1 : start]


def _in_blockquote(text: str, start: int) -> bool:
    line_start = text.rfind("\n", 0, start) + 1
    return text[line_start:start].lstrip().startswith(">")


def _ends_in_question(text: str, end: int) -> bool:
    for char in text[end : end + 160]:
        if char in ".!?\n":
            return char == "?"
    return False


def find_claims(text: str) -> list[str]:
    """Completion phrases in the assistant's own prose, minus quotes, code and questions."""
    text = FENCE_RE.sub("", text)
    found, seen = [], set()
    for pattern in CLAIM_PATTERNS:
        for match in pattern.finditer(text):
            phrase = " ".join(match.group(0).split())
            key = phrase.lower()
            if key in seen or _ends_in_question(text, match.end()):
                continue
            if _in_backticks(text, match.start(), match.end()):
                continue
            if _in_blockquote(text, match.start()):
                continue
            seen.add(key)
            found.append(phrase)
    return found[:MAX_LISTED]


def verdict(turn: list[dict]) -> str | None:
    """None when silent; otherwise the warning text."""
    text = last_assistant_text(turn)
    if text is None:
        return None
    claims = find_claims(text)
    if not claims:
        return None
    edited, shell_writes, verified = summarize_turn(turn)
    if verified or not (edited or shell_writes):
        return None

    touched = edited[:MAX_LISTED] + [f"shell: {cmd}" for cmd in shell_writes[:MAX_LISTED]]
    return (
        "CLAIM WITHOUT EVIDENCE: this turn edited files and ended on a completion "
        "claim, but no verification tool ran since the last prompt.\n"
        "Claimed:\n" + "\n".join(f'  - "{c}"' for c in claims) + "\n"
        "Touched:\n" + "\n".join(f"  - {t}" for t in touched) + "\n"
        "Run the check the claim rests on (code-verify.py --check on the touched "
        "files at minimum; pytest/ctest where one exists) and report its actual "
        "result, or scope the claim to what was verified by reading. "
        "Escape hatch: SS_CLAIM_CHECK=off."
    )


def check(transcript_path: str) -> str | None:
    """Read the transcript, waiting briefly for the final assistant entry to land."""
    turn: list[dict] = []
    for attempt in range(RETRY_ATTEMPTS):
        turn = turn_entries(load_entries(transcript_path))
        if last_assistant_text(turn) is not None:
            break
        if attempt < RETRY_ATTEMPTS - 1:
            time.sleep(RETRY_DELAY_S)
    return verdict(turn)


def main() -> None:
    try:
        if os.environ.get("SS_CLAIM_CHECK", "on").lower() == "off":
            sys.exit(0)
        event = json.load(sys.stdin)
        if event.get("stop_hook_active"):
            sys.exit(0)
        transcript_path = event.get("transcript_path")
        if transcript_path:
            message = check(str(transcript_path))
            if message is not None:
                print(json.dumps({"systemMessage": message}))
    except SystemExit:
        raise
    except Exception:
        pass
    sys.exit(0)


if __name__ == "__main__":
    main()
