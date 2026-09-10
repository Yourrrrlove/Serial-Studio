#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Alex Spataru <https://serial-studio.com/>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Serial Studio layering gate for the `core/` static libraries (spec 0076).

`core/` holds the layered static libraries; `app/` holds the composition root
and the executable. The layering only survives if it is mechanical: nothing in
the build stops a `core/` translation unit from including `app/src/...` once the
include roots overlap, and nothing stops a moved header from staying behind in
the executable's `HEADERS` list, where it would be moc'ed twice.

Two include conventions live side by side. `Core` and `Protocols` are reached
through their directory name (`#include "Core/SSAssert.h"`, root = `core/`);
the five partition libraries carved out of `app/src` keep their ORIGINAL
relative paths, so each of `core/Pipeline`, `core/Devices`, `core/Storage`,
`core/Api`, `core/Ui` is an include root of its own and `#include
"Core/DataModel/Frame.h"` still resolves. That only stays unambiguous while no two
roots hold the same relative path, which is what `include-ambiguous` gates.

Every layer is STRICT (spec 0077): any include outside its allowed set is an
error. The ratchet of spec 0076 stays as machinery (`DEBT_LAYERS`, the edge
baseline `scripts/layer-baseline.json`) but no layer is on it any more, so the
baseline holds no edges; the graph in `LAYERS` below is the tree.

The CMake side is checked too: each `core/<Layer>/CMakeLists.txt` may name as
an include root only its own directory and the roots of layers it may include,
and may link only `SerialStudio::` targets of those layers; the closure is
transitive through the allowed layers' own graphs, which is what a PUBLIC link
exports. `core/CMakeLists.txt` may not link the partitions to each other.

Checks:

    include-unresolved   a quoted `#include` that resolves against none of the
                         include roots                                  (error)
    include-ambiguous    a quoted `#include` that resolves to more than one
                         file across the include roots                  (error)
    layer-upward         a STRICT layer includes something outside its own
                         layer and its allowed lower layers             (error)
    layer-debt-growth    a ratcheted edge carries more includes than the
                         checked-in baseline allows (no layer is ratcheted
                         since spec 0077; kept for the machinery)        (error)
    cmake-root-violation a core/<Layer>/CMakeLists.txt names an include root
                         or a SerialStudio:: link outside the layer's
                         graph, or the root list links partitions          (error)
    core-unowned         a source under core/ listed in zero or in more than
                         one core/**/CMakeLists.txt                     (error)
    pair-split           a .cpp and its sibling .h listed in different
                         CMakeLists (AUTOMOC would moc the header twice) (error)
    cmake-missing        a CMake source entry naming a file not on disk  (error)
    moc-double-listed    a core/ source still listed in app/CMakeLists.txt
                                                                        (error)

Angle includes are system/Qt and are never checked. Build-generated headers are
allowlisted by pattern in `GENERATED_INCLUDES` below.

Usage:
    python3 scripts/layer-verify.py             # plain-text findings
    python3 scripts/layer-verify.py --verbose   # plus the per-edge debt table
    python3 scripts/layer-verify.py --json      # machine-readable findings
    python3 scripts/layer-verify.py --accept    # re-seed scripts/layer-baseline.json

Exit codes:
    0 - clean
    1 - errors found
    2 - argument error
"""

from __future__ import annotations

import argparse
import datetime
import fnmatch
import functools
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# The intended dependency graph: what each layer is allowed to reach down into.
# A layer always sees itself; everything else is an upward or sideways include.
# Core and Protocols are the finished layers and are held to this strictly; the
# five partition layers carry their violations as ratcheted debt instead.
LAYERS = {
    "Core": (),
    "Protocols": ("Core",),
    "Pipeline": ("Core", "Protocols"),
    "Devices": ("Core", "Protocols"),
    "Storage": ("Core", "Protocols", "Pipeline"),
    "Api": ("Core", "Pipeline", "Devices", "Storage"),
    "Ui": ("Core", "Pipeline", "Devices", "Storage", "Api"),
    "App": ("Core", "Protocols", "Pipeline", "Devices", "Storage", "Api", "Ui"),
    "Tests": (
        "Core",
        "Protocols",
        "Pipeline",
        "Devices",
        "Storage",
        "Api",
        "Ui",
        "App",
    ),
}

# Layers whose out-of-graph includes are ratcheted debt rather than errors. Empty since spec 0077:
# every layer is strict, and the tuple stays so a future carve-out can ratchet its first days.
DEBT_LAYERS: tuple[str, ...] = ()

# The library target each core/ layer builds, as named in its CMakeLists.txt.
LAYER_TARGETS = {
    layer: f"SerialStudio{layer}" for layer in LAYERS if layer not in ("App", "Tests")
}

# Layers reached through their own directory name, from the core/ root.
PREFIXED_LAYERS = ("Core", "Protocols")

CORE_ROOT = "core"
APP_ROOTS = ("app/src", "app/tests")
BASELINE = REPO_ROOT / "scripts" / "layer-baseline.json"

# Include roots, tried after the includer's own directory, in this order. The
# core/ entry only ever answers a Core/ or Protocols/ prefixed include.
INCLUDE_ROOTS = (
    "core",
    "core/Pipeline",
    "core/Devices",
    "core/Storage",
    "core/Api",
    "core/Ui",
    "app/src",
    "app/tests",
)

# CMake variables expanded before a source entry is checked against disk. Each
# value is relative to the repository root; ${CMAKE_CURRENT_SOURCE_DIR} instead
# resolves against the listing file's own directory and is handled separately.
CMAKE_VARS = {
    "${SS_APP_SRC}": "app/src",
    "${SS_CORE_SRC}": "core",
    "${CMAKE_SOURCE_DIR}": ".",
}
CMAKE_CURRENT_DIR = "${CMAKE_CURRENT_SOURCE_DIR}"

SOURCE_SUFFIXES = (".h", ".cpp", ".mm", ".c")
SKIP_DIRS = frozenset({"ThirdParty", "build", "__pycache__", ".git", "node_modules"})

# Build-generated headers: they exist only in the build tree, so no on-disk
# resolution can succeed. Each pattern names where it is generated.
GENERATED_INCLUDES = (
    # Qt AUTOMOC, emitted into the target's build dir (every app/tests/tst_*.cpp)
    "*.moc",
    # protoc / grpc_cpp_plugin custom command, app/CMakeLists.txt:1541-1570
    "*.pb.h",
    # cmake/GenLicenseGuards.cmake writes ${CMAKE_BINARY_DIR}/generated/
    "LicenseGuards.generated.h",
)

_INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
_CMAKE_TOKEN_RE = re.compile(r"[A-Za-z0-9_./+${}-]+\.(?:h|cpp|c|mm)\b")
_CMAKE_CALL_RE = re.compile(
    r"target_(include_directories|link_libraries)\s*\(\s*([^\s()]+)([^()]*)\)", re.S
)
_CMAKE_CORE_ROOT_RE = re.compile(r"^\$\{CMAKE_SOURCE_DIR\}/core/([A-Za-z]+)/?$")
_CMAKE_APP_ROOT_RE = re.compile(r"^\$\{CMAKE_SOURCE_DIR\}/app/src/?$")
_CMAKE_LINK_RE = re.compile(r"^SerialStudio(?:::)?([A-Za-z]+)$")
_CMAKE_SIBLING_ROOT_RE = re.compile(
    r"^\$\{CMAKE_CURRENT_SOURCE_DIR\}/\.\./([A-Za-z]+)/?$"
)


# ---------------------------------------------------------------------------
# Findings
# ---------------------------------------------------------------------------


class Report:
    """Collects `rule/path/line/message` findings and renders them."""

    def __init__(self) -> None:
        self.errors: list[dict] = []
        self.notes: list[str] = []

    def add(self, rule, path, line, message, details=None) -> None:
        rel = path if isinstance(path, str) else _rel(path)
        entry = {"rule": rule, "path": rel, "line": line, "message": message}
        if details:
            entry["details"] = list(details)
        self.errors.append(entry)

    def note(self, message: str) -> None:
        self.notes.append(message)

    def counts(self) -> dict:
        out: dict = {}
        for err in self.errors:
            out[err["rule"]] = out.get(err["rule"], 0) + 1
        return out

    def has(self, *rules: str) -> bool:
        """True when any finding carries one of `rules`."""
        return any(err["rule"] in rules for err in self.errors)


def _rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------


def iter_sources(root: Path):
    """Yields every C/C++ source under `root`, skipping vendored/build dirs."""
    if not root.is_dir():
        return
    for path in sorted(root.rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES or not path.is_file():
            continue
        if SKIP_DIRS.intersection(path.relative_to(root).parts):
            continue
        yield path


def scan_targets() -> list[Path]:
    """Returns every source file subject to the include checks."""
    roots = [REPO_ROOT / CORE_ROOT] + [REPO_ROOT / r for r in APP_ROOTS]
    files: list[Path] = []
    for root in roots:
        files.extend(iter_sources(root))
    return files


def read_lines(path: Path) -> list[str]:
    """Reads `path` as UTF-8 text, tolerating stray bytes."""
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


# ---------------------------------------------------------------------------
# Include resolution
# ---------------------------------------------------------------------------


@functools.lru_cache(maxsize=None)
def _is_file(path: Path) -> bool:
    """Cached `is_file`; every include retries the same handful of roots."""
    return path.is_file()


def is_generated(target: str) -> bool:
    """True when the include names a build-time generated header."""
    name = target.rsplit("/", 1)[-1]
    return any(fnmatch.fnmatch(name, pat) for pat in GENERATED_INCLUDES)


def _root_candidate(root: str, target: str) -> Path | None:
    """Maps an include target onto one include root, honouring the prefixes."""
    if root == CORE_ROOT:
        head = target.split("/", 1)[0]
        if head not in PREFIXED_LAYERS:
            return None
    return REPO_ROOT / root / target


def resolve_include(includer: Path, target: str) -> list[Path]:
    """Returns every distinct file a quoted include resolves to, best first."""
    candidates = [includer.parent / target]
    for root in INCLUDE_ROOTS:
        cand = _root_candidate(root, target)
        if cand is not None:
            candidates.append(cand)
    found: list[Path] = []
    for cand in candidates:
        if not _is_file(cand):
            continue
        real = cand.resolve()
        if real not in found:
            found.append(real)
    return found


def iter_includes(path: Path):
    """Yields `(line_number, include_target)` for every quoted include."""
    for number, text in enumerate(read_lines(path), start=1):
        match = _INCLUDE_RE.match(text)
        if match:
            yield number, match.group(1)


def layer_of(path: Path) -> str | None:
    """Returns the layer owning `path`: a core/ layer, App, Tests or None.

    A source sitting directly under core/ (no layer directory) gets the pseudo-layer "<root>"
    so it is still held to the downward-only rule rather than escaping it.
    """
    rel = _rel(path)
    parts = rel.split("/")
    if parts[0] == CORE_ROOT:
        return parts[1] if len(parts) > 2 else "<root>"
    if rel.startswith("app/tests/"):
        return "Tests"
    if rel.startswith("app/src/"):
        return "App"
    return None


# ---------------------------------------------------------------------------
# Include checks
# ---------------------------------------------------------------------------


def _allowed_layers(layer: str) -> set[str]:
    """The layers `layer` may include from, itself included."""
    return {layer} | set(LAYERS.get(layer, ()))


def _closure_layers(layer: str) -> set[str]:
    """The layers a PUBLIC link may legitimately expose to `layer`: its allowed layers and,
    transitively, theirs. Protocols reaches Api through Pipeline this way without Api being
    allowed to include it directly."""
    seen: set[str] = set()
    pending = [layer]
    while pending:
        current = pending.pop()
        if current in seen:
            continue
        seen.add(current)
        pending.extend(LAYERS.get(current, ()))
    return seen


def _record_violation(report, edges, source, number, target, resolved, layer):
    """Files one out-of-graph include as a strict error or as ratcheted debt."""
    reached = layer_of(resolved) or "Unknown"
    message = (
        f'core/{layer} may not include "{target}" ({_rel(resolved)}); '
        f"allowed layers: {', '.join(sorted(_allowed_layers(layer)))}"
    )
    if layer not in DEBT_LAYERS:
        report.add("layer-upward", source, number, message)
        return
    entry = {
        "path": _rel(source),
        "line": number,
        "message": f'#include "{target}" -> {_rel(resolved)}',
    }
    edges.setdefault(f"{layer}->{reached}", []).append(entry)


def _check_one_include(report, edges, source, layer, number, target):
    """Resolves one include and files whatever it violates."""
    resolved = resolve_include(source, target)
    if not resolved:
        report.add(
            "include-unresolved",
            source,
            number,
            f'#include "{target}" resolves against no include root',
        )
        return
    if len(resolved) > 1:
        report.add(
            "include-ambiguous",
            source,
            number,
            f'#include "{target}" resolves in {len(resolved)} include roots',
            details=[_rel(p) for p in resolved],
        )
    if layer is None:
        return
    if layer_of(resolved[0]) not in _allowed_layers(layer):
        _record_violation(report, edges, source, number, target, resolved[0], layer)


def check_includes(report: Report) -> dict:
    """Runs the include checks; returns the per-edge debt found in the tree."""
    edges: dict = {}
    for source in scan_targets():
        layer = layer_of(source)
        for number, target in iter_includes(source):
            if is_generated(target):
                continue
            _check_one_include(report, edges, source, layer, number, target)
    return edges


# ---------------------------------------------------------------------------
# Debt ratchet
# ---------------------------------------------------------------------------


def load_baseline(report: Report) -> dict:
    """Reads the checked-in edge baseline; a missing file counts as empty."""
    if not BASELINE.is_file():
        report.note(f"{_rel(BASELINE)} is missing; every debt edge counts as growth")
        return {}
    data = json.loads(BASELINE.read_text(encoding="utf-8"))
    return dict(data.get("edges", {}))


def write_baseline(edges: dict) -> None:
    """Rewrites the edge baseline from the current tree."""
    payload = {
        "purpose": (
            "Out-of-graph includes each ratcheted core/ layer carries today. Empty since "
            "spec 0077: every layer is strict, so an out-of-graph include is an error, not "
            "debt. scripts/layer-verify.py fails when an edge grows past its count."
        ),
        "regenerate": "python3 scripts/layer-verify.py --accept",
        "generated": datetime.date.today().isoformat(),
        "edges": {edge: len(items) for edge, items in sorted(edges.items())},
    }
    BASELINE.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def check_debt(report: Report, edges: dict, baseline: dict) -> None:
    """Compares each directed edge against its baseline count."""
    for edge in sorted(set(edges) | set(baseline)):
        items = edges.get(edge, [])
        allowed = baseline.get(edge, 0)
        if len(items) > allowed:
            report.add(
                "layer-debt-growth",
                _rel(BASELINE),
                0,
                f"{edge}: {len(items)} include(s), baseline {allowed}",
                details=[f"{i['path']}:{i['line']}: {i['message']}" for i in items],
            )
        elif len(items) < allowed:
            report.note(
                f"{edge}: {len(items)} include(s), baseline {allowed} "
                f"-- re-seed with --accept"
            )


# ---------------------------------------------------------------------------
# CMake parsing
# ---------------------------------------------------------------------------


def strip_cmake_comments(text: str) -> str:
    """Drops `#` comments so commented-out source entries are not parsed."""
    out = []
    for line in text.splitlines():
        hash_at = line.find("#")
        out.append(line if hash_at < 0 else line[:hash_at])
    return "\n".join(out)


def cmake_tokens(path: Path) -> list[tuple[int, str]]:
    """Returns `(line_number, token)` for every source-file token in `path`."""
    tokens: list[tuple[int, str]] = []
    text = strip_cmake_comments(path.read_text(encoding="utf-8", errors="replace"))
    for number, line in enumerate(text.splitlines(), start=1):
        for match in _CMAKE_TOKEN_RE.finditer(line):
            tokens.append((number, match.group(0)))
    return tokens


def expand_token(cml: Path, token: str) -> Path | None:
    """Resolves a CMake source token to a path, or None when a var is unknown."""
    if token.startswith(CMAKE_CURRENT_DIR):
        return (cml.parent / token[len(CMAKE_CURRENT_DIR) :].lstrip("/")).resolve()
    for name, replacement in CMAKE_VARS.items():
        if token.startswith(name):
            tail = token[len(name) :].lstrip("/")
            return (REPO_ROOT / replacement / tail).resolve()
    if "${" in token:
        return None
    return (cml.parent / token).resolve()


def core_cmakelists() -> list[Path]:
    """Every CMakeLists.txt under core/, deepest paths last."""
    root = REPO_ROOT / CORE_ROOT
    if not root.is_dir():
        return []
    return sorted(root.rglob("CMakeLists.txt"))


def core_owners(report: Report) -> dict:
    """Maps each core/ file to the CMakeLists that list it, flagging misses."""
    owners: dict = {}
    for cml in core_cmakelists():
        for number, token in cmake_tokens(cml):
            resolved = expand_token(cml, token)
            if resolved is None:
                continue
            listed_in = owners.setdefault(resolved, [])
            if cml not in listed_in:
                listed_in.append(cml)
            if not resolved.is_file():
                report.add(
                    "cmake-missing",
                    cml,
                    number,
                    f"{token} does not exist ({_rel(resolved)})",
                )
    return owners


def check_core_ownership(report: Report) -> None:
    """Every .h/.cpp under core/ belongs to exactly one core target."""
    roots = core_cmakelists()
    if not roots:
        report.add(
            "core-unowned",
            f"{CORE_ROOT}/CMakeLists.txt",
            0,
            "no CMakeLists.txt found under core/; nothing owns the layer sources",
        )
        return
    owners = core_owners(report)
    for source in iter_sources(REPO_ROOT / CORE_ROOT):
        listed = owners.get(source.resolve(), [])
        if not listed:
            report.add("core-unowned", source, 0, "listed in no core/**/CMakeLists.txt")
        elif len(listed) > 1:
            names = ", ".join(_rel(p) for p in listed)
            report.add("core-unowned", source, 0, f"listed in {len(listed)}: {names}")
    check_pair_split(report, owners)


def check_pair_split(report: Report, owners: dict) -> None:
    """A .cpp and its sibling .h must be listed in the same CMakeLists.

    AUTOMOC moc's the sibling header of every listed .cpp, so listing that header from another
    target moc's it twice and the link fails on duplicate staticMetaObject symbols.
    """
    for source in iter_sources(REPO_ROOT / CORE_ROOT):
        if source.suffix not in (".cpp", ".mm"):
            continue
        header = source.with_suffix(".h")
        if not header.is_file():
            continue
        owns_cpp = owners.get(source.resolve(), [])
        owns_hdr = owners.get(header.resolve(), [])
        if not owns_cpp or not owns_hdr or owns_cpp == owns_hdr:
            continue
        report.add(
            "pair-split",
            source,
            0,
            f"listed in {_rel(owns_cpp[0])} but {header.name} is listed in "
            f"{_rel(owns_hdr[0])}; AUTOMOC would moc the header twice",
        )


def cmake_graph_entries(text: str) -> list[tuple[int, str, str, str]]:
    """Returns `(line, target, kind, token)` for every root or link a CMake text names.

    `kind` is "root" for a target_include_directories() entry and "link" for a
    target_link_libraries() entry; the line is where the call starts. A link may name the
    alias (`SerialStudio::Ui`) or the target (`SerialStudioUi`); a root may be spelled from
    `${CMAKE_SOURCE_DIR}/core/<Layer>` or `${CMAKE_CURRENT_SOURCE_DIR}/../<Layer>`.
    """
    text = strip_cmake_comments(text)
    entries: list[tuple[int, str, str, str]] = []
    for match in _CMAKE_CALL_RE.finditer(text):
        line = text.count("\n", 0, match.start()) + 1
        kind = "root" if match.group(1) == "include_directories" else "link"
        target = match.group(2)
        for token in match.group(3).split():
            if token in ("PUBLIC", "PRIVATE", "INTERFACE"):
                continue
            entries.append((line, target, kind, token))
    return entries


def _graph_token_layer(kind: str, token: str) -> str | None:
    """Maps a root or link token onto a layer name; None for Qt, third-party and variables."""
    if kind == "root":
        if _CMAKE_APP_ROOT_RE.match(token):
            return "App"
        match = _CMAKE_CORE_ROOT_RE.match(token) or _CMAKE_SIBLING_ROOT_RE.match(token)
        return match.group(1) if match else None
    match = _CMAKE_LINK_RE.match(token)
    return match.group(1) if match else None


def check_cmake_graph(report: Report, root: Path | None = None) -> None:
    """Every core/<Layer>/CMakeLists.txt names only roots and links inside the layer's graph."""
    base = (root or REPO_ROOT) / CORE_ROOT
    targets = {name: layer for layer, name in LAYER_TARGETS.items()}
    for layer in LAYER_TARGETS:
        cml = base / layer / "CMakeLists.txt"
        if not cml.is_file():
            continue
        for line, target, kind, token in cmake_graph_entries(read_text(cml)):
            owner = targets.get(target)
            if owner is None:
                continue
            allowed = _closure_layers(owner)
            reached = _graph_token_layer(kind, token)
            if reached is None or reached in allowed:
                continue
            report.add(
                "cmake-root-violation",
                cml,
                line,
                f"{target} names {token} as a {kind}; allowed layers: "
                f"{', '.join(sorted(allowed))}",
            )
    root_cml = base / "CMakeLists.txt"
    if not root_cml.is_file():
        return
    for line, target, kind, token in cmake_graph_entries(read_text(root_cml)):
        if kind != "link":
            continue
        layer = targets.get(target)
        reached = _graph_token_layer(kind, token)
        if layer is None or (
            reached is not None and reached not in _closure_layers(layer)
        ):
            report.add(
                "cmake-root-violation",
                root_cml,
                line,
                f"{target} links {token} from the root list; each partition links only its "
                "own CMakeLists.txt's downward set",
            )


def read_text(path: Path) -> str:
    """Reads `path` as UTF-8 text, tolerating stray bytes."""
    return path.read_text(encoding="utf-8", errors="replace")


def check_app_cmake(report: Report) -> None:
    """App-side CMake entries must exist, and must never name a core/ source."""
    app_cml = REPO_ROOT / "app" / "CMakeLists.txt"
    if app_cml.is_file():
        for number, token in cmake_tokens(app_cml):
            _check_app_token(report, app_cml, number, token)

    tests_cml = REPO_ROOT / "app" / "tests" / "CMakeLists.txt"
    if not tests_cml.is_file():
        return
    for number, token in cmake_tokens(tests_cml):
        resolved = expand_token(tests_cml, token)
        if resolved is not None and not resolved.is_file():
            report.add("cmake-missing", tests_cml, number, f"{token} does not exist")


def _check_app_token(report: Report, cml: Path, number: int, token: str) -> None:
    """Flags a missing src/ entry, or a core/ source listed by the executable."""
    if token.startswith("src/") and not (cml.parent / token).is_file():
        report.add("cmake-missing", cml, number, f"{token} does not exist")
    if _names_core_source(token):
        report.add(
            "moc-double-listed",
            cml,
            number,
            f"{token} lives under core/; listing it here moc's it twice",
        )


def _names_core_source(token: str) -> bool:
    """True when a CMake token spells out a source that lives under core/."""
    if f"{CORE_ROOT}/" not in token:
        return False
    tail = token.split(f"{CORE_ROOT}/", 1)[1]
    return (REPO_ROOT / CORE_ROOT / tail).is_file()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def render(report: Report, edges: dict, args) -> None:
    """Prints the findings, the notes and the per-rule summary."""
    counts = report.counts()
    if args.json:
        payload = {
            "errors": report.errors,
            "notes": report.notes,
            "edges": {edge: len(items) for edge, items in sorted(edges.items())},
            "summary": {"total": len(report.errors), "rules": counts},
        }
        print(json.dumps(payload, indent=2))
        return
    for err in sorted(report.errors, key=lambda e: (e["rule"], e["path"], e["line"])):
        print(f"{err['path']}:{err['line']}: {err['rule']}: {err['message']}")
        for detail in err.get("details", []):
            print(f"    {detail}")
    for note in report.notes:
        print(f"layer-verify: note: {note}")
    if args.verbose:
        render_edges(edges)
    rules = ", ".join(f"{k} {v}" for k, v in sorted(counts.items())) or "none"
    print(f"layer-verify: {len(report.errors)} error(s) [{rules}]")


def render_edges(edges: dict) -> None:
    """Advisory listing of every ratcheted edge and the includes behind it."""
    print("layer-verify: ratcheted edges (advisory)")
    for edge, items in sorted(edges.items()):
        print(f"  {edge}: {len(items)}")
        for item in items:
            print(f"    {item['path']}:{item['line']}: {item['message']}")


def main(argv: list[str]) -> int:
    """Runs every layering check and returns the process exit code."""
    parser = argparse.ArgumentParser(
        description="Verify the core/ layering, ownership and CMake entries."
    )
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    parser.add_argument("--verbose", action="store_true", help="per-edge debt listing")
    parser.add_argument(
        "--accept", action="store_true", help="re-seed scripts/layer-baseline.json"
    )
    args = parser.parse_args(argv)

    report = Report()
    edges = check_includes(report)
    if args.accept:
        return accept(report, edges)

    check_debt(report, edges, load_baseline(report))
    check_core_ownership(report)
    check_cmake_graph(report)
    check_app_cmake(report)
    render(report, edges, args)
    return 1 if report.errors else 0


def accept(report: Report, edges: dict) -> int:
    """Rewrites the baseline, unless the include census is untrustworthy."""
    blockers = (
        "include-unresolved",
        "include-ambiguous",
        "layer-upward",
        "cmake-root-violation",
    )
    check_cmake_graph(report)
    if report.has(*blockers):
        for err in report.errors:
            if err["rule"] in blockers:
                print(f"{err['path']}:{err['line']}: {err['rule']}: {err['message']}")
        print("layer-verify: --accept refused while strict include errors stand")
        return 1
    write_baseline(edges)
    total = sum(len(items) for items in edges.values())
    print(f"layer-verify: baseline seeded, {len(edges)} edge(s), {total} include(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
