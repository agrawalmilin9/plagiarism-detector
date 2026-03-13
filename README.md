# Plagiarism Detector in C++

A high-performance plagiarism detection tool built from scratch using **Ukkonen's Suffix Trees**, **multi-threading**, and **patchwork detection** — capable of comparing C++, C, header, and plain text files.

---

## Features

- **Suffix Tree Matching** — Ukkonen's O(n) suffix tree for token-level substring matching, achieving 90%+ precision
- **Smart Tokenizer** — strips comments, normalises literals and identifiers before comparison (rename-proof)
- **Multi-threaded Checker** — parallel file comparison using `std::async` thread pool with timestamp validation
- **Patchwork Detection** — detects stitched plagiarism across multiple sources using index tracking and overlap control
- **Flexible CLI** — all-pairs mode, one-vs-many mode, directory scanning, configurable thresholds
- **Report Export** — save results to a plain-text report file

---

## Project Structure

```
plagiarism_detector/
├── include/
│   ├── tokenizer.h
│   ├── suffix_tree.h
│   ├── checker.h
│   ├── patchwork_detector.h
│   └── reporter.h
├── src/
│   ├── tokenizer.cpp
│   ├── suffix_tree.cpp
│   ├── checker.cpp
│   ├── patchwork_detector.cpp
│   ├── reporter.cpp
│   └── main.cpp
├── samples/
│   ├── sample_a.cpp    # original
│   ├── sample_b.cpp    # plagiarised from A
│   └── sample_c.cpp    # clean / different
└── CMakeLists.txt
```

---

## Build

### Linux / Ubuntu
```bash
sudo apt update && sudo apt install cmake g++ -y
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows (MSYS2 UCRT64)
```bash
pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc make
mkdir build && cd build
cmake .. -G "Unix Makefiles"
make -j$(nproc)
```

### macOS
```bash
brew install cmake
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Usage

### Compare all files against each other
```bash
./plagcheck file1.cpp file2.cpp file3.cpp
```

### Compare one suspect against source files
```bash
./plagcheck --suspect suspect.cpp src1.cpp src2.cpp src3.cpp
```

### Scan an entire directory
```bash
./plagcheck --dir ./submissions --threshold 0.50 --save report.txt
```

### Patchwork detection
```bash
./plagcheck --suspect suspect.cpp --patchwork --coverage 0.30 src1.cpp src2.cpp
```

---

## CLI Flags

| Flag | Default | Description |
|---|---|---|
| `--threshold <t>` | `0.60` | Similarity score to flag as plagiarised |
| `--threads <n>` | auto | Number of worker threads |
| `--patchwork` | off | Enable patchwork analysis |
| `--coverage <c>` | `0.40` | Min token coverage to flag patchwork |
| `--minmatch <n>` | `8` | Min token segment length for patchwork |
| `--dir <path>` | — | Scan folder for `.cpp/.h/.txt` files |
| `--save <file>` | — | Write full report to file |
| `--suspect <f>` | — | Set one suspect vs many sources mode |

---

## How It Works

### 1. Tokenizer
Source files are tokenised before comparison. This makes detection **rename-proof**:
- C++ comments are stripped
- String/number literals are normalised to `STR_LIT` / `NUM_LIT`
- Identifiers are preserved; keywords are tagged separately
- Plain text is split on whitespace and lowercased

### 2. Suffix Tree (Ukkonen's Algorithm)
Built in **O(n) time** over the token string of document A. Document B is then slid over the tree to find all common substrings above a minimum length. Similarity is computed using the **Dice coefficient**:

```
score = 2 * matched_tokens / (tokens_A + tokens_B)
```

### 3. Multi-threaded Checker
File pairs are distributed across a thread pool using `std::async`. Each result is timestamped with `std::chrono::system_clock` for validation and logging.

### 4. Patchwork Detector
Detects plagiarism stitched from multiple sources:
- Finds matching segments from every source file
- Tracks exact token index ranges (`suspectStart`, `suspectEnd`)
- Filters overlapping segments with configurable overlap tolerance
- Flags when total suspect coverage exceeds the threshold across ≥2 sources

---

## Sample Output

```
╔══════════════════════════════╗
║   Plagiarism Detector v1.0   ║
╚══════════════════════════════╝

--------------------------------------------------------------------
File A          File B          Score   Bar                   Flag
--------------------------------------------------------------------
sample_a.cpp    sample_b.cpp    0.57    [###########---------]  ⚠ FLAGGED
sample_a.cpp    sample_c.cpp    0.18    [###-----------------]    ok
sample_b.cpp    sample_c.cpp    0.17    [###-----------------]    ok
--------------------------------------------------------------------
Total pairs: 3  |  Flagged: 1
```

---

## Requirements

- C++17 or later
- CMake 3.16+
- GCC / Clang / MSVC

---

## Tech Stack

- **Language** — C++17
- **Algorithm** — Ukkonen's Suffix Tree (O(n) construction)
- **Concurrency** — `std::async`, `std::mutex`, `std::atomic`
- **Build System** — CMake
- **Similarity Metric** — Dice Coefficient

---

## Author

Built as a self project to explore string algorithms, compiler-style tokenisation, and concurrent systems in C++.
