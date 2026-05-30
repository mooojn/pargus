"use client";
import Link from "next/link";

export default function LandingPage() {
  return (
    <div style={{ width: "100%", overflowX: "hidden" }}>
      {/* Hero Section */}
      <section
        style={{
          position: "relative",
          background: "linear-gradient(135deg, #0f172a 0%, #1e293b 100%)",
          color: "#ffffff",
          padding: "80px 24px 100px",
          textAlign: "center",
        }}
      >
        {/* Background Subtle Gradient Glows */}
        <div
          style={{
            position: "absolute",
            top: "20%",
            left: "50%",
            transform: "translate(-50%, -50%)",
            width: "500px",
            height: "500px",
            background: "radial-gradient(circle, rgba(37,99,235,0.15) 0%, rgba(0,0,0,0) 70%)",
            pointerEvents: "none",
          }}
        />

        <div style={{ maxWidth: 800, margin: "0 auto", position: "relative", zIndex: 10 }}>
          <div style={{ display: "inline-block", marginBottom: 16 }}>
            <span
              style={{
                background: "rgba(37, 99, 235, 0.15)",
                color: "#60a5fa",
                border: "1px solid rgba(96, 165, 250, 0.3)",
                padding: "4px 12px",
                borderRadius: "20px",
                fontSize: "12px",
                fontWeight: 600,
                letterSpacing: "0.05em",
                textTransform: "uppercase",
              }}
            >
              Parallel & Distributed Computing Project
            </span>
          </div>

          <h1
            style={{
              fontSize: "46px",
              fontWeight: 800,
              lineHeight: 1.15,
              letterSpacing: "-0.03em",
              marginBottom: 20,
              color: "#f8fafc",
            }}
          >
            Pargus Document Analysis Engine
          </h1>

          <p
            style={{
              fontSize: "18px",
              color: "#94a3b8",
              lineHeight: 1.6,
              maxWidth: 620,
              margin: "0 auto 36px",
            }}
          >
            A high-performance pipeline combining parallel Jaccard plagiarism checks, MinHash locality-sensitive hashing, and n-gram perplexity AI detection.
          </p>

          <div style={{ display: "flex", justifyContent: "center", gap: 14 }}>
            <Link
              href="/upload"
              className="btn btn-primary btn-lg"
              style={{
                background: "var(--accent)",
                color: "#ffffff",
                padding: "0 32px",
                fontSize: "15px",
                fontWeight: 600,
              }}
            >
              Analyze Corpus
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" style={{ marginLeft: 4 }}>
                <polyline points="9 18 15 12 9 6" />
              </svg>
            </Link>

            <Link
              href="/benchmark"
              className="btn btn-secondary btn-lg"
              style={{
                background: "rgba(255, 255, 255, 0.05)",
                color: "#f1f5f9",
                border: "1px solid rgba(255, 255, 255, 0.15)",
                padding: "0 32px",
                fontSize: "15px",
                fontWeight: 600,
              }}
            >
              View Benchmarks
            </Link>
          </div>
        </div>
      </section>

      {/* Stats Section */}
      <section style={{ maxWidth: 1000, margin: "-40px auto 60px", padding: "0 24px", position: "relative", zIndex: 20 }}>
        <div
          className="card"
          style={{
            background: "rgba(255, 255, 255, 0.95)",
            backdropFilter: "blur(20px)",
            padding: "24px 32px",
            display: "grid",
            gridTemplateColumns: "repeat(3, 1fr)",
            gap: 24,
            textAlign: "center",
            boxShadow: "0 20px 25px -5px rgba(0, 0, 0, 0.05), 0 10px 10px -5px rgba(0, 0, 0, 0.02)",
          }}
        >
          <div>
            <div style={{ fontSize: "36px", fontWeight: 800, color: "var(--accent)", marginBottom: 4 }}>&gt; 6.0x</div>
            <div style={{ fontSize: "13px", fontWeight: 600, color: "var(--text-secondary)", textTransform: "uppercase" }}>OpenMP Scaling Speedup</div>
          </div>
          <div style={{ borderLeft: "1px solid var(--border)", borderRight: "1px solid var(--border)" }}>
            <div style={{ fontSize: "36px", fontWeight: 800, color: "var(--accent)", marginBottom: 4 }}>&lt; 2.0s</div>
            <div style={{ fontSize: "13px", fontWeight: 600, color: "var(--text-secondary)", textTransform: "uppercase" }}>Analysis time (100 docs)</div>
          </div>
          <div>
            <div style={{ fontSize: "36px", fontWeight: 800, color: "var(--accent)", marginBottom: 4 }}>&gt; 80%</div>
            <div style={{ fontSize: "13px", fontWeight: 600, color: "var(--text-secondary)", textTransform: "uppercase" }}>Comparison reduction via LSH</div>
          </div>
        </div>
      </section>

      {/* Pipeline Showcase */}
      <section style={{ maxWidth: 1000, margin: "0 auto 80px", padding: "0 24px" }}>
        <div style={{ textAlign: "center", marginBottom: 44 }}>
          <h2 style={{ fontSize: "24px", fontWeight: 700, marginBottom: 12 }}>Core Architecture Pipeline</h2>
          <p style={{ color: "var(--text-secondary)", fontSize: "14.5px", maxWidth: 520, margin: "0 auto" }}>
            Pargus divides document comparison and authorship analysis into five highly parallelized pipeline stages.
          </p>
        </div>

        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 20 }}>
          {[
            {
              stage: "Stage 01",
              title: "Concurrency-Enhanced I/O",
              tech: "POSIX Pthreads",
              desc: "Read document corpora concurrently from disk using dedicated I/O reader threads, preventing file loading bottlenecks and streaming content into atomic memory buffers.",
            },
            {
              stage: "Stage 02",
              title: "Tokenization & Parallel TF-IDF",
              tech: "OpenMP schedule(static)",
              desc: "Perform lowercase normalization, stopword filtering, and term frequency calculation on independent threads. Global document frequencies are merged using lightweight atomic reduction.",
            },
            {
              stage: "Stage 03",
              title: "MinHash & Locality-Sensitive Hashing",
              tech: "OpenMP static bucketing",
              desc: "Extract document shingles and compute signatures via independent hash functions. Bands of signatures are mapped into LSH buckets to quickly filter out safe document pairs.",
            },
            {
              stage: "Stage 04",
              title: "Dynamic Scoring (Cosine + Rabin-Karp)",
              tech: "OpenMP schedule(dynamic)",
              desc: "Fetch candidate pairs from LSH buckets. A dynamic thread-safe queue processes pairs using high-accuracy cosine vector overlap combined with Rabin-Karp rolling hashes.",
            },
            {
              stage: "Stage 05",
              title: "N-gram Language Perplexity",
              tech: "OpenMP per-doc scoring",
              desc: "Evaluate document perplexity and variance against trained human text models to verify AI authorship. Flag documents showing low perplexity and low variance.",
            },
          ].map((item, idx) => (
            <div
              key={idx}
              className="card fade-in"
              style={{
                padding: "24px",
                display: "flex",
                flexDirection: "column",
                gap: 12,
                gridColumn: idx === 4 ? "span 2" : "auto",
              }}
            >
              <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
                <span className="mono" style={{ color: "var(--accent)", fontWeight: 700, fontSize: "12px" }}>{item.stage}</span>
                <span className="badge badge-blue" style={{ fontSize: "11px" }}>{item.tech}</span>
              </div>
              <h3 style={{ fontSize: "16px", fontWeight: 700 }}>{item.title}</h3>
              <p style={{ color: "var(--text-secondary)", fontSize: "13.5px", lineHeight: 1.5 }}>{item.desc}</p>
            </div>
          ))}
        </div>
      </section>

      {/* Highlights Section */}
      <section style={{ background: "#ffffff", borderTop: "1px solid var(--border)", borderBottom: "1px solid var(--border)", padding: "60px 24px" }}>
        <div style={{ maxWidth: 1000, margin: "0 auto" }}>
          <div style={{ display: "grid", gridTemplateColumns: "1.2fr 1fr", gap: 48, alignItems: "center" }}>
            <div>
              <h2 style={{ fontSize: "24px", fontWeight: 700, marginBottom: 16 }}>Why Parallel Plagiarism Detection?</h2>
              <p style={{ color: "var(--text-secondary)", fontSize: "14.5px", marginBottom: 20 }}>
                In academic environments, comparing large collections of documents results in exponential growth of computations. An O(n²) comparison path on 1,000 files requires nearly half a million iterations.
              </p>
              <p style={{ color: "var(--text-secondary)", fontSize: "14.5px", marginBottom: 24 }}>
                Pargus utilizes dual optimization strategies: reducing total comparisons by over 80% using MinHash/LSH filters, and parallelizing all stages using OpenMP and Pthreads. The result is instant feedback, even on consumer-grade hardware.
              </p>
              <div style={{ display: "flex", gap: 12 }}>
                <Link href="/upload" className="btn btn-primary">Start Analysis</Link>
                <Link href="/benchmark" className="btn btn-secondary">Run Performance Suite</Link>
              </div>
            </div>
            <div
              className="card"
              style={{
                padding: "24px",
                background: "linear-gradient(to bottom right, var(--accent-light), #ffffff)",
                borderColor: "var(--accent-light-border)",
              }}
            >
              <h3 style={{ fontSize: "15px", fontWeight: 700, marginBottom: 16, display: "flex", alignItems: "center", gap: 8 }}>
                <span style={{ display: "inline-block", width: 6, height: 6, borderRadius: "50%", background: "var(--accent)" }} />
                Tech Stack at a Glance
              </h3>
              <ul style={{ listStyle: "none", display: "flex", flexDirection: "column", gap: 10 }}>
                {[
                  ["C Core", "Tokenizers, TF-IDF, MinHash/LSH, Cosine, Rabin-Karp, Perplexity Engine"],
                  ["OpenMP / Pthreads", "Shared-memory parallel orchestration & dynamic scheduling"],
                  ["FastAPI Bridge", "Python wrapper layer for asynchronous subprocess command runs"],
                  ["Next.js / Tailwind", "Modern metrics dashboard, dynamic charts, and file loading"],
                ].map(([title, desc]) => (
                  <li key={title} style={{ fontSize: "13px" }}>
                    <strong style={{ color: "var(--text-primary)", display: "block", marginBottom: 2 }}>{title}</strong>
                    <span style={{ color: "var(--text-secondary)" }}>{desc}</span>
                  </li>
                ))}
              </ul>
            </div>
          </div>
        </div>
      </section>

      {/* Footer */}
      <footer style={{ padding: "40px 24px", textAlign: "center", color: "var(--text-muted)" }}>
        <p style={{ fontSize: "13px", marginBottom: 6 }}>
          Pargus — Parallel & Distributed Computing Project
        </p>
        <p style={{ fontSize: "12px" }}>
          Developed by <strong>Munees Tariq (2023-CS-32)</strong> · Supervised by <strong>Sir Waqas Ali</strong> · UET Lahore
        </p>
      </footer>
    </div>
  );
}
