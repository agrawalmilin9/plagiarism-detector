#include "checker.h"
#include "patchwork_detector.h"
#include "reporter.h"
#include "tokenizer.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <thread>

namespace fs = std::filesystem;

// ─── Usage ────────────────────────────────────────────────────────────────────
static void usage(const char* prog) {
    std::cout <<
R"(
Plagiarism Detector — Usage
───────────────────────────
  )" << prog << R"( [options] <file1> <file2> [file3 ...]

Modes:
  --all-pairs      Compare every file against every other (default)
  --suspect <f>    Compare one suspect file against all others

Options:
  --threshold <t>  Similarity threshold to flag (default: 0.60)
  --threads  <n>   Worker thread count (default: hardware_concurrency)
  --patchwork      Run patchwork analysis on the first/suspect file
  --minmatch <n>   Minimum token-segment length for patchwork (default: 8)
  --coverage <c>   Coverage threshold for patchwork flag (default: 0.40)
  --save <path>    Save report to file
  --dir <path>     Load all .cpp/.h/.txt files from a directory
  --help           Show this help

Examples:
  )" << prog << R"( a.cpp b.cpp c.cpp
  )" << prog << R"( --suspect suspect.cpp --patchwork src1.cpp src2.cpp src3.cpp
  )" << prog << R"( --dir ./submissions --threshold 0.50 --save report.txt
)" << "\n";
}

// ─── Load files from a directory ─────────────────────────────────────────────
static std::vector<std::string> loadDir(const std::string& dir) {
    std::vector<std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" ||
            ext == ".c"   || ext == ".h"  || ext == ".hpp" ||
            ext == ".txt") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    // ── Parse arguments ──
    bool        allPairs   = true;
    bool        doPatchwork = false;
    double      threshold  = 0.60;
    int         threads    = 0;
    int         minMatch   = 8;
    double      coverage   = 0.40;
    std::string suspectFile;
    std::string saveFile;
    std::string dirPath;
    std::vector<std::string> files;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help")        { usage(argv[0]); return 0; }
        else if (arg == "--all-pairs")   allPairs = true;
        else if (arg == "--patchwork")   doPatchwork = true;
        else if (arg == "--suspect" && i+1 < argc) {
            suspectFile = argv[++i]; allPairs = false;
        }
        else if (arg == "--threshold" && i+1 < argc) threshold = std::stod(argv[++i]);
        else if (arg == "--threads"   && i+1 < argc) threads   = std::stoi(argv[++i]);
        else if (arg == "--minmatch"  && i+1 < argc) minMatch  = std::stoi(argv[++i]);
        else if (arg == "--coverage"  && i+1 < argc) coverage  = std::stod(argv[++i]);
        else if (arg == "--save"      && i+1 < argc) saveFile  = argv[++i];
        else if (arg == "--dir"       && i+1 < argc) dirPath   = argv[++i];
        else files.push_back(arg);
    }

    if (!dirPath.empty()) {
        auto dirFiles = loadDir(dirPath);
        files.insert(files.end(), dirFiles.begin(), dirFiles.end());
    }

    if (files.empty() && suspectFile.empty()) {
        std::cerr << "Error: no input files specified.\n";
        usage(argv[0]); return 1;
    }

    // ── Print config ──
    std::cout << "\n╔══════════════════════════════╗\n"
              << "║   Plagiarism Detector v1.0   ║\n"
              << "╚══════════════════════════════╝\n\n";
    std::cout << "Threshold : " << threshold << "\n"
              << "Threads   : " << (threads > 0 ? threads
                                               : (int)std::thread::hardware_concurrency())
              << "\n";
    if (!suspectFile.empty())
        std::cout << "Suspect   : " << suspectFile << "\n";
    std::cout << "Files     : " << files.size() << "\n\n";

    Checker checker(threshold, threads);

    // ── Progress callback ──
    auto progress = [](int done, int total) {
        int pct = (int)(100.0 * done / total);
        std::cout << "\r  Comparing... " << done << "/" << total
                  << " (" << pct << "%)   " << std::flush;
    };

    // ── Run comparison ──
    std::vector<CompareResult> results;
    if (!suspectFile.empty()) {
        results = checker.checkOne(suspectFile, files, progress);
    } else {
        results = checker.checkAll(files, progress);
    }
    std::cout << "\n";

    Reporter::printSummary(results);

    if (!saveFile.empty())
        Reporter::saveReport(results, saveFile);

    // ── Patchwork analysis ──
    if (doPatchwork) {
        std::string target = suspectFile.empty()
                           ? (files.empty() ? "" : files[0])
                           : suspectFile;
        if (target.empty()) {
            std::cerr << "Patchwork: no suspect file to analyze.\n";
        } else {
            std::cout << "\nRunning patchwork analysis on: " << target << "\n";

            // Build token strings for suspect and all sources
            std::string suspectTokenStr;
            try {
                auto tok = Tokenizer::tokenizeFile(target);
                suspectTokenStr = Tokenizer::tokensToString(tok);
            } catch (const std::exception& e) {
                std::cerr << "Error tokenizing suspect: " << e.what() << "\n";
                return 1;
            }

            std::vector<std::pair<std::string,std::string>> sources;
            for (const auto& f : files) {
                if (f == target) continue;
                try {
                    auto tok = Tokenizer::tokenizeFile(f);
                    sources.push_back({f, Tokenizer::tokensToString(tok)});
                } catch (...) {}
            }

            PatchworkDetector pd(minMatch, coverage, 0.10);
            auto report = pd.analyze(target, suspectTokenStr, sources);
            Reporter::printPatchwork(report);

            if (!saveFile.empty()) {
                std::string pwFile = saveFile + ".patchwork.txt";
                Reporter::savePatchworkReport(report, pwFile);
            }
        }
    }

    return 0;
}
