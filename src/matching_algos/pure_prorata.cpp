#include "orderbook.h"
#include <memory>

Trades Orderbook::matchPureProrata(std::shared_ptr<Order> taker_order)
{
    Trades all_trades;

    while(taker_order->GetRemainingQuantity() > 0) {
        Price best_price;
        Side maker_side = taker_order->IsBuy() ? Side::Sell : Side::Buy;

        if (taker_order->IsBuy()) {
            if (!hasAsks()) break;
            best_price  = getBestAskPrice();
            if (taker_order->GetPrice() < best_price) break;
        } else {
            if (!hasBids()) break;
            best_price = getBestBidPrice();
            if (taker_order->GetPrice() > best_price) break;
        }

        PriceLevel level = getPriceLevel(best_price);
        if (level.orders.empty()) break;

        Quantity taker_qty = taker_order->GetRemainingQuantity();
        Quantity level_total_qty = level.qty;

        std::vector<std::pair<std::shared_ptr<Order>, Quantity>> allocations;
        allocations.reserve(level.orders.size());
        Quantity total_allocated = 0;

        // Base allocation - try as much as possible
        for (const auto& maker_order : level.orders) {
            Quantity maker_qty = maker_order->GetRemainingQuantity();
            Quantity fill = static_cast<Quantity>((static_cast<uint64_t>(taker_qty) * maker_qty) / level_total_qty);
            fill = std::min(fill, maker_qty);
            allocations.push_back({maker_order, fill}); 
            total_allocated += fill;
        }

        // Remainder distribution
        Quantity remainder = taker_qty - total_allocated;
        while(remainder > 0) {
            auto best_it = allocations.end();
            Quantity max_unfilled_qty = 0;

            for (auto it = allocations.begin(); it != allocations.end(); ++it) {
                Quantity unfilled_qty = it->first->GetRemainingQuantity() - it->second;
                if (unfilled_qty > max_unfilled_qty) {
                    max_unfilled_qty = unfilled_qty;
                    best_it = it;
                }
            }

            if (best_it != allocations.end()) {
                best_it->second += 1;
                remainder -= 1;
            } else {
                break;
            }
        }

        Quantity level_vol_filled = 0;
        for (auto& [maker_order, fill_qty] : allocations) {
            if (fill_qty > 0) {
                maker_order->Fill(fill_qty);
                taker_order->Fill(fill_qty);
                level_vol_filled += fill_qty;

                TradeInfo maker_info{ maker_order->GetOrderId(), fill_qty, best_price };
                TradeInfo taker_info{ taker_order->GetOrderId(), fill_qty, best_price };
                all_trades.push_back(taker_order->IsBuy()
                    ? Trade{ taker_info, maker_info }
                    : Trade{ maker_info, taker_info });
            }
        }
        cleanFilledOrders(best_price, maker_side);
        if (level_vol_filled == 0) break;
    }
    
    return all_trades;
}
