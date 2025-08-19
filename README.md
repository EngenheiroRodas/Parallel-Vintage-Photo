# `Parallel Vintage Photo`

**Concurrent Programming (2024/2025) — Final Project**  
A multi-threaded C program that applies an *old-photo* effect to `.jpeg` images with **dynamic workload distribution**. (Grade: **18/20**)

---

## Overview

- Scans a given directory for `.jpeg` images.
- Sorts them either alphabetically (`-name`) or by file size (`-size`).
- Spawns a user-defined number of worker threads, that get dynamically allocated.
- Saves results in `<input_dir>/old-photos` with the same filenames.
- Skips images already processed in previous runs.
- Allows live progress checks (press `S` + Enter during execution), writing a timing file upon completion.
- Includes a script to calculate speedup when using multiple threads (found in `scripts/`).

---

## Requirements

### System & Dependencies
- **OS & tools**: Linux, WSL, or macOS; `gcc`.
- **libGD** for image manipulation:  
  - **Ubuntu/Debian**:
    ```bash
    sudo apt install libgd-dev
    ```
  - **Fedora / CentOS / RedHat**:
    ```bash
    sudo dnf install gd-devel
    ```
  - **macOS (Homebrew)**:
    ```bash
    brew install gd
    ```

### Python Environment
The analysis script (timing/speedup) requires Python.  

1. Create and activate a virtual environment:
   ```bash
   python -m venv .venv
   source .venv/bin/activate   # Linux/macOS
   .venv\Scripts\activate      # Windows
   ```

2. Install the required packages:
   ```bash
   pip install -r requirements.txt
   ```

> ⚠️ Running the bash script directly will fail if dependencies are missing, since it calls a Python script to process timing files.

---

## Usage

```bash
./photo-old <images_dir> <num_threads> -name|-size
```

Examples:

```bash
./photo-old ./ex_directory 3 -size
./photo-old ./ex_directory_2 8 -name
./photo-old . 1 -name
```

You can also use the helper script:

```bash
./scripts/runner.sh
```

---

## Output

- Processed images → `<images_dir>/old-photos`
- Timing file → `<images_dir>/timing_B_<n>-size.txt` or `<images_dir>/timing_B_<n>-name.txt`
- If the Python script is run:  
  - An Excel file with speedup timings  
  - Corresponding plots  
