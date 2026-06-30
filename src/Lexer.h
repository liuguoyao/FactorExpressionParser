#ifndef FEP_LEXER_H
#define FEP_LEXER_H

#include "Token.h"
#include <string>

class Lexer {
public:
    explicit Lexer(const std::string& src);

    /// Return the current token without consuming it.
    const Token& peek() const;

    /// Consume and return the current token, then advance.
    Token consume();

    /// Consume the current token if it matches the given type.
    /// Returns true if matched, false otherwise (token not consumed).
    bool consume_if(TokenType type);

private:
    std::string src_;
    size_t pos_;
    Token current_;

    void advance();
    void skip_whitespace();
    Token read_number();
    Token read_identifier_or_keyword();
};

#endif // FEP_LEXER_H
