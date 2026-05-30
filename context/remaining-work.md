# Pargus Remaining Work

This file tracks what is still left after the completed C engine implementation,
FastAPI server implementation, and benchmark tooling implementation.
It is based on `context/pargus-requirements.md` and the current repository state.

## Engine Status

The `engine/` CLI is complete for the required core-engine deliverable:

- reads `.txt` documents from an input directory
- supports OpenMP analysis mode
- supports serial baseline mode
- supports POSIX/WSL Pthreads document loading mode
- tokenizes, removes stopwords, and applies lightweight suffix normalization
- builds sparse TF-IDF vectors
- generates MinHash signatures and LSH candidate pairs
- scores candidate pairs with cosine similarity and Rabin-Karp token fingerprint overlap
- writes a symmetric `similarity_matrix.csv`
- writes flagged plagiarism pairs in `report.json`
- trains an n-gram model from `--corpus`, with a small fallback corpus
- writes real `ai_scores.csv`
- escapes JSON and CSV output strings
- writes benchmark timing fields in `report.json` and stdout

MPI is not implemented, but `context/pargus-requirements.md` marks MPI as optional.
Engine completion details are archived in `context/engine-completion-plan.md`.

## API Status

The `api/` FastAPI bridge is now implemented for the required server layer:

- provides a root landing page at `GET /`
- provides a health check at `GET /api/health`
- accepts `.txt` uploads through `POST /api/analyze`
- creates per-job input/output folders under `api/jobs/`
- runs the C engine through a Python subprocess wrapper
- resolves the engine dynamically from `<project-root>/engine/build/Pargus`
- supports an optional engine path override in `api/config.py`
- tracks queued, running, complete, and failed job states
- exposes job polling through `GET /api/jobs/{job_id}`
- normalizes engine output through `GET /api/results/{job_id}`
- exposes downloads for matrix CSV, AI scores CSV, and raw report JSON
- includes a benchmark endpoint at `GET /api/benchmark`
- includes API dependency and WSL/Ubuntu setup docs in `api/README.md`

The API is designed for WSL/Ubuntu because the current engine build target is
Linux. Running the API from Windows PowerShell requires a Windows-built
`Pargus.exe`, otherwise use WSL/Ubuntu.

## Benchmark Status

The `benchmarks/` layer is now implemented:

- `benchmarks/generate_dataset.py` creates deterministic synthetic essay corpora
- generated corpora include human-style documents, AI-like documents, and copied passages
- `benchmarks/run_benchmark.py` runs the engine across thread counts
- benchmark runs collect runtime, speedup, stage timings, flagged pairs, and AI-flagged document counts
- optional corpus-size runs are supported through `--sizes`
- `benchmarks/plot_speedup.py` writes speedup, stage breakdown, and runtime-vs-size PNG graphs
- `benchmarks/requirements.txt` lists the plotting dependency
- `benchmarks/README.md` documents the benchmark workflow

## Frontend Status

The `frontend/` (Next.js) dashboard is now completed:

- Modern Next.js dashboard with a responsive design
- Drag-and-drop file uploader (`/`) with configuration parameters (similarity and AI thresholds, thread counts)
- Real-time job status page (`/jobs/[jobId]`) showing progress and current stage indicators
- Interactive results dashboard (`/results/[jobId]`) displaying summary statistics, similarity heatmaps, flagged pairs, and AI authorship tables
- Benchmark visualization page (`/benchmark`) showcasing speedup vs thread count charts, runtime comparison bar charts, and per-stage stacked execution timelines
- Document/report download actions (matrix CSV, AI scores CSV, raw JSON)

## Project Work Still Missing

The repository still needs these required project layers:

- `docs/`
  - final report
  - runtime comparison table
  - speedup graph
  - per-stage breakdown graph
  - design discussion
  - Amdahl's Law discussion

- `scripts/`
  - optional Wikipedia download/extraction helper

- top-level developer shortcuts:
  - optional top-level `Makefile`
  - root README with full build/run instructions

## Data Still Missing

The repo currently has a small sample dataset and a generator for synthetic
benchmark corpora. The requirements still call for committed or archived
project datasets and benchmark outputs:

- `data/wiki_plaintext.txt` or another large human corpus
- `data/synthetic_essays/`
- `data/chatgpt_essays/`
- `data/mixed_corpus/`
- generated benchmark corpora of 100, 500, and 1000 documents, if needed for the report
- benchmark JSON outputs under `benchmarks/results/`
- generated PNG graphs for the final report

## Recommended Next Implementation Order

1. Run and archive benchmark outputs:
   - generate 100, 500, and 1000 document corpora as needed
   - run thread scaling benchmarks
   - save generated PNG graphs for the final report

2. Prepare final report:
   - collect benchmark data
   - include speedup and per-stage graphs
   - explain remaining limitations honestly

## Practical Completion Definition

The engine should already work from a clean WSL checkout:

```bash
cmake -S engine -B engine/build -DCMAKE_BUILD_TYPE=Release
cmake --build engine/build
ctest --test-dir engine/build

./engine/build/Pargus \
  --input ./data/sample \
  --corpus ./data/sample_corpus.txt \
  --threads 4 \
  --sim-threshold 0.75 \
  --ai-threshold 50.0 \
  --out-dir ./output \
  --benchmark \
  --verbose
```

Then:

```bash
python3 benchmarks/generate_dataset.py --num-docs 100 --out data/mixed_corpus --clean
python3 benchmarks/run_benchmark.py --engine ./engine/build/Pargus --input data/mixed_corpus
python3 benchmarks/plot_speedup.py --results benchmarks/results/benchmark_latest.json
python3 -m uvicorn api.main:app --reload --host 127.0.0.1 --port 8000
npm run dev --prefix app
```

The final deliverables should include generated CSV/JSON outputs, benchmark graphs,
and a written report explaining the parallel design and measured speedup.
