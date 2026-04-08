#include "order.h"
#include <list>
#include <map>
#include <unordered_map>

using Orders = std::list<std::shared_ptr<Order>>;
using Bids = std::map<Price, Orders, std::greater<Price>>;
using Asks = std::map<Price, Orders, std::less<Price>>;

using OrderLocation = std::list<std::shared_ptr<Order>>::iterator;
using OrderMap = std::unordered_map<OrderId, OrderLocation>;
