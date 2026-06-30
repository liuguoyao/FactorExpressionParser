#include <catch_amalgamated.hpp>

#include "fepCore.h"
#include <cmath>
#include <map>
#include <string>
#include <vector>

using Vec = std::vector<double>;
using Sym = std::map<std::string, Vec>;

static bool approx(const Vec& a, const Vec& b, double eps = 1e-9) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::isfinite(a[i]) || std::isfinite(b[i])) {
            if (std::fabs(a[i] - b[i]) > eps) return false;
        }
        // both NaN -> considered equal at this index
    }
    return true;
}

// ---- element-wise & broadcasting ------------------------------------------

TEST_CASE("eval constant arithmetic yields length-1 series", "[eval]") {
    REQUIRE(approx(eval("42", Sym{}), Vec{42.0}));
    REQUIRE(approx(eval("1 + 2 * 3", Sym{}), Vec{7.0}));
    REQUIRE(approx(eval("2 ^ 10", Sym{}), Vec{1024.0}));
}

TEST_CASE("eval variable lookup returns the series", "[eval]") {
    Sym sym{{"close", Vec{10, 11, 12}}};
    REQUIRE(approx(eval("close", sym), Vec{10, 11, 12}));
}

TEST_CASE("eval point-wise subtraction of two series", "[eval]") {
    Sym sym{{"close", Vec{10, 11, 12}}, {"open", Vec{9, 10, 11}}};
    REQUIRE(approx(eval("close - open", sym), Vec{1, 1, 1}));
}

TEST_CASE("eval broadcasting scalar*series", "[eval]") {
    Sym sym{{"close", Vec{10, 20, 30}}};
    REQUIRE(approx(eval("close * 2", sym), Vec{20, 40, 60}));
    REQUIRE(approx(eval("2 * close", sym), Vec{20, 40, 60}));
}

TEST_CASE("eval undefined variable throws", "[eval]") {
    REQUIRE_THROWS_AS(eval("close", Sym{}), std::runtime_error);
}

TEST_CASE("eval length mismatch in binary op throws", "[eval]") {
    Sym sym{{"a", Vec{1, 2, 3}}, {"b", Vec{1, 2}}};
    REQUIRE_THROWS_AS(eval("a + b", sym), std::runtime_error);
}

// ---- element-wise math functions (point-wise) -----------------------------

TEST_CASE("eval abs point-wise", "[eval]") {
    Sym sym{{"x", Vec{-5, 0, 5}}};
    REQUIRE(approx(eval("abs(x)", sym), Vec{5, 0, 5}));
}

TEST_CASE("eval log/exp round trip point-wise", "[eval]") {
    Sym sym{{"x", Vec{1.0, 2.0, 3.0}}};
    Vec r = eval("log(exp(x))", sym);
    REQUIRE(approx(r, Vec{1.0, 2.0, 3.0}));
}

TEST_CASE("eval sqrt point-wise", "[eval]") {
    Sym sym{{"x", Vec{9.0, 4.0, 0.0}}};
    REQUIRE(approx(eval("sqrt(x)", sym), Vec{3.0, 2.0, 0.0}));
}

TEST_CASE("eval sign point-wise", "[eval]") {
    Sym sym{{"x", Vec{-3.0, 0.0, 3.0}}};
    REQUIRE(approx(eval("sign(x)", sym), Vec{-1.0, 0.0, 1.0}));
}

TEST_CASE("eval signedpower point-wise", "[eval]") {
    Sym sym{{"x", Vec{-2.0, 3.0, -4.0}}};
    // signedpower(x,2): sign(x)*|x|^2 -> {-4, 9, -16}
    REQUIRE(approx(eval("signedpower(x, 2)", sym), Vec{-4.0, 9.0, -16.0}));
}

// ---- comparisons & ternary (point-wise) -----------------------------------

TEST_CASE("eval comparison point-wise", "[eval]") {
    Sym sym{{"a", Vec{3, 2, 5}}, {"b", Vec{2, 3, 5}}};
    REQUIRE(approx(eval("a > b", sym), Vec{1, 0, 0}));
    REQUIRE(approx(eval("a == b", sym), Vec{0, 0, 1}));
    REQUIRE(approx(eval("a != b", sym), Vec{1, 1, 0}));
}

TEST_CASE("eval ternary point-wise", "[eval]") {
    Sym sym{{"a", Vec{3, 2}}, {"b", Vec{2, 3}}};
    // a > b ? a : b  -> {3, 3}
    REQUIRE(approx(eval("a > b ? a : b", sym), Vec{3, 3}));
}

// ---- time-series functions -------------------------------------------------

TEST_CASE("eval delay shifts series and front-fills NaN", "[eval]") {
    Sym sym{{"x", Vec{1, 2, 3, 4, 5}}};
    Vec r = eval("delay(x, 2)", sym);
    REQUIRE(std::isnan(r[0]));
    REQUIRE(std::isnan(r[1]));
    REQUIRE(approx(Vec(r.begin() + 2, r.end()), Vec{1, 2, 3}));
}

TEST_CASE("eval delta = x - delay(x,d)", "[eval]") {
    Sym sym{{"x", Vec{10, 12, 15, 20}}};
    Vec r = eval("delta(x, 1)", sym);
    REQUIRE(std::isnan(r[0]));
    REQUIRE(approx(Vec(r.begin() + 1, r.end()), Vec{2, 3, 5}));
}

TEST_CASE("eval ts_sum rolling window", "[eval]") {
    Sym sym{{"x", Vec{1, 2, 3, 4, 5}}};
    Vec r = eval("ts_sum(x, 3)", sym);
    REQUIRE(std::isnan(r[0]));
    REQUIRE(std::isnan(r[1]));
    REQUIRE(approx(Vec(r.begin() + 2, r.end()), Vec{6, 9, 12}));
}

TEST_CASE("eval ts_mean rolling window", "[eval]") {
    Sym sym{{"x", Vec{2, 4, 6, 8, 10}}};
    Vec r = eval("ts_mean(x, 2)", sym);
    REQUIRE(std::isnan(r[0]));
    REQUIRE(approx(Vec(r.begin() + 1, r.end()), Vec{3, 5, 7, 9}));
}

TEST_CASE("eval ts_std rolling window", "[eval]") {
    // window of a constant series -> std 0
    Sym sym{{"x", Vec{5, 5, 5, 5}}};
    Vec r = eval("ts_std(x, 3)", sym);
    REQUIRE(std::isnan(r[0]));
    REQUIRE(std::isnan(r[1]));
    REQUIRE(approx(Vec(r.begin() + 2, r.end()), Vec{0, 0}));
}

TEST_CASE("eval nested time-series: ts_mean(delay(x))", "[eval]") {
    Sym sym{{"x", Vec{1, 2, 3, 4}}};
    // delay(x,1) = [NaN,1,2,3]; ts_mean(_,2) -> [NaN,NaN,NaN,(1+2)/2=1.5]
    Vec r = eval("ts_mean(delay(x, 1), 2)", sym);
    REQUIRE(approx(Vec(r.begin(), r.end()), Vec{
        std::nan(""), std::nan(""), std::nan(""), 1.5}));
}

// ---- arity & placeholders --------------------------------------------------

TEST_CASE("eval arity errors", "[eval]") {
    REQUIRE_THROWS_AS(eval("abs()", Sym{}), std::runtime_error);
    REQUIRE_THROWS_AS(eval("abs(1, 2)", Sym{}), std::runtime_error);
    REQUIRE_THROWS_AS(eval("delay(x)", Sym{{"x", Vec{1}}}), std::runtime_error);
}

TEST_CASE("eval cross-sectional functions throw", "[eval]") {
    Sym sym{{"x", Vec{1, 2, 3}}};
    REQUIRE_THROWS_AS(eval("rank(x)", sym), std::runtime_error);
    REQUIRE_THROWS_AS(eval("zscore(x)", sym), std::runtime_error);
}

TEST_CASE("eval unknown function throws", "[eval]") {
    REQUIRE_THROWS_AS(eval("foobar(1)", Sym{}), std::runtime_error);
}
