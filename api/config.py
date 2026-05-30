from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
API_DIR = ROOT_DIR / "api"
JOBS_DIR = API_DIR / "jobs"

DEFAULT_ENGINE_PATH = ROOT_DIR / "engine" / "build" / "Pargus"
# Optional escape hatch for unusual installs. Leave empty for the project-relative default above.
ENGINE_PATH_OVERRIDE = ""
DEFAULT_CORPUS_PATH = ROOT_DIR / "data" / "sample_corpus.txt"
DEFAULT_BENCHMARK_INPUT = ROOT_DIR / "data" / "sample"

MAX_FILES = 200
MAX_FILE_BYTES = 5 * 1024 * 1024
DEFAULT_THREADS = 4
DEFAULT_SIM_THRESHOLD = 0.75
DEFAULT_AI_THRESHOLD = 50.0
DEFAULT_MODE = "openmp"
ENGINE_TIMEOUT_SECONDS = 10 * 60

ALLOWED_MODES = {"openmp", "pthreads", "serial"}
