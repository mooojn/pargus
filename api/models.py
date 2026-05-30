from typing import Any, Dict, List, Literal, Optional

from pydantic import BaseModel, Field


JobStatusValue = Literal["queued", "running", "complete", "failed"]


class AnalyzeResponse(BaseModel):
    job_id: str
    status: JobStatusValue
    num_docs: int


class JobStatus(BaseModel):
    job_id: str
    status: JobStatusValue
    progress: float = Field(ge=0.0, le=1.0)
    elapsed_ms: int
    num_docs: int = 0
    error: Optional[str] = None


class BenchmarkPoint(BaseModel):
    threads: int
    runtime_ms: float
    speedup: float


class BenchmarkResponse(BaseModel):
    results: List[BenchmarkPoint]
    stage_times_ms: Dict[str, Any] = Field(default_factory=dict)

