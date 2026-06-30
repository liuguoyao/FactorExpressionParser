#ifndef FEP_EXPR_H
#define FEP_EXPR_H

#include <cmath>
#include <map>
#include <memory>
#include <stdexcept>
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

/// Evaluator over an AST treating each value as a 1-D time series
/// (std::vector<double>), since every variable such as `close` denotes "a
/// period of" that field. A bare number literal evaluates to a length-1 series.
///
/// Element-wise operators/functions act point-wise, with numpy-style
/// broadcasting: a length-1 operand is replicated to the other side's length.
/// Time-series functions (delay/delta/ts_sum/ts_mean/ts_std) roll a window
/// along the time axis; positions whose window is not yet full yield NaN.
/// Cross-sectional functions (rank/zscore/scale/group_*/ind_neutralize) have
/// no meaning over a single series and throw.
class Evaluator {
public:
    using Vec = std::vector<double>;

    explicit Evaluator(const std::map<std::string, Vec>& symbols)
        : symbols_(symbols) {}

    Vec eval(const Expr& expr) const { return eval_expr(expr); }

private:
    const std::map<std::string, Vec>& symbols_;

    static double sign(double x) {
        if (x > 0.0) return 1.0;
        if (x < 0.0) return -1.0;
        return 0.0;
    }

    // Broadcast two series to a common length (numpy rules): equal lengths are
    // fine; a length-1 operand replicates to the other side's length.
    static void broadcast(Vec& a, Vec& b) {
        if (a.size() == b.size()) return;
        if (a.size() == 1) { double x = a[0]; a.assign(b.size(), x); return; }
        if (b.size() == 1) { double x = b[0]; b.assign(a.size(), x); return; }
        throw std::runtime_error("length mismatch in binary op: " +
                                 std::to_string(a.size()) + " vs " +
                                 std::to_string(b.size()));
    }

    // Extract an integer window parameter from a (length-1) series.
    static int as_window(const Vec& v, const std::string& fname) {
        if (v.size() != 1) {
            throw std::runtime_error("function '" + fname +
                                     "' expects a scalar window argument, got length " +
                                     std::to_string(v.size()));
        }
        double d = v[0];
        if (d < 0.0 || d != static_cast<int>(d)) {
            throw std::runtime_error("function '" + fname +
                                     "' expects a non-negative integer window, got " +
                                     std::to_string(d));
        }
        return static_cast<int>(d);
    }

    // --- time-series primitives -------------------------------------------

    static Vec ts_delay(const Vec& x, int d) {
        Vec r(x.size(), std::nan(""));
        for (size_t i = 0; i < x.size(); ++i)
            if (static_cast<int>(i) >= d) r[i] = x[i - d];
        return r;
    }

    static Vec ts_delta(const Vec& x, int d) {
        Vec r(x.size(), std::nan(""));
        for (size_t i = 0; i < x.size(); ++i)
            if (static_cast<int>(i) >= d) r[i] = x[i] - x[i - d];
        return r;
    }

    static Vec ts_sum(const Vec& x, int d) {
        Vec r(x.size(), std::nan(""));
        double win = 0.0;
        for (size_t i = 0; i < x.size(); ++i) {
            win += x[i];
            if (static_cast<int>(i) >= d - 1) {
                r[i] = win;
                win -= x[i - d + 1]; // element leaving the window
            }
        }
        return r;
    }

    Vec eval_expr(const Expr& expr) const {
        if (auto* p = dynamic_cast<const NumberExpr*>(&expr)) {
            return Vec{p->value};
        }
        if (auto* p = dynamic_cast<const IdentExpr*>(&expr)) {
            auto it = symbols_.find(p->name);
            if (it == symbols_.end()) {
                throw std::runtime_error("undefined variable '" + p->name + "'");
            }
            return it->second;
        }
        if (auto* p = dynamic_cast<const UnaryExpr*>(&expr)) {
            Vec v = eval_expr(*p->operand);
            if (p->op == "-") for (double& x : v) x = -x;
            return v;
        }
        if (auto* p = dynamic_cast<const BinaryExpr*>(&expr)) {
            Vec l = eval_expr(*p->left);
            Vec r = eval_expr(*p->right);
            broadcast(l, r);
            const std::string& op = p->op;
            for (size_t i = 0; i < l.size(); ++i) {
                if      (op == "+")  l[i] = l[i] + r[i];
                else if (op == "-")  l[i] = l[i] - r[i];
                else if (op == "*")  l[i] = l[i] * r[i];
                else if (op == "/")  l[i] = l[i] / r[i];
                else if (op == "^")  l[i] = std::pow(l[i], r[i]);
                else if (op == ">")  l[i] = l[i] >  r[i] ? 1.0 : 0.0;
                else if (op == "<")  l[i] = l[i] <  r[i] ? 1.0 : 0.0;
                else if (op == ">=") l[i] = l[i] >= r[i] ? 1.0 : 0.0;
                else if (op == "<=") l[i] = l[i] <= r[i] ? 1.0 : 0.0;
                else if (op == "==") l[i] = l[i] == r[i] ? 1.0 : 0.0;
                else if (op == "!=") l[i] = l[i] != r[i] ? 1.0 : 0.0;
                else throw std::runtime_error("unknown binary operator '" + op + "'");
            }
            return l;
        }
        if (auto* p = dynamic_cast<const TernaryExpr*>(&expr)) {
            Vec c = eval_expr(*p->cond);
            Vec t = eval_expr(*p->then_expr);
            Vec e = eval_expr(*p->else_expr);
            broadcast(c, t); broadcast(c, e); // common length == c.size()
            for (size_t i = 0; i < c.size(); ++i)
                t[i] = c[i] != 0.0 ? t[i] : e[i];
            return t;
        }
        if (auto* p = dynamic_cast<const CallExpr*>(&expr)) {
            return eval_call(*p);
        }
        throw std::runtime_error("unknown AST node type");
    }

    Vec eval_call(const CallExpr& node) const {
        const std::vector<std::unique_ptr<Expr>>& a = node.args;
        const std::string& name = node.callee;

        // Element-wise unary math.
        if (name == "abs" || name == "log" || name == "exp" ||
            name == "sqrt" || name == "sign") {
            if (a.size() != 1)
                throw std::runtime_error("function '" + name +
                                         "' expects 1 argument, got " +
                                         std::to_string(a.size()));
            Vec v = eval_expr(*a[0]);
            for (double& x : v) {
                if      (name == "abs")  x = std::fabs(x);
                else if (name == "log")  x = std::log(x);
                else if (name == "exp")  x = std::exp(x);
                else if (name == "sqrt") x = std::sqrt(x);
                else if (name == "sign") x = sign(x);
            }
            return v;
        }
        // signedpower(x, a) — a is a scalar exponent, applied point-wise.
        if (name == "signedpower") {
            if (a.size() != 2)
                throw std::runtime_error(
                    "function 'signedpower' expects 2 arguments, got " +
                    std::to_string(a.size()));
            Vec v = eval_expr(*a[0]);
            double pw = as_window(eval_expr(*a[1]), name); // reuse scalar check
            for (double& x : v) x = sign(x) * std::pow(std::fabs(x), pw);
            return v;
        }

        // Time-series functions.
        if (name == "delay" || name == "delta") {
            if (a.size() != 2)
                throw std::runtime_error("function '" + name +
                                         "' expects 2 arguments, got " +
                                         std::to_string(a.size()));
            Vec v = eval_expr(*a[0]);
            int d = as_window(eval_expr(*a[1]), name);
            return (name == "delay") ? ts_delay(v, d) : ts_delta(v, d);
        }
        if (name == "ts_sum" || name == "ts_mean" || name == "ts_std") {
            if (a.size() != 2)
                throw std::runtime_error("function '" + name +
                                         "' expects 2 arguments, got " +
                                         std::to_string(a.size()));
            Vec v = eval_expr(*a[0]);
            int d = as_window(eval_expr(*a[1]), name);
            Vec s = ts_sum(v, d);
            if (name == "ts_sum") return s;
            for (size_t i = 0; i < s.size(); ++i) {
                if (std::isnan(s[i])) continue; // window not yet full
                if (name == "ts_mean") {
                    s[i] /= d;
                } else { // ts_std — population std over the window
                    double m = s[i] / d;
                    double win2 = 0.0;
                    for (int k = 0; k < d; ++k) {
                        double y = v[i - k];
                        win2 += y * y;
                    }
                    double var = win2 / d - m * m;
                    s[i] = var <= 0.0 ? 0.0 : std::sqrt(var);
                }
            }
            return s;
        }

        // Cross-sectional functions have no meaning over a single series.
        throw std::runtime_error("function '" + name +
                                 "' not implemented (cross-sectional)");
    }
};

#endif // FEP_EXPR_H
