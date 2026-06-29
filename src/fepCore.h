#ifndef FEP_CORE_H
#define FEP_CORE_H

#include <string>

/// Parse a factor expression string and return the result.
/// The expression language supports element-wise operators, time-series
/// functions, and cross-sectional functions over a 3D data array.
///
/// Example: fep("rank(ts_mean(close, 5) / delay(close, 10))")
std::string fep(const std::string& expr);

#endif // FEP_CORE_H
