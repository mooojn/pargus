#!/usr/bin/env python3
"""Plot speedup and runtime graphs from Pargus benchmark JSON."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any, Dict, List

import matplotlib.pyplot as plt


def load_results(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def fit_amdahl(points: List[Dict[str, Any]]) -> float:
    if len(points) < 2:
        return 0.0

    best_fraction = 0.0
    best_error = math.inf
    for step in range(0, 1001):
        fraction = step / 1000
        error = 0.0
        for point in points:
            threads = max(1, int(point["threads"]))
            observed = float(point.get("speedup", 1.0))
            predicted = 1.0 / (fraction + ((1.0 - fraction) / threads))
            error += (observed - predicted) ** 2
        if error < best_error:
            best_error = error
            best_fraction = fraction
    return best_fraction


def plot_speedup(points: List[Dict[str, Any]], out_dir: Path) -> None:
    if not points:
        return

    threads = [int(point["threads"]) for point in points]
    speedups = [float(point.get("speedup", 0.0)) for point in points]
    serial_fraction = fit_amdahl(points)
    amdahl = [1.0 / (serial_fraction + ((1.0 - serial_fraction) / max(1, thread))) for thread in threads]

    plt.figure(figsize=(8, 5))
    plt.plot(threads, speedups, marker="o", label="Measured")
    plt.plot(threads, amdahl, linestyle="--", label=f"Amdahl fit f={serial_fraction:.3f}")
    plt.xlabel("Threads")
    plt.ylabel("Speedup")
    plt.title("Pargus Speedup vs Thread Count")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "speedup_vs_threads.png", dpi=160)
    plt.close()


def plot_runtime_by_size(points: List[Dict[str, Any]], out_dir: Path) -> None:
    if not points:
        return

    docs = [int(point["docs"]) for point in points]
    runtime = [float(point.get("runtime_ms", 0.0)) for point in points]

    plt.figure(figsize=(8, 5))
    plt.bar([str(item) for item in docs], runtime)
    plt.xlabel("Documents")
    plt.ylabel("Runtime (ms)")
    plt.title("Pargus Runtime vs Corpus Size")
    plt.tight_layout()
    plt.savefig(out_dir / "runtime_vs_corpus_size.png", dpi=160)
    plt.close()


def plot_stage_breakdown(points: List[Dict[str, Any]], out_dir: Path) -> None:
    if not points:
        return

    stages = ["io", "tokenize_tfidf", "minhash_lsh", "similarity", "perplexity", "write_outputs"]
    labels = [str(point.get("threads", point.get("docs", ""))) for point in points]
    bottoms = [0.0] * len(points)

    plt.figure(figsize=(9, 5))
    for stage in stages:
        values = [float(point.get("stage_times_ms", {}).get(stage, 0.0)) for point in points]
        plt.bar(labels, values, bottom=bottoms, label=stage)
        bottoms = [bottom + value for bottom, value in zip(bottoms, values)]

    plt.xlabel("Threads")
    plt.ylabel("Runtime (ms)")
    plt.title("Pargus Per-Stage Runtime Breakdown")
    plt.legend(fontsize=8)
    plt.tight_layout()
    plt.savefig(out_dir / "stage_breakdown.png", dpi=160)
    plt.close()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot Pargus benchmark graphs.")
    parser.add_argument("--results", type=Path, default=Path("benchmarks/results/benchmark_latest.json"), help="Benchmark JSON path.")
    parser.add_argument("--out-dir", type=Path, default=None, help="Output directory for PNG graphs.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    data = load_results(args.results)
    out_dir = args.out_dir or args.results.parent
    out_dir.mkdir(parents=True, exist_ok=True)

    thread_results = data.get("thread_results", [])
    size_results = data.get("size_results", [])

    plot_speedup(thread_results, out_dir)
    plot_stage_breakdown(thread_results, out_dir)
    plot_runtime_by_size(size_results, out_dir)
    print(f"Wrote benchmark graphs to {out_dir}")


if __name__ == "__main__":
    main()

