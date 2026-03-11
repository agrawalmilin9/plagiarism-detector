#include "tokenizer.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

// ─── C++ keyword set ─────────────────────────────────────────────────────────
static const std::unordered_set<std::string> CPP_KEYWORDS = {
    "alignas","alignof","and","and_eq","asm","auto","bitand","bitor","bool",
    "break","case","catch","char","char8_t","char16_t","char32_t","class",
    "compl","concept","const","consteval","constexpr","constinit","const_cast",
    "continue","co_await","co_return","co_yield","decltype","default","delete",
    "do","double","dynamic_cast","else","enum","explicit","export","extern",
    "false","float","for","friend","goto","if","inline","int","long","mutable",
    "namespace","new","noexcept","not","not_eq","nullptr","operator","or",
    "or_eq","private","protected","public","register","reinterpret_cast",
    "requires","return","short","signed","sizeof","static","static_assert",
    "static_cast","struct","switch","template","this","thread_local","throw",
    "true","try","typedef","typeid","typename","union","unsigned","using",
    "virtual","void","volatile","wchar_t","while","xor","xor_eq","override",
    "final","import","module"
};

bool Tokenizer::isCppKeyword(const std::string& w) {
    return CPP_KEYWORDS.count(w) > 0;
}

bool Tokenizer::isCppFile(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "cpp" || ext == "cc" || ext == "cxx" ||
           ext == "c"   || ext == "h"  || ext == "hpp" || ext == "hxx";
}

// ─── C++ tokenizer ────────────────────────────────────────────────────────────
std::vector<Token> Tokenizer::tokenizeCpp(const std::string& src) {
    std::vector<Token> tokens;
    size_t i = 0, n = src.size();
    int    line = 1;

    auto push = [&](std::string val, TokenType t) {
        tokens.push_back({std::move(val), t, line});
    };

    while (i < n) {
        char c = src[i];

        // Newline
        if (c == '\n') { ++line; ++i; continue; }

        // Whitespace
        if (std::isspace((unsigned char)c)) { ++i; continue; }

        // Single-line comment
        if (c == '/' && i + 1 < n && src[i+1] == '/') {
            while (i < n && src[i] != '\n') ++i;
            continue;
        }

        // Multi-line comment
        if (c == '/' && i + 1 < n && src[i+1] == '*') {
            i += 2;
            while (i + 1 < n && !(src[i] == '*' && src[i+1] == '/')) {
                if (src[i] == '\n') ++line;
                ++i;
            }
            i += 2; // skip */
            continue;
        }

        // Preprocessor directive – treat as one token
        if (c == '#') {
            std::string pp;
            while (i < n && src[i] != '\n') pp += src[i++];
            // Normalise: keep only the directive name (#include, #define…)
            std::istringstream ss(pp);
            std::string directive; ss >> directive;
            push(directive, TokenType::PREPROCESSOR);
            continue;
        }

        // String literal
        if (c == '"' || c == '\'') {
            char delim = c; ++i;
            while (i < n && src[i] != delim) {
                if (src[i] == '\\') ++i; // skip escape
                if (i < n && src[i] == '\n') ++line;
                ++i;
            }
            ++i; // closing quote
            push("STR_LIT", TokenType::LITERAL);
            continue;
        }

        // Number literal
        if (std::isdigit((unsigned char)c) ||
            (c == '.' && i+1 < n && std::isdigit((unsigned char)src[i+1]))) {
            while (i < n && (std::isalnum((unsigned char)src[i]) || src[i] == '.' ||
                             src[i] == '_' || src[i] == 'x' || src[i] == 'X'))
                ++i;
            push("NUM_LIT", TokenType::LITERAL);
            continue;
        }

        // Identifier / keyword
        if (std::isalpha((unsigned char)c) || c == '_') {
            std::string word;
            while (i < n && (std::isalnum((unsigned char)src[i]) || src[i] == '_'))
                word += src[i++];
            TokenType t = isCppKeyword(word) ? TokenType::KEYWORD
                                             : TokenType::IDENTIFIER;
            push(word, t);
            continue;
        }

        // Operators / punctuation
        push(std::string(1, c), TokenType::OPERATOR);
        ++i;
    }
    return tokens;
}

// ─── Plain-text tokenizer ────────────────────────────────────────────────────
std::vector<Token> Tokenizer::tokenizePlainText(const std::string& src) {
    std::vector<Token> tokens;
    int line = 1;
    size_t i = 0, n = src.size();
    while (i < n) {
        if (src[i] == '\n') { ++line; ++i; continue; }
        if (std::isspace((unsigned char)src[i])) { ++i; continue; }
        if (std::isalnum((unsigned char)src[i])) {
            std::string word;
            while (i < n && std::isalnum((unsigned char)src[i]))
                word += src[i++];
            // Lowercase for case-insensitive comparison
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            tokens.push_back({word, TokenType::IDENTIFIER, line});
        } else {
            tokens.push_back({std::string(1, src[i]), TokenType::PUNCTUATION, line});
            ++i;
        }
    }
    return tokens;
}

// ─── Public API ──────────────────────────────────────────────────────────────
std::vector<Token> Tokenizer::tokenize(const std::string& source, bool isCpp) {
    return isCpp ? tokenizeCpp(source) : tokenizePlainText(source);
}

std::vector<Token> Tokenizer::tokenizeFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::string src((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
    bool cpp = isCppFile(path);
    return tokenize(src, cpp);
}

std::string Tokenizer::tokensToString(const std::vector<Token>& tokens) {
    std::string result;
    result.reserve(tokens.size() * 8);
    for (const auto& t : tokens) {
        if (!result.empty()) result += ' ';
        result += t.value;
    }
    return result;
}
