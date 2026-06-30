#include "fepCore.h"
#include <iostream>

int main() {
    const char* tests[] = {
        "rank(ts_mean(close, 5) / delay(close, 10))",
        "close + volume * 2",
        "abs(close - open) / (high - low + 0.001)",
        "close >= open ? close : open",
        "zscore(ts_sum(returns, 20))",
        "1 + 2 * 3 ^ 4",
        "invalid!!token",
        nullptr
    };

    for (const char** p = tests; *p; ++p) {
        std::cout << "Input:  " << *p << "\n"
                  << "Output: " << fep(*p) << "\n\n";
    }

    return 0;
}
