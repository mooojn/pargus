"use client";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
  ReferenceLine,
} from "recharts";
import { BenchmarkResult } from "@/lib/api";

interface AmdahlPoint {
  threads: number;
  speedup: number;
}

interface Props {
  results: BenchmarkResult[];
  amdahl: AmdahlPoint[];
}

interface TooltipPayloadEntry {
  name: string;
  value: number;
  color: string;
}

function CustomTooltip({ active, payload, label }: { active?: boolean; payload?: TooltipPayloadEntry[]; label?: number }) {
  if (!active || !payload?.length) return null;
  return (
    <div style={{
      background: "var(--bg-card)",
      border: "1px solid var(--border)",
      borderRadius: 8,
      padding: "10px 14px",
      boxShadow: "var(--shadow-md)",
      fontSize: 13,
    }}>
      <p style={{ fontWeight: 600, marginBottom: 6, color: "var(--text-primary)" }}>{label} thread{label === 1 ? "" : "s"}</p>
      {payload.map((entry) => (
        <div key={entry.name} style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 2 }}>
          <div style={{ width: 10, height: 10, borderRadius: "50%", background: entry.color }} />
          <span style={{ color: "var(--text-secondary)" }}>{entry.name}:</span>
          <span className="mono" style={{ fontWeight: 600, color: "var(--text-primary)" }}>{Number(entry.value).toFixed(2)}×</span>
        </div>
      ))}
    </div>
  );
}

export default function SpeedupChart({ results, amdahl }: Props) {
  if (!results || results.length === 0) return null;

  const baseline = results[0]?.runtime_ms ?? 1;

  // Merge measured + amdahl by thread count
  const amdahlMap = Object.fromEntries(amdahl.map((a) => [a.threads, a.speedup]));
  const allThreads = Array.from(new Set([...results.map((r) => r.threads), ...amdahl.map((a) => a.threads)])).sort((a, b) => a - b);

  const measuredMap = Object.fromEntries(results.map((r) => [r.threads, baseline / r.runtime_ms]));

  const chartData = allThreads.map((t) => ({
    threads: t,
    measured: measuredMap[t] ?? null,
    amdahl: amdahlMap[t] ?? null,
    ideal: t,
  }));

  return (
    <div>
      <ResponsiveContainer width="100%" height={300}>
        <LineChart data={chartData} margin={{ top: 8, right: 24, left: 0, bottom: 0 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="var(--border)" />
          <XAxis
            dataKey="threads"
            label={{ value: "Threads", position: "insideBottom", offset: -2, fontSize: 12, fill: "var(--text-muted)" }}
            tick={{ fontSize: 12, fill: "var(--text-muted)" }}
            tickLine={false}
          />
          <YAxis
            label={{ value: "Speedup ×", angle: -90, position: "insideLeft", offset: 12, fontSize: 12, fill: "var(--text-muted)" }}
            tick={{ fontSize: 12, fill: "var(--text-muted)" }}
            tickLine={false}
            axisLine={false}
          />
          <Tooltip content={<CustomTooltip />} />
          <Legend
            verticalAlign="top"
            iconType="circle"
            iconSize={8}
            wrapperStyle={{ fontSize: 12, paddingBottom: 8 }}
          />
          {/* Ideal linear */}
          <Line
            dataKey="ideal"
            name="Ideal linear"
            stroke="#d1d5db"
            strokeWidth={1.5}
            strokeDasharray="4 4"
            dot={false}
            connectNulls
          />
          {/* Amdahl */}
          <Line
            dataKey="amdahl"
            name="Amdahl (f=0.15)"
            stroke="#6366f1"
            strokeWidth={2}
            strokeDasharray="6 3"
            dot={false}
            connectNulls
          />
          {/* Measured */}
          <Line
            dataKey="measured"
            name="Measured"
            stroke="#2563eb"
            strokeWidth={2.5}
            dot={{ fill: "#2563eb", r: 5, strokeWidth: 2, stroke: "#fff" }}
            activeDot={{ r: 7 }}
            connectNulls
          />
          <ReferenceLine y={1} stroke="var(--border-strong)" strokeDasharray="3 3" />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
