#!/usr/bin/env python3
import argparse
import difflib
import pathlib
import re
import subprocess
import sys


def normalize_line(line: str) -> str:
    line = line.rstrip()
    line = re.sub(r"\s+", " ", line)
    return line


def read_cases(path: pathlib.Path):
    cases = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        label, rel = line.split("|", 1)
        cases.append((label.strip(), rel.strip()))
    return cases


def run_case(exe: pathlib.Path, cfg: pathlib.Path) -> str:
    proc = subprocess.run(
        [str(exe), str(cfg)],
        capture_output=True,
        text=True,
        check=False,
    )
    return (proc.stdout or "") + (proc.stderr or "")


def compare_to_golden(label: str, output: str, golden_dir: pathlib.Path):
    golden_file = golden_dir / f"{label}.golden.txt"
    if not golden_file.exists():
        golden_file.write_text(output, encoding="utf-8")
        return True, f"[INIT] Created golden: {golden_file}"

    golden = golden_file.read_text(encoding="utf-8")
    out_lines = [normalize_line(x) for x in output.splitlines()]
    gold_lines = [normalize_line(x) for x in golden.splitlines()]
    if out_lines == gold_lines:
        return True, f"[PASS] {label}"

    diff = "\n".join(
        difflib.unified_diff(gold_lines, out_lines, fromfile="golden", tofile="current", n=2)
    )
    return False, f"[FAIL] {label}\n{diff}"


def main():
    parser = argparse.ArgumentParser(description="BookSim2 compatibility regression runner")
    parser.add_argument("--exe", required=True, type=pathlib.Path)
    parser.add_argument("--repo-root", required=True, type=pathlib.Path)
    parser.add_argument("--cases", default="tests/baseline_cases.txt")
    parser.add_argument("--golden-dir", default="tests/golden")
    args = parser.parse_args()

    cases = read_cases(args.repo_root / args.cases)
    golden_dir = args.repo_root / args.golden_dir
    golden_dir.mkdir(parents=True, exist_ok=True)

    ok = True
    for label, rel_path in cases:
        cfg = args.repo_root / rel_path
        output = run_case(args.exe, cfg)
        passed, msg = compare_to_golden(label, output, golden_dir)
        print(msg)
        ok = ok and passed

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
