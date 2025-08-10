# `Parallel Vintage Photo`

**Concurrent Programming (2024/2025) — Final Project**

A C program that applies an *old-photo* effect to a set of `.jpeg` images using multiple worker threads. (Graded: **18/20**).

---

# Overview

`photo-old`:

- Reads all `.jpeg` files from a user-specified directory.
- Sorts the image list either alphabetically (`-name`) or by increasing file size (`-size`) before processing.
- Spawns N worker threads (user-specified). Each worker repeatedly **fetches the next available image** from the shared list and processes it until no images remain.
- Produces processed images with the same filenames under a subdirectory `old-photos` inside the input directory.
- Prints runtime statistics on-demand: press `S` (then Enter) while the program runs to display number processed, number remaining, and average processing time for completed images.
- Skips already-produced output files when re-run (so partial runs do not re-process existing outputs).
- Produces a timing file named `timing-<n>-size.txt` or `timing-<n>-name.txt` (where `<n>` is the number of threads and suffix depends on sorting option) in the input directory containing timing instrumentation.

---

# Features

- Dynamic (work-stealing-like) assignment: workers pick the next image at runtime.
- Two sorting modes before work begins: `-name` or `-size`.
- Safe re-runs: existing processed images are detected and skipped.
- Runtime statistics on demand (S + Enter).
- Timing instrumentation: total execution, per-thread times, and non-parallel section time are saved to a timing file.

---

# Requirements

- POSIX-compatible OS: Linux, WSL, or macOS; C compiler (gcc); `make`.
- libGD package 
 How to install depencdencies


# Build 

From the repository root (where `Makefile` is located):

```bash
make
```

Common targets (depending on the Makefile in your repo):

```bash
make            # build the program
make clean      # remove build artifacts
```

---

# Usage

```
./photo-old <images_dir> <num_threads> -name|-size
```


1. `images_dir` — directory containing the `.jpeg` images to process (relative `./...` or absolute `/...`).
2. `num_threads` — positive integer number of worker threads to spawn.
3. `-name` or `-size` — before processing the images are sorted either alphabetically (`-name`) or by increasing file size (`-size`).

Examples:

```bash
./photo-old ./ex_directory 3 -size
./photo-old ./ex_directory_2 8 -name
./photo-old . 1 -name
```

While the program runs, type `S` and press Enter to print statistics (processed, remaining, average processing time).

---

# Output

- A directory created inside `images_dir` named `old-photos` will contain the processed images, using the same filenames as the originals.
- A timing file is created in `images_dir` with the name:
  - `timing-<n>-size.txt` (if `-size` was used)
  - `timing-<n>-name.txt` (if `-name` was used)

Timing file contents should include:
- total execution time
- per-thread execution times
- time spent in non-parallel (serial) sections

(Exact format is whatever your code writes — confirm the format in your code and adjust README if desired.)

---

# How it works (brief)

1. `main()` scans `images_dir`, selects files with `.jpeg` extension, and stores their names in memory.
2. The list is sorted according to the selected mode (`-name` or `-size`).
3. `main()` spawns `num_threads` worker threads. Each worker:
   - atomically picks the next unprocessed filename from the shared list (a global counter or a protected queue),
   - checks whether the corresponding output file already exists (skip if present),
   - applies the 4-photo-transformations to produce the aged-photo output,
   - updates counters/timers used for statistics and instrumentation.
4. When there are no more images, workers exit and `main()` joins them and writes the timing file.

This matches the dynamic assignment design described in the project specification.

---

# Instrumentation & Timing

The program collects timing information as required by the spec. The produced timing file should contain:
- `total_execution_time`: elapsed time for the entire run
- `thread_times`: execution time for each thread (wall-clock or CPU time — consistent with how you implemented it)
- `non_parallel_time`: time spent in serial sections (e.g., scanning directory, sorting, creating output directory)

If you want to re-run experiments for speedups, run the program with different `num_threads` and keep the timing files (or append results to a CSV) for plotting and analysis.

---

# Performance (EXAMPLE — replace with your actual results)

> **Important:** the table and numbers below are **PLACEHOLDERS / EXAMPLES** to show format only. Replace them with your measured data before final submission.

**Example results table**

| threads | total_time (s) | speedup vs 1-thread |
|--------:|---------------:|--------------------:|
| 1       | 120.00         | 1.00                |
| 2       | 62.00          | 1.94                |
| 4       | 33.50          | 3.58                |
| 8       | 19.80          | 6.06                |

**How to present your real results**
- Put measured timings in a CSV (e.g., `results/timings.csv`) with columns: `threads,total_time,non_parallel_time,thread0_time,...`
- Compute speedup = `T(1) / T(N)`.
- Include charts (e.g., total time vs threads, speedup vs threads) in your report.

---

# Repository structure (suggested / expected for submission)

```
/
├─ Makefile
├─ README.md
├─ photo-old        # compiled binary (or built in bin/)
├─ src/                       # C source files (if applicable)
├─ report.pdf                 # your project report
├─ results/                   # timing files and experiment CSVs
└─ datasets/                  # NOT included in .zip submission; example images (ignored)
```

**Submission note (from project spec):** When submitting the final `.zip` (for the course), include:
- all code for `photo-old`,
- the `Makefile`,
- your report,
- the timing/result files used in the report.  
Do **not** include the image dataset files themselves.

---

# To do / Customization checklist

- [ ] Verify and, if needed, update list of runtime dependencies (e.g., image libraries) in **Requirements**.
- [ ] Replace the example performance table with your measured data and add graphs.
- [ ] Add author/contact info if you want it visible on GitHub.
- [ ] Add a `LICENSE` file if you want to pick a license (e.g., MIT).
