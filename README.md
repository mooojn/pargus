# Pargus

Pargus is a Parallel and Distributed Computing course project for document similarity checking and AI-authorship analysis.

The project is designed around a C/C++ core engine that performs the heavy text analysis work in parallel. A Python API and web interface can be added on top of the engine for uploads, reports, and benchmark visualization.

## What It Does

- Reads a folder of text documents.
- Builds TF-IDF vectors for document similarity.
- Uses MinHash and LSH to reduce pairwise comparisons.
- Scores candidate document pairs with cosine similarity and Rabin-Karp fingerprints.
- Estimates AI-authorship signals using n-gram perplexity and burstiness.
- Produces CSV and JSON reports.
- Measures parallel speedup across multiple thread counts.

## Main Components

```text
engine/      C/C++ core analysis engine
api/         Python FastAPI bridge, planned
frontend/    Next.js dashboard, planned
benchmarks/  Benchmark scripts and speedup graphs
data/        Sample documents and corpora
tasks/       Requirements and implementation task breakdowns
output/      Generated reports
```

## Planned Stack

- C/C++ for the core engine
- OpenMP for document processing and scoring
- Pthreads for I/O and pipeline queues
- CMake for engine builds
- Python/FastAPI for the API layer
- Next.js and Tailwind CSS for the dashboard

## Engine Output

The core engine is expected to generate:

- `similarity_matrix.csv`
- `ai_scores.csv`
- `report.json`

