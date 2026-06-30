#include "factor_evaluator.h"
#include "ast.h"
#include "panel_types.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

// ---- helpers -------------------------------------------------------------

float PanelEvaluator::sign(float x) {
    if (x > 0.0f) return 1.0f;
    if (x < 0.0f) return -1.0f;
    return 0.0f;
}

float PanelEvaluator::nan() {
    return std::numeric_limits<float>::quiet_NaN();
}

MatrixPtr PanelEvaluator::make_matrix(int T, int N) {
    return std::make_shared<Matrix>(static_cast<size_t>(T) * N,
                                    std::numeric_limits<float>::quiet_NaN());
}

MatrixPtr PanelEvaluator::make_const_matrix(int T, int N, float val) {
    return std::make_shared<Matrix>(static_cast<size_t>(T) * N, val);
}

int PanelEvaluator::as_window(const Matrix& m, const std::string& fname) {
    if (m.empty()) {
        throw std::runtime_error("function '" + fname +
                                 "' got empty matrix as window argument");
    }
    double d = static_cast<double>(m[0]);
    if (d < 0.0 || d != static_cast<int>(d)) {
        throw std::runtime_error("function '" + fname +
                                 "' expects a non-negative integer window, got " +
                                 std::to_string(d));
    }
    return static_cast<int>(d);
}

MatrixPtr PanelEvaluator::eval(const Expr& expr) const {
    return eval_expr(expr);
}

PanelEvaluator::PanelEvaluator(const std::map<std::string, MatrixPtr>& symbols,
                               int T, int N)
    : symbols_(symbols), T_(T), N_(N) {}

// ---- dispatch ------------------------------------------------------------

MatrixPtr PanelEvaluator::eval_expr(const Expr& expr) const {
    if (auto* p = dynamic_cast<const NumberExpr*>(&expr)) {
        auto m = make_const_matrix(T_, N_, static_cast<float>(p->value));
        return m;
    }
    if (auto* p = dynamic_cast<const IdentExpr*>(&expr)) {
        auto it = symbols_.find(p->name);
        if (it == symbols_.end()) {
            throw std::runtime_error("undefined variable '" + p->name + "'");
        }
        return it->second;
    }
    if (auto* p = dynamic_cast<const UnaryExpr*>(&expr)) {
        auto v = eval_expr(*p->operand);
        if (p->op == "-") {
            auto r = make_matrix(T_, N_);
            for (int i = 0; i < T_ * N_; ++i) {
                float x = (*v)[i];
                r->at(i) = std::isnan(x) ? nan() : -x;
            }
            return r;
        }
        return v;
    }
    if (auto* p = dynamic_cast<const BinaryExpr*>(&expr)) {
        return eval_binary(*p);
    }
    if (auto* p = dynamic_cast<const TernaryExpr*>(&expr)) {
        return eval_ternary(*p);
    }
    if (auto* p = dynamic_cast<const CallExpr*>(&expr)) {
        return eval_call(*p);
    }
    throw std::runtime_error("unknown AST node type");
}

MatrixPtr PanelEvaluator::eval_binary(const BinaryExpr& node) const {
    auto l = eval_expr(*node.left);
    auto r = eval_expr(*node.right);
    const std::string& op = node.op;
    auto out = make_matrix(T_, N_);
    for (int i = 0; i < T_ * N_; ++i) {
        float a = (*l)[i];
        float b = (*r)[i];
        if (std::isnan(a) || std::isnan(b)) {
            out->at(i) = nan();
            continue;
        }
        float result;
        if      (op == "+")  result = a + b;
        else if (op == "-")  result = a - b;
        else if (op == "*")  result = a * b;
        else if (op == "/")  result = a / b;
        else if (op == "^")  result = static_cast<float>(std::pow(a, b));
        else if (op == ">")  result = a >  b ? 1.0f : 0.0f;
        else if (op == "<")  result = a <  b ? 1.0f : 0.0f;
        else if (op == ">=") result = a >= b ? 1.0f : 0.0f;
        else if (op == "<=") result = a <= b ? 1.0f : 0.0f;
        else if (op == "==") result = a == b ? 1.0f : 0.0f;
        else if (op == "!=") result = a != b ? 1.0f : 0.0f;
        else throw std::runtime_error("unknown binary operator '" + op + "'");
        out->at(i) = result;
    }
    return out;
}

MatrixPtr PanelEvaluator::eval_ternary(const TernaryExpr& node) const {
    auto c = eval_expr(*node.cond);
    auto t = eval_expr(*node.then_expr);
    auto e = eval_expr(*node.else_expr);
    auto out = make_matrix(T_, N_);
    for (int i = 0; i < T_ * N_; ++i) {
        float cond_val = (*c)[i];
        if (std::isnan(cond_val)) {
            out->at(i) = nan();
        } else {
            out->at(i) = (cond_val != 0.0f) ? (*t)[i] : (*e)[i];
        }
    }
    return out;
}

// ---- element-wise unary math ---------------------------------------------

MatrixPtr PanelEvaluator::eval_math_unary(const std::string& name,
                                          const Matrix& v) const {
    auto out = make_matrix(T_, N_);
    for (int i = 0; i < T_ * N_; ++i) {
        float x = v[i];
        if (std::isnan(x)) {
            out->at(i) = nan();
            continue;
        }
        if      (name == "abs")  out->at(i) = std::fabs(x);
        else if (name == "log")  out->at(i) = std::log(x);
        else if (name == "exp")  out->at(i) = std::exp(x);
        else if (name == "sqrt") out->at(i) = std::sqrt(x);
        else if (name == "sign") out->at(i) = sign(x);
    }
    return out;
}

// ---- time-series ---------------------------------------------------------

MatrixPtr PanelEvaluator::ts_delay(const Matrix& x, int d) const {
    auto out = make_matrix(T_, N_);
    for (int n = 0; n < N_; ++n) {
        for (int t = 0; t < T_; ++t) {
            int idx = t * N_ + n;
            if (t >= d) {
                out->at(idx) = x[(t - d) * N_ + n];
            } else {
                out->at(idx) = nan();
            }
        }
    }
    return out;
}

MatrixPtr PanelEvaluator::ts_delta(const Matrix& x, int d) const {
    auto out = make_matrix(T_, N_);
    for (int n = 0; n < N_; ++n) {
        for (int t = 0; t < T_; ++t) {
            int idx = t * N_ + n;
            float val = x[idx];
            if (t < d || std::isnan(val)) {
                out->at(idx) = t < d ? nan() : nan();
            } else {
                float prev = x[(t - d) * N_ + n];
                out->at(idx) = std::isnan(prev) ? nan() : val - prev;
            }
        }
    }
    return out;
}

MatrixPtr PanelEvaluator::ts_sum(const Matrix& x, int d) const {
    auto out = make_matrix(T_, N_);
    for (int n = 0; n < N_; ++n) {
        for (int t = 0; t < T_; ++t) {
            if (t < d - 1) continue;
            double sum = 0.0;
            int count = 0;
            for (int k = 0; k < d; ++k) {
                float val = x[(t - k) * N_ + n];
                if (!std::isnan(val)) {
                    sum += val;
                    ++count;
                }
            }
            if (count > 0) {
                out->at(t * N_ + n) = static_cast<float>(sum);
            }
        }
    }
    return out;
}

MatrixPtr PanelEvaluator::ts_mean(const Matrix& x, int d) const {
    auto sum = ts_sum(x, d);
    for (int i = 0; i < T_ * N_; ++i) {
        float s = (*sum)[i];
        if (!std::isnan(s)) {
            sum->at(i) = s / static_cast<float>(d);
        }
    }
    return sum;
}

MatrixPtr PanelEvaluator::ts_std(const Matrix& x, int d) const {
    auto out = make_matrix(T_, N_);
    for (int n = 0; n < N_; ++n) {
        for (int t = 0; t < T_; ++t) {
            if (t < d - 1) continue;
            int idx = t * N_ + n;
            double sum = 0.0;
            int count = 0;
            for (int k = 0; k < d; ++k) {
                float val = x[(t - k) * N_ + n];
                if (!std::isnan(val)) {
                    sum += val;
                    ++count;
                }
            }
            if (count == 0) continue;
            double mean = sum / count;
            double var_sum = 0.0;
            for (int k = 0; k < d; ++k) {
                float val = x[(t - k) * N_ + n];
                if (!std::isnan(val)) {
                    double diff = val - mean;
                    var_sum += diff * diff;
                }
            }
            double var = var_sum / count;
            out->at(idx) = var <= 0.0 ? 0.0f : static_cast<float>(std::sqrt(var));
        }
    }
    return out;
}

MatrixPtr PanelEvaluator::ts_max(const Matrix& x, int d) const {
    // TODO: optimize with MatrixPool later
    auto out = make_matrix(T_, N_);
    for (int n = 0; n < N_; ++n) {
        for (int t = 0; t < T_; ++t) {
            if (t < d - 1) continue;
            int idx = t * N_ + n;
            bool found = false;
            float mx = 0.0f;
            for (int k = 0; k < d; ++k) {
                float val = x[(t - k) * N_ + n];
                if (!std::isnan(val)) {
                    if (!found || val > mx) {
                        mx = val;
                        found = true;
                    }
                }
            }
            if (found) out->at(idx) = mx;
        }
    }
    return out;
}

// ---- cross-sectional -----------------------------------------------------

MatrixPtr PanelEvaluator::cs_rank(const Matrix& x) const {
    // TODO: optimize with MatrixPool later
    auto out = make_matrix(T_, N_);
    std::vector<std::pair<float, int>> buf;
    buf.reserve(static_cast<size_t>(N_));
    for (int t = 0; t < T_; ++t) {
        buf.clear();
        for (int n = 0; n < N_; ++n) {
            float val = x[t * N_ + n];
            if (!std::isnan(val)) {
                buf.emplace_back(val, n);
            }
        }
        int M = static_cast<int>(buf.size());
        if (M == 0) continue;
        std::sort(buf.begin(), buf.end(),
                  [](const std::pair<float, int>& a,
                     const std::pair<float, int>& b) {
                      return a.first < b.first;
                  });
        for (int r = 0; r < M; ++r) {
            int n = buf[r].second;
            int first_same = r;
            while (first_same > 0 && buf[first_same - 1].first == buf[r].first) {
                --first_same;
            }
            float rank = static_cast<float>(first_same) / static_cast<float>(M);
            out->at(t * N_ + n) = rank;
        }
    }
    return out;
}

// ---- call dispatch -------------------------------------------------------

MatrixPtr PanelEvaluator::eval_call(const CallExpr& node) const {
    const auto& a = node.args;
    const std::string& name = node.callee;

    if (name == "abs" || name == "log" || name == "exp" ||
        name == "sqrt" || name == "sign") {
        if (a.size() != 1)
            throw std::runtime_error("function '" + name +
                                     "' expects 1 argument, got " +
                                     std::to_string(a.size()));
        return eval_math_unary(name, *eval_expr(*a[0]));
    }

    if (name == "signedpower") {
        if (a.size() != 2)
            throw std::runtime_error(
                "function 'signedpower' expects 2 arguments, got " +
                std::to_string(a.size()));
        auto v = eval_expr(*a[0]);
        float pw = static_cast<float>(as_window(*eval_expr(*a[1]), name));
        auto out = make_matrix(T_, N_);
        for (int i = 0; i < T_ * N_; ++i) {
            float x = (*v)[i];
            if (std::isnan(x)) {
                out->at(i) = nan();
            } else {
                out->at(i) = sign(x) * std::pow(std::fabs(x), pw);
            }
        }
        return out;
    }

    if (name == "delay" || name == "delta") {
        if (a.size() != 2)
            throw std::runtime_error("function '" + name +
                                     "' expects 2 arguments, got " +
                                     std::to_string(a.size()));
        auto v = eval_expr(*a[0]);
        int d = as_window(*eval_expr(*a[1]), name);
        return (name == "delay") ? ts_delay(*v, d) : ts_delta(*v, d);
    }

    if (name == "ts_sum" || name == "ts_mean" ||
        name == "ts_std" || name == "ts_max") {
        if (a.size() != 2)
            throw std::runtime_error("function '" + name +
                                     "' expects 2 arguments, got " +
                                     std::to_string(a.size()));
        auto v = eval_expr(*a[0]);
        int d = as_window(*eval_expr(*a[1]), name);
        if (name == "ts_sum")  return ts_sum(*v, d);
        if (name == "ts_mean") return ts_mean(*v, d);
        if (name == "ts_std")  return ts_std(*v, d);
        return ts_max(*v, d);
    }

    if (name == "rank") {
        if (a.size() != 1)
            throw std::runtime_error("function 'rank' expects 1 argument, got " +
                                     std::to_string(a.size()));
        return cs_rank(*eval_expr(*a[0]));
    }

    throw std::runtime_error("unknown function '" + name + "'");
}
