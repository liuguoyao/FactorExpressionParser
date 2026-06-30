#include "fepCore.h"
#include "Lexer.h"
#include "Parser.h"
#include "Expr.h"
#include <stdexcept>

std::string fep(const std::string& expr) {
    try {
        Lexer lexer(expr);
        Parser parser(lexer);
        auto ast = parser.parse();
        return ast->to_string();
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

std::vector<double> eval(const std::string& expr,
            const std::map<std::string, std::vector<double>>& symbols) {
    Lexer lexer(expr);
    Parser parser(lexer);
    auto ast = parser.parse();
    Evaluator ev(symbols);
    return ev.eval(*ast);
}
