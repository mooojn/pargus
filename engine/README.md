# Pargus Engine

This folder contains the C core engine for Pargus.

The engine currently implements the Part 1 through Part 4 core plagiarism pipeline:

- CLI argument parsing
- `.txt` document discovery and loading
- tokenization
- stopword removal
- lightweight suffix normalization
- TF-IDF vocabulary and sparse vector construction
- MinHash signature generation
- LSH candidate pair generation
- sparse cosine similarity scoring
- Rabin-Karp token fingerprint overlap scoring
- weighted plagiarism score calculation
- symmetric similarity matrix generation
- thresholded flagged plagiarism pairs in `report.json`
- output directory creation
- real `similarity_matrix.csv`
- placeholder `ai_scores.csv`
- real plagiarism sections in `report.json`
- basic benchmark timing output

AI-authorship scoring is still a placeholder and will be added in Part 5.

## Environment

Use WSL Ubuntu for building and running the engine.

Install the baseline tools:

```bash
sudo apt update
sudo apt install -y build-essential cmake make gcc g++
```

## Build

From the repository root:

```bash
cmake -S engine -B engine/build -DCMAKE_BUILD_TYPE=Release
cmake --build engine/build
```

The binary will be created at:

```text
engine/build/Pargus
```

## Run Sample

From the repository root:

```bash
./engine/build/Pargus \
  --input ./data/sample \
  --out-dir ./output \
  --verbose \
  --benchmark
```

Expected output files:

```text
output/similarity_matrix.csv
output/ai_scores.csv
output/report.json
```

The run also prints a TF-IDF summary:

```text
TF-IDF: documents=3 total_tokens=... vocabulary=... build_ms=...
MinHash/LSH: signatures=3 signature_length=100 total_pairs=3 candidate_pairs=... build_ms=...
Similarity: candidate_pairs=... flagged_pairs=... threshold=0.750 build_ms=...
```

`similarity_matrix.csv` is square and symmetric, with `1.000` on the diagonal.
Only LSH candidate pairs are scored off-diagonal; non-candidate pairs remain `0.000`.
The combined plagiarism score is:

```text
combined = 0.6 * cosine_similarity + 0.4 * rabin_karp_overlap
```

Pairs with `combined >= --sim-threshold` are listed in the `flagged_pairs`
section of `report.json` with cosine, Rabin-Karp, and combined scores.

## CLI Options

```bash
./engine/build/Pargus --input DIR [options]
```

Required:

```text
--input DIR              Directory containing .txt documents
```

Options:

```text
--corpus FILE            Corpus path for later n-gram training
--out-dir DIR            Output directory, default ./output
--threads N              Thread count, default 4
--sim-threshold VALUE    Similarity threshold 0.0 to 1.0, default 0.75
--ai-threshold VALUE     AI threshold, default 50.0
--bands N                LSH bands, default 20
--rows N                 LSH rows, default 5
--ngram N                N-gram order, default 3
--mode MODE              openmp or serial, default openmp
--benchmark              Print timing information
--verbose                Print loaded documents
--help                   Show usage
```

## Direct GCC Build

Use this only if CMake is not available:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -I engine \
  engine/main.c \
  engine/common/string_utils.c \
  engine/common/timing.c \
  engine/config/args.c \
  engine/io/reader.c \
  engine/io/writer.c \
  engine/nlp/stopwords.c \
  engine/nlp/tokenizer.c \
  engine/nlp/tfidf.c \
  engine/nlp/minhash.c \
  engine/nlp/lsh.c \
  engine/nlp/similarity.c \
  -o engine/Pargus \
  -lm
```

Then run:

```bash
./engine/Pargus --input ./data/sample --out-dir ./output --verbose --benchmark
```
