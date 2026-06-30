#include "fepCore.h"
#include "Lexer.h"
#include "Parser.h"
#include "Expr.h"

MatrixPtr eval_panel(const std::string& expr,
                     const std::map<std::string, MatrixPtr>& symbols,
                     int T, int N) {
    Lexer lexer(expr);
    Parser parser(lexer);
    auto ast = parser.parse();
    PanelEvaluator evaluator(symbols, T, N);
    return evaluator.eval(*ast);
}
