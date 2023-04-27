//
// Created by v2ray on 2023/3/4.
//

#ifndef GPT3BOT_EXCHANGE_H
#define GPT3BOT_EXCHANGE_H

#include "../model/Embedding.h"
#include "utility"

namespace chat {

    class Exchange {
        std::string input_;
        std::vector<float> input_embeddings_;
        std::string response_;
        std::vector<float> response_embeddings_;
        long long time_ms_;

    public:
        Exchange(std::string input, std::vector<float> input_embeddings, long long time_ms);
        Exchange(std::string input, std::vector<float> input_embeddings, std::string response, long long time_ms);
        Exchange(std::string input, std::vector<float> input_embeddings, std::string response,
                 std::vector<float> response_embeddings, long long int time_ms);
        virtual ~Exchange();
        bool operator==(const Exchange& rhs) const;
        bool operator!=(const Exchange& rhs) const;
        friend std::size_t hash_value(const Exchange& instance);

        [[nodiscard]] const std::string& getInput() const;
        [[nodiscard]] const std::vector<float>& getInputEmbeddings() const;
        [[nodiscard]] const std::string& getResponse() const;
        [[nodiscard]] bool hasResponse() const;
        void setResponse(const std::string& response);
        [[nodiscard]] const std::vector<float>& getResponseEmbeddings() const;
        [[nodiscard]] bool hasResponseEmbeddings() const;
        void setResponseEmbeddings(const std::vector<float>& response_embeddings);
        [[nodiscard]] const long long& getTimeMS() const;
    };
} // chat

namespace std {

    template <>
    struct hash<chat::Exchange> {
        size_t operator()(const chat::Exchange& instance) const;
    };
}

#endif //GPT3BOT_EXCHANGE_H
