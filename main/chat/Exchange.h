//
// Created by v2ray on 2023/3/4.
//

#ifndef GPT3BOT_EXCHANGE_H
#define GPT3BOT_EXCHANGE_H

#include "../model/Embedding.h"
#include "utility"

namespace chat {
    using namespace std;

    class Exchange {
        string input_;
        vector<float> input_embeddings_;
        string response_;
        long long time_ms_;

    public:
        Exchange(string input, vector<float> input_embeddings, long long time_ms);
        Exchange(string input, vector<float> input_embeddings, string response, long long time_ms);
        virtual ~Exchange();

        [[nodiscard]] double compare_similarity(const vector<float>& embeddings) const;

        [[nodiscard]] const string& getInput() const;
        [[nodiscard]] const vector<float>& getInputEmbeddings() const;
        [[nodiscard]] const string& getResponse() const;
        [[nodiscard]] bool hasResponse() const;
        void setResponse(const string& response);
        [[nodiscard]] long long getTimeMS() const;
    };
} // chat

#endif //GPT3BOT_EXCHANGE_H
