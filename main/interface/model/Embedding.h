//
// Created by v2ray on 2023/3/3.
//

#ifndef GPT3BOT_EMBEDDING_H
#define GPT3BOT_EMBEDDING_H

#include "nlohmann/json.hpp"
#include "../util/SystemUtils.h"
#include "../util/CURLUtils.h"
#include "../util/TermUtils.h"
#include "../util/TokenUtils.h"
#include "../network/APIKey.h"
#include "iostream"
#include "string"
#include "vector"
#include "cmath"

namespace emb {

    double cosine_similarity(const std::vector<float>& vec_a, const std::vector<float>& vec_b);
    std::pair<std::shared_ptr<std::vector<float>>, api::APIKeyStatus> get_embeddings(const std::string& text, const std::string& api_key);
} // emb

#endif //GPT3BOT_EMBEDDING_H
