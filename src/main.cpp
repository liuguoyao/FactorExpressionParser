#include "fepCore.h"
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    // ---- build a tiny panel: T=5 days, N=3 stocks -----------------------
    int T = 5, N = 3;
    auto close = std::make_shared<Matrix>(T * N);
    auto volume = std::make_shared<Matrix>(T * N);

    // Fill close with day-over-day data per stock
    // Stock 0: 10, 11, 12, 13, 14
    // Stock 1: 20, 21, 22, 23, 24
    // Stock 2: 30, 31, 32, 33, 34
    for (int t = 0; t < T; ++t) {
        for (int n = 0; n < N; ++n) {
            int idx = t * N + n;
            close->at(idx) = 10.0f + n * 10.0f + static_cast<float>(t);
            volume->at(idx) = 1000.0f + n * 500.0f + static_cast<float>(t * 10);
        }
    }

    // Introduce some NaN (stock 1 missing on day 2)
    close->at(2 * N + 1) = std::numeric_limits<float>::quiet_NaN();

    std::map<std::string, MatrixPtr> symbols;
    symbols["close"]  = close;
    symbols["volume"] = volume;

    auto print_mat = [&](const std::string& label, const Matrix& m) {
        std::cout << label << ":\n";
        for (int t = 0; t < T; ++t) {
            std::cout << "  day " << t << ":";
            for (int n = 0; n < N; ++n) {
                float v = m[t * N + n];
                if (std::isnan(v)) std::cout << "   NaN";
                else               std::cout << " " << v;
            }
            std::cout << "\n";
        }
    };

    print_mat("close", *close);
    print_mat("volume", *volume);

    // ---- demo: element-wise arithmetic ----------------------------------
    print_mat("close + volume * 2",
              *eval_panel("close + volume * 2", symbols, T, N));

    // ---- demo: time-series ts_max ---------------------------------------
    print_mat("ts_max(close, 3)",
              *eval_panel("ts_max(close, 3)", symbols, T, N));

    // ---- demo: cross-sectional rank --------------------------------------
    print_mat("rank(close)",
              *eval_panel("rank(close)", symbols, T, N));

    // ---- demo: composite -------------------------------------------------
    print_mat("rank(ts_max(close, 2) / delay(close, 1))",
              *eval_panel("rank(ts_max(close, 2) / delay(close, 1))",
                          symbols, T, N));

    return 0;
}
