#!/usr/bin/env python3
"""Run Pargus engine benchmarks and write structured JSON results."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, Iterable, List


ROOT_DIR = Path(__file__).resolve().parents[1]


def parse_csv_ints(value: str) -> List[int]:
    items = []
    for raw in value.split(","):
        raw = raw.strip()
        if raw:
            items.append(int(raw))
    if not items:
        raise argparse.ArgumentTypeError("expected at least one integer")
    return items


def read_report(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def run_engine(
    *,
    engine: Path,
    input_dir: Path,
    corpus: Path,
    out_dir: Path,
    threads: int,
    sim_threshold: float,
    ai_threshold: float,
    mode: str,
) -> Dict[str, Any]:
    command = [
        str(engine),
        "--input",
        str(input_dir),
        "--out-dir",
        str(out_dir),
        "--threads",
        str(threads),
        "--sim-threshold",
        f"{sim_threshold:.6f}",
        "--ai-threshold",
        f"{ai_threshold:.6f}",
        "--mode",
        mode,
        "--benchmark",
    ]
    if corpus.exists():
        command.extend(["--corpus", str(corpus)])

    started = time.perf_counter()
    completed = subprocess.run(
        command,
        cwd=ROOT_DIR,
        capture_output=True,
        text=True,
        check=False,
    )
    wall_ms = (time.perf_counter() - started) * 1000

    if completed.returncode != 0:
        raise RuntimeError(
            "engine run failed\n"
            f"command: {' '.join(command)}\n"
            f"stdout:\n{completed.stdout}\n"
            f"stderr:\n{completed.stderr}"
        )

    report_path = out_dir / "report.json"
    if not report_path.exists():
        raise RuntimeError(f"engine did not write {report_path}")

    report = read_report(report_path)
    stage_times = report.get("benchmark", {}).get("stage_times_ms", {})
    runtime_ms = float(stage_times.get("total") or wall_ms)
    return {
        "threads": threads,
        "mode": mode,
        "runtime_ms": runtime_ms,
        "wall_ms": wall_ms,
        "stage_times_ms": stage_times,
        "candidate_generation": report.get("benchmark", {}).get("candidate_generation", {}),
        "flagged_pairs": len(report.get("flagged_pairs", [])),
        "ai_flagged_docs": sum(1 for item in report.get("ai_scores", []) if item.get("flagged")),
        "stdout": completed.stdout,
        "stderr": completed.stderr,
        "out_dir": str(out_dir),
    }


def copy_latest(result_path: Path, latest_path: Path) -> None:
    latest_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(result_path, latest_path)


def ensure_engine(engine: Path) -> None:
    if not engine.exists():
        raise SystemExit(
            f"Engine not found at {engine}. Build it first:\n"
            "cmake -S engine -B engine/build -DCMAKE_BUILD_TYPE=Release\n"
            "cmake --build engine/build"
        )


def run_thread_suite(args: argparse.Namespace, run_dir: Path) -> List[Dict[str, Any]]:
    rows = []
    baseline = None
    for threads in args.threads:
        mode = "serial" if threads == 1 else args.mode
        out_dir = run_dir / f"threads_{threads}"
        out_dir.mkdir(parents=True, exist_ok=True)
        row = run_engine(
            engine=args.engine,
            input_dir=args.input,
            corpus=args.corpus,
            out_dir=out_dir,
            threads=threads,
            sim_threshold=args.sim_threshold,
            ai_threshold=args.ai_threshold,
            mode=mode,
        )
        if baseline is None:
            baseline = row["runtime_ms"] or 1.0
        row["speedup"] = baseline / row["runtime_ms"] if row["runtime_ms"] else 0.0
        rows.append(row)
        print(f"threads={threads} mode={mode} runtime_ms={row['runtime_ms']:.3f} speedup={row['speedup']:.3f}")
    return rows


def run_size_suite(args: argparse.Namespace, run_dir: Path) -> List[Dict[str, Any]]:
    if not args.sizes:
        return []

    generator = ROOT_DIR / "benchmarks" / "generate_dataset.py"
    rows = []
    for size in args.sizes:
        dataset_dir = run_dir / "generated" / f"docs_{size}"
        subprocess.run(
            [sys.executable, str(generator), "--num-docs", str(size), "--out", str(dataset_dir), "--clean"],
            cwd=ROOT_DIR,
            check=True,
        )
        out_dir = run_dir / f"size_{size}"
        row = run_engine(
            engine=args.engine,
            input_dir=dataset_dir,
            corpus=args.corpus,
            out_dir=out_dir,
            threads=args.size_threads,
            sim_threshold=args.sim_threshold,
            ai_threshold=args.ai_threshold,
            mode=args.mode,
        )
        row["docs"] = size
        rows.append(row)
        print(f"docs={size} threads={args.size_threads} runtime_ms={row['runtime_ms']:.3f}")
    return rows


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run Pargus engine benchmarks.")
    parser.add_argument("--engine", type=Path, default=Path("engine/build/Pargus"), help="Path to Pargus engine binary.")
    parser.add_argument("--input", type=Path, default=Path("data/sample"), help="Input document directory.")
    parser.add_argument("--corpus", type=Path, default=Path("data/sample_corpus.txt"), help="N-gram training corpus.")
    parser.add_argument("--results-dir", type=Path, default=Path("benchmarks/results"), help="Directory for benchmark JSON and outputs.")
    parser.add_argument("--threads", type=parse_csv_ints, default=[1, 2, 4, 8], help="Comma-separated thread counts.")
    parser.add_argument("--sizes", type=parse_csv_ints, default=[], help="Optional comma-separated generated corpus sizes.")
    parser.add_argument("--size-threads", type=int, default=8, help="Thread count used for --sizes runs.")
    parser.add_argument("--mode", choices=["openmp", "pthreads"], default="openmp", help="Parallel engine mode.")
    parser.add_argument("--sim-threshold", type=float, default=0.75, help="Similarity flag threshold.")
    parser.add_argument("--ai-threshold", type=float, default=50.0, help="AI-authorship flag threshold.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    args.engine = args.engine.resolve()
    args.input = args.input.resolve()
    args.corpus = args.corpus.resolve()
    args.results_dir.mkdir(parents=True, exist_ok=True)

    ensure_engine(args.engine)
    if not args.input.exists():
        raise SystemExit(f"Input directory not found: {args.input}")

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    run_dir = args.results_dir / f"run_{stamp}"
    run_dir.mkdir(parents=True, exist_ok=True)

    thread_results = run_thread_suite(args, run_dir)
    size_results = run_size_suite(args, run_dir)

    payload = {
        "created_at": stamp,
        "engine": str(args.engine),
        "input": str(args.input),
        "corpus": str(args.corpus),
        "thread_results": thread_results,
        "size_results": size_results,
    }

    result_path = args.results_dir / f"benchmark_{stamp}.json"
    result_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    copy_latest(result_path, args.results_dir / "benchmark_latest.json")
    print(f"Wrote {result_path}")


if __name__ == "__main__":
    main()

