#include <catch_amalgamated.hpp>

#include "fepCore.h"
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <vector>

using Sym = std::map<std::string, MatrixPtr>;

static float nan_val() { return std::numeric_limits<float>::quiet_NaN(); }

// ---- helpers: build small panels ------------------------------------------

/// T x N panel filled with a constant value.
static MatrixPtr const_panel(int T, int N, float v) {
    auto m = std::make_shared<Matrix>(static_cast<size_t>(T) * N, v);
    return m;
}

/// T x N panel filled with stock-specific time-series:
///   stock n: 1.0f + n + 0.1f * (t+1)
static MatrixPtr seq_panel(int T, int N) {
    auto m = std::make_shared<Matrix>(static_cast<size_t>(T) * N);
    for (int t = 0; t < T; ++t)
        for (int n = 0; n < N; ++n)
            m->at(t * N + n) = 1.0f + static_cast<float>(n) + 0.1f * static_cast<float>(t + 1);
    return m;
}

static bool approx(const Matrix& a, const Matrix& b, float eps = 1e-5f) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        bool a_nan = std::isnan(a[i]);
        bool b_nan = std::isnan(b[i]);
        if (a_nan != b_nan) return false;
        if (!a_nan && std::fabs(a[i] - b[i]) > eps) return false;
    }
    return true;
}

// ---- element-wise arithmetic over T x N -----------------------------------

TEST_CASE("eval_panel constant arithmetic", "[eval]") {
    // 42 → panel all 42.0
    auto r = eval_panel("42", Sym{}, 3, 2);
    REQUIRE(r->size() == 6u);
    for (auto v : *r) REQUIRE(v == 42.0f);
}

TEST_CASE("eval_panel variable lookup", "[eval]") {
    Sym sym{{"close", const_panel(3, 2, 10.0f)}};
    auto r = eval_panel("close", sym, 3, 2);
    REQUIRE(approx(*r, *const_panel(3, 2, 10.0f)));
}

TEST_CASE("eval_panel binary arithmetic point-wise", "[eval]") {
    Sym sym{{"a", const_panel(4, 3, 5.0f)}, {"b", const_panel(4, 3, 3.0f)}};
    // a + b → all 8.0
    auto r = eval_panel("a + b", sym, 4, 3);
    REQUIRE(approx(*r, *const_panel(4, 3, 8.0f)));
    // a * b → all 15.0
    r = eval_panel("a * b", sym, 4, 3);
    REQUIRE(approx(*r, *const_panel(4, 3, 15.0f)));
}

TEST_CASE("eval_panel power operator", "[eval]") {
    Sym sym{{"a", const_panel(2, 2, 3.0f)}};
    auto r = eval_panel("a ^ 2", sym, 2, 2);
    REQUIRE(approx(*r, *const_panel(2, 2, 9.0f)));
}

TEST_CASE("eval_panel comparison operators", "[eval]") {
    Sym sym{{"a", const_panel(3, 2, 5.0f)}, {"b", const_panel(3, 2, 3.0f)}};
    REQUIRE(approx(*eval_panel("a > b", sym, 3, 2), *const_panel(3, 2, 1.0f)));
    REQUIRE(approx(*eval_panel("a < b", sym, 3, 2), *const_panel(3, 2, 0.0f)));
    REQUIRE(approx(*eval_panel("a == b", sym, 3, 2), *const_panel(3, 2, 0.0f)));
    REQUIRE(approx(*eval_panel("a != b", sym, 3, 2), *const_panel(3, 2, 1.0f)));
}

TEST_CASE("eval_panel ternary conditional", "[eval]") {
    // a > 0 ? b : c   (a = [3, -1], b = [10, 20], c = [100, 200])
    int T = 2, N = 1;
    auto a = std::make_shared<Matrix>(Matrix{3.0f, -1.0f});
    auto b = std::make_shared<Matrix>(Matrix{10.0f, 20.0f});
    auto c = std::make_shared<Matrix>(Matrix{100.0f, 200.0f});
    Sym sym{{"a", a}, {"b", b}, {"c", c}};
    auto r = eval_panel("a > 0 ? b : c", sym, T, N);
    REQUIRE(approx(*r, Matrix{10.0f, 200.0f}));
}

// ---- NaN propagation ------------------------------------------------------

TEST_CASE("eval_panel NaN propagates in binary ops", "[eval]") {
    int T = 2, N = 2;
    auto a = std::make_shared<Matrix>(Matrix{1.0f, nan_val(), 3.0f, 4.0f});
    auto b = std::make_shared<Matrix>(Matrix{nan_val(), 2.0f, 3.0f, 4.0f});
    Sym sym{{"a", a}, {"b", b}};
    auto r = eval_panel("a + b", sym, T, N);
    REQUIRE(std::isnan((*r)[0])); // NaN + anything = NaN
    REQUIRE(std::isnan((*r)[1])); // anything + NaN = NaN
    REQUIRE((*r)[2] == 6.0f);
    REQUIRE((*r)[3] == 8.0f);
}

TEST_CASE("eval_panel NaN propagates in unary ops", "[eval]") {
    int T = 1, N = 2;
    auto v = std::make_shared<Matrix>(Matrix{1.0f, nan_val()});
    Sym sym{{"x", v}};
    auto r = eval_panel("abs(x)", sym, T, N);
    REQUIRE((*r)[0] == 1.0f);
    REQUIRE(std::isnan((*r)[1]));
}

TEST_CASE("eval_panel NaN in ternary yields NaN", "[eval]") {
    int T = 1, N = 2;
    auto cond = std::make_shared<Matrix>(Matrix{nan_val(), 1.0f});
    auto tv = std::make_shared<Matrix>(Matrix{10.0f, 20.0f});
    auto fv = std::make_shared<Matrix>(Matrix{100.0f, 200.0f});
    Sym sym{{"c", cond}, {"t", tv}, {"f", fv}};
    auto r = eval_panel("c ? t : f", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE((*r)[1] == 20.0f);
}

// ---- element-wise math functions ------------------------------------------

TEST_CASE("eval_panel abs point-wise", "[eval]") {
    Sym sym{{"x", const_panel(2, 2, -5.0f)}};
    auto r = eval_panel("abs(x)", sym, 2, 2);
    REQUIRE(approx(*r, *const_panel(2, 2, 5.0f)));
}

TEST_CASE("eval_panel signedpower point-wise", "[eval]") {
    int T = 1, N = 3;
    auto x = std::make_shared<Matrix>(Matrix{-2.0f, 3.0f, -4.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("signedpower(x, 2)", sym, T, N);
    REQUIRE((*r)[0] == -4.0f);
    REQUIRE((*r)[1] == 9.0f);
    REQUIRE((*r)[2] == -16.0f);
}

// ---- time-series functions over T x N -------------------------------------

TEST_CASE("eval_panel delay shifts per stock", "[eval]") {
    int T = 5, N = 2;
    auto x = std::make_shared<Matrix>(static_cast<size_t>(T) * N);
    // Stock 0: 1,2,3,4,5   Stock 1: 10,20,30,40,50
    for (int t = 0; t < T; ++t) {
        x->at(t * N + 0) = static_cast<float>(t + 1);
        x->at(t * N + 1) = static_cast<float>((t + 1) * 10);
    }
    Sym sym{{"x", x}};
    auto r = eval_panel("delay(x, 2)", sym, T, N);
    // Stock 0: NaN, NaN, 1, 2, 3
    REQUIRE(std::isnan(r->at(0 * N + 0)));
    REQUIRE(std::isnan(r->at(1 * N + 0)));
    REQUIRE(r->at(2 * N + 0) == 1.0f);
    REQUIRE(r->at(3 * N + 0) == 2.0f);
    REQUIRE(r->at(4 * N + 0) == 3.0f);
    // Stock 1: NaN, NaN, 10, 20, 30
    REQUIRE(std::isnan(r->at(0 * N + 1)));
    REQUIRE(std::isnan(r->at(1 * N + 1)));
    REQUIRE(r->at(2 * N + 1) == 10.0f);
    REQUIRE(r->at(3 * N + 1) == 20.0f);
    REQUIRE(r->at(4 * N + 1) == 30.0f);
}

TEST_CASE("eval_panel delta per stock", "[eval]") {
    int T = 4, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{10.0f, 12.0f, 15.0f, 20.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("delta(x, 1)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE((*r)[1] == 2.0f);
    REQUIRE((*r)[2] == 3.0f);
    REQUIRE((*r)[3] == 5.0f);
}

TEST_CASE("eval_panel ts_sum rolling per stock", "[eval]") {
    int T = 5, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{1.0f, 2.0f, 3.0f, 4.0f, 5.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_sum(x, 3)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE(std::isnan((*r)[1]));
    REQUIRE((*r)[2] == 6.0f);  // 1+2+3
    REQUIRE((*r)[3] == 9.0f);  // 2+3+4
    REQUIRE((*r)[4] == 12.0f); // 3+4+5
}

TEST_CASE("eval_panel ts_sum ignores NaN in window", "[eval]") {
    int T = 4, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{1.0f, nan_val(), 3.0f, 4.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_sum(x, 3)", sym, T, N);
    // t=2 window {1,NaN,3} -> sum=4
    REQUIRE((*r)[2] == 4.0f);
    // t=3 window {NaN,3,4} -> sum=7
    REQUIRE((*r)[3] == 7.0f);
}

TEST_CASE("eval_panel ts_sum all-NaN window yields NaN", "[eval]") {
    int T = 3, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{nan_val(), nan_val(), 1.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_sum(x, 2)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE(std::isnan((*r)[1])); // window {NaN,NaN}
    REQUIRE((*r)[2] == 1.0f);     // window {NaN,1}
}

TEST_CASE("eval_panel ts_mean rolling per stock", "[eval]") {
    int T = 4, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{2.0f, 4.0f, 6.0f, 8.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_mean(x, 2)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE((*r)[1] == 3.0f); // (2+4)/2
    REQUIRE((*r)[2] == 5.0f); // (4+6)/2
    REQUIRE((*r)[3] == 7.0f); // (6+8)/2
}

TEST_CASE("eval_panel ts_std rolling per stock", "[eval]") {
    int T = 4, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{5.0f, 5.0f, 5.0f, 5.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_std(x, 3)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE(std::isnan((*r)[1]));
    REQUIRE((*r)[2] == 0.0f);
    REQUIRE((*r)[3] == 0.0f);
}

TEST_CASE("eval_panel ts_max rolling per stock", "[eval]") {
    int T = 5, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{1.0f, 5.0f, 3.0f, 7.0f, 2.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_max(x, 3)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE(std::isnan((*r)[1]));
    REQUIRE((*r)[2] == 5.0f); // max{1,5,3}
    REQUIRE((*r)[3] == 7.0f); // max{5,3,7}
    REQUIRE((*r)[4] == 7.0f); // max{3,7,2}
}

TEST_CASE("eval_panel ts_max ignores NaN in window", "[eval]") {
    int T = 4, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{1.0f, nan_val(), 5.0f, 3.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_max(x, 3)", sym, T, N);
    // t=2 window {1,NaN,5} -> max=5
    REQUIRE((*r)[2] == 5.0f);
    // t=3 window {NaN,5,3} -> max=5
    REQUIRE((*r)[3] == 5.0f);
}

TEST_CASE("eval_panel ts_max all-NaN window yields NaN", "[eval]") {
    int T = 3, N = 1;
    auto x = std::make_shared<Matrix>(Matrix{nan_val(), nan_val(), 1.0f});
    Sym sym{{"x", x}};
    auto r = eval_panel("ts_max(x, 2)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE(std::isnan((*r)[1])); // window {NaN,NaN}
    REQUIRE((*r)[2] == 1.0f);     // window {NaN,1}
}

// ---- cross-sectional rank -------------------------------------------------

TEST_CASE("eval_panel rank cross-section day by day", "[eval]") {
    // T=3, N=4   values per day:
    int T = 3, N = 4;
    auto x = std::make_shared<Matrix>(Matrix{
        // day0      day1      day2
        10.0f, 30.0f, 20.0f, 40.0f,  // t=0
        5.0f,  3.0f,  4.0f,  1.0f,   // t=1
        0.0f,  0.0f,  1.0f,  2.0f    // t=2
    });
    Sym sym{{"x", x}};
    auto r = eval_panel("rank(x)", sym, T, N);
    // Day 0: values {10,30,20,40} sorted -> 10(0),20(1),30(2),40(3)
    // ranks: 10→0/4=0.0, 30→2/4=0.5, 20→1/4=0.25, 40→3/4=0.75
    REQUIRE((*r)[0 * N + 0] == 0.0f);      // 10
    REQUIRE((*r)[0 * N + 1] == 0.5f);      // 30
    REQUIRE((*r)[0 * N + 2] == 0.25f);     // 20
    REQUIRE((*r)[0 * N + 3] == 0.75f);     // 40
    // Day 1: {5,3,4,1} -> sorted 1(0),3(1),4(2),5(3)
    // ranks: 5→3/4=0.75, 3→1/4=0.25, 4→2/4=0.5, 1→0/4=0.0
    REQUIRE((*r)[1 * N + 0] == 0.75f);
    REQUIRE((*r)[1 * N + 1] == 0.25f);
    REQUIRE((*r)[1 * N + 2] == 0.5f);
    REQUIRE((*r)[1 * N + 3] == 0.0f);
    // Day 2: {0,0,1,2} -> tie handling: 0,0 rank both 0/4=0.0, 1→2/4=0.5, 2→3/4=0.75
    REQUIRE((*r)[2 * N + 0] == 0.0f);
    REQUIRE((*r)[2 * N + 1] == 0.0f);  // tie with [0]
    REQUIRE((*r)[2 * N + 2] == 0.5f);
    REQUIRE((*r)[2 * N + 3] == 0.75f);
}

TEST_CASE("eval_panel rank excludes NaN stocks", "[eval]") {
    int T = 2, N = 3;
    auto x = std::make_shared<Matrix>(Matrix{
        1.0f, nan_val(), 3.0f,  // day 0: NaN excluded -> M=2
        4.0f, 2.0f, nan_val()    // day 1: NaN excluded -> M=2
    });
    Sym sym{{"x", x}};
    auto r = eval_panel("rank(x)", sym, T, N);
    // Day 0: {1,NaN,3} -> valid values {1,3}, ranks 0/2=0.0, 1/2=0.5
    REQUIRE((*r)[0 * N + 0] == 0.0f);   // 1 → rank 0
    REQUIRE(std::isnan((*r)[0 * N + 1])); // NaN stays NaN
    REQUIRE((*r)[0 * N + 2] == 0.5f);   // 3 → rank 1
    // Day 1: {4,2,NaN} -> ranks 1/2=0.5, 0/2=0.0
    REQUIRE((*r)[1 * N + 0] == 0.5f);
    REQUIRE((*r)[1 * N + 1] == 0.0f);
    REQUIRE(std::isnan((*r)[1 * N + 2]));
}

TEST_CASE("eval_panel rank all-NaN day yields all NaN", "[eval]") {
    int T = 2, N = 2;
    auto x = std::make_shared<Matrix>(Matrix{
        nan_val(), nan_val(),
        1.0f, 2.0f
    });
    Sym sym{{"x", x}};
    auto r = eval_panel("rank(x)", sym, T, N);
    REQUIRE(std::isnan((*r)[0]));
    REQUIRE(std::isnan((*r)[1]));
    REQUIRE((*r)[2] == 0.0f); // 1 → rank 0/2
    REQUIRE((*r)[3] == 0.5f); // 2 → rank 1/2
}

// ---- nested & composite expressions ---------------------------------------

TEST_CASE("eval_panel composite: rank(ts_max / delay)", "[eval]") {
    int T = 5, N = 2;
    auto x = seq_panel(T, N); // stock-specific sequences
    Sym sym{{"x", x}};
    // Just check it runs without error and returns correct shape
    auto r = eval_panel("rank(ts_max(x, 2) / delay(x, 1))", sym, T, N);
    REQUIRE(r->size() == static_cast<size_t>(T * N));
}

// ---- error cases ----------------------------------------------------------

TEST_CASE("eval_panel undefined variable throws", "[eval]") {
    REQUIRE_THROWS_AS(eval_panel("close", Sym{}, 2, 2), std::runtime_error);
}

TEST_CASE("eval_panel arity errors", "[eval]") {
    Sym sym{{"x", const_panel(2, 2, 1.0f)}};
    REQUIRE_THROWS_AS(eval_panel("abs()", sym, 2, 2), std::runtime_error);
    REQUIRE_THROWS_AS(eval_panel("abs(1, 2)", sym, 2, 2), std::runtime_error);
    REQUIRE_THROWS_AS(eval_panel("ts_sum(x)", sym, 2, 2), std::runtime_error);
    REQUIRE_THROWS_AS(eval_panel("rank()", sym, 2, 2), std::runtime_error);
}

TEST_CASE("eval_panel unknown function throws", "[eval]") {
    REQUIRE_THROWS_AS(eval_panel("foobar(1)", Sym{}, 2, 2), std::runtime_error);
}
