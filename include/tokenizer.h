#pragma once
#include <string>
#include <vector>

// Token types
enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    LITERAL,
    OPERATOR,
    PUNCTUATION,
    PREPROCESSOR,
    UNKNOWN
};

struct Token {
    std::string value;
    TokenType   type;
    int         line;   // original source line (for reporting)
};

class Tokenizer {
public:
    // Tokenize a raw source file (C++ or plain text).
    // For C++ files: strips comments, normalises whitespace, maps keywords.
    // For plain text: splits on whitespace / punctuation.
    static std::vector<Token> tokenize(const std::string& source, bool isCpp);

    // Read a file from disk and tokenize it.
    static std::vector<Token> tokenizeFile(const std::string& path);

    // Convert a token sequence to a normalised string (space-separated values).
    static std::string tokensToString(const std::vector<Token>& tokens);

private:
    static bool isCppFile(const std::string& path);
    static std::vector<Token> tokenizeCpp(const std::string& src);
    static std::vector<Token> tokenizePlainText(const std::string& src);
    static bool isCppKeyword(const std::string& word);
};
