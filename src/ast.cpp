#include "ast.h"
#include <string>
#include <vector>

std::string NumberExpr::to_string() const {
    std::string s = std::to_string(value);
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        auto end = s.find_last_not_of('0');
        if (end > dot) s.erase(end + 1);
        else           s.erase(dot);
    }
    return s;
}

std::string IdentExpr::to_string() const {
    return name;
}

std::string UnaryExpr::to_string() const {
    return "(" + op + operand->to_string() + ")";
}

std::string BinaryExpr::to_string() const {
    return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
}

std::string TernaryExpr::to_string() const {
    return "(" + cond->to_string() + " ? " + then_expr->to_string() +
           " : " + else_expr->to_string() + ")";
}

std::string CallExpr::to_string() const {
    std::string s = callee + "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) s += ", ";
        s += args[i]->to_string();
    }
    s += ")";
    return s;
}
