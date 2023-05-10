//
// Created by v2ray on 2023/5/10.
//

#ifndef GPT3BOT_EXCHANGEHISTORY_H
#define GPT3BOT_EXCHANGEHISTORY_H

#include "../storage/MemoryAdaptor.h"
#include "Exchange.h"

namespace chat {

    class ExchangeHistory : public db::MemoryAdaptor<chat::Exchange> {
        using Memory = std::list<std::shared_ptr<chat::Exchange>>;

    public:
        ExchangeHistory();
        explicit ExchangeHistory(const Memory& memory);
        explicit ExchangeHistory(const nlohmann::json& j);
        ~ExchangeHistory() override;

        ExchangeHistory& operator=(const MemoryAdaptor& other) override;

        [[nodiscard]] nlohmann::json to_json() const override;
    };
} // db

#endif //GPT3BOT_EXCHANGEHISTORY_H
