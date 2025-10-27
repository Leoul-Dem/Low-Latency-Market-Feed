//
// Created by MyPC on 10/26/2025.
//

#ifndef LOW_LATENCY_MARKET_FEED_BINANCE_FEED_PARSER_H
#define LOW_LATENCY_MARKET_FEED_BINANCE_FEED_PARSER_H

#include <string>
#include <cstdint>

class Binance_Feed{
private:
    //std::string Event_Type;
    uint64_t Event_Time;
    std::string Symbol;
    uint32_t TradeID;
    std::string Price;
    std::string Quantity;
    //uint64_t Trade_Time;
    bool Buyer;

public:
    Binance_Feed(uint64_t event_time_, std::string symbol_, uint32_t tradeid_,
                std::string price_, std::string quantity_, bool buyer_):
        Event_Time(event_time_),
        Symbol(symbol_),
        TradeID(tradeid_),
        Price(price_),
        Quantity(quantity_),
        Buyer(buyer_)
    {}

    ~Binance_Feed(){}

};

#endif //LOW_LATENCY_MARKET_FEED_BINANCE_FEED_PARSER_H