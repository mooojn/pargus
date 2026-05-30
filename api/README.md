# Pargus API

FastAPI bridge between the frontend and the Pargus C engine.

Pargus is intended to run in an Ubuntu/Linux environment. On Windows, use WSL
Ubuntu so the API can call the Linux engine binary at `engine/build/Pargus`.

## 1. Install Python On Ubuntu/WSL

```bash
sudo apt update
sudo apt install -y python3 python3-pip python3-venv
```


Check installation:

```bash
python3 --version
pip3 --version
```

## 2. Go To The Project Root

Example path when the repo is on the Windows desktop:

```bash
cd "/mnt/c/Users/Laptop Solutions/Desktop/pargus"
```

If the repo is somewhere else, `cd` into that folder instead.

## 3. Create A Virtual Environment

This step is optional, but recommended so the API dependencies stay separate
from your system Python packages.

```bash
python3 -m venv .venv-wsl
source .venv-wsl/bin/activate
```

When the virtual environment is active, your terminal prompt usually shows:

```text
(.venv-wsl)
```

To leave the virtual environment later:

```bash
deactivate
```

## 4. Install API Libraries

From the project root:

```bash
python -m pip install --upgrade pip
python -m pip install -r api/requirements.txt
```

If you skipped the virtual environment, use:

```bash
python3 -m pip install -r api/requirements.txt
```

## 5. Build The Engine First

The API expects the engine binary here:

```text
engine/build/Pargus
```

Build it from the project root:

```bash
cmake -S engine -B engine/build -DCMAKE_BUILD_TYPE=Release
cmake --build engine/build
```

## 6. Start The API Server

From the project root:

```bash
python -m uvicorn api.main:app --reload --host 127.0.0.1 --port 8000
```

Open:

```text
http://127.0.0.1:8000/
```

Interactive API docs:

```text
http://127.0.0.1:8000/docs
```

## Engine Path

By default the API dynamically uses:

```text
<project-root>/engine/build/Pargus
```

If you need a custom path, edit `api/config.py`:

```python
ENGINE_PATH_OVERRIDE = "/custom/path/to/Pargus"
```

For normal Ubuntu/WSL use, keep it empty:

```python
ENGINE_PATH_OVERRIDE = ""
```

## Endpoints

- `GET /`
- `GET /api/health`
- `POST /api/analyze`
- `GET /api/jobs/{job_id}`
- `GET /api/results/{job_id}`
- `GET /api/results/{job_id}/matrix.csv`
- `GET /api/results/{job_id}/ai-scores.csv`
- `GET /api/results/{job_id}/report.json`
- `GET /api/benchmark`
