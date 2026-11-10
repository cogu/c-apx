"""Integration tests for apx-node --check CLI option."""

import subprocess
from pathlib import Path

import pytest

from conftest import _set_pdeathsig


def test_apx_node_check_valid_file(apx_node_bin):
    """Test --check on a valid APX definition file."""
    repo_root = Path(__file__).resolve().parent.parent
    apx_file = repo_root / "example" / "nodes" / "unsigned_listener.apx"

    proc = subprocess.run(
        [apx_node_bin, "--check", str(apx_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert proc.returncode == 0
    assert "Initializing APX connection" not in proc.stdout
    assert "Closing APX connection" not in proc.stdout
    assert "Provide-Ports: 0, Require-Ports: 6" in proc.stdout


def test_apx_node_check_valid_sender_file(apx_node_bin):
    """Test --check on another valid APX definition file reporting provide ports."""
    repo_root = Path(__file__).resolve().parent.parent
    apx_file = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    proc = subprocess.run(
        [apx_node_bin, "--check", str(apx_file)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert proc.returncode == 0
    assert "Initializing APX connection" not in proc.stdout
    assert "Closing APX connection" not in proc.stdout
    assert "Provide-Ports: 6, Require-Ports: 0" in proc.stdout


def test_apx_node_check_compilation_error_reports_line(apx_node_bin, tmp_path):
    """Test --check reports line number and error on compilation failure."""
    bad_apx = tmp_path / "bad_compile.apx"
    bad_apx.write_text(
        "APX/1.2\n"
        'N"TestNode"\n'
        'P"BadPort"u:=1\n'
        'P"GoodPort"S:=0\n'
    )

    proc = subprocess.run(
        [apx_node_bin, "--check", str(bad_apx)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert proc.returncode == 1
    assert "Parsing" in proc.stdout and "Failed" in proc.stdout
    assert "Error on line 3: Element type error" in proc.stderr


def test_apx_node_check_parse_error_reports_line(apx_node_bin, tmp_path):
    """Test --check reports line number on parse error."""
    bad_apx = tmp_path / "bad_parse.apx"
    bad_apx.write_text(
        "APX/1.2\n"
        'N"TestNode"\n'
        'P"GoodPort"S:=0\n'
        'P"BadSyntax"$$$\n'
    )

    proc = subprocess.run(
        [apx_node_bin, "--check", str(bad_apx)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert proc.returncode == 1
    assert "Parsing" in proc.stdout and "Failed" in proc.stdout
    assert "Error on line 4: Parse error" in proc.stderr


def test_apx_node_check_init_value_range_error_reports_line(apx_node_bin, tmp_path):
    """Test --check reports line number on initial value out of range."""
    bad_apx = tmp_path / "bad_init.apx"
    bad_apx.write_text(
        "APX/1.2\n"
        'N"TestNode"\n'
        'P"CabTiltLockWarning"C(0,7):=15\n'
    )

    proc = subprocess.run(
        [apx_node_bin, "--check", str(bad_apx)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert proc.returncode == 1
    assert "Parsing" in proc.stdout and "Failed" in proc.stdout
    assert "Error on line 3: Value out of range" in proc.stderr


def test_apx_node_check_nonexistent_file(apx_node_bin):
    """Test --check fails when file does not exist."""
    proc = subprocess.run(
        [apx_node_bin, "--check", "/tmp/nonexistent_apx_file_12345.apx"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert proc.returncode == 1
    assert "Could not read file" in proc.stderr


def test_apx_node_check_in_help(apx_node_bin):
    """Test that --check is listed in --help usage output."""
    proc = subprocess.run(
        [apx_node_bin, "--help"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert proc.returncode == 0
    assert "--check" in proc.stdout

