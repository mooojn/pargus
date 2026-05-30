"use client";
import { useState } from "react";
import { getBenchmark, BenchmarkResponse } from "@/lib/api";
import SpeedupChart from "@/components/SpeedupChart";

export default function BenchmarkPage() {
  const [loading, setLoading] = useState(false);
  const [data, setData] = useState<BenchmarkResponse | null>(null);
  const [error, setError] = useState<string | null>(null);

  const runBenchmark = async () => {
    setLoading(true);
    setError(null);
    try {
      const result = await getBenchmark();
      setData(result);
    } catch {
      setError("Failed to connect to API. Make sure the backend is running on port 8000.");
    } finally {
      setLoading(false);
    }
  };

  // Compute Amdahl speedup for overlay
  const amdahlData = (f = 0.15) =>
    [1, 2, 4, 8, 16].map((p) => ({ threads: p, speedup: 1 / (f + (1 - f) / p) }));

  const stageNames: Record<string, string> = {
    io: "I/O",
    tfidf: "TF-IDF",
    minhash: "MinHash",
    cosine_rk: "Cosine + RK",
    perplexity: "Perplexity",
  };

  const stageColors: Record<string, string> = {
    io: "#6366f1",
    tfidf: "#2563eb",
    minhash: "#0891b2",
    cosine_rk: "#d97706",
    perplexity: "#dc2626",
  };

  return (
    <div style={{ maxWidth: 1000, margin: "0 auto", padding: "36px 24px", width: "100%" }}>
      {/* ── Header ─────────────────────────────────────────────────── */}
      <div className="fade-in" style={{ marginBottom: 32 }}>
        <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 8 }}>
          <span className="badge badge-blue">Performance</span>
        </div>
        <h1 style={{ fontSize: 26, fontWeight: 700, letterSpacing: "-0.02em", marginBottom: 8 }}>
          Benchmark Suite
        </h1>
        <p style={{ color: "var(--text-secondary)", fontSize: 14, maxWidth: 520 }}>
          Run the engine across thread counts to measure speedup, per-stage breakdown, and Amdahl&apos;s Law adherence.
        </p>
      </div>

      {/* ── Action card ─────────────────────────────────────────────── */}
      <div className="card fade-in" style={{ padding: "24px 28px", marginBottom: 24, display: "flex", alignItems: "center", justifyContent: "space-between", gap: 24, flexWrap: "wrap" }}>
        <div>
          <h2 style={{ fontSize: 15, fontWeight: 600, marginBottom: 4 }}>Run Full Benchmark</h2>
          <p style={{ color: "var(--text-muted)", fontSize: 13 }}>
            Executes the C engine with 1, 2, 4, and 8 threads on a fixed corpus. Takes ~30–60s.
          </p>
        </div>
        <button className="btn btn-primary btn-lg" onClick={runBenchmark} disabled={loading} style={{ minWidth: 180 }}>
          {loading ? (
            <>
              <svg className="spin" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5"><path d="M21 12a9 9 0 1 1-6.219-8.56" /></svg>
              Running…
            </>
          ) : (
            <>
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><polygon points="5 3 19 12 5 21 5 3" /></svg>
              Run Benchmark
            </>
          )}
        </button>
      </div>

      {error && (
        <div className="fade-in" style={{ padding: "12px 16px", background: "var(--danger-light)", border: "1px solid #fecaca", borderRadius: "var(--radius-sm)", color: "var(--danger)", fontSize: 13, marginBottom: 20 }}>
          {error}
        </div>
      )}

      {/* ── Results ─────────────────────────────────────────────────── */}
      {data && (
        <div className="fade-in" style={{ display: "flex", flexDirection: "column", gap: 20 }}>
          {/* Speedup chart */}
          <div className="card" style={{ padding: 24 }}>
            <div style={{ marginBottom: 16 }}>
              <h2 style={{ fontSize: 15, fontWeight: 600, marginBottom: 4 }}>Speedup vs Thread Count</h2>
              <p style={{ color: "var(--text-muted)", fontSize: 13 }}>
                Measured speedup (blue) vs Amdahl&apos;s Law prediction (dashed, f=0.15 serial fraction)
              </p>
            </div>
            <SpeedupChart results={data.results} amdahl={amdahlData()} />
          </div>

          {/* Runtime table */}
          <div className="card" style={{ overflow: "hidden" }}>
            <div style={{ padding: "16px 20px", borderBottom: "1px solid var(--border)" }}>
              <h2 style={{ fontSize: 15, fontWeight: 600 }}>Raw Benchmark Data</h2>
            </div>
            <div className="table-wrap">
              <table>
                <thead>
                  <tr>
                    <th>Threads</th>
                    <th>Runtime (ms)</th>
                    <th>Speedup</th>
                    <th>Efficiency</th>
                  </tr>
                </thead>
                <tbody>
                  {data.results.map((r, i) => {
                    const baseline = data.results[0]?.runtime_ms ?? r.runtime_ms;
                    const speedup = baseline / r.runtime_ms;
                    const efficiency = speedup / r.threads;
                    return (
                      <tr key={i}>
                        <td className="mono" style={{ fontWeight: 600 }}>{r.threads}</td>
                        <td className="mono">{r.runtime_ms.toLocaleString()}</td>
                        <td>
                          <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
                            <div style={{ width: 80, height: 5, background: "var(--border)", borderRadius: 3, overflow: "hidden" }}>
                              <div style={{ width: `${Math.min(speedup / 8, 1) * 100}%`, height: "100%", background: "var(--accent)", borderRadius: 3 }} />
                            </div>
                            <span className="mono" style={{ fontWeight: 600, color: "var(--accent)" }}>{speedup.toFixed(2)}×</span>
                          </div>
                        </td>
                        <td className="mono" style={{ color: efficiency > 0.7 ? "var(--success)" : efficiency > 0.5 ? "var(--warning)" : "var(--danger)" }}>
                          {(efficiency * 100).toFixed(1)}%
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          </div>

          {/* Stage breakdown */}
          {data.stage_times && (
            <div className="card" style={{ padding: 24 }}>
              <h2 style={{ fontSize: 15, fontWeight: 600, marginBottom: 16 }}>Per-Stage Time Breakdown</h2>
              <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
                {Object.entries(data.stage_times).map(([key, ms]) => {
                  const total = Object.values(data.stage_times!).reduce((a, b) => a + b, 0);
                  const pct = (ms / total) * 100;
                  return (
                    <div key={key}>
                      <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 6 }}>
                        <span style={{ fontSize: 13, fontWeight: 500 }}>{stageNames[key] ?? key}</span>
                        <span className="mono" style={{ fontSize: 13, color: "var(--text-muted)" }}>{ms} ms · {pct.toFixed(1)}%</span>
                      </div>
                      <div className="progress-track">
                        <div style={{ height: "100%", width: `${pct}%`, background: stageColors[key] ?? "var(--accent)", borderRadius: 3, transition: "width 0.6s ease" }} />
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
          )}

          {/* Amdahl's Law info */}
          <div className="card fade-in" style={{ padding: "18px 24px", background: "var(--accent-light)", border: "1px solid var(--accent-light-border)" }}>
            <h3 style={{ fontSize: 13, fontWeight: 600, marginBottom: 6, color: "var(--accent)" }}>Amdahl&apos;s Law</h3>
            <p style={{ fontSize: 13, color: "var(--text-secondary)", lineHeight: 1.7 }}>
              <span className="mono">S(p) ≤ 1 / (f + (1−f)/p)</span> where <span className="mono">f</span> = serial fraction and <span className="mono">p</span> = thread count.
              At 8 threads with <span className="mono">f=0.15</span>, the theoretical maximum speedup is{" "}
              <span className="mono" style={{ fontWeight: 700 }}>{(1 / (0.15 + 0.85 / 8)).toFixed(2)}×</span>.
              The per-stage breakdown above shows which stages dominate and limit further gains.
            </p>
          </div>
        </div>
      )}

      {/* ── Placeholder when not run yet ────────────────────────────── */}
      {!data && !loading && (
        <div className="card fade-in" style={{ padding: "56px 24px", textAlign: "center" }}>
          <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="var(--border-strong)" strokeWidth="1.5" strokeLinecap="round" style={{ margin: "0 auto 16px", display: "block" }}>
            <polyline points="22 12 18 12 15 21 9 3 6 12 2 12" />
          </svg>
          <p style={{ color: "var(--text-muted)", fontSize: 14 }}>Click &ldquo;Run Benchmark&rdquo; to measure parallel speedup.</p>
        </div>
      )}
    </div>
  );
}
