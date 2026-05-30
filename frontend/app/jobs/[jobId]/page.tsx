"use client";
import { useEffect, useState, useRef } from "react";
import { useRouter, useParams } from "next/navigation";
import { getJob, JobResponse } from "@/lib/api";

const STAGES = ["I/O", "TF-IDF", "MinHash", "Cosine + RK", "Perplexity", "Done"];

function stageFromProgress(progress: number): number {
  if (progress < 0.15) return 0;
  if (progress < 0.35) return 1;
  if (progress < 0.55) return 2;
  if (progress < 0.75) return 3;
  if (progress < 0.92) return 4;
  return 5;
}

export default function JobPage() {
  const { jobId } = useParams<{ jobId: string }>();
  const router = useRouter();
  const [job, setJob] = useState<JobResponse | null>(null);
  const [elapsed, setElapsed] = useState(0);
  const timerRef = useRef<NodeJS.Timeout | null>(null);
  const pollRef = useRef<NodeJS.Timeout | null>(null);
  const startRef = useRef<number>(Date.now());

  useEffect(() => {
    // Elapsed timer
    timerRef.current = setInterval(() => setElapsed(Math.round((Date.now() - startRef.current) / 1000)), 500);

    // Poll job status
    const poll = async () => {
      try {
        const data = await getJob(jobId);
        setJob(data);
        if (data.status === "complete") {
          clearInterval(timerRef.current!);
          setTimeout(() => router.push(`/results/${jobId}`), 800);
          return;
        }
        if (data.status === "failed") {
          clearInterval(timerRef.current!);
          return;
        }
      } catch {
        // backend not ready yet, keep polling
      }
      pollRef.current = setTimeout(poll, 1000);
    };
    poll();

    return () => {
      clearInterval(timerRef.current!);
      clearTimeout(pollRef.current!);
    };
  }, [jobId, router]);

  const progress = job?.progress ?? 0;
  const currentStage = stageFromProgress(progress);
  const status = job?.status ?? "queued";
  const isFailed = status === "failed";

  return (
    <div style={{ maxWidth: 600, margin: "80px auto", padding: "0 24px" }}>
      <div className="card fade-in" style={{ padding: 40 }}>
        {/* Header */}
        <div style={{ textAlign: "center", marginBottom: 36 }}>
          {isFailed ? (
            <div style={{ width: 56, height: 56, borderRadius: "50%", background: "var(--danger-light)", border: "1px solid #fecaca", display: "flex", alignItems: "center", justifyContent: "center", margin: "0 auto 16px" }}>
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="var(--danger)" strokeWidth="2" strokeLinecap="round"><circle cx="12" cy="12" r="10" /><line x1="15" y1="9" x2="9" y2="15" /><line x1="9" y1="9" x2="15" y2="15" /></svg>
            </div>
          ) : status === "complete" ? (
            <div style={{ width: 56, height: 56, borderRadius: "50%", background: "var(--success-light)", border: "1px solid #bbf7d0", display: "flex", alignItems: "center", justifyContent: "center", margin: "0 auto 16px" }}>
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="var(--success)" strokeWidth="2.5" strokeLinecap="round"><polyline points="20 6 9 17 4 12" /></svg>
            </div>
          ) : (
            <div style={{ width: 56, height: 56, borderRadius: "50%", background: "var(--accent-light)", border: "1px solid var(--accent-light-border)", display: "flex", alignItems: "center", justifyContent: "center", margin: "0 auto 16px" }}>
              <svg className="spin" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="var(--accent)" strokeWidth="2.5" strokeLinecap="round">
                <path d="M21 12a9 9 0 1 1-6.219-8.56" />
              </svg>
            </div>
          )}
          <h1 style={{ fontSize: 20, fontWeight: 700, marginBottom: 6 }}>
            {isFailed ? "Analysis Failed" : status === "complete" ? "Complete!" : "Analyzing Documents"}
          </h1>
          <p style={{ color: "var(--text-muted)", fontSize: 13 }}>
            Job <span className="mono">{jobId}</span>
          </p>
        </div>

        {/* Progress bar */}
        {!isFailed && (
          <div style={{ marginBottom: 32 }}>
            <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
              <span style={{ fontSize: 13, color: "var(--text-muted)" }}>Progress</span>
              <span className="mono" style={{ fontSize: 13, fontWeight: 600, color: "var(--accent)" }}>{Math.round(progress * 100)}%</span>
            </div>
            <div className="progress-track">
              <div className="progress-fill" style={{ width: `${Math.round(progress * 100)}%` }} />
            </div>
          </div>
        )}

        {/* Stage indicators */}
        <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 32, gap: 4 }}>
          {STAGES.map((stage, i) => {
            const done = i < currentStage;
            const active = i === currentStage && !isFailed;
            return (
              <div key={stage} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 6, flex: 1 }}>
                <div
                  style={{
                    width: 28,
                    height: 28,
                    borderRadius: "50%",
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "center",
                    fontSize: 11,
                    fontWeight: 700,
                    background: done ? "var(--success)" : active ? "var(--accent)" : "var(--bg-hover)",
                    color: done || active ? "#fff" : "var(--text-muted)",
                    border: `2px solid ${done ? "var(--success)" : active ? "var(--accent)" : "var(--border)"}`,
                    transition: "all 0.3s",
                  }}
                >
                  {done ? (
                    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3" strokeLinecap="round"><polyline points="20 6 9 17 4 12" /></svg>
                  ) : (
                    i + 1
                  )}
                </div>
                <span style={{ fontSize: 10, color: active ? "var(--accent)" : done ? "var(--success)" : "var(--text-muted)", fontWeight: active || done ? 600 : 400, textAlign: "center", lineHeight: 1.2 }}>
                  {stage}
                </span>
              </div>
            );
          })}
        </div>

        {/* Stats */}
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 12 }}>
          {[
            { label: "Elapsed", value: `${elapsed}s` },
            { label: "Documents", value: job?.num_docs ?? "—" },
            { label: "Status", value: status },
          ].map(({ label, value }) => (
            <div key={label} style={{ background: "var(--bg)", borderRadius: "var(--radius-sm)", padding: "12px 14px", border: "1px solid var(--border)" }}>
              <div style={{ fontSize: 11, color: "var(--text-muted)", marginBottom: 4 }}>{label}</div>
              <div className="mono" style={{ fontSize: 15, fontWeight: 700 }}>{value}</div>
            </div>
          ))}
        </div>

        {isFailed && (
          <div style={{ marginTop: 20, padding: "14px 16px", background: "var(--danger-light)", border: "1px solid #fecaca", borderRadius: "var(--radius-sm)", fontSize: 13, color: "var(--danger)" }}>
            The analysis job failed. Make sure the backend API is running and the engine binary exists at <span className="mono">engine/build/Pargus</span>.
          </div>
        )}
      </div>
    </div>
  );
}
