#include <catch_amalgamated.hpp>

#include "fepCore.h"
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using MatrixPtr = std::shared_ptr<Matrix>;
using Sym = std::map<std::string, MatrixPtr>;

static float nan_val() { return std::numeric_limits<float>::quiet_NaN(); }

/// Build a panel with monotonic time-series per stock:
///   stock n: 1.0f + n * 0.001f + 0.1f * (t+1)
static MatrixPtr seq_panel(int T, int N) {
    auto m = std::make_shared<Matrix>(static_cast<size_t>(T) * N);
    for (int t = 0; t < T; ++t)
        for (int n = 0; n < N; ++n)
            m->at(t * N + n) = 1.0f + static_cast<float>(n) * 0.001f +
                               0.1f * static_cast<float>(t + 1);
    return m;
}

/// Panel with 5% NaN scattered in.
static MatrixPtr noisy_panel(int T, int N) {
    auto m = seq_panel(T, N);
    for (int i = 0; i < T * N; ++i) {
        if (i % 20 == 0) m->at(i) = nan_val();
    }
    return m;
}

// All benchmarks use a panel of T=1000 days, N=500 stocks (500k elements).

// --- element-wise ops ------------------------------------------------------

TEST_CASE("bench panel element-wise add 500k", "[perf]") {
    Sym sym{{"a", seq_panel(1000, 500)}, {"b", seq_panel(1000, 500)}};
    BENCHMARK("a + b  (1000x500)") {
        return eval_panel("a + b", sym, 1000, 500);
    };
}

TEST_CASE("bench panel compound arithmetic 500k", "[perf]") {
    Sym sym{{"a", seq_panel(1000, 500)}, {"b", seq_panel(1000, 500)}};
    BENCHMARK("a * 2 + b  (1000x500)") {
        return eval_panel("a * 2 + b", sym, 1000, 500);
    };
}

// --- element-wise math -----------------------------------------------------

TEST_CASE("bench panel abs(log(x)) 500k", "[perf]") {
    Sym sym{{"x", seq_panel(1000, 500)}};
    BENCHMARK("abs(log(x))  (1000x500)") {
        return eval_panel("abs(log(x))", sym, 1000, 500);
    };
}

// --- time-series ops -------------------------------------------------------

TEST_CASE("bench panel ts_mean(20) 500k", "[perf]") {
    Sym sym{{"x", seq_panel(1000, 500)}};
    BENCHMARK("ts_mean(x, 20)  (1000x500)") {
        return eval_panel("ts_mean(x, 20)", sym, 1000, 500);
    };
}

TEST_CASE("bench panel ts_std(20) 500k", "[perf]") {
    Sym sym{{"x", seq_panel(1000, 500)}};
    BENCHMARK("ts_std(x, 20)  (1000x500)") {
        return eval_panel("ts_std(x, 20)", sym, 1000, 500);
    };
}

TEST_CASE("bench panel ts_max(20) 500k", "[perf]") {
    Sym sym{{"x", seq_panel(1000, 500)}};
    BENCHMARK("ts_max(x, 20)  (1000x500)") {
        return eval_panel("ts_max(x, 20)", sym, 1000, 500);
    };
}

// --- cross-sectional ops ---------------------------------------------------

TEST_CASE("bench panel rank 500k", "[perf]") {
    Sym sym{{"x", seq_panel(1000, 500)}};
    BENCHMARK("rank(x)  (1000x500)") {
        return eval_panel("rank(x)", sym, 1000, 500);
    };
}

TEST_CASE("bench panel rank with NaN 500k", "[perf]") {
    Sym sym{{"x", noisy_panel(1000, 500)}};
    BENCHMARK("rank(x) with NaN  (1000x500)") {
        return eval_panel("rank(x)", sym, 1000, 500);
    };
}

// --- composite realistic alpha ---------------------------------------------

TEST_CASE("bench panel composite alpha 500k", "[perf]") {
    Sym sym{{"close", seq_panel(1000, 500)}};
    BENCHMARK("rank(ts_mean(close,5)/delay(close,10))  (1000x500)") {
        return eval_panel("rank(ts_mean(close, 5) / delay(close, 10))",
                          sym, 1000, 500);
    };
}

TEST_CASE("bench panel composite with ts_max 500k", "[perf]") {
    Sym sym{{"close", seq_panel(1000, 500)}};
    BENCHMARK("rank(ts_max(close,5)/delay(close,10))  (1000x500)") {
        return eval_panel("rank(ts_max(close, 5) / delay(close, 10))",
                          sym, 1000, 500);
    };
}
