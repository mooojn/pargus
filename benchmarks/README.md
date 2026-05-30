# Pargus Benchmarks

Benchmark helpers for the Pargus C engine.

Run these from the project root in WSL/Ubuntu.

## 1. Generate A Dataset

```bash
python3 benchmarks/generate_dataset.py --num-docs 100 --out data/mixed_corpus --clean
```

## 2. Run Benchmarks

```bash
python3 benchmarks/run_benchmark.py \
  --engine ./engine/build/Pargus \
  --input data/mixed_corpus \
  --corpus data/sample_corpus.txt \
  --threads 1,2,4,8
```

Results are written under:

```text
benchmarks/results/
```

## 3. Plot Graphs

Install plotting dependency if needed:

```bash
python3 -m pip install matplotlib
```

Then:

```bash
python3 benchmarks/plot_speedup.py --results benchmarks/results/benchmark_latest.json
```

PNG graphs are written next to the benchmark JSON.

