# SPDX-FileCopyrightText: 2026 Alex Spataru <https://serial-studio.com/>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Tests for scripts/layer-verify.py (spec 0077 T68): every layer is strict, and the CMake side of
the graph is checked from the same LAYERS table the include rule uses."""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
REPO = SCRIPTS.parent


def _load():
    sys.path.insert(0, str(SCRIPTS))
    spec = importlib.util.spec_from_file_location(
        "layer_verify", SCRIPTS / "layer-verify.py"
    )
    module = importlib.util.module_from_spec(spec)
    sys.modules["layer_verify"] = module
    spec.loader.exec_module(module)
    return module


LV = _load()


def test_no_layer_is_ratcheted_any_more():
    """AC1: the debt machinery stays, but no layer sits on it."""
    assert LV.DEBT_LAYERS == ()


def test_the_checked_in_baseline_holds_no_edges():
    report = LV.Report()
    assert LV.load_baseline(report) == {}


def test_an_upward_include_in_a_partition_is_a_strict_error():
    """A scratch include from Pipeline into Ui fails with layer-upward, not as debt."""
    report = LV.Report()
    edges: dict = {}
    source = REPO / "core" / "Pipeline" / "DataModel" / "Scratch.cpp"
    resolved = REPO / "core" / "Ui" / "UI" / "Dashboard.h"
    LV._record_violation(
        report, edges, source, 7, "UI/Dashboard.h", resolved, "Pipeline"
    )
    assert report.has("layer-upward")
    assert edges == {}


def test_cmake_graph_entries_parse_roots_and_links():
    text = """
target_include_directories(
  SerialStudioStorage PUBLIC

  ${CMAKE_SOURCE_DIR}/core/Storage
  ${CMAKE_SOURCE_DIR}/core/Pipeline # trailing comment
)
target_link_libraries(SerialStudioStorage PUBLIC ${QT_LIBS} SerialStudio::Core mdf)
"""
    entries = LV.cmake_graph_entries(text)
    roots = [t for _, target, kind, t in entries if kind == "root"]
    links = [t for _, target, kind, t in entries if kind == "link"]
    assert roots == [
        "${CMAKE_SOURCE_DIR}/core/Storage",
        "${CMAKE_SOURCE_DIR}/core/Pipeline",
    ]
    assert links == ["${QT_LIBS}", "SerialStudio::Core", "mdf"]


def _fake_tree(tmp_path: Path, layer: str, body: str) -> Path:
    cml = tmp_path / "core" / layer / "CMakeLists.txt"
    cml.parent.mkdir(parents=True)
    cml.write_text(body, encoding="utf-8")
    return tmp_path


def test_a_root_outside_the_graph_fails(tmp_path):
    root = _fake_tree(
        tmp_path,
        "Pipeline",
        "target_include_directories(SerialStudioPipeline PUBLIC\n"
        "  ${CMAKE_SOURCE_DIR}/core/Pipeline\n  ${CMAKE_SOURCE_DIR}/core/Ui\n)\n",
    )
    report = LV.Report()
    LV.check_cmake_graph(report, root)
    assert report.has("cmake-root-violation")
    assert "core/Ui" in report.errors[0]["message"]


def test_an_app_src_root_fails_for_every_layer(tmp_path):
    root = _fake_tree(
        tmp_path,
        "Ui",
        "target_include_directories(SerialStudioUi PUBLIC ${CMAKE_SOURCE_DIR}/app/src)\n",
    )
    report = LV.Report()
    LV.check_cmake_graph(report, root)
    assert report.has("cmake-root-violation")


def test_a_sideways_link_fails(tmp_path):
    root = _fake_tree(
        tmp_path,
        "Devices",
        "target_link_libraries(SerialStudioDevices PUBLIC SerialStudio::Core SerialStudio::Storage)\n",
    )
    report = LV.Report()
    LV.check_cmake_graph(report, root)
    assert report.has("cmake-root-violation")
    assert "SerialStudio::Storage" in report.errors[0]["message"]


def test_the_transitive_closure_is_allowed(tmp_path):
    """Api links Storage, whose PUBLIC set exports Pipeline and Protocols; that is not a
    violation even though Api may not include Protocols directly."""
    root = _fake_tree(
        tmp_path,
        "Api",
        "target_include_directories(SerialStudioApi PUBLIC\n"
        "  ${CMAKE_SOURCE_DIR}/core/Api\n  ${CMAKE_SOURCE_DIR}/core/Storage\n)\n"
        "target_link_libraries(SerialStudioApi PUBLIC SerialStudio::Core SerialStudio::Protocols"
        " SerialStudio::Storage luajit)\n",
    )
    report = LV.Report()
    LV.check_cmake_graph(report, root)
    assert not report.errors


def test_the_root_list_may_not_link_partitions_to_each_other(tmp_path):
    root = tmp_path
    (root / "core").mkdir()
    (root / "core" / "CMakeLists.txt").write_text(
        "target_link_libraries(SerialStudioPipeline PUBLIC SerialStudio::Ui)\n",
        encoding="utf-8",
    )
    report = LV.Report()
    LV.check_cmake_graph(report, root)
    assert report.has("cmake-root-violation")


def test_the_repository_graph_is_clean():
    report = LV.Report()
    LV.check_cmake_graph(report)
    assert not report.errors, [e["message"] for e in report.errors]
