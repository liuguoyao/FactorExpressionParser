#ifndef FEP_EXPR_H
#define FEP_EXPR_H

#include <memory>
#include <string>
#include <vector>

struct Expr {
    virtual ~Expr() = default;
    virtual std::string to_string() const = 0;
};

struct NumberExpr : Expr {
    double value;
    explicit NumberExpr(double v) : value(v) {}
    std::string to_string() const override {
        std::string s = std::to_string(value);
        // Remove trailing zeros
        auto dot = s.find('.');
        if (dot != std::string::npos) {
            auto end = s.find_last_not_of('0');
            if (end > dot) s.erase(end + 1);
            else           s.erase(dot); // ".0" -> ""
        }
        return s;
    }
};

struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(const std::string& n) : name(n) {}
    std::string to_string() const override { return name; }
};

struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(const std::string& o, std::unique_ptr<Expr> e)
        : op(o), operand(std::move(e)) {}
    std::string to_string() const override {
        return "(" + op + operand->to_string() + ")";
    }
};

struct BinaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::string o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    std::string to_string() const override {
        return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
    }
};

struct TernaryExpr : Expr {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> then_expr;
    std::unique_ptr<Expr> else_expr;
    TernaryExpr(std::unique_ptr<Expr> c, std::unique_ptr<Expr> t,
                std::unique_ptr<Expr> e)
        : cond(std::move(c)), then_expr(std::move(t)), else_expr(std::move(e)) {}
    std::string to_string() const override {
        return "(" + cond->to_string() + " ? " + then_expr->to_string() +
               " : " + else_expr->to_string() + ")";
    }
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(const std::string& c, std::vector<std::unique_ptr<Expr>> a)
        : callee(c), args(std::move(a)) {}
    std::string to_string() const override {
        std::string s = callee + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) s += ", ";
            s += args[i]->to_string();
        }
        s += ")";
        return s;
    }
};

#endif // FEP_EXPR_H
