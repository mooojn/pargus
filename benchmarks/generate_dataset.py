#!/usr/bin/env python3
"""Generate deterministic synthetic essay corpora for Pargus benchmarks."""

from __future__ import annotations

import argparse
import json
import random
import shutil
from pathlib import Path
from typing import Dict, List


TOPICS: Dict[str, List[str]] = {
    "parallel_computing": [
        "parallel algorithms improve throughput by splitting independent work across processing units",
        "shared memory systems require careful synchronization to protect common data structures",
        "cache locality and false sharing can strongly influence observed speedup",
        "dynamic scheduling helps balance irregular workloads across worker threads",
    ],
    "academic_integrity": [
        "academic writing depends on citation discipline and original synthesis of source material",
        "similarity analysis can highlight copied passages that require human review",
        "plagiarism detection systems compare documents through lexical and semantic evidence",
        "fair review processes combine automated signals with instructor judgment",
    ],
    "machine_learning": [
        "language models estimate token probabilities from statistical patterns in training corpora",
        "perplexity measures how surprising a sequence appears under a language model",
        "burstiness describes variation in sentence structure and information density",
        "classification tools require evaluation on held out examples to measure accuracy",
    ],
    "systems_design": [
        "modular software separates input handling analysis logic and reporting responsibilities",
        "benchmarking should record runtime stage costs and reproducible configuration metadata",
        "robust command line tools validate arguments and return clear failure messages",
        "portable projects document their build steps and runtime assumptions",
    ],
}

AI_STYLE_SENTENCES = [
    "This essay presents a comprehensive overview of the topic in a clear and structured manner",
    "It is important to consider both the benefits and limitations of the proposed approach",
    "Overall the evidence suggests that careful implementation can produce meaningful results",
    "In conclusion the topic remains significant for students researchers and practitioners",
]


def build_paragraph(topic: str, rng: random.Random, sentence_count: int) -> str:
    sentences = []
    topic_sentences = TOPICS[topic]
    for _ in range(sentence_count):
        base = rng.choice(topic_sentences)
        connector = rng.choice(
            [
                "This observation is useful in classroom experiments",
                "The same idea appears in practical engineering workflows",
                "Students can verify the claim through controlled measurement",
                "The result becomes clearer when examples are compared directly",
            ]
        )
        sentences.append(f"{base}. {connector}.")
    return " ".join(sentences)


def build_human_doc(index: int, rng: random.Random) -> str:
    topic = list(TOPICS.keys())[index % len(TOPICS)]
    paragraphs = [
        f"Essay {index:03d} discusses {topic.replace('_', ' ')}.",
        build_paragraph(topic, rng, rng.randint(5, 8)),
        build_paragraph(rng.choice(list(TOPICS.keys())), rng, rng.randint(3, 5)),
    ]
    return "\n\n".join(paragraphs) + "\n"


def build_ai_like_doc(index: int, rng: random.Random) -> str:
    topic = list(TOPICS.keys())[index % len(TOPICS)]
    body = []
    for _ in range(10):
        body.append(rng.choice(AI_STYLE_SENTENCES))
        body.append(rng.choice(TOPICS[topic]))
    return f"Essay {index:03d} generated style sample.\n\n" + ". ".join(body) + ".\n"


def inject_plagiarism(documents: List[str], rng: random.Random) -> List[Dict[str, int]]:
    events: List[Dict[str, int]] = []
    if len(documents) < 4:
        return events

    pair_count = max(1, len(documents) // 10)
    for _ in range(pair_count):
        source = rng.randrange(0, len(documents))
        target = rng.randrange(0, len(documents))
        if source == target:
            target = (target + 1) % len(documents)

        source_sentences = [part.strip() for part in documents[source].split(".") if part.strip()]
        if len(source_sentences) < 3:
            continue

        start = rng.randrange(0, max(1, len(source_sentences) - 2))
        copied = ". ".join(source_sentences[start : start + 3]) + "."
        documents[target] = documents[target] + "\n" + copied + "\n"
        events.append({"source": source, "target": target, "sentences_copied": 3})

    return events


def generate_dataset(num_docs: int, out_dir: Path, seed: int, clean: bool) -> None:
    if clean and out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    rng = random.Random(seed)
    documents: List[str] = []
    ai_like_indexes = set(rng.sample(range(num_docs), k=max(1, num_docs // 5)))

    for index in range(num_docs):
        if index in ai_like_indexes:
            documents.append(build_ai_like_doc(index, rng))
        else:
            documents.append(build_human_doc(index, rng))

    plagiarism_events = inject_plagiarism(documents, rng)

    for index, content in enumerate(documents, start=1):
        path = out_dir / f"essay_{index:04d}.txt"
        path.write_text(content, encoding="utf-8")

    metadata = {
        "num_docs": num_docs,
        "seed": seed,
        "ai_like_documents": sorted(index + 1 for index in ai_like_indexes),
        "plagiarism_events": plagiarism_events,
    }
    (out_dir / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate a synthetic Pargus benchmark corpus.")
    parser.add_argument("--num-docs", type=int, default=100, help="Number of .txt essays to generate.")
    parser.add_argument("--out", type=Path, default=Path("data/mixed_corpus"), help="Output directory.")
    parser.add_argument("--seed", type=int, default=2026, help="Random seed for reproducible data.")
    parser.add_argument("--clean", action="store_true", help="Delete the output directory before writing.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.num_docs < 1:
        raise SystemExit("--num-docs must be at least 1")
    generate_dataset(args.num_docs, args.out, args.seed, args.clean)
    print(f"Generated {args.num_docs} documents in {args.out}")


if __name__ == "__main__":
    main()

