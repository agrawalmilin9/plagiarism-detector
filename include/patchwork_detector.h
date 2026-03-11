#pragma once
#include <string>
#include <vector>

struct PatchworkSegment {
    int  sourceStart;   // token index in the source document
    int  sourceEnd;
    int  suspectStart;  // token index in the suspect document
    int  suspectEnd;
    std::string sourceFile;
    double overlapRatio; // fraction of suspect tokens covered
};

struct PatchworkReport {
    std::string suspectFile;
    std::vector<PatchworkSegment> segments;
    double totalCoverage;  // 0-1: fraction of suspect covered by stitched segments
    bool   isPatchwork;    // true when coverage exceeds threshold
};

class PatchworkDetector {
public:
    // minSegmentLen  : minimum token-length of a segment to count.
    // coverageThresh : fraction of suspect tokens that must be covered to flag.
    // overlapAllowed : max fractional overlap between consecutive segments.
    explicit PatchworkDetector(int minSegmentLen   = 8,
                               double coverageThresh = 0.40,
                               double overlapAllowed = 0.10);

    // Compare one suspect document against multiple source documents.
    // sourceTokens: list of (filename, token-string) pairs.
    PatchworkReport analyze(
        const std::string& suspectFile,
        const std::string& suspectTokenStr,
        const std::vector<std::pair<std::string,std::string>>& sources) const;

private:
    int    minSegmentLen_;
    double coverageThresh_;
    double overlapAllowed_;

    // Mark which token positions in the suspect are covered by stitched segments.
    std::vector<bool> buildCoverageMap(
        int suspectLen,
        const std::vector<PatchworkSegment>& segs) const;
};
