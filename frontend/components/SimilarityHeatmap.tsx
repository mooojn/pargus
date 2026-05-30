"use client";
import { useMemo, useState } from "react";

interface Props {
  matrix: number[][];
  names: string[];
}

function scoreToColor(score: number): string {
  // green (low) → yellow (mid) → red (high)
  if (score >= 1) return "#dc2626";
  if (score === 0) return "#f0fdf4";
  if (score < 0.5) {
    // green → yellow
    const t = score / 0.5;
    const r = Math.round(22 + t * (234 - 22));
    const g = Math.round(163 + t * (179 - 163));
    const b = Math.round(74 + t * (8 - 74));
    return `rgb(${r},${g},${b})`;
  } else {
    // yellow → red
    const t = (score - 0.5) / 0.5;
    const r = Math.round(234 + t * (220 - 234));
    const g = Math.round(179 + t * (38 - 179));
    const b = Math.round(8 + t * (38 - 8));
    return `rgb(${r},${g},${b})`;
  }
}

function textColor(score: number): string {
  return score > 0.6 ? "#fff" : "#1f2937";
}

const MAX_VISIBLE = 20; // cap label clutter

export default function SimilarityHeatmap({ matrix, names }: Props) {
  const [hoveredCell, setHoveredCell] = useState<{ i: number; j: number } | null>(null);
  const [page, setPage] = useState(0);

  const n = matrix?.length ?? 0;
  const pageSize = Math.min(MAX_VISIBLE, n);
  const start = page * pageSize;
  const slice = useMemo(() => {
    const indices = Array.from({ length: Math.min(pageSize, n - start) }, (_, k) => start + k);
    return indices;
  }, [n, page, pageSize, start]);

  const totalPages = Math.ceil(n / pageSize);

  if (!matrix || n === 0) {
    return (
      <div style={{ padding: "40px 0", textAlign: "center", color: "var(--text-muted)", fontSize: 14 }}>
        No similarity matrix data available.
      </div>
    );
  }

  const labelFor = (i: number) => {
    const name = names?.[i] ?? `doc_${i}`;
    return name.length > 14 ? name.slice(0, 12) + "…" : name;
  };

  const cellSize = Math.max(28, Math.min(52, Math.floor(560 / slice.length)));

  return (
    <div>
      {/* Legend */}
      <div style={{ display: "flex", alignItems: "center", gap: 12, marginBottom: 16 }}>
        <span style={{ fontSize: 12, color: "var(--text-muted)" }}>Similarity:</span>
        <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
          {[0, 0.25, 0.5, 0.75, 1].map((v) => (
            <div key={v} style={{ display: "flex", alignItems: "center", gap: 3 }}>
              <div style={{ width: 20, height: 12, borderRadius: 3, background: scoreToColor(v), border: "1px solid rgba(0,0,0,0.08)" }} />
              <span className="mono" style={{ fontSize: 10, color: "var(--text-muted)" }}>{v.toFixed(2)}</span>
            </div>
          ))}
        </div>
        {n > MAX_VISIBLE && (
          <span style={{ marginLeft: "auto", fontSize: 12, color: "var(--text-muted)" }}>
            Showing docs {start + 1}–{Math.min(start + pageSize, n)} of {n}
          </span>
        )}
      </div>

      {/* Tooltip */}
      {hoveredCell && (
        <div style={{
          position: "fixed", top: 0, left: 0, zIndex: 100,
          pointerEvents: "none",
        }}>
          <div style={{
            background: "var(--text-primary)", color: "#fff",
            padding: "8px 12px", borderRadius: 8, fontSize: 12,
            position: "absolute", top: 8, left: 8,
            whiteSpace: "nowrap",
          }}>
            <strong>{names?.[hoveredCell.i] ?? `doc_${hoveredCell.i}`}</strong>
            {" vs "}
            <strong>{names?.[hoveredCell.j] ?? `doc_${hoveredCell.j}`}</strong>
            <br />
            Score: <span className="mono">{matrix[hoveredCell.i]?.[hoveredCell.j]?.toFixed(4) ?? "—"}</span>
          </div>
        </div>
      )}

      {/* Matrix grid */}
      <div style={{ overflowX: "auto" }}>
        <div style={{ display: "inline-block", minWidth: "fit-content" }}>
          {/* Column labels */}
          <div style={{ display: "flex", marginLeft: 100 }}>
            {slice.map((j) => (
              <div
                key={j}
                style={{
                  width: cellSize,
                  fontSize: 9,
                  color: "var(--text-muted)",
                  textAlign: "center",
                  transform: "rotate(-45deg)",
                  transformOrigin: "bottom left",
                  whiteSpace: "nowrap",
                  marginBottom: 4,
                  height: 60,
                  lineHeight: "60px",
                  paddingLeft: 4,
                }}
              >
                {labelFor(j)}
              </div>
            ))}
          </div>

          {/* Rows */}
          {slice.map((i) => (
            <div key={i} style={{ display: "flex", alignItems: "center" }}>
              {/* Row label */}
              <div style={{ width: 100, fontSize: 11, color: "var(--text-muted)", textAlign: "right", paddingRight: 10, whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>
                {labelFor(i)}
              </div>
              {/* Cells */}
              {slice.map((j) => {
                const score = matrix[i]?.[j] ?? 0;
                const isHovered = hoveredCell?.i === i && hoveredCell?.j === j;
                return (
                  <div
                    key={j}
                    onMouseEnter={() => setHoveredCell({ i, j })}
                    onMouseLeave={() => setHoveredCell(null)}
                    style={{
                      width: cellSize,
                      height: cellSize,
                      background: scoreToColor(score),
                      border: isHovered ? "2px solid var(--text-primary)" : "1px solid rgba(255,255,255,0.4)",
                      display: "flex",
                      alignItems: "center",
                      justifyContent: "center",
                      fontSize: Math.max(8, cellSize * 0.22),
                      fontFamily: "monospace",
                      color: textColor(score),
                      cursor: "default",
                      transition: "transform 0.1s",
                      transform: isHovered ? "scale(1.1)" : "scale(1)",
                      zIndex: isHovered ? 2 : 1,
                      position: "relative",
                      fontWeight: score > 0.75 ? 700 : 400,
                    }}
                  >
                    {cellSize > 36 ? score.toFixed(2) : ""}
                  </div>
                );
              })}
            </div>
          ))}
        </div>
      </div>

      {/* Pagination */}
      {totalPages > 1 && (
        <div style={{ display: "flex", justifyContent: "center", gap: 8, marginTop: 16 }}>
          <button className="btn btn-secondary" style={{ height: 30, padding: "0 12px", fontSize: 12 }} disabled={page === 0} onClick={() => setPage((p) => p - 1)}>← Prev</button>
          <span style={{ fontSize: 13, color: "var(--text-muted)", display: "flex", alignItems: "center" }}>Page {page + 1} / {totalPages}</span>
          <button className="btn btn-secondary" style={{ height: 30, padding: "0 12px", fontSize: 12 }} disabled={page === totalPages - 1} onClick={() => setPage((p) => p + 1)}>Next →</button>
        </div>
      )}
    </div>
  );
}
