#ifndef FEP_FACTOR_EVALUATOR_H
#define FEP_FACTOR_EVALUATOR_H

#include "ast.h"
#include "panel_types.h"

#include <map>
#include <string>

/// Evaluate a factor expression AST over a 2-D panel of shape T x N,
/// stored row-major (t * N + n).  All intermediate results are returned
/// as shared_ptr<Matrix> to avoid value copies.  NaN propagation follows
/// the rules in AGENTS.md §2.
class PanelEvaluator {
public:
    PanelEvaluator(const std::map<std::string, MatrixPtr>& symbols,
                   int T, int N);

    MatrixPtr eval(const Expr& expr) const;

private:
    const std::map<std::string, MatrixPtr>& symbols_;
    int T_;
    int N_;

    // ---- helpers ---------------------------------------------------------

    static float sign(float x);
    static float nan();
    static MatrixPtr make_matrix(int T, int N);
    static MatrixPtr make_const_matrix(int T, int N, float val);
    static int as_window(const Matrix& m, const std::string& fname);

    // ---- dispatch --------------------------------------------------------

    MatrixPtr eval_expr(const Expr& expr) const;
    MatrixPtr eval_binary(const BinaryExpr& node) const;
    MatrixPtr eval_ternary(const TernaryExpr& node) const;

    // ---- element-wise ----------------------------------------------------

    MatrixPtr eval_math_unary(const std::string& name, const Matrix& v) const;

    // ---- time-series -----------------------------------------------------

    MatrixPtr ts_delay(const Matrix& x, int d) const;
    MatrixPtr ts_delta(const Matrix& x, int d) const;
    MatrixPtr ts_sum(const Matrix& x, int d) const;
    MatrixPtr ts_mean(const Matrix& x, int d) const;
    MatrixPtr ts_std(const Matrix& x, int d) const;
    MatrixPtr ts_max(const Matrix& x, int d) const;

    // ---- cross-sectional -------------------------------------------------

    MatrixPtr cs_rank(const Matrix& x) const;

    // ---- call dispatch ---------------------------------------------------

    MatrixPtr eval_call(const CallExpr& node) const;
};

#endif // FEP_FACTOR_EVALUATOR_H
