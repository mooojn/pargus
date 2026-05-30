import json
import os
import platform
import subprocess
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

try:
    from .config import (
        DEFAULT_CORPUS_PATH,
        DEFAULT_ENGINE_PATH,
        ENGINE_PATH_OVERRIDE,
        ENGINE_TIMEOUT_SECONDS,
        ROOT_DIR,
    )
except ImportError:  # Allows: cd api && uvicorn main:app
    from config import (
        DEFAULT_CORPUS_PATH,
        DEFAULT_ENGINE_PATH,
        ENGINE_PATH_OVERRIDE,
        ENGINE_TIMEOUT_SECONDS,
        ROOT_DIR,
    )


def resolve_engine_path() -> Path:
    if ENGINE_PATH_OVERRIDE:
        return Path(ENGINE_PATH_OVERRIDE).expanduser().resolve()

    configured = os.getenv("PARGUS_ENGINE_PATH")
    if configured:
        return Path(configured).expanduser().resolve()

    windows_path = DEFAULT_ENGINE_PATH.with_suffix(".exe")
    if platform.system().lower() == "windows" and windows_path.exists():
        return windows_path

    if DEFAULT_ENGINE_PATH.exists():
        return DEFAULT_ENGINE_PATH

    if windows_path.exists():
        return windows_path

    return DEFAULT_ENGINE_PATH


def run_engine(
    *,
    input_dir: Path,
    out_dir: Path,
    threads: int,
    sim_threshold: float,
    ai_threshold: float,
    mode: str,
    corpus_path: Optional[Path] = None,
    benchmark: bool = True,
) -> Dict[str, Any]:
    engine_path = resolve_engine_path()
    if not engine_path.exists():
        raise FileNotFoundError(
            f"Engine binary not found at {engine_path}. Build it with: "
            "cmake -S engine -B engine/build && cmake --build engine/build"
        )
    if platform.system().lower() == "windows" and engine_path.suffix.lower() != ".exe":
        raise RuntimeError(
            f"Engine binary at {engine_path} is not a Windows executable. "
            "Build the engine on Windows so it creates engine/build/Pargus.exe, "
            "or run the API from WSL/Linux with the Linux engine binary."
        )

    corpus = corpus_path or DEFAULT_CORPUS_PATH
    command: List[str] = [
        str(engine_path),
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
    ]
    if corpus.exists():
        command.extend(["--corpus", str(corpus)])
    if benchmark:
        command.append("--benchmark")

    started = time.perf_counter()
    completed = subprocess.run(
        command,
        cwd=str(ROOT_DIR),
        capture_output=True,
        text=True,
        timeout=ENGINE_TIMEOUT_SECONDS,
        check=False,
    )
    elapsed_ms = (time.perf_counter() - started) * 1000

    if completed.returncode != 0:
        stderr = completed.stderr.strip()
        stdout = completed.stdout.strip()
        detail = stderr or stdout or f"engine exited with code {completed.returncode}"
        raise RuntimeError(detail)

    report_path = out_dir / "report.json"
    if not report_path.exists():
        raise FileNotFoundError(f"Engine finished but did not write {report_path}")

    with report_path.open("r", encoding="utf-8") as handle:
        report = json.load(handle)

    report["_engine"] = {
        "elapsed_ms": elapsed_ms,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }
    return report


def normalize_report(job_id: str, report: Dict[str, Any]) -> Dict[str, Any]:
    benchmark = report.get("benchmark", {})
    stage_times = benchmark.get("stage_times_ms", {})
    runtime_ms = float(stage_times.get("total") or report.get("_engine", {}).get("elapsed_ms") or 0.0)
    docs = report.get("documents", [])

    flagged_pairs = []
    for pair in report.get("flagged_pairs", []):
        flagged_pairs.append(
            {
                **pair,
                "doc_a": pair.get("filename_a", pair.get("doc_a")),
                "doc_b": pair.get("filename_b", pair.get("doc_b")),
                "score": pair.get("combined", pair.get("score", 0.0)),
            }
        )

    return {
        "job_id": job_id,
        "num_docs": report.get("meta", {}).get("num_docs", len(docs)),
        "documents": docs,
        "similarity_matrix": report.get("similarity_matrix", []),
        "flagged_pairs": flagged_pairs,
        "ai_scores": report.get("ai_scores", []),
        "runtime_ms": runtime_ms,
        "speedup": report.get("benchmark", {}).get("speedup_vs_serial"),
        "meta": report.get("meta", {}),
        "benchmark": benchmark,
    }
