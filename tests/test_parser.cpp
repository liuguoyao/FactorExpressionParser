#include <catch_amalgamated.hpp>

#include "Lexer.h"
#include "Parser.h"
#include <memory>

TEST_CASE("Parser handles simple number", "[parser]") {
    Lexer lex("42");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "42");
}

TEST_CASE("Parser handles identifier", "[parser]") {
    Lexer lex("close");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "close");
}

TEST_CASE("Parser handles addition", "[parser]") {
    Lexer lex("1 + 2");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "(1 + 2)");
}

TEST_CASE("Parser respects operator precedence", "[parser]") {
    Lexer lex("2 + 3 * 4");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "(2 + (3 * 4))");
}

TEST_CASE("Parser respects power right-associativity", "[parser]") {
    Lexer lex("2 ^ 3 ^ 4");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "(2 ^ (3 ^ 4))");
}

TEST_CASE("Parser handles parenthesised expressions", "[parser]") {
    Lexer lex("(1 + 2) * 3");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "((1 + 2) * 3)");
}

TEST_CASE("Parser handles unary minus", "[parser]") {
    Lexer lex("-x");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "(-x)");
}

TEST_CASE("Parser handles unary plus", "[parser]") {
    Lexer lex("+x");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "(+x)");
}

TEST_CASE("Parser handles function call no args", "[parser]") {
    Lexer lex("f()");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "f()");
}

TEST_CASE("Parser handles function call one arg", "[parser]") {
    Lexer lex("abs(x)");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "abs(x)");
}

TEST_CASE("Parser handles function call multiple args", "[parser]") {
    Lexer lex("ts_mean(close, 5)");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "ts_mean(close, 5)");
}

TEST_CASE("Parser handles nested function calls", "[parser]") {
    Lexer lex("rank(ts_mean(close, 5))");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "rank(ts_mean(close, 5))");
}

TEST_CASE("Parser handles comparison operators", "[parser]") {
    Lexer lex("close >= open");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "(close >= open)");
}

TEST_CASE("Parser handles chained comparisons", "[parser]") {
    Lexer lex("a < b == c > d");
    Parser parser(lex);
    auto ast = parser.parse();
    // All comparison ops have same precedence, left-assoc
    REQUIRE(ast->to_string() == "(((a < b) == c) > d)");
}

TEST_CASE("Parser handles ternary expression", "[parser]") {
    Lexer lex("a ? b : c");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "(a ? b : c)");
}

TEST_CASE("Parser handles sample alpha expression", "[parser]") {
    Lexer lex("rank(ts_mean(close, 5) / delay(close, 10))");
    Parser parser(lex);
    auto ast = parser.parse();
    REQUIRE(ast->to_string() == "rank((ts_mean(close, 5) / delay(close, 10)))");
}

TEST_CASE("Parser rejects empty input", "[parser]") {
    Lexer lex("");
    Parser parser(lex);
    REQUIRE_THROWS_AS(parser.parse(), std::runtime_error);
}

TEST_CASE("Parser rejects mismatched parens", "[parser]") {
    Lexer lex("(");
    Parser parser(lex);
    REQUIRE_THROWS_AS(parser.parse(), std::runtime_error);
}

TEST_CASE("Parser rejects trailing junk", "[parser]") {
    Lexer lex("1 2");
    Parser parser(lex);
    REQUIRE_THROWS_AS(parser.parse(), std::runtime_error);
}

TEST_CASE("Parser rejects call on non-identifier", "[parser]") {
    Lexer lex("(1 + 2)()");
    Parser parser(lex);
    REQUIRE_THROWS_AS(parser.parse(), std::runtime_error);
}
