#include "alias.h"
#include <iostream>
#include <vector>

struct TradeInfo {
    OrderId orderId;
    Quantity qty;
    Price price;
};

struct Trade {
    TradeInfo bidInfo;
    TradeInfo askInfo;
};

using Trades = std::vector<Trade>;
