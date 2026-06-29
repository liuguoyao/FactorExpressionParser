#include "Lexer.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& src)
    : src_(src), pos_(0) {
    advance();
}

const Token& Lexer::peek() const {
    return current_;
}

Token Lexer::consume() {
    Token tok = current_;
    advance();
    return tok;
}

bool Lexer::consume_if(TokenType type) {
    if (current_.type == type) {
        advance();
        return true;
    }
    return false;
}

void Lexer::skip_whitespace() {
    while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t' ||
                                  src_[pos_] == '\n' || src_[pos_] == '\r')) {
        ++pos_;
    }
}

void Lexer::advance() {
    skip_whitespace();
    if (pos_ >= src_.size()) {
        current_ = Token(TokenType::END, "", pos_);
        return;
    }

    char c = src_[pos_];

    // Number literal
    if (std::isdigit(c) || (c == '.' && pos_ + 1 < src_.size() &&
                            std::isdigit(src_[pos_ + 1]))) {
        current_ = read_number();
        return;
    }

    // Identifier (letter or underscore)
    if (std::isalpha(c) || c == '_') {
        current_ = read_identifier_or_keyword();
        return;
    }

    // Multi-character operators
    size_t start = pos_;
    ++pos_;

    switch (c) {
    case '+': current_ = Token(TokenType::PLUS,   "+", start); break;
    case '-': current_ = Token(TokenType::MINUS,  "-", start); break;
    case '*': current_ = Token(TokenType::STAR,   "*", start); break;
    case '/': current_ = Token(TokenType::SLASH,  "/", start); break;
    case '^': current_ = Token(TokenType::CARET,  "^", start); break;
    case '(': current_ = Token(TokenType::LPAREN,  "(", start); break;
    case ')': current_ = Token(TokenType::RPAREN,  ")", start); break;
    case ',': current_ = Token(TokenType::COMMA,   ",", start); break;
    case '?': current_ = Token(TokenType::QUESTION,"?", start); break;
    case ':': current_ = Token(TokenType::COLON,   ":", start); break;
    case '<':
        if (pos_ < src_.size() && src_[pos_] == '=') {
            ++pos_;
            current_ = Token(TokenType::LE, "<=", start);
        } else {
            current_ = Token(TokenType::LT, "<", start);
        }
        break;
    case '>':
        if (pos_ < src_.size() && src_[pos_] == '=') {
            ++pos_;
            current_ = Token(TokenType::GE, ">=", start);
        } else {
            current_ = Token(TokenType::GT, ">", start);
        }
        break;
    case '=':
        if (pos_ < src_.size() && src_[pos_] == '=') {
            ++pos_;
            current_ = Token(TokenType::EQ, "==", start);
        } else {
            throw std::runtime_error("unexpected '=' at position " +
                                     std::to_string(start));
        }
        break;
    case '!':
        if (pos_ < src_.size() && src_[pos_] == '=') {
            ++pos_;
            current_ = Token(TokenType::NE, "!=", start);
        } else {
            throw std::runtime_error("unexpected '!' at position " +
                                     std::to_string(start));
        }
        break;
    default:
        throw std::runtime_error(std::string("unexpected character '") +
                                 c + "' at position " + std::to_string(start));
    }
}

Token Lexer::read_number() {
    size_t start = pos_;
    while (pos_ < src_.size() && std::isdigit(src_[pos_])) ++pos_;
    bool is_double = false;
    if (pos_ < src_.size() && src_[pos_] == '.') {
        is_double = true;
        ++pos_;
        while (pos_ < src_.size() && std::isdigit(src_[pos_])) ++pos_;
    }
    std::string num = src_.substr(start, pos_ - start);
    Token tok(TokenType::NUMBER, num, start);
    tok.value = std::stod(num);
    return tok;
}

Token Lexer::read_identifier_or_keyword() {
    size_t start = pos_;
    while (pos_ < src_.size() &&
           (std::isalnum(src_[pos_]) || src_[pos_] == '_')) {
        ++pos_;
    }
    std::string id = src_.substr(start, pos_ - start);
    return Token(TokenType::IDENTIFIER, id, start);
}
