#include <iostream>
#include <print>
#include "orderbook.h"
#include <tsl/robin_map.h>

int main() {
    tsl::robin_map<int, int> map {{1, 23}, {5, 45}, {12, 78}};

    std::cout << map[5];
    return 0;
}
