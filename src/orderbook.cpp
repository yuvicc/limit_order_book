#include "orderbook.h"

void Orderbook::CancelOrderInternal(OrderId orderId)
{
    if(!orderMap.contains(orderId)) 
        return;

    auto mapIt = orderMap.find(orderId);

    OrderLocation orderLoc = mapIt->second;
    std::shared_ptr<Order> order = *orderLoc;

    order->Cancel();
    Price price = order->GetPrice();

    // Remove order from approprite side of the book
    if (order->IsBuy()) {
        auto bookIt = m_bids.find(price);
        if (bookIt != m_bids.end()) {
            bookIt->second.erase(orderLoc);
        }

        if (bookIt->second.empty()) {
            m_bids.erase(bookIt);
        }
    } else {
        auto bookIt = m_asks.find(price);
        if (bookIt != m_asks.end()) {
            bookIt->second.erase(orderLoc);
        }

        if (bookIt->second.empty()) {
            m_asks.erase(bookIt);
        }
    }

    orderMap.erase(orderId);
}
