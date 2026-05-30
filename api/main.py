import json
import shutil
import time
from pathlib import Path
from typing import Dict, List
from uuid import uuid4

from fastapi import BackgroundTasks, FastAPI, File, Form, HTTPException, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse, HTMLResponse

try:
    from .config import (
        ALLOWED_MODES,
        DEFAULT_AI_THRESHOLD,
        DEFAULT_BENCHMARK_INPUT,
        DEFAULT_MODE,
        DEFAULT_SIM_THRESHOLD,
        DEFAULT_THREADS,
        JOBS_DIR,
        MAX_FILE_BYTES,
        MAX_FILES,
    )
    from .engine import normalize_report, run_engine
    from .models import AnalyzeResponse, BenchmarkResponse, JobStatus
except ImportError:  # Allows: cd api && uvicorn main:app
    from config import (
        ALLOWED_MODES,
        DEFAULT_AI_THRESHOLD,
        DEFAULT_BENCHMARK_INPUT,
        DEFAULT_MODE,
        DEFAULT_SIM_THRESHOLD,
        DEFAULT_THREADS,
        JOBS_DIR,
        MAX_FILE_BYTES,
        MAX_FILES,
    )
    from engine import normalize_report, run_engine
    from models import AnalyzeResponse, BenchmarkResponse, JobStatus


app = FastAPI(title="Pargus API", version="0.1.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://127.0.0.1:3000"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

JOBS: Dict[str, Dict[str, object]] = {}


@app.get("/", response_class=HTMLResponse)
def home() -> str:
    return """
    <!doctype html>
    <html lang="en">
      <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Pargus API</title>
        <style>
          body {
            margin: 0;
            font-family: Arial, sans-serif;
            background: #f8fafc;
            color: #0f172a;
          }
          main {
            max-width: 760px;
            margin: 72px auto;
            padding: 0 24px;
          }
          h1 {
            margin: 0 0 12px;
            font-size: 40px;
          }
          p {
            color: #475569;
            font-size: 18px;
            line-height: 1.6;
          }
          a {
            color: #b45309;
            font-weight: 700;
          }
          .actions {
            display: flex;
            flex-wrap: wrap;
            gap: 12px;
            margin-top: 28px;
          }
          .button {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            min-height: 44px;
            padding: 0 18px;
            border-radius: 8px;
            background: #0f172a;
            color: #ffffff;
            text-decoration: none;
          }
          .button.secondary {
            background: #e2e8f0;
            color: #0f172a;
          }
          code {
            background: #e2e8f0;
            border-radius: 6px;
            padding: 3px 7px;
          }
        </style>
      </head>
      <body>
        <main>
          <h1>Pargus API</h1>
          <p>FastAPI server for the Pargus plagiarism and AI-authorship engine.</p>
          <p>Use <code>/api/health</code> for health checks, <code>/api/analyze</code> for uploads, or open <a href="/docs">/docs</a> for the interactive API docs.</p>
          <div class="actions">
            <a class="button" href="/docs">Open API Docs</a>
            <a class="button secondary" href="/redoc">View ReDoc</a>
            <a class="button secondary" href="/api/health">Health Check</a>
            <a class="button secondary" href="/api/benchmark">Run Benchmark</a>
          </div>
        </main>
      </body>
    </html>
    """


def now_ms() -> int:
    return int(time.time() * 1000)


def job_dir(job_id: str) -> Path:
    return JOBS_DIR / job_id


def update_job(job_id: str, **values: object) -> None:
    JOBS.setdefault(job_id, {}).update(values)


def get_job_or_404(job_id: str) -> Dict[str, object]:
    job = JOBS.get(job_id)
    if not job:
        status_path = job_dir(job_id) / "status.json"
        if status_path.exists():
            with status_path.open("r", encoding="utf-8") as handle:
                job = json.load(handle)
            JOBS[job_id] = job
    if not job:
        raise HTTPException(status_code=404, detail="job not found")
    return job


def persist_job(job_id: str) -> None:
    path = job_dir(job_id) / "status.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(JOBS[job_id], handle, indent=2)


def run_analysis_job(
    job_id: str,
    threads: int,
    sim_threshold: float,
    ai_threshold: float,
    mode: str,
) -> None:
    started = now_ms()
    update_job(job_id, status="running", progress=0.25, started_at=started)
    persist_job(job_id)

    base = job_dir(job_id)
    try:
        report = run_engine(
            input_dir=base / "input",
            out_dir=base / "output",
            threads=threads,
            sim_threshold=sim_threshold,
            ai_threshold=ai_threshold,
            mode=mode,
        )
        normalized = normalize_report(job_id, report)
        with (base / "result.json").open("w", encoding="utf-8") as handle:
            json.dump(normalized, handle, indent=2)
        update_job(job_id, status="complete", progress=1.0, completed_at=now_ms(), error=None)
    except Exception as exc:
        update_job(job_id, status="failed", progress=1.0, completed_at=now_ms(), error=str(exc))
    finally:
        persist_job(job_id)


async def save_uploads(job_id: str, files: List[UploadFile]) -> int:
    if not files:
        raise HTTPException(status_code=400, detail="upload at least one .txt file")
    if len(files) > MAX_FILES:
        raise HTTPException(status_code=400, detail=f"upload at most {MAX_FILES} files")

    input_dir = job_dir(job_id) / "input"
    output_dir = job_dir(job_id) / "output"
    input_dir.mkdir(parents=True, exist_ok=True)
    output_dir.mkdir(parents=True, exist_ok=True)

    saved = 0
    for upload in files:
        filename = Path(upload.filename or "").name
        if not filename.lower().endswith(".txt"):
            raise HTTPException(status_code=400, detail=f"{filename or 'file'} is not a .txt file")

        destination = input_dir / filename
        size = 0
        with destination.open("wb") as handle:
            while chunk := await upload.read(1024 * 1024):
                size += len(chunk)
                if size > MAX_FILE_BYTES:
                    raise HTTPException(status_code=400, detail=f"{filename} exceeds 5 MB")
                handle.write(chunk)
        saved += 1

    return saved


@app.get("/api/health")
def health() -> Dict[str, str]:
    return {"status": "ok"}


@app.post("/api/analyze", response_model=AnalyzeResponse, status_code=202)
async def analyze(
    background_tasks: BackgroundTasks,
    files: List[UploadFile] = File(...),
    sim_threshold: float = Form(DEFAULT_SIM_THRESHOLD),
    ai_threshold: float = Form(DEFAULT_AI_THRESHOLD),
    threads: int = Form(DEFAULT_THREADS),
    mode: str = Form(DEFAULT_MODE),
) -> AnalyzeResponse:
    if not 0.0 <= sim_threshold <= 1.0:
        raise HTTPException(status_code=400, detail="sim_threshold must be between 0.0 and 1.0")
    if ai_threshold < 0:
        raise HTTPException(status_code=400, detail="ai_threshold must be non-negative")
    if threads < 1:
        raise HTTPException(status_code=400, detail="threads must be at least 1")
    if mode not in ALLOWED_MODES:
        raise HTTPException(status_code=400, detail=f"mode must be one of {sorted(ALLOWED_MODES)}")

    job_id = uuid4().hex[:12]
    created = now_ms()
    num_docs = await save_uploads(job_id, files)
    update_job(
        job_id,
        job_id=job_id,
        status="queued",
        progress=0.0,
        created_at=created,
        num_docs=num_docs,
        error=None,
    )
    persist_job(job_id)
    background_tasks.add_task(run_analysis_job, job_id, threads, sim_threshold, ai_threshold, mode)
    return AnalyzeResponse(job_id=job_id, status="queued", num_docs=num_docs)


@app.get("/api/jobs/{job_id}", response_model=JobStatus)
def job_status(job_id: str) -> JobStatus:
    job = get_job_or_404(job_id)
    started = int(job.get("started_at") or job.get("created_at") or now_ms())
    completed = int(job.get("completed_at") or now_ms())
    return JobStatus(
        job_id=job_id,
        status=job.get("status", "failed"),
        progress=float(job.get("progress", 0.0)),
        elapsed_ms=max(0, completed - started),
        num_docs=int(job.get("num_docs", 0)),
        error=job.get("error"),
    )


@app.get("/api/results/{job_id}")
def results(job_id: str) -> Dict[str, object]:
    get_job_or_404(job_id)
    result_path = job_dir(job_id) / "result.json"
    if not result_path.exists():
        raise HTTPException(status_code=409, detail="results are not ready")
    with result_path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


@app.get("/api/results/{job_id}/matrix.csv")
def matrix_csv(job_id: str) -> FileResponse:
    get_job_or_404(job_id)
    path = job_dir(job_id) / "output" / "similarity_matrix.csv"
    if not path.exists():
        raise HTTPException(status_code=404, detail="matrix CSV not found")
    return FileResponse(path, media_type="text/csv", filename=f"pargus-{job_id}-matrix.csv")


@app.get("/api/results/{job_id}/ai-scores.csv")
def ai_scores_csv(job_id: str) -> FileResponse:
    get_job_or_404(job_id)
    path = job_dir(job_id) / "output" / "ai_scores.csv"
    if not path.exists():
        raise HTTPException(status_code=404, detail="AI scores CSV not found")
    return FileResponse(path, media_type="text/csv", filename=f"pargus-{job_id}-ai-scores.csv")


@app.get("/api/results/{job_id}/report.json")
def report_json(job_id: str) -> FileResponse:
    get_job_or_404(job_id)
    path = job_dir(job_id) / "output" / "report.json"
    if not path.exists():
        raise HTTPException(status_code=404, detail="report JSON not found")
    return FileResponse(path, media_type="application/json", filename=f"pargus-{job_id}-report.json")


@app.get("/api/benchmark", response_model=BenchmarkResponse)
def benchmark() -> BenchmarkResponse:
    if not DEFAULT_BENCHMARK_INPUT.exists():
        raise HTTPException(status_code=404, detail="sample benchmark input not found")

    try:
        benchmark_id = f"benchmark-{uuid4().hex[:8]}"
        base = job_dir(benchmark_id)
        input_dir = base / "input"
        shutil.copytree(DEFAULT_BENCHMARK_INPUT, input_dir)

        points = []
        latest_stage_times = {}
        baseline = None
        for threads in [1, 2, 4, 8]:
            mode = "serial" if threads == 1 else "openmp"
            out_dir = base / f"out-{threads}"
            report = run_engine(
                input_dir=input_dir,
                out_dir=out_dir,
                threads=threads,
                sim_threshold=DEFAULT_SIM_THRESHOLD,
                ai_threshold=DEFAULT_AI_THRESHOLD,
                mode=mode,
            )
            runtime_ms = float(report.get("benchmark", {}).get("stage_times_ms", {}).get("total", 0.0))
            if baseline is None:
                baseline = runtime_ms or 1.0
            latest_stage_times = report.get("benchmark", {}).get("stage_times_ms", {})
            points.append({"threads": threads, "runtime_ms": runtime_ms, "speedup": baseline / runtime_ms if runtime_ms else 0.0})

        return BenchmarkResponse(results=points, stage_times_ms=latest_stage_times)
    except Exception as exc:
        raise HTTPException(status_code=500, detail=str(exc)) from exc
