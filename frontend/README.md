# Pargus — Frontend Web Dashboard

This is the Next.js frontend web application for **Pargus** (Parallel Pipeline for Plagiarism Detection and AI Authorship Analysis). It provides an interactive interface to trigger corpus analysis, configure parameters, view execution progress, inspect plagiarism matrices, and visualize performance benchmarks.

---

## 🛠️ Tech Stack & Styling

- **Framework**: Next.js 14+ (App Router)
- **State Management & Logic**: React 19 (Hooks, Context, Refs)
- **API Communication**: Axios (with custom wrappers in `lib/api.ts`)
- **Styling**: Tailwind CSS v4 + Custom design tokens (in `app/globals.css`)
- **Visualizations & Charts**: Recharts (Heatmap grid, line charts, and stage bars)

---

## 📁 Directory Structure

```
frontend/
├── app/
│   ├── page.tsx                    # Premium Landing Page (Home)
│   ├── upload/
│   │   └── page.tsx                # Drag-and-drop document uploader & configuration
│   ├── jobs/
│   │   └── [jobId]/
│   │       └── page.tsx            # Real-time job polling and stage progress bars
│   ├── results/
│   │   └── [jobId]/
│   │       └── page.tsx            # Results dashboard (summary cards, LSH matrix, tables)
│   ├── benchmark/
│   │   └── page.tsx                # Thread execution time comparisons and Amdahl's Law fit
│   ├── layout.tsx                  # Root layout containing the shared Navbar
│   └── globals.css                 # Global custom styles and color variables
├── components/
│   ├── Navbar.tsx                  # Navigation header bar
│   ├── SimilarityHeatmap.tsx       # Recharts-based matrix cell visualization
│   └── SpeedupChart.tsx            # Scaling & stage timeline comparison charts
├── lib/
│   └── api.ts                      # Axios wrappers mapping endpoints to FastAPI backend
├── package.json
└── tsconfig.json
```

---

## 🚀 Getting Started

### 1. Prerequisites

Make sure the FastAPI backend is running. By default, the frontend attempts to connect to the backend at `http://127.0.0.1:8000`.

### 2. Install Dependencies

Using `pnpm` (recommended) or `npm`:

```bash
pnpm install
# or
npm install
```

### 3. Run Development Server

Launch the dev environment locally:

```bash
pnpm dev
# or
npm run dev
```

Open [http://localhost:3000](http://localhost:3000) to access the landing page.

### 4. Build for Production

Compile a production bundle:

```bash
pnpm build
npm run start
```

---

## 🔗 Route Map & Pages

1. **Home Page (`/`)**: High-impact landing page highlighting parallel engine architecture, speedup benchmarks, and project statistics.
2. **Analysis Config (`/upload`)**: Select the parallelism engine (OpenMP or Pthreads), choose thread count, set thresholds, and drop target `.txt` files.
3. **Job Tracker (`/jobs/[jobId]`)**: Shows which stage (I/O, TF-IDF, MinHash/LSH, Cosine/RK, Perplexity) is executing.
4. **Results Hub (`/results/[jobId]`)**: Includes similarity matrices, AI-flagged documents, and direct CSV/JSON report downloads.
5. **Benchmarks (`/benchmark`)**: Triggers engine scaling test runs and visualizes Amdahl's Law curves.
