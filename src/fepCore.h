#ifndef FEP_CORE_H
#define FEP_CORE_H

#include <map>
#include <string>
#include <vector>

/// Parse a factor expression string and return its AST pretty-printed form.
/// The expression language supports element-wise operators, time-series
/// functions, and cross-sectional functions over a 3D data array.
///
/// Example: fep("rank(ts_mean(close, 5) / delay(close, 10))")
std::string fep(const std::string& expr);

/// Parse and evaluate a factor expression as a 1-D time series.
///
/// Each variable in `symbols` is a period of that field, e.g.
/// {"close": {10.0, 11.0, 9.5, ...}}. Element-wise operators/functions act
/// point-wise (with numpy-style broadcasting); time-series functions roll a
/// window along the time axis. Cross-sectional functions throw.
///
/// Throws std::runtime_error on any lex/parse/eval failure.
std::vector<double> eval(const std::string& expr,
                         const std::map<std::string, std::vector<double>>& symbols);

#endif // FEP_CORE_H
