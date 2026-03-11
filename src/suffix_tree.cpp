#include "suffix_tree.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <functional>
#include <stdexcept>

// ─── Ukkonen's suffix tree ────────────────────────────────────────────────────
// Reference: https://cp-algorithms.com/string/suffix-tree-ukkonen.html

static const int INF = INT_MAX / 2;

SuffixTree::SuffixTree(const std::string& text) : text_(text) {
    build();
}

SuffixTree::~SuffixTree() {
    for (auto* n : nodes_) delete n;
}

int SuffixTree::newNode(int start, int* end) {
    nodes_.push_back(new SuffixNode(start, end));
    return (int)nodes_.size() - 1;
}

void SuffixTree::build() {
    // Append sentinel
    text_ += '\0';
    int n = (int)text_.size();

    // Allocate root
    leafEnd_ = -1;
    root_    = newNode(-1, new int(-1));
    nodes_[root_]->suffixLink = root_;

    activeNode_   = root_;
    activeEdge_   = -1;
    activeLength_ = 0;
    remaining_    = 0;
    size_         = 0;

    for (int i = 0; i < n; ++i)
        extend(i);

    setSuffixIndexDFS(root_, 0);
}

void SuffixTree::extend(int pos) {
    ++leafEnd_;
    ++remaining_;

    int lastNewInternal = -1;

    while (remaining_ > 0) {
        if (activeLength_ == 0)
            activeEdge_ = pos;

        char ae = text_[activeEdge_];
        auto& children = nodes_[activeNode_]->children;

        if (children.find(ae) == children.end()) {
            // Rule 2: create new leaf
            int leaf = newNode(pos, new int(INF));
            *nodes_[leaf]->end = INF; // will be updated via leafEnd_ via pointer trick
            // We use a real end pointer shared for leaves
            delete nodes_[leaf]->end;
            nodes_[leaf]->end = &leafEnd_;
            children[ae] = leaf;

            if (lastNewInternal != -1) {
                nodes_[lastNewInternal]->suffixLink = root_;
                lastNewInternal = -1;
            }
        } else {
            int next = children[ae];
            int edgeLen = nodes_[next]->edgeLength();

            if (activeLength_ >= edgeLen) {
                activeEdge_ += edgeLen;
                activeLength_ -= edgeLen;
                activeNode_ = next;
                continue;
            }

            // Rule 3: current char already on edge
            if (text_[nodes_[next]->start + activeLength_] == text_[pos]) {
                ++activeLength_;
                if (lastNewInternal != -1)
                    nodes_[lastNewInternal]->suffixLink = activeNode_;
                break;
            }

            // Rule 2: split edge
            int split = newNode(nodes_[next]->start,
                                new int(nodes_[next]->start + activeLength_ - 1));
            children[ae] = split;

            int leaf = newNode(pos, new int(INF));
            delete nodes_[leaf]->end;
            nodes_[leaf]->end = &leafEnd_;
            nodes_[split]->children[text_[pos]] = leaf;

            nodes_[next]->start += activeLength_;
            nodes_[split]->children[text_[nodes_[next]->start]] = next;

            if (lastNewInternal != -1)
                nodes_[lastNewInternal]->suffixLink = split;
            lastNewInternal = split;
        }

        --remaining_;
        if (activeNode_ == root_ && activeLength_ > 0) {
            --activeLength_;
            activeEdge_ = pos - remaining_ + 1;
        } else if (nodes_[activeNode_]->suffixLink != 0) {
            activeNode_ = nodes_[activeNode_]->suffixLink;
        } else {
            activeNode_ = root_;
        }
    }
}

void SuffixTree::setSuffixIndexDFS(int node, int labelHeight) {
    if (node == -1) return;
    bool isLeaf = nodes_[node]->children.empty();
    if (isLeaf) {
        nodes_[node]->suffixIndex =
            (int)text_.size() - 1 - labelHeight; // -1 for sentinel
    }
    for (auto& [ch, child] : nodes_[node]->children) {
        setSuffixIndexDFS(child, labelHeight + nodes_[child]->edgeLength());
    }
}

// ─── Substring search (DFS walk) ─────────────────────────────────────────────
bool SuffixTree::hasSubstring(const std::string& pat) const {
    if (pat.empty()) return true;
    int cur = root_;
    size_t i = 0;
    while (i < pat.size()) {
        char c = pat[i];
        auto it = nodes_[cur]->children.find(c);
        if (it == nodes_[cur]->children.end()) return false;
        int child = it->second;
        int edgeStart = nodes_[child]->start;
        int edgeEnd   = std::min(*nodes_[child]->end, (int)text_.size() - 2);
        for (int j = edgeStart; j <= edgeEnd && i < pat.size(); ++j, ++i) {
            if (text_[j] != pat[i]) return false;
        }
        cur = child;
    }
    return true;
}

// ─── Longest Common Substring via Generalised suffix tree approach ────────────
// We concatenate textA + '$' + textB + '#' and mark which suffixes belong
// to which document during DFS.

int SuffixTree::longestCommonSubstring(const std::string& textB) const {
    // Build a generalised suffix tree over text_(A) + sep + textB
    std::string combined = text_;   // text_ already ends with '\0' sentinel
    combined.pop_back();            // remove sentinel
    combined += '$';
    int sepPos = (int)combined.size() - 1; // position of separator
    combined += textB;
    combined += '#';

    SuffixTree gen(combined.substr(0, combined.size() - 0)); // full string
    // Walk DFS tracking which documents each subtree contains
    // Return deepest internal node with leaves from both docs

    struct Frame { int node; int depth; };
    int best = 0;

    std::function<std::pair<bool,bool>(int,int)> dfs =
        [&](int node, int depth) -> std::pair<bool,bool> {
        bool hasA = false, hasB = false;
        if (gen.nodes_[node]->children.empty()) {
            // Leaf – determine which doc
            int si = gen.nodes_[node]->suffixIndex;
            hasA = (si <= sepPos);
            hasB = (si >  sepPos);
        } else {
            for (auto& [ch, child] : gen.nodes_[node]->children) {
                int childDepth = depth + gen.nodes_[child]->edgeLength();
                auto [cA, cB] = dfs(child, childDepth);
                hasA = hasA || cA;
                hasB = hasB || cB;
            }
        }
        if (hasA && hasB && node != gen.root_) {
            int nodeDepth = depth;
            if (nodeDepth > best) best = nodeDepth;
        }
        return {hasA, hasB};
    };

    dfs(gen.root_, 0);
    // Subtract 1 for the separator character
    return std::max(0, best - 1);
}

// ─── Find all matches ─────────────────────────────────────────────────────────
std::vector<MatchResult> SuffixTree::findMatches(const std::string& textB,
                                                  int minMatchLen) const {
    std::vector<MatchResult> results;

    // Sliding window: for each starting position in B, find longest match in A.
    // Complexity: O(|B| * |A|) worst-case but bounded by suffix tree depth.
    size_t i = 0;
    while (i < textB.size()) {
        // Try to extend match from position i in B
        int cur = root_;
        int matchLen = 0;
        int bestMatchStart = -1;
        size_t j = i;

        while (j < textB.size()) {
            char c = textB[j];
            auto it = nodes_[cur]->children.find(c);
            if (it == nodes_[cur]->children.end()) break;
            int child = it->second;
            int edgeStart = nodes_[child]->start;
            int edgeEnd   = std::min(*nodes_[child]->end, (int)text_.size() - 2);

            bool matched = true;
            for (int k = edgeStart; k <= edgeEnd && j < textB.size(); ++k, ++j) {
                if (text_[k] != textB[j]) { matched = false; break; }
                ++matchLen;
            }
            if (!matched) break;
            cur = child;
        }

        if (matchLen >= minMatchLen) {
            // Find suffix index by walking leaves under cur
            std::function<int(int)> firstLeaf = [&](int node) -> int {
                if (nodes_[node]->suffixIndex >= 0)
                    return nodes_[node]->suffixIndex;
                for (auto& [ch, child] : nodes_[node]->children) {
                    int r = firstLeaf(child);
                    if (r >= 0) return r;
                }
                return -1;
            };
            int si = firstLeaf(cur);
            if (si >= 0) {
                MatchResult mr;
                mr.startA  = si;
                mr.startB  = (int)i;
                mr.length  = matchLen;
                results.push_back(mr);
            }
            i += matchLen; // skip matched portion
        } else {
            ++i;
        }
    }

    std::sort(results.begin(), results.end(),
              [](const MatchResult& a, const MatchResult& b){
                  return a.length > b.length;
              });
    return results;
}
