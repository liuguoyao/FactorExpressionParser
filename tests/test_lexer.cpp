#include <catch_amalgamated.hpp>

#include "Token.h"
#include "Lexer.h"

TEST_CASE("Lexer handles numbers", "[lexer]") {
    Lexer lex("42");
    REQUIRE(lex.peek().type == TokenType::NUMBER);
    REQUIRE(lex.peek().value == 42.0);

    auto tok = lex.consume();
    REQUIRE(tok.text == "42");

    REQUIRE(lex.peek().type == TokenType::END);
}

TEST_CASE("Lexer handles floating point numbers", "[lexer]") {
    Lexer lex("3.14");
    REQUIRE(lex.peek().type == TokenType::NUMBER);
    auto tok = lex.consume();
    REQUIRE(tok.text == "3.14");
    REQUIRE(tok.value == Catch::Approx(3.14));
}

TEST_CASE("Lexer handles identifiers", "[lexer]") {
    Lexer lex("close");
    REQUIRE(lex.peek().type == TokenType::IDENTIFIER);
    REQUIRE(lex.peek().text == "close");
}

TEST_CASE("Lexer handles identifiers with underscores and digits", "[lexer]") {
    Lexer lex("ts_mean_5");
    REQUIRE(lex.peek().type == TokenType::IDENTIFIER);
    REQUIRE(lex.peek().text == "ts_mean_5");
}

TEST_CASE("Lexer handles single-character operators", "[lexer]") {
    Lexer lex("+-*/^(),?:");
    REQUIRE(lex.consume().type == TokenType::PLUS);
    REQUIRE(lex.consume().type == TokenType::MINUS);
    REQUIRE(lex.consume().type == TokenType::STAR);
    REQUIRE(lex.consume().type == TokenType::SLASH);
    REQUIRE(lex.consume().type == TokenType::CARET);
    REQUIRE(lex.consume().type == TokenType::LPAREN);
    REQUIRE(lex.consume().type == TokenType::RPAREN);
    REQUIRE(lex.consume().type == TokenType::COMMA);
    REQUIRE(lex.consume().type == TokenType::QUESTION);
    REQUIRE(lex.consume().type == TokenType::COLON);
    REQUIRE(lex.peek().type == TokenType::END);
}

TEST_CASE("Lexer handles comparison operators", "[lexer]") {
    Lexer lex("< > <= >= == !=");
    REQUIRE(lex.consume().type == TokenType::LT);
    REQUIRE(lex.consume().type == TokenType::GT);
    REQUIRE(lex.consume().type == TokenType::LE);
    REQUIRE(lex.consume().type == TokenType::GE);
    REQUIRE(lex.consume().type == TokenType::EQ);
    REQUIRE(lex.consume().type == TokenType::NE);
    REQUIRE(lex.peek().type == TokenType::END);
}

TEST_CASE("Lexer skips whitespace", "[lexer]") {
    Lexer lex("  close  +  volume  ");
    REQUIRE(lex.consume().text == "close");
    REQUIRE(lex.consume().type == TokenType::PLUS);
    REQUIRE(lex.consume().text == "volume");
    REQUIRE(lex.peek().type == TokenType::END);
}

TEST_CASE("Lexer rejects invalid character", "[lexer]") {
    REQUIRE_THROWS_AS(Lexer("@"), std::runtime_error);
}

TEST_CASE("Lexer rejects single '='", "[lexer]") {
    REQUIRE_THROWS_AS(Lexer("="), std::runtime_error);
}

TEST_CASE("Lexer rejects single '!'", "[lexer]") {
    REQUIRE_THROWS_AS(Lexer("!"), std::runtime_error);
}

TEST_CASE("Lexer handles complex expression", "[lexer]") {
    Lexer lex("rank(ts_mean(close, 5))");
    REQUIRE(lex.consume().text == "rank");
    REQUIRE(lex.consume().type == TokenType::LPAREN);
    REQUIRE(lex.consume().text == "ts_mean");
    REQUIRE(lex.consume().type == TokenType::LPAREN);
    REQUIRE(lex.consume().text == "close");
    REQUIRE(lex.consume().type == TokenType::COMMA);
    REQUIRE(lex.consume().type == TokenType::NUMBER);
    REQUIRE(lex.consume().type == TokenType::RPAREN);
    REQUIRE(lex.consume().type == TokenType::RPAREN);
    REQUIRE(lex.peek().type == TokenType::END);
}
