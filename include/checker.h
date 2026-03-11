#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <chrono>

struct CompareResult {
    std::string fileA;
    std::string fileB;
    double      similarityScore;   // 0-1
    int         matchedTokens;
    int         totalTokensA;
    int         totalTokensB;
    int         longestMatch;      // token count
    bool        flagged;           // true if above threshold
    std::chrono::system_clock::time_point timestamp;
};

using ProgressCallback = std::function<void(int done, int total)>;

class Checker {
public:
    // similarityThreshold : score >= this → flagged
    // numThreads          : worker thread count (0 = hardware_concurrency)
    explicit Checker(double similarityThreshold = 0.60,
                     int    numThreads          = 0);

    // Compare every pair in the file list; returns all results.
    std::vector<CompareResult> checkAll(
        const std::vector<std::string>& files,
        ProgressCallback cb = nullptr);

    // Compare one suspect against a list of source files.
    std::vector<CompareResult> checkOne(
        const std::string&              suspectFile,
        const std::vector<std::string>& sourceFiles,
        ProgressCallback cb = nullptr);

    void setThreshold(double t) { threshold_ = t; }
    double getThreshold() const { return threshold_; }

private:
    double threshold_;
    int    numThreads_;
    std::mutex resultsMutex_;

    CompareResult comparePair(const std::string& fileA,
                              const std::string& fileB);
    double computeSimilarity(int matchedTokens, int totalA, int totalB);
};
