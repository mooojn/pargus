"use client";
import { useState, useCallback, useRef } from "react";
import { useRouter } from "next/navigation";
import { analyzeDocuments } from "@/lib/api";

const THREAD_OPTIONS = [1, 2, 4, 8, 16];

export default function UploadPage() {
  const router = useRouter();
  const fileInputRef = useRef<HTMLInputElement>(null);

  const [files, setFiles] = useState<File[]>([]);
  const [dragging, setDragging] = useState(false);
  const [simThreshold, setSimThreshold] = useState(0.75);
  const [aiThreshold, setAiThreshold] = useState(50);
  const [threads, setThreads] = useState(8);
  const [mode, setMode] = useState<"openmp" | "pthreads">("openmp");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // ── File handling ─────────────────────────────────────────────────────────
  const addFiles = useCallback((incoming: FileList | null) => {
    if (!incoming) return;
    const valid: File[] = [];
    const warnings: string[] = [];
    Array.from(incoming).forEach((f) => {
      if (!f.name.endsWith(".txt")) { warnings.push(`${f.name}: not a .txt file`); return; }
      if (f.size > 5 * 1024 * 1024) { warnings.push(`${f.name}: exceeds 5 MB`); return; }
      valid.push(f);
    });
    setFiles((prev) => {
      const merged = [...prev, ...valid];
      if (merged.length > 200) { setError("Maximum 200 files allowed."); return prev.slice(0, 200); }
      return merged;
    });
    if (warnings.length) setError(warnings.join("\n"));
    else setError(null);
  }, []);

  const removeFile = (idx: number) => setFiles((p) => p.filter((_, i) => i !== idx));

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      setDragging(false);
      addFiles(e.dataTransfer.files);
    },
    [addFiles]
  );

  // ── Submit ─────────────────────────────────────────────────────────────────
  const handleAnalyze = async () => {
    if (!files.length) { setError("Please upload at least one .txt file."); return; }
    setLoading(true);
    setError(null);
    try {
      const job = await analyzeDocuments({ files, sim_threshold: simThreshold, ai_threshold: aiThreshold, threads, mode });
      router.push(`/jobs/${job.job_id}`);
    } catch (e: unknown) {
      const msg = e instanceof Error ? e.message : "Failed to connect to API. Make sure the backend is running.";
      setError(msg);
      setLoading(false);
    }
  };

  return (
    <div style={{ maxWidth: 900, margin: "0 auto", padding: "40px 24px", width: "100%" }}>
      {/* ── Header ─────────────────────────────────────────────────────── */}
      <div className="fade-in" style={{ marginBottom: 36 }}>
        <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 8 }}>
          <span className="badge badge-blue">PDC · OpenMP / Pthreads</span>
        </div>
        <h1 style={{ fontSize: 28, fontWeight: 700, letterSpacing: "-0.025em", marginBottom: 8 }}>
          Analyze Documents
        </h1>
        <p style={{ color: "var(--text-secondary)", fontSize: 15, maxWidth: 560 }}>
          Upload student essays to detect plagiarism and AI-authored text using parallel TF-IDF, MinHash/LSH, and n-gram perplexity analysis.
        </p>
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 340px", gap: 20, alignItems: "start" }}>
        {/* ── Left: Uploader ─────────────────────────────────────────── */}
        <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>
          {/* Drop zone */}
          <div
            className="card fade-in"
            onDragOver={(e) => { e.preventDefault(); setDragging(true); }}
            onDragLeave={() => setDragging(false)}
            onDrop={handleDrop}
            onClick={() => fileInputRef.current?.click()}
            style={{
              padding: "48px 24px",
              textAlign: "center",
              cursor: "pointer",
              border: `2px dashed ${dragging ? "var(--accent)" : "var(--border)"}`,
              background: dragging ? "var(--accent-light)" : "var(--bg-card)",
              transition: "all 0.2s",
              borderRadius: "var(--radius)",
            }}
          >
            <input
              ref={fileInputRef}
              type="file"
              multiple
              accept=".txt"
              style={{ display: "none" }}
              onChange={(e) => addFiles(e.target.files)}
            />
            <div style={{ marginBottom: 12 }}>
              <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke={dragging ? "var(--accent)" : "var(--text-muted)"} strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" style={{ margin: "0 auto" }}>
                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
                <polyline points="17 8 12 3 7 8" />
                <line x1="12" y1="3" x2="12" y2="15" />
              </svg>
            </div>
            <p style={{ fontWeight: 600, color: dragging ? "var(--accent)" : "var(--text-primary)", marginBottom: 4 }}>
              Drop .txt files here
            </p>
            <p style={{ color: "var(--text-muted)", fontSize: 13 }}>or click to browse · max 200 files · 5 MB each</p>
          </div>

          {/* File list */}
          {files.length > 0 && (
            <div className="card fade-in" style={{ overflow: "hidden" }}>
              <div style={{ padding: "12px 16px", borderBottom: "1px solid var(--border)", display: "flex", alignItems: "center", justifyContent: "space-between" }}>
                <span style={{ fontWeight: 600, fontSize: 13 }}>
                  Files
                  <span className="badge badge-blue" style={{ marginLeft: 8 }}>{files.length}</span>
                </span>
                <button className="btn btn-secondary" style={{ height: 28, fontSize: 12, padding: "0 10px" }} onClick={(e) => { e.stopPropagation(); setFiles([]); }}>
                  Clear all
                </button>
              </div>
              <div style={{ maxHeight: 260, overflowY: "auto" }}>
                {files.map((f, i) => (
                  <div key={i} style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "9px 16px", borderBottom: i < files.length - 1 ? "1px solid var(--border)" : "none" }}>
                    <div style={{ display: "flex", alignItems: "center", gap: 10, minWidth: 0 }}>
                      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="var(--text-muted)" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" style={{ flexShrink: 0 }}>
                        <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z" />
                        <polyline points="14 2 14 8 20 8" />
                      </svg>
                      <span style={{ fontSize: 13, color: "var(--text-primary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{f.name}</span>
                    </div>
                    <div style={{ display: "flex", alignItems: "center", gap: 12, flexShrink: 0 }}>
                      <span className="mono" style={{ fontSize: 12, color: "var(--text-muted)" }}>{(f.size / 1024).toFixed(1)} KB</span>
                      <button onClick={(e) => { e.stopPropagation(); removeFile(i); }} style={{ background: "none", border: "none", cursor: "pointer", color: "var(--text-muted)", padding: 2, lineHeight: 1 }}>
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round">
                          <line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" />
                        </svg>
                      </button>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Error */}
          {error && (
            <div style={{ padding: "12px 16px", background: "var(--danger-light)", border: "1px solid #fecaca", borderRadius: "var(--radius-sm)", color: "var(--danger)", fontSize: 13, whiteSpace: "pre-line" }}>
              {error}
            </div>
          )}
        </div>

        {/* ── Right: Config panel ─────────────────────────────────────── */}
        <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>
          <div className="card fade-in" style={{ padding: 20 }}>
            <h2 style={{ fontSize: 14, fontWeight: 600, marginBottom: 20 }}>Configuration</h2>

            <div style={{ display: "flex", flexDirection: "column", gap: 20 }}>
              {/* Sim threshold */}
              <div>
                <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                  <label style={{ fontSize: 13, fontWeight: 500 }}>Similarity Threshold</label>
                  <span className="mono badge badge-blue" style={{ fontSize: 12 }}>{simThreshold.toFixed(2)}</span>
                </div>
                <input type="range" min={0} max={1} step={0.01} value={simThreshold} onChange={(e) => setSimThreshold(Number(e.target.value))} />
                <p style={{ fontSize: 11, color: "var(--text-muted)", marginTop: 4 }}>Pairs above this score are flagged</p>
              </div>

              {/* AI threshold */}
              <div>
                <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                  <label style={{ fontSize: 13, fontWeight: 500 }}>AI Perplexity Threshold</label>
                  <span className="mono badge badge-orange" style={{ fontSize: 12 }}>{aiThreshold}</span>
                </div>
                <input type="range" min={10} max={200} step={1} value={aiThreshold} onChange={(e) => setAiThreshold(Number(e.target.value))} />
                <p style={{ fontSize: 11, color: "var(--text-muted)", marginTop: 4 }}>Lower perplexity → more likely AI-written</p>
              </div>

              {/* Thread count */}
              <div>
                <label style={{ fontSize: 13, fontWeight: 500, display: "block", marginBottom: 8 }}>Thread Count</label>
                <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
                  {THREAD_OPTIONS.map((t) => (
                    <button
                      key={t}
                      onClick={() => setThreads(t)}
                      style={{
                        padding: "5px 12px",
                        fontSize: 13,
                        fontWeight: 500,
                        borderRadius: "var(--radius-sm)",
                        border: `1px solid ${threads === t ? "var(--accent)" : "var(--border)"}`,
                        background: threads === t ? "var(--accent-light)" : "var(--bg-card)",
                        color: threads === t ? "var(--accent)" : "var(--text-secondary)",
                        cursor: "pointer",
                        transition: "all 0.15s",
                      }}
                    >
                      {t}
                    </button>
                  ))}
                </div>
              </div>

              {/* Mode */}
              <div>
                <label style={{ fontSize: 13, fontWeight: 500, display: "block", marginBottom: 8 }}>Parallelism Mode</label>
                <div style={{ display: "flex", gap: 6 }}>
                  {(["openmp", "pthreads"] as const).map((m) => (
                    <button
                      key={m}
                      onClick={() => setMode(m)}
                      style={{
                        flex: 1,
                        padding: "7px 0",
                        fontSize: 13,
                        fontWeight: 500,
                        borderRadius: "var(--radius-sm)",
                        border: `1px solid ${mode === m ? "var(--accent)" : "var(--border)"}`,
                        background: mode === m ? "var(--accent-light)" : "var(--bg-card)",
                        color: mode === m ? "var(--accent)" : "var(--text-secondary)",
                        cursor: "pointer",
                        transition: "all 0.15s",
                        textTransform: "uppercase",
                        letterSpacing: "0.04em",
                      }}
                    >
                      {m}
                    </button>
                  ))}
                </div>
              </div>
            </div>
          </div>

          {/* Summary */}
          <div className="card fade-in" style={{ padding: 16 }}>
            <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
              {[
                ["Files", files.length ? `${files.length} document${files.length > 1 ? "s" : ""}` : "None selected"],
                ["Mode", mode.toUpperCase()],
                ["Threads", threads],
                ["Sim threshold", simThreshold.toFixed(2)],
                ["AI threshold", aiThreshold],
              ].map(([k, v]) => (
                <div key={String(k)} style={{ display: "flex", justifyContent: "space-between", fontSize: 13 }}>
                  <span style={{ color: "var(--text-muted)" }}>{k}</span>
                  <span className="mono" style={{ fontWeight: 500, color: "var(--text-primary)" }}>{v}</span>
                </div>
              ))}
            </div>
          </div>

          {/* Analyze btn */}
          <button
            className="btn btn-primary btn-lg fade-in"
            style={{ width: "100%" }}
            disabled={loading || files.length === 0}
            onClick={handleAnalyze}
          >
            {loading ? (
              <>
                <svg className="spin" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5"><circle cx="12" cy="12" r="10" /><path d="M12 6v6l4 2" /></svg>
                Submitting…
              </>
            ) : (
              <>
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"><polygon points="5 3 19 12 5 21 5 3" /></svg>
                Analyze
              </>
            )}
          </button>
        </div>
      </div>

      {/* ── How it works ────────────────────────────────────────────────── */}
      <div className="card fade-in" style={{ marginTop: 32, padding: "24px 28px" }}>
        <h2 style={{ fontSize: 14, fontWeight: 600, marginBottom: 16 }}>How it works</h2>
        <div style={{ display: "grid", gridTemplateColumns: "repeat(5, 1fr)", gap: 0 }}>
          {[
            { num: "01", title: "I/O", desc: "Pthreads read docs concurrently" },
            { num: "02", title: "TF-IDF", desc: "OpenMP vectorises all documents" },
            { num: "03", title: "MinHash / LSH", desc: "Candidate pairs filtered in parallel" },
            { num: "04", title: "Cosine + RK", desc: "Dynamic work queue scores pairs" },
            { num: "05", title: "Perplexity", desc: "N-gram AI authorship scoring" },
          ].map((step, i, arr) => (
            <div key={step.num} style={{ display: "flex", alignItems: "stretch", gap: 0 }}>
              <div style={{ flex: 1, padding: "0 16px", borderRight: i < arr.length - 1 ? "1px solid var(--border)" : "none" }}>
                <div className="mono" style={{ fontSize: 11, color: "var(--accent)", fontWeight: 700, marginBottom: 4 }}>{step.num}</div>
                <div style={{ fontWeight: 600, fontSize: 13, marginBottom: 2 }}>{step.title}</div>
                <div style={{ fontSize: 12, color: "var(--text-muted)", lineHeight: 1.5 }}>{step.desc}</div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
