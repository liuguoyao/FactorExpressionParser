#ifndef FEP_AST_H
#define FEP_AST_H

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
    std::string to_string() const override;
};

struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(const std::string& n) : name(n) {}
    std::string to_string() const override;
};

struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(const std::string& o, std::unique_ptr<Expr> e)
        : op(o), operand(std::move(e)) {}
    std::string to_string() const override;
};

struct BinaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::string o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    std::string to_string() const override;
};

struct TernaryExpr : Expr {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> then_expr;
    std::unique_ptr<Expr> else_expr;
    TernaryExpr(std::unique_ptr<Expr> c, std::unique_ptr<Expr> t,
                std::unique_ptr<Expr> e)
        : cond(std::move(c)), then_expr(std::move(t)), else_expr(std::move(e)) {}
    std::string to_string() const override;
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(const std::string& c, std::vector<std::unique_ptr<Expr>> a)
        : callee(c), args(std::move(a)) {}
    std::string to_string() const override;
};

#endif // FEP_AST_H
