#include "reporter.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>

// ─── Helpers ──────────────────────────────────────────────────────────────────
std::string Reporter::scoreBar(double score, int width) {
    int filled = (int)(score * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
        bar += (i < filled) ? '#' : '-';
    bar += "]";
    return bar;
}

std::string Reporter::formatTimestamp(
        const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

// ─── Summary table ────────────────────────────────────────────────────────────
void Reporter::printSummary(const std::vector<CompareResult>& results) {
    if (results.empty()) {
        std::cout << "No results to display.\n";
        return;
    }

    // Column widths
    int fwA = 30, fwB = 30;
    for (const auto& r : results) {
        fwA = std::max(fwA, (int)r.fileA.size());
        fwB = std::max(fwB, (int)r.fileB.size());
    }
    fwA = std::min(fwA, 40);
    fwB = std::min(fwB, 40);

    auto trunc = [](const std::string& s, int w) {
        if ((int)s.size() <= w) return s;
        return "..." + s.substr(s.size() - w + 3);
    };

    std::string sep(fwA + fwB + 50, '-');
    std::cout << "\n" << sep << "\n";
    std::cout << std::left
              << std::setw(fwA) << "File A"
              << std::setw(fwB) << "File B"
              << std::setw(8)   << "Score"
              << std::setw(22)  << "Bar"
              << std::setw(10)  << "Matched"
              << "Flag\n";
    std::cout << sep << "\n";

    for (const auto& r : results) {
        std::string flag = r.flagged ? "⚠ FLAGGED" : "  ok";
        std::cout << std::left
                  << std::setw(fwA) << trunc(r.fileA, fwA)
                  << std::setw(fwB) << trunc(r.fileB, fwB)
                  << std::setw(8)   << std::fixed << std::setprecision(2)
                                    << r.similarityScore
                  << std::setw(22)  << scoreBar(r.similarityScore)
                  << std::setw(10)  << r.matchedTokens
                  << flag << "\n";
    }
    std::cout << sep << "\n\n";

    int flagged = (int)std::count_if(results.begin(), results.end(),
                                     [](const CompareResult& r){ return r.flagged; });
    std::cout << "Total pairs: " << results.size()
              << "  |  Flagged: " << flagged << "\n\n";
}

// ─── Patchwork report ─────────────────────────────────────────────────────────
void Reporter::printPatchwork(const PatchworkReport& report) {
    std::cout << "\n=== Patchwork Analysis: " << report.suspectFile << " ===\n";
    std::cout << "Total coverage  : "
              << std::fixed << std::setprecision(1)
              << report.totalCoverage * 100.0 << "%\n";
    std::cout << "Verdict         : "
              << (report.isPatchwork ? "PATCHWORK DETECTED" : "Not flagged") << "\n";
    std::cout << "Segments found  : " << report.segments.size() << "\n\n";

    for (size_t i = 0; i < report.segments.size(); ++i) {
        const auto& seg = report.segments[i];
        std::cout << "  [" << i+1 << "] Source: " << seg.sourceFile << "\n"
                  << "      Source tokens : [" << seg.sourceStart
                  << ", " << seg.sourceEnd << "]\n"
                  << "      Suspect tokens: [" << seg.suspectStart
                  << ", " << seg.suspectEnd << "]\n"
                  << "      Overlap ratio : "
                  << std::fixed << std::setprecision(2)
                  << seg.overlapRatio * 100.0 << "%\n\n";
    }
}

// ─── File output ──────────────────────────────────────────────────────────────
void Reporter::saveReport(const std::vector<CompareResult>& results,
                          const std::string& outPath) {
    std::ofstream f(outPath);
    if (!f) {
        std::cerr << "Cannot write report to: " << outPath << "\n";
        return;
    }
    f << "Plagiarism Detector Report\n"
      << "Generated: " << formatTimestamp(std::chrono::system_clock::now())
      << "\n\n";

    for (const auto& r : results) {
        f << "FileA      : " << r.fileA << "\n"
          << "FileB      : " << r.fileB << "\n"
          << "Similarity : " << std::fixed << std::setprecision(4)
                             << r.similarityScore << "\n"
          << "Matched    : " << r.matchedTokens << " tokens\n"
          << "TotalA     : " << r.totalTokensA << " tokens\n"
          << "TotalB     : " << r.totalTokensB << " tokens\n"
          << "LongestMatch: " << r.longestMatch << " tokens\n"
          << "Flagged    : " << (r.flagged ? "YES" : "NO") << "\n"
          << "Timestamp  : " << formatTimestamp(r.timestamp) << "\n"
          << "---\n";
    }
    std::cout << "Report saved to: " << outPath << "\n";
}

void Reporter::savePatchworkReport(const PatchworkReport& report,
                                   const std::string& outPath) {
    std::ofstream f(outPath);
    if (!f) {
        std::cerr << "Cannot write patchwork report to: " << outPath << "\n";
        return;
    }
    f << "Patchwork Analysis Report\n"
      << "Generated : "
      << formatTimestamp(std::chrono::system_clock::now()) << "\n"
      << "Suspect   : " << report.suspectFile << "\n"
      << "Coverage  : " << std::fixed << std::setprecision(2)
                        << report.totalCoverage * 100.0 << "%\n"
      << "Verdict   : " << (report.isPatchwork ? "PATCHWORK" : "Clean") << "\n\n";

    for (size_t i = 0; i < report.segments.size(); ++i) {
        const auto& seg = report.segments[i];
        f << "Segment " << i+1 << ":\n"
          << "  Source      : " << seg.sourceFile << "\n"
          << "  SourceRange : [" << seg.sourceStart << ", " << seg.sourceEnd << "]\n"
          << "  SuspectRange: [" << seg.suspectStart << ", " << seg.suspectEnd << "]\n"
          << "  Overlap%    : "
          << std::fixed << std::setprecision(2)
          << seg.overlapRatio * 100.0 << "%\n\n";
    }
    std::cout << "Patchwork report saved to: " << outPath << "\n";
}
