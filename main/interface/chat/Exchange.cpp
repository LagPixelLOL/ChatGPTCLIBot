//
// Created by v2ray on 2023/3/4.
//

#include "Exchange.h"

namespace chat {

    //class Exchange start:
    Exchange::Exchange(std::string input, std::vector<float> input_embeddings, long long time_ms) :
    input_(std::move(input)), input_embeddings_(std::move(input_embeddings)), time_ms_(time_ms) {}

    Exchange::Exchange(std::string input, std::vector<float> input_embeddings, std::string response, long long time_ms) :
    Exchange(std::move(input), std::move(input_embeddings), time_ms) {
        response_ = std::move(response);
    }

    Exchange::~Exchange() = default;

    /**
     * Compare the similarity between this and another embeddings.
     * @param embeddings The embeddings to compare with.
     * @return The similarity between this and embeddings, value > 0.8 is considered similar.
     */
    double Exchange::compare_similarity(const std::vector<float>& embeddings) const {
        return emb::cosine_similarity(input_embeddings_, embeddings);
    }

    const std::string& Exchange::getInput() const {
        return input_;
    }

    const std::vector<float>& Exchange::getInputEmbeddings() const {
        return input_embeddings_;
    }

    const std::string& Exchange::getResponse() const {
        return response_;
    }

    bool Exchange::hasResponse() const {
        return !response_.empty();
    }

    void Exchange::setResponse(const std::string& response) {
        response_ = response;
    }

    long long Exchange::getTimeMS() const {
        return time_ms_;
    }
    //class Exchange end.
} // chat