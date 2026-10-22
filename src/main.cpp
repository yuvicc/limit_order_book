#include <iostream>
#include <cassert>
#include "orderbook.h"

static OrderId nextId = 1;

std::shared_ptr<Order> makeLimitBuy(Price price, Quantity qty, TimeInForce tif = TimeInForce::GTC) {
    return std::make_shared<Order>(nextId++, OrderType::Limit, price, qty, Side::Buy, tif);
}

std::shared_ptr<Order> makeLimitSell(Price price, Quantity qty, TimeInForce tif = TimeInForce::GTC) {
    return std::make_shared<Order>(nextId++, OrderType::Limit, price, qty, Side::Sell, tif);
}

void printTrades(const Trades& trades) {
    if (trades.empty()) {
        std::cout << "  [no trades]\n";
        return;
    }
    for (const auto& t : trades) {
        std::cout << "  TRADE => "
                  << "BidOrder#" << t.bidInfo.orderId
                  << " x AskOrder#" << t.askInfo.orderId
                  << " | qty: "    << t.bidInfo.qty
                  << " @ price: "  << t.bidInfo.price
                  << "\n";
    }
}

void section(const std::string& title) {
    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "══════════════════════════════════════\n";
}

// ── Tests ──────────────────────────────────────────────────────────────────

// 1. No match — orders rest in the book
void testNoMatch() {
    section("TEST 1: No match — orders rest");
    Orderbook book;

    auto buy  = makeLimitBuy (100, 10);
    auto sell = makeLimitSell(101, 10);   // ask > bid, no cross

    auto t1 = book.addOrder(buy);
    auto t2 = book.addOrder(sell);

    printTrades(t1);
    printTrades(t2);

    assert(t1.empty() && t2.empty());
    assert(book.size() == 2);
    std::cout << "  PASSED\n";
}

// 2. Full match — both orders completely filled
void testFullMatch() {
    section("TEST 2: Full match");
    Orderbook book;

    auto buy  = makeLimitBuy (100, 10);
    auto sell = makeLimitSell(100, 10);

    book.addOrder(buy);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.qty == 10);
    assert(trades[0].askInfo.qty == 10);
    assert(trades[0].bidInfo.price == 100);
    assert(buy->GetOrderStatus() == OrderStatus::Filled);
    assert(sell->GetOrderStatus() == OrderStatus::Filled);
    assert(book.size() == 0);             // both orders gone
    std::cout << "  PASSED\n";
}

// 3. Partial match — larger order partially filled, rests in book
void testPartialMatch() {
    section("TEST 3: Partial match");
    Orderbook book;

    auto buy  = makeLimitBuy (100, 20);
    auto sell = makeLimitSell(100, 10);   // sell is smaller

    book.addOrder(buy);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.qty == 10);  // only 10 filled
    assert(book.size() == 1);             // buy order still resting
    assert(buy->GetRemainingQuantity() == 10);
    assert(buy->GetOrderStatus() == OrderStatus::PartiallyFilled);
    assert(book.getBestBidPrice() == 100);
    std::cout << "  PASSED\n";
}

// 4. Cancel order — order removed, no longer matches
void testCancel() {
    section("TEST 4: Cancel order");
    Orderbook book;

    auto buy  = makeLimitBuy(100, 10);
    book.addOrder(buy);
    assert(book.size() == 1);

    book.cancelOrder(buy->GetOrderId());
    assert(book.size() == 0);
    assert(buy->IsCancelled());
    assert(!book.hasBids());

    // Now add a matching sell — should NOT trade
    auto sell = makeLimitSell(100, 10);
    auto trades = book.addOrder(sell);
    printTrades(trades);

    assert(trades.empty());
    assert(book.size() == 1);             // sell rests unmatched
    std::cout << "  PASSED\n";
}

// 5. Multiple price levels — best price matched first
void testPricePriority() {
    section("TEST 5: Price priority");
    Orderbook book;

    auto buy1 = makeLimitBuy(99,  10);
    auto buy2 = makeLimitBuy(100, 10);   // better bid — matched first
    book.addOrder(buy1);
    book.addOrder(buy2);

    auto sell   = makeLimitSell(99, 10);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.orderId == buy2->GetOrderId()); // best bid matched
    assert(trades[0].bidInfo.price == 100);   // executes at the resting bid, not the sell's 99
    assert(book.size() == 1);            // buy1 still resting
    std::cout << "  PASSED\n";
}

// 6. FIFO at same price level
void testTimePriority() {
    section("TEST 6: FIFO / time priority at same price");
    Orderbook book;

    auto buy1 = makeLimitBuy(100, 10);   // arrives first
    auto buy2 = makeLimitBuy(100, 10);
    book.addOrder(buy1);
    book.addOrder(buy2);

    auto sell   = makeLimitSell(100, 10);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.orderId == buy1->GetOrderId()); // first in, first matched
    assert(book.size() == 1);
    std::cout << "  PASSED\n";
}

// 7. Duplicate OrderId — must throw
void testDuplicateOrderId() {
    section("TEST 7: Duplicate OrderId throws");
    Orderbook book;

    auto buy = makeLimitBuy(100, 10);
    book.addOrder(buy);

    // Re-use the same order pointer (same ID)
    try {
        book.addOrder(buy);
        assert(false && "Should have thrown");
    } catch (const std::logic_error& e) {
        std::cout << "  Caught expected exception: " << e.what() << "\n";
    }
    std::cout << "  PASSED\n";
}

// 8. Multi-level sweep — one large order eats multiple price levels
void testMultiLevelSweep() {
    section("TEST 8: Multi-level sweep");
    Orderbook book;

    auto s1 = makeLimitSell(100, 5);
    auto s2 = makeLimitSell(101, 5);
    auto s3 = makeLimitSell(102, 5);
    book.addOrder(s1);
    book.addOrder(s2);
    book.addOrder(s3);

    auto buy    = makeLimitBuy(102, 15);  // sweeps all three levels
    auto trades = book.addOrder(buy);

    printTrades(trades);

    assert(trades.size() == 3);
    assert(trades[0].askInfo.price == 100);
    assert(trades[1].askInfo.price == 101);
    assert(trades[2].askInfo.price == 102);
    assert(book.size() == 0);
    std::cout << "  PASSED\n";
}

// 9. Aggressive buy crosses the spread — trade at the resting ask price
void testAggressorGetsMakerPrice() {
    section("TEST 9: Aggressive buy fills at resting ask price");
    Orderbook book;

    auto sell = makeLimitSell(100, 10);
    book.addOrder(sell);

    auto buy    = makeLimitBuy(105, 10);  // willing to pay 105, but 100 is available
    auto trades = book.addOrder(buy);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.price == 100);
    std::cout << "  PASSED\n";
}

// 10. Sell sweeps multiple bid levels, best (highest) first
void testSellSideSweep() {
    section("TEST 10: Sell-side multi-level sweep");
    Orderbook book;

    auto b1 = makeLimitBuy(100, 5);
    auto b2 = makeLimitBuy(99,  5);
    auto b3 = makeLimitBuy(98,  5);
    book.addOrder(b1);
    book.addOrder(b2);
    book.addOrder(b3);

    auto sell   = makeLimitSell(99, 20);  // takes 100 and 99, stops before 98
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 2);
    assert(trades[0].bidInfo.orderId == b1->GetOrderId() && trades[0].bidInfo.price == 100);
    assert(trades[1].bidInfo.orderId == b2->GetOrderId() && trades[1].bidInfo.price == 99);
    assert(sell->GetRemainingQuantity() == 10);
    assert(book.getBestAskPrice() == 99);  // remainder rests as the new best ask
    assert(book.getBestBidPrice() == 98);
    assert(book.size() == 2);
    std::cout << "  PASSED\n";
}

// 11. Cancel a partially filled order, and cancel an unknown id (no-op)
void testCancelPartiallyFilled() {
    section("TEST 11: Cancel partially filled + unknown id");
    Orderbook book;

    auto buy = makeLimitBuy(100, 20);
    book.addOrder(buy);
    book.addOrder(makeLimitSell(100, 5));
    assert(buy->GetRemainingQuantity() == 15);

    book.cancelOrder(buy->GetOrderId());
    assert(book.size() == 0);
    assert(buy->IsCancelled());

    book.cancelOrder(9999);               // must not throw or crash
    assert(book.size() == 0);
    std::cout << "  PASSED\n";
}

// 12. IOC — fills what it can, remainder is cancelled (never rests)
void testIOC() {
    section("TEST 12: IOC partial fill, remainder cancelled");
    Orderbook book;

    book.addOrder(makeLimitSell(100, 5));

    auto ioc    = makeLimitBuy(100, 10, TimeInForce::IOC);
    auto trades = book.addOrder(ioc);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.qty == 5);
    assert(ioc->IsCancelled());
    assert(ioc->GetRemainingQuantity() == 5);
    assert(book.size() == 0);             // the IOC remainder did not rest
    std::cout << "  PASSED\n";
}

// 13. IOC with no liquidity — no trades, nothing rests
void testIOCNoLiquidity() {
    section("TEST 13: IOC with no liquidity");
    Orderbook book;

    book.addOrder(makeLimitSell(101, 5)); // above the IOC limit

    auto ioc    = makeLimitBuy(100, 10, TimeInForce::IOC);
    auto trades = book.addOrder(ioc);

    printTrades(trades);

    assert(trades.empty());
    assert(ioc->IsCancelled());
    assert(book.size() == 1);             // only the original sell
    assert(!book.hasBids());
    std::cout << "  PASSED\n";
}

// 14. FOK — not enough liquidity, rejected and book untouched
void testFOKReject() {
    section("TEST 14: FOK rejected when it cannot fully fill");
    Orderbook book;

    auto s1 = makeLimitSell(100, 5);
    auto s2 = makeLimitSell(101, 5);
    book.addOrder(s1);
    book.addOrder(s2);

    auto fok    = makeLimitBuy(101, 15, TimeInForce::FOK);  // only 10 available
    auto trades = book.addOrder(fok);

    printTrades(trades);

    assert(trades.empty());
    assert(s1->GetRemainingQuantity() == 5);
    assert(s2->GetRemainingQuantity() == 5);
    assert(book.size() == 2);
    std::cout << "  PASSED\n";
}

// 15. FOK — enough liquidity across levels, fully filled
void testFOKFill() {
    section("TEST 15: FOK fully filled across levels");
    Orderbook book;

    book.addOrder(makeLimitSell(100, 5));
    book.addOrder(makeLimitSell(101, 5));

    auto fok    = makeLimitBuy(101, 10, TimeInForce::FOK);
    auto trades = book.addOrder(fok);

    printTrades(trades);

    assert(trades.size() == 2);
    assert(fok->IsFilled());
    assert(book.size() == 0);
    std::cout << "  PASSED\n";
}

// 16. Pro-rata — fills split proportionally to resting size
void testProRataProportional() {
    section("TEST 16: Pro-rata proportional allocation");
    Orderbook book(MatchingAlgo::ProRata);

    auto s1 = makeLimitSell(100, 30);
    auto s2 = makeLimitSell(100, 20);
    auto s3 = makeLimitSell(100, 50);
    book.addOrder(s1);
    book.addOrder(s2);
    book.addOrder(s3);

    auto buy    = makeLimitBuy(100, 10);  // 10% of the 100 lots resting
    auto trades = book.addOrder(buy);

    printTrades(trades);

    assert(trades.size() == 3);
    assert(s1->GetFilledQuantity() == 3);
    assert(s2->GetFilledQuantity() == 2);
    assert(s3->GetFilledQuantity() == 5);
    assert(trades[0].bidInfo.orderId == buy->GetOrderId());  // taker is the buyer
    assert(trades[0].askInfo.orderId == s1->GetOrderId());
    assert(buy->IsFilled());
    assert(book.size() == 3);
    std::cout << "  PASSED\n";
}

// 17. Pro-rata — rounding remainder goes to the largest unfilled order
void testProRataRemainder() {
    section("TEST 17: Pro-rata remainder allocation");
    Orderbook book(MatchingAlgo::ProRata);

    auto s1 = makeLimitSell(100, 30);
    auto s2 = makeLimitSell(100, 20);
    auto s3 = makeLimitSell(100, 50);
    book.addOrder(s1);
    book.addOrder(s2);
    book.addOrder(s3);

    // floor(7*.3)=2, floor(7*.2)=1, floor(7*.5)=3 -> 6, leftover 1 lot to s3
    auto buy    = makeLimitBuy(100, 7);
    auto trades = book.addOrder(buy);

    printTrades(trades);

    assert(s1->GetFilledQuantity() == 2);
    assert(s2->GetFilledQuantity() == 1);
    assert(s3->GetFilledQuantity() == 4);
    assert(buy->IsFilled());
    std::cout << "  PASSED\n";
}

// 18. Pro-rata — sell taker sweeps a level, leftover rests in the book
void testProRataSweepAndRest() {
    section("TEST 18: Pro-rata sweep, remainder rests");
    Orderbook book(MatchingAlgo::ProRata);

    auto b1 = makeLimitBuy(100, 5);
    auto b2 = makeLimitBuy(100, 5);
    book.addOrder(b1);
    book.addOrder(b2);

    auto sell   = makeLimitSell(100, 15);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 2);
    assert(trades[0].askInfo.orderId == sell->GetOrderId());  // taker is the seller
    assert(b1->IsFilled() && b2->IsFilled());
    assert(sell->GetRemainingQuantity() == 5);
    assert(!book.hasBids());
    assert(book.getBestAskPrice() == 100);
    assert(book.size() == 1);
    std::cout << "  PASSED\n";
}

int main() {
    testNoMatch();
    testFullMatch();
    testPartialMatch();
    testCancel();
    testPricePriority();
    testTimePriority();
    testDuplicateOrderId();
    testMultiLevelSweep();
    testAggressorGetsMakerPrice();
    testSellSideSweep();
    testCancelPartiallyFilled();
    testIOC();
    testIOCNoLiquidity();
    testFOKReject();
    testFOKFill();
    testProRataProportional();
    testProRataRemainder();
    testProRataSweepAndRest();

    std::cout << "\nAll tests passed.\n";
    return 0;
}
