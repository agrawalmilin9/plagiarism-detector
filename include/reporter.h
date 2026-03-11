#pragma once
#include "checker.h"
#include "patchwork_detector.h"
#include <string>
#include <vector>

class Reporter {
public:
    // Print a summary table to stdout.
    static void printSummary(const std::vector<CompareResult>& results);

    // Print detailed patchwork report.
    static void printPatchwork(const PatchworkReport& report);

    // Save full results as a plain-text report file.
    static void saveReport(const std::vector<CompareResult>& results,
                           const std::string& outPath);

    // Save patchwork report to file.
    static void savePatchworkReport(const PatchworkReport& report,
                                    const std::string& outPath);

private:
    static std::string scoreBar(double score, int width = 20);
    static std::string formatTimestamp(
        const std::chrono::system_clock::time_point& tp);
};
