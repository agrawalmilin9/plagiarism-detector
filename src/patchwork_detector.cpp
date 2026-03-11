#include "patchwork_detector.h"
#include "suffix_tree.h"
#include <algorithm>
#include <numeric>

PatchworkDetector::PatchworkDetector(int minSegmentLen,
                                     double coverageThresh,
                                     double overlapAllowed)
    : minSegmentLen_(minSegmentLen)
    , coverageThresh_(coverageThresh)
    , overlapAllowed_(overlapAllowed)
{}

// Build a boolean coverage map over the suspect tokens.
std::vector<bool> PatchworkDetector::buildCoverageMap(
        int suspectLen,
        const std::vector<PatchworkSegment>& segs) const {
    std::vector<bool> covered(suspectLen, false);
    for (const auto& seg : segs) {
        int lo = std::max(0, seg.suspectStart);
        int hi = std::min(suspectLen - 1, seg.suspectEnd);
        for (int i = lo; i <= hi; ++i) covered[i] = true;
    }
    return covered;
}

PatchworkReport PatchworkDetector::analyze(
        const std::string& suspectFile,
        const std::string& suspectTokenStr,
        const std::vector<std::pair<std::string,std::string>>& sources) const {

    PatchworkReport report;
    report.suspectFile  = suspectFile;
    report.isPatchwork  = false;

    if (suspectTokenStr.empty()) return report;

    // Count suspect tokens (space-separated)
    int suspectTokenCount = 1;
    for (char c : suspectTokenStr)
        if (c == ' ') ++suspectTokenCount;

    // Collect segments from every source file
    std::vector<PatchworkSegment> allSegments;

    for (const auto& [srcFile, srcTokenStr] : sources) {
        if (srcTokenStr.empty()) continue;

        SuffixTree tree(srcTokenStr);
        auto matches = tree.findMatches(suspectTokenStr, minSegmentLen_);

        for (const auto& m : matches) {
            if (m.length < minSegmentLen_) continue;

            // Convert char offsets → token indices
            auto charToTok = [](const std::string& s, int charPos) -> int {
                int tok = 0;
                for (int i = 0; i < charPos && i < (int)s.size(); ++i)
                    if (s[i] == ' ') ++tok;
                return tok;
            };

            PatchworkSegment seg;
            seg.sourceFile    = srcFile;
            seg.sourceStart   = charToTok(srcTokenStr,   m.startA);
            seg.sourceEnd     = charToTok(srcTokenStr,   m.startA + m.length);
            seg.suspectStart  = charToTok(suspectTokenStr, m.startB);
            seg.suspectEnd    = charToTok(suspectTokenStr, m.startB + m.length);
            int segLen        = seg.suspectEnd - seg.suspectStart + 1;
            seg.overlapRatio  = (double)segLen / suspectTokenCount;
            allSegments.push_back(seg);
        }
    }

    // Sort by suspect start position
    std::sort(allSegments.begin(), allSegments.end(),
              [](const PatchworkSegment& a, const PatchworkSegment& b){
                  return a.suspectStart < b.suspectStart;
              });

    // Filter overlapping segments: greedily keep non-overlapping ones
    std::vector<PatchworkSegment> filtered;
    int lastEnd = -1;
    for (const auto& seg : allSegments) {
        int allowedOverlap = (int)((seg.suspectEnd - seg.suspectStart) *
                                   overlapAllowed_);
        if (seg.suspectStart > lastEnd - allowedOverlap) {
            filtered.push_back(seg);
            lastEnd = seg.suspectEnd;
        }
    }

    report.segments = filtered;

    // Compute total coverage
    auto covMap = buildCoverageMap(suspectTokenCount, filtered);
    int coveredCount = (int)std::count(covMap.begin(), covMap.end(), true);
    report.totalCoverage = (suspectTokenCount > 0)
                           ? (double)coveredCount / suspectTokenCount
                           : 0.0;
    report.isPatchwork = (report.totalCoverage >= coverageThresh_)
                         && (filtered.size() >= 2); // at least 2 sources

    return report;
}
