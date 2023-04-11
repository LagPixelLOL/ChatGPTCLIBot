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
        long long time_ms_;

    public:
        Exchange(std::string input, std::vector<float> input_embeddings, long long time_ms);
        Exchange(std::string input, std::vector<float> input_embeddings, std::string response, long long time_ms);
        virtual ~Exchange();

        [[nodiscard]] double compare_similarity(const std::vector<float>& embeddings) const;

        [[nodiscard]] const std::string& getInput() const;
        [[nodiscard]] const std::vector<float>& getInputEmbeddings() const;
        [[nodiscard]] const std::string& getResponse() const;
        [[nodiscard]] bool hasResponse() const;
        void setResponse(const std::string& response);
        [[nodiscard]] long long getTimeMS() const;
    };
} // chat

#endif //GPT3BOT_EXCHANGE_H
