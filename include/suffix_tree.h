#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// ─── Ukkonen's online suffix tree ────────────────────────────────────────────
// Builds in O(n) time; used for substring matching between two documents.

struct SuffixNode {
    std::unordered_map<char, int> children; // char → child node index
    int suffixLink = 0;
    int start;           // edge label start in the text
    int* end;            // edge label end (pointer allows leaf trick)
    int suffixIndex = -1;// filled during DFS; -1 for internal nodes

    SuffixNode(int start, int* end) : start(start), end(end) {}
    int edgeLength() const { return *end - start + 1; }
};

struct MatchResult {
    int  startA;    // token offset in document A
    int  startB;    // token offset in document B
    int  length;    // match length in tokens
};

class SuffixTree {
public:
    explicit SuffixTree(const std::string& text);
    ~SuffixTree();

    // Find all common substrings of document B that also appear in this tree.
    // Returns matches sorted by length descending.
    std::vector<MatchResult> findMatches(const std::string& textB,
                                         int minMatchLen = 5) const;

    // Longest Common Substring length (fast path).
    int longestCommonSubstring(const std::string& textB) const;

    const std::string& getText() const { return text_; }

private:
    std::string text_;
    std::vector<SuffixNode*> nodes_;
    int root_ = 0;
    int leafEnd_ = -1; // global end shared by all leaves

    // Ukkonen builder state
    int  activeNode_   = 0;
    int  activeEdge_   = -1;
    int  activeLength_ = 0;
    int  remaining_    = 0;
    int  size_         = 0;

    int  newNode(int start, int* end);
    void extend(int pos);
    void build();
    void setSuffixIndexDFS(int node, int labelHeight);

    // Search helpers
    bool hasSubstring(const std::string& pattern) const;
    int  walkDown(int node) const;
};
