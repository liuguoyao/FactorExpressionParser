#include "fepCore.h"
#include "Lexer.h"
#include "Parser.h"
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
