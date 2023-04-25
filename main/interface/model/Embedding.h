//
// Created by v2ray on 2023/3/3.
//

#ifndef GPT3BOT_EMBEDDING_H
#define GPT3BOT_EMBEDDING_H

#include "../util/TokenUtils.h"
#include "../network/APIKey.h"
#include "iostream"
#include "cmath"

namespace emb {

    double cosine_similarity(const std::vector<float>& vec_a, const std::vector<float>& vec_b);
    std::vector<std::vector<float>> get_embeddings(const std::vector<std::string>& texts, const std::string& api_key,
                                                   api::APIKeyStatus& key_status_in);
} // emb

#endif //GPT3BOT_EMBEDDING_H
