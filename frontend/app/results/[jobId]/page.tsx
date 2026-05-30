"use client";
import { useEffect, useState } from "react";
import { useParams } from "next/navigation";
import Link from "next/link";
import { getResults, ResultsResponse, matrixCsvUrl, aiCsvUrl, reportJsonUrl } from "@/lib/api";
import SimilarityHeatmap from "@/components/SimilarityHeatmap";

export default function ResultsPage() {
  const { jobId } = useParams<{ jobId: string }>();
  const [data, setData] = useState<ResultsResponse | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [activeTab, setActiveTab] = useState<"matrix" | "pairs" | "ai">("pairs");

  useEffect(() => {
    getResults(jobId)
      .then(setData)
      .catch(() => setError("Failed to load results."))
      .finally(() => setLoading(false));
  }, [jobId]);

  if (loading) return <LoadingSkeleton />;
  if (error || !data) return (
    <div style={{ maxWidth: 600, margin: "80px auto", padding: "0 24px" }}>
      <div className="card" style={{ padding: 40, textAlign: "center" }}>
        <p style={{ color: "var(--danger)", marginBottom: 16 }}>{error ?? "No data found."}</p>
        <Link href="/" className="btn btn-primary">← Back to Upload</Link>
      </div>
    </div>
  );

  const flaggedCount = data.flagged_pairs?.length ?? 0;
  const aiFlaggedCount = data.ai_scores?.filter((s) => s.flagged).length ?? 0;

  return (
    <div style={{ maxWidth: 1100, margin: "0 auto", padding: "36px 24px", width: "100%" }}>
      {/* ── Header ───────────────────────────────────────────────────── */}
      <div className="fade-in" style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", marginBottom: 28, gap: 16, flexWrap: "wrap" }}>
        <div>
          <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 6 }}>
            <Link href="/" style={{ color: "var(--text-muted)", fontSize: 13, textDecoration: "none", display: "flex", alignItems: "center", gap: 4 }}>
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><polyline points="15 18 9 12 15 6" /></svg>
              New Analysis
            </Link>
          </div>
          <h1 style={{ fontSize: 24, fontWeight: 700, letterSpacing: "-0.02em" }}>Results Dashboard</h1>
          <p style={{ color: "var(--text-muted)", fontSize: 13, marginTop: 4 }}>
            Job <span className="mono">{jobId}</span>
          </p>
        </div>
        {/* Downloads */}
        <div style={{ display: "flex", gap: 8, flexWrap: "wrap" }}>
          <a href={matrixCsvUrl(jobId)} className="btn btn-secondary" download>
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" /><polyline points="7 10 12 15 17 10" /><line x1="12" y1="15" x2="12" y2="3" /></svg>
            Matrix CSV
          </a>
          <a href={aiCsvUrl(jobId)} className="btn btn-secondary" download>
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" /><polyline points="7 10 12 15 17 10" /><line x1="12" y1="15" x2="12" y2="3" /></svg>
            AI Scores CSV
          </a>
          <a href={reportJsonUrl(jobId)} className="btn btn-primary" download>
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" /><polyline points="7 10 12 15 17 10" /><line x1="12" y1="15" x2="12" y2="3" /></svg>
            Full Report JSON
          </a>
        </div>
      </div>

      {/* ── Summary Cards ────────────────────────────────────────────── */}
      <div className="fade-in" style={{ display: "grid", gridTemplateColumns: "repeat(4, 1fr)", gap: 14, marginBottom: 28 }}>
        {[
          { label: "Total Documents", value: data.num_docs, icon: "docs", color: "var(--accent)", bg: "var(--accent-light)" },
          { label: "Flagged Pairs", value: flaggedCount, icon: "flag", color: "#d97706", bg: "#fffbeb" },
          { label: "AI-Flagged Docs", value: aiFlaggedCount, icon: "ai", color: "var(--danger)", bg: "var(--danger-light)" },
          { label: "Runtime", value: `${(data.runtime_ms / 1000).toFixed(2)}s`, icon: "time", color: "var(--success)", bg: "var(--success-light)" },
        ].map(({ label, value, icon, color, bg }) => (
          <div key={label} className="card" style={{ padding: "18px 20px" }}>
            <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", marginBottom: 10 }}>
              <span style={{ fontSize: 12, color: "var(--text-muted)", fontWeight: 500 }}>{label}</span>
              <div style={{ width: 30, height: 30, borderRadius: 8, background: bg, display: "flex", alignItems: "center", justifyContent: "center" }}>
                {icon === "docs" && <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth="2" strokeLinecap="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z" /><polyline points="14 2 14 8 20 8" /></svg>}
                {icon === "flag" && <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth="2" strokeLinecap="round"><path d="M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1z" /><line x1="4" y1="22" x2="4" y2="15" /></svg>}
                {icon === "ai" && <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth="2" strokeLinecap="round"><circle cx="12" cy="12" r="3" /><path d="M12 1v4M12 19v4M4.22 4.22l2.83 2.83M16.95 16.95l2.83 2.83M1 12h4M19 12h4M4.22 19.78l2.83-2.83M16.95 7.05l2.83-2.83" /></svg>}
                {icon === "time" && <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke={color} strokeWidth="2" strokeLinecap="round"><circle cx="12" cy="12" r="10" /><polyline points="12 6 12 12 16 14" /></svg>}
              </div>
            </div>
            <div className="mono" style={{ fontSize: 26, fontWeight: 700, color: "var(--text-primary)", lineHeight: 1 }}>{value}</div>
            {data.speedup && icon === "time" && (
              <div style={{ fontSize: 11, color: "var(--text-muted)", marginTop: 4 }}>
                {data.speedup.toFixed(1)}× speedup
              </div>
            )}
          </div>
        ))}
      </div>

      {/* ── Tabs ────────────────────────────────────────────────────── */}
      <div className="fade-in" style={{ display: "flex", gap: 2, marginBottom: 16, background: "var(--bg-hover)", padding: 4, borderRadius: "var(--radius-sm)", width: "fit-content" }}>
        {(["pairs", "matrix", "ai"] as const).map((tab) => {
          const labels: Record<string, string> = { pairs: `Flagged Pairs (${flaggedCount})`, matrix: "Similarity Matrix", ai: `AI Scores (${data.ai_scores?.length ?? 0})` };
          return (
            <button
              key={tab}
              onClick={() => setActiveTab(tab)}
              style={{
                padding: "7px 16px",
                fontSize: 13,
                fontWeight: 500,
                borderRadius: 6,
                border: "none",
                cursor: "pointer",
                background: activeTab === tab ? "var(--bg-card)" : "transparent",
                color: activeTab === tab ? "var(--text-primary)" : "var(--text-muted)",
                boxShadow: activeTab === tab ? "var(--shadow-sm)" : "none",
                transition: "all 0.15s",
              }}
            >
              {labels[tab]}
            </button>
          );
        })}
      </div>

      {/* ── Tab content ─────────────────────────────────────────────── */}
      {activeTab === "pairs" && (
        <div className="card fade-in" style={{ overflow: "hidden" }}>
          {flaggedCount === 0 ? (
            <div style={{ padding: 48, textAlign: "center", color: "var(--text-muted)" }}>
              <svg width="36" height="36" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" style={{ margin: "0 auto 12px", display: "block" }}><polyline points="20 6 9 17 4 12" /></svg>
              No flagged pairs found — all documents appear original.
            </div>
          ) : (
            <div className="table-wrap">
              <table>
                <thead>
                  <tr>
                    <th>Document A</th>
                    <th>Document B</th>
                    <th>Score</th>
                    <th>Severity</th>
                  </tr>
                </thead>
                <tbody>
                  {data.flagged_pairs.sort((a, b) => b.score - a.score).map((pair, i) => (
                    <tr key={i}>
                      <td className="mono" style={{ fontSize: 13 }}>{pair.doc_a}</td>
                      <td className="mono" style={{ fontSize: 13 }}>{pair.doc_b}</td>
                      <td>
                        <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
                          <div style={{ width: 80, height: 5, background: "var(--border)", borderRadius: 3, overflow: "hidden" }}>
                            <div style={{ width: `${pair.score * 100}%`, height: "100%", background: pair.score > 0.9 ? "var(--danger)" : pair.score > 0.8 ? "var(--warning)" : "var(--accent)", borderRadius: 3 }} />
                          </div>
                          <span className="mono" style={{ fontSize: 13, fontWeight: 600 }}>{pair.score.toFixed(3)}</span>
                        </div>
                      </td>
                      <td>
                        <span className={`badge ${pair.score > 0.9 ? "badge-red" : pair.score > 0.8 ? "badge-orange" : "badge-blue"}`}>
                          {pair.score > 0.9 ? "High" : pair.score > 0.8 ? "Medium" : "Low"}
                        </span>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      )}

      {activeTab === "matrix" && (
        <div className="card fade-in" style={{ padding: 24 }}>
          <p style={{ fontSize: 13, color: "var(--text-muted)", marginBottom: 16 }}>
            Pairwise cosine + Rabin-Karp similarity scores. Green = low, yellow = medium, red = high.
          </p>
          <SimilarityHeatmap matrix={data.similarity_matrix} names={data.doc_names ?? []} />
        </div>
      )}

      {activeTab === "ai" && (
        <div className="card fade-in" style={{ overflow: "hidden" }}>
          {(!data.ai_scores || data.ai_scores.length === 0) ? (
            <div style={{ padding: 48, textAlign: "center", color: "var(--text-muted)" }}>No AI scores available.</div>
          ) : (
            <div className="table-wrap">
              <table>
                <thead>
                  <tr>
                    <th>Filename</th>
                    <th>Mean Perplexity</th>
                    <th>PPL Variance</th>
                    <th>AI Score</th>
                    <th>Status</th>
                  </tr>
                </thead>
                <tbody>
                  {data.ai_scores.sort((a, b) => (b.ai_score ?? 0) - (a.ai_score ?? 0)).map((s, i) => {
                    const variance = s.ppl_variance ?? (s as any).perplexity_variance;
                    return (
                      <tr key={i}>
                        <td className="mono" style={{ fontSize: 13 }}>{s.filename}</td>
                        <td className="mono">{s.mean_perplexity?.toFixed(1) ?? "—"}</td>
                        <td className="mono">{variance?.toFixed(1) ?? "—"}</td>
                        <td>
                          <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
                            <div style={{ width: 80, height: 5, background: "var(--border)", borderRadius: 3, overflow: "hidden" }}>
                              <div style={{ width: `${(s.ai_score ?? 0) * 100}%`, height: "100%", background: (s.ai_score ?? 0) > 0.7 ? "var(--danger)" : (s.ai_score ?? 0) > 0.4 ? "var(--warning)" : "var(--success)", borderRadius: 3 }} />
                            </div>
                            <span className="mono" style={{ fontWeight: 600 }}>{s.ai_score?.toFixed(2) ?? "—"}</span>
                          </div>
                        </td>
                        <td>
                          <span className={`badge ${s.flagged ? "badge-red" : "badge-green"}`}>
                            {s.flagged ? "⚠ Flagged" : "✓ Human"}
                          </span>
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          )}
        </div>
      )}
    </div>
  );
}

function LoadingSkeleton() {
  return (
    <div style={{ maxWidth: 1100, margin: "0 auto", padding: "36px 24px" }}>
      <div style={{ marginBottom: 28 }}>
        <div className="skeleton" style={{ width: 180, height: 28, marginBottom: 8 }} />
        <div className="skeleton" style={{ width: 260, height: 16 }} />
      </div>
      <div style={{ display: "grid", gridTemplateColumns: "repeat(4,1fr)", gap: 14, marginBottom: 28 }}>
        {[0,1,2,3].map(i => <div key={i} className="skeleton card" style={{ height: 90 }} />)}
      </div>
      <div className="skeleton" style={{ height: 400, borderRadius: "var(--radius)" }} />
    </div>
  );
}
