#include "checker.h"
#include "tokenizer.h"
#include "suffix_tree.h"
#include <thread>
#include <future>
#include <queue>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <algorithm>

Checker::Checker(double similarityThreshold, int numThreads)
    : threshold_(similarityThreshold)
    , numThreads_(numThreads > 0 ? numThreads
                                 : (int)std::thread::hardware_concurrency())
{
    if (numThreads_ < 1) numThreads_ = 2;
}

double Checker::computeSimilarity(int matched, int totalA, int totalB) {
    if (totalA == 0 || totalB == 0) return 0.0;
    // Dice coefficient: 2*matched / (|A| + |B|)
    return (2.0 * matched) / (totalA + totalB);
}

CompareResult Checker::comparePair(const std::string& fileA,
                                   const std::string& fileB) {
    CompareResult r;
    r.fileA     = fileA;
    r.fileB     = fileB;
    r.timestamp = std::chrono::system_clock::now();

    try {
        auto tokA = Tokenizer::tokenizeFile(fileA);
        auto tokB = Tokenizer::tokenizeFile(fileB);

        std::string strA = Tokenizer::tokensToString(tokA);
        std::string strB = Tokenizer::tokensToString(tokB);

        r.totalTokensA = (int)tokA.size();
        r.totalTokensB = (int)tokB.size();

        if (tokA.empty() || tokB.empty()) {
            r.similarityScore = 0.0;
            r.matchedTokens   = 0;
            r.longestMatch    = 0;
            r.flagged         = false;
            return r;
        }

        SuffixTree tree(strA);
        auto matches = tree.findMatches(strB, /*minMatchLen=*/5);

        // Count matched characters (proxy for tokens via space counting)
        int matchedChars = 0;
        for (auto& m : matches) matchedChars += m.length;

        // Convert char-level matched to approximate token count
        // Average token is ~6 chars + 1 space = 7
        int avgTokLen = (strA.size() > 0 && tokA.size() > 0)
                        ? (int)(strA.size() / tokA.size()) + 1
                        : 7;
        r.matchedTokens = matchedChars / avgTokLen;
        r.longestMatch  = matches.empty() ? 0 : (matches[0].length / avgTokLen);

        r.similarityScore = computeSimilarity(r.matchedTokens,
                                              r.totalTokensA,
                                              r.totalTokensB);
        r.flagged = (r.similarityScore >= threshold_);
    } catch (const std::exception& e) {
        std::cerr << "[Checker] Error comparing " << fileA
                  << " vs " << fileB << ": " << e.what() << "\n";
        r.similarityScore = 0.0;
        r.matchedTokens   = 0;
        r.longestMatch    = 0;
        r.flagged         = false;
    }
    return r;
}

// ─── All-pairs comparison ─────────────────────────────────────────────────────
std::vector<CompareResult> Checker::checkAll(
        const std::vector<std::string>& files,
        ProgressCallback cb) {

    // Build work queue of pairs
    std::vector<std::pair<int,int>> pairs;
    for (int i = 0; i < (int)files.size(); ++i)
        for (int j = i+1; j < (int)files.size(); ++j)
            pairs.push_back({i, j});

    int total = (int)pairs.size();
    std::atomic<int> done{0};
    std::vector<CompareResult> results;
    results.reserve(total);

    // Thread pool via futures
    std::vector<std::future<CompareResult>> futures;
    futures.reserve(total);

    // Semaphore-like batching to cap active threads
    int batch = numThreads_;
    for (int b = 0; b < total; b += batch) {
        int end = std::min(b + batch, total);
        for (int k = b; k < end; ++k) {
            auto [i, j] = pairs[k];
            futures.push_back(std::async(std::launch::async,
                [this, &files, i, j]() {
                    return comparePair(files[i], files[j]);
                }));
        }
        for (int k = b; k < end; ++k) {
            results.push_back(futures[k].get());
            ++done;
            if (cb) cb(done.load(), total);
        }
    }

    std::sort(results.begin(), results.end(),
              [](const CompareResult& a, const CompareResult& b){
                  return a.similarityScore > b.similarityScore;
              });
    return results;
}

// ─── One-vs-many comparison ───────────────────────────────────────────────────
std::vector<CompareResult> Checker::checkOne(
        const std::string&              suspectFile,
        const std::vector<std::string>& sourceFiles,
        ProgressCallback cb) {

    int total = (int)sourceFiles.size();
    std::atomic<int> done{0};
    std::vector<CompareResult> results;
    results.reserve(total);

    std::vector<std::future<CompareResult>> futures;
    futures.reserve(total);

    int batch = numThreads_;
    for (int b = 0; b < total; b += batch) {
        int end = std::min(b + batch, total);
        for (int k = b; k < end; ++k) {
            futures.push_back(std::async(std::launch::async,
                [this, &suspectFile, &sourceFiles, k]() {
                    return comparePair(suspectFile, sourceFiles[k]);
                }));
        }
        for (int k = b; k < end; ++k) {
            results.push_back(futures[k].get());
            ++done;
            if (cb) cb(done.load(), total);
        }
    }

    std::sort(results.begin(), results.end(),
              [](const CompareResult& a, const CompareResult& b){
                  return a.similarityScore > b.similarityScore;
              });
    return results;
}
