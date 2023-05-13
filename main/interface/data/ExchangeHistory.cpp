//
// Created by v2ray on 2023/5/10.
//

#include "ExchangeHistory.h"

namespace chat {

    //class ExchangeHistory start:
    ExchangeHistory::ExchangeHistory() = default;

    ExchangeHistory::ExchangeHistory(const ExchangeHistory::Memory& memory) : MemoryAdaptor(memory) {}

    /**
     * @throw std::invalid_argument If j is not an array or chat::Exchange constructor throws.
     */
    ExchangeHistory::ExchangeHistory(const nlohmann::json& j) {
        if (!j.is_array()) {
            throw std::invalid_argument("Argument is not an array.");
        }
        for (const auto& e : j) {
            push_back(std::make_shared<Exchange>(e));
        }
    }

    ExchangeHistory::~ExchangeHistory() = default;

    ExchangeHistory& ExchangeHistory::operator=(const MemoryAdaptor& other) {
        MemoryAdaptor::operator=(other);
        return *this;
    }

    Messages ExchangeHistory::to_messages() const {
        Messages messages;
        for (const auto& e : *this) {
            messages.emplace_back(e->getInput(), Messages::Role::USER);
            if (e->hasResponse()) {
                messages.emplace_back(e->getResponse(), Messages::Role::ASSISTANT);
            }
        }
        return messages;
    }

    nlohmann::json ExchangeHistory::to_json() const {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& e : *this) {
            j.push_back(e->to_json());
        }
        return j;
    }
    //class ExchangeHistory end.
} // db