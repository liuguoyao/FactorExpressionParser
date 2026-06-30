#ifndef FEP_CORE_H
#define FEP_CORE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

using Matrix = std::vector<float>;
using MatrixPtr = std::shared_ptr<Matrix>;

/// Parse and evaluate a factor expression over a 2-D panel of shape T x N
/// (T time periods, N stocks), stored row-major (t * N + n).
///
/// Each variable in `symbols` must have exactly T * N elements.  The returned
/// MatrixPtr has the same shape.
///
/// Throws std::runtime_error on any lex/parse/eval failure.
MatrixPtr eval_panel(const std::string& expr,
                     const std::map<std::string, MatrixPtr>& symbols,
                     int T, int N);

#endif // FEP_CORE_H
