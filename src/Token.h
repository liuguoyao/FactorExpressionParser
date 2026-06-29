#ifndef FEP_TOKEN_H
#define FEP_TOKEN_H

#include <string>

enum class TokenType {
    END,
    NUMBER,
    IDENTIFIER,
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    CARET,      // ^
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    EQ,         // ==
    NE,         // !=
    LPAREN,     // (
    RPAREN,     // )
    COMMA,      // ,
    QUESTION,   // ?
    COLON,      // :
};

struct Token {
    TokenType type;
    std::string text;
    double value = 0.0; // valid when type == NUMBER
    size_t pos = 0;     // position in source string

    Token() : type(TokenType::END), pos(0) {}
    Token(TokenType t, const std::string& txt, size_t p)
        : type(t), text(txt), pos(p) {}
};

#endif // FEP_TOKEN_H
