import axios from "axios";

const BASE = process.env.NEXT_PUBLIC_API_URL ?? "http://127.0.0.1:8000";

const api = axios.create({ baseURL: BASE, timeout: 30_000 });

// ── Types ──────────────────────────────────────────────────────────────────

export interface AnalyzeRequest {
  files: File[];
  sim_threshold: number;
  ai_threshold: number;
  threads: number;
  mode: "openmp" | "pthreads";
}

export interface JobResponse {
  job_id: string;
  status: "queued" | "running" | "complete" | "failed";
  progress?: number;
  elapsed_ms?: number;
  num_docs?: number;
}

export interface FlaggedPair {
  doc_a: string;
  doc_b: string;
  score: number;
}

export interface AIScore {
  filename: string;
  mean_perplexity: number;
  ppl_variance: number;
  ai_score: number;
  flagged: boolean;
}

export interface ResultsResponse {
  job_id: string;
  num_docs: number;
  similarity_matrix: number[][];
  doc_names: string[];
  flagged_pairs: FlaggedPair[];
  ai_scores: AIScore[];
  runtime_ms: number;
  speedup?: number;
}

export interface BenchmarkResult {
  threads: number;
  runtime_ms: number;
  speedup?: number;
}

export interface StageTime {
  io: number;
  tfidf: number;
  minhash: number;
  cosine_rk: number;
  perplexity: number;
}

export interface BenchmarkResponse {
  results: BenchmarkResult[];
  stage_times?: StageTime;
  speedup_graph_url?: string;
}

// ── API calls ──────────────────────────────────────────────────────────────

export async function analyzeDocuments(req: AnalyzeRequest): Promise<JobResponse> {
  const form = new FormData();
  req.files.forEach((f) => form.append("files", f));
  form.append("sim_threshold", String(req.sim_threshold));
  form.append("ai_threshold", String(req.ai_threshold));
  form.append("threads", String(req.threads));
  form.append("mode", req.mode);
  const { data } = await api.post<JobResponse>("/api/analyze", form);
  return data;
}

export async function getJob(jobId: string): Promise<JobResponse> {
  const { data } = await api.get<JobResponse>(`/api/jobs/${jobId}`);
  return data;
}

export async function getResults(jobId: string): Promise<ResultsResponse> {
  const { data } = await api.get<ResultsResponse>(`/api/results/${jobId}`);
  return data;
}

export async function getBenchmark(): Promise<BenchmarkResponse> {
  const { data } = await api.get<BenchmarkResponse>("/api/benchmark");
  return data;
}

export function matrixCsvUrl(jobId: string) {
  return `${BASE}/api/results/${jobId}/matrix.csv`;
}
export function aiCsvUrl(jobId: string) {
  return `${BASE}/api/results/${jobId}/ai-scores.csv`;
}
export function reportJsonUrl(jobId: string) {
  return `${BASE}/api/results/${jobId}/report.json`;
}
