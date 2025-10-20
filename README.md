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
   python -m venv .venv # Creating it

   source .venv/bin/activate   # Linux/macOS
   .venv\Scripts\activate      # Windows
   ```

2. Install the required packages:
   ```bash
   pip install -r requirements.txt
   ```

---

## Usage

```bash
./build/photo-old <images_dir> <num_threads> -name|-size
```

Examples:

```bash
./build/photo-old ./photos 3 -size
./build/photo-old ./photos 8 -name
./build/photo-old . 1 -name
```
---

## Output

- Processed images → `<images_dir>/old-photos`
- Timing file → `<images_dir>/timing_B_<n>-size.txt` or `<images_dir>/timing_B_<n>-name.txt`



## Helper Script for Speedup Plotting and Automation

To automate testing and speedup analysis, you can use the provided helper script:

```bash
./scripts/runner.sh
```

Make sure you run it from the **project’s root directory** (the folder containing `python/`, `build/`, and `scripts/`).

- **User input**  
  The script will ask for:
  - the **directory** containing your photos  
  - the **mode** (`-name` or `-size`)

---

### ⚙️ Precautions

- **C executable location**  
  Your compiled binary (`photo-old`) must be inside the `build/` directory.  

- **Thread configuration**  
  The script currently tests a fixed number of threads:  
  ```bash
  threads_num=(1 2 4 8 16)  # array of threads to test
  ```  
  You can modify this line in [`runner.sh`](https://github.com/EngenheiroRodas/Parallel-Vintage-Photo/tree/timing_processing/scripts/runner.sh#L12) to customize the thread counts.

> ⚠️ Running the bash script directly will fail if dependencies are missing, since it calls a Python script to process timing files.

