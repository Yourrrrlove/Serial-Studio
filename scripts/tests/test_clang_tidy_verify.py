# SPDX-FileCopyrightText: 2020-2026 Alex Spataru
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-SerialStudio-Commercial
"""
Skip-path and report-shape tests for scripts/clang-tidy-verify.py.

The script is advisory and must never turn a missing tool or build directory into a failed
sanitize run, so the skip path is what these tests pin: no clang-tidy, or no
compile_commands.json, exits 0 with one line and writes no report, and only --strict turns that
into exit 2. The parser and report writer run on canned clang-tidy output, so the .tidy-report
shape stays aligned with .code-report on a machine that has no compile database.

Run: pytest scripts/tests/test_clang_tidy_verify.py
"""

import importlib.util
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SCRIPT = REPO / "scripts" / "clang-tidy-verify.py"
SOURCE = "core/Ui/UI/SnapGuides.cpp"


def _load_module():
    """Import the script under a legal module name; dataclasses resolve the deferred annotations
    through sys.modules, so the module must be registered before it executes."""
    spec = importlib.util.spec_from_file_location("clang_tidy_verify", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    sys.modules["clang_tidy_verify"] = module
    spec.loader.exec_module(module)
    return module


def _run(*args, cwd=REPO):
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=cwd,
        capture_output=True,
        text=True,
        check=False,
    )


def test_missing_clang_tidy_skips_with_exit_zero(tmp_path):
    report = tmp_path / ".tidy-report"
    result = _run(
        SOURCE,
        "--clang-tidy",
        str(tmp_path / "no-such-clang-tidy"),
        "--build-dir",
        str(tmp_path),
        "--report",
        str(report),
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "skipping" in result.stdout
    assert not report.exists()


def test_missing_compile_db_skips_and_strict_reports_two(tmp_path):
    report = tmp_path / ".tidy-report"
    common = (
        SOURCE,
        "--clang-tidy",
        sys.executable,
        "--build-dir",
        str(tmp_path),
        "--report",
        str(report),
    )
    relaxed = _run(*common)
    assert relaxed.returncode == 0, relaxed.stdout + relaxed.stderr
    assert "compile_commands.json" in relaxed.stdout
    assert not report.exists()

    strict = _run(*common, "--strict")
    assert strict.returncode == 2, strict.stdout + strict.stderr


def test_parse_keeps_first_party_findings_and_dedupes(tmp_path):
    module = _load_module()
    first_party = REPO / SOURCE
    qt_header = Path("C:/Developer/Qt/6.11.0/msvc2022_64/include/QtCore/qnumeric.h")
    output = "\n".join(
        [
            f"{first_party}:36:15: warning: do not declare C-style arrays [modernize-avoid-c-arrays]",
            f"{first_party}:36:15: warning: do not declare C-style arrays [modernize-avoid-c-arrays]",
            f"{qt_header}:120:5: warning: unreachable code [clang-diagnostic-unreachable-code]",
            "3 warnings generated.",
        ]
    )
    findings = module.parse_findings(output)
    assert [(f.path, f.line, f.check) for f in findings] == [
        (SOURCE, 36, "modernize-avoid-c-arrays")
    ]

    report = tmp_path / ".tidy-report"
    module.write_report(report, findings)
    text = report.read_text(encoding="utf-8")
    assert "## Advisories (1)" in text
    assert "### `modernize-avoid-c-arrays` (1)" in text
    assert f"- `{SOURCE}:36`" in text

    module.write_report(report, [])
    assert not report.exists()
