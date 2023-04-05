//
// Created by v2ray on 2023/3/3.
//

#ifndef GPT3BOT_EMBEDDING_H
#define GPT3BOT_EMBEDDING_H

#include "nlohmann/json.hpp"
#include "../util/SystemUtils.h"
#include "../util/CURLUtils.h"
#include "../util/TermUtils.h"
#include "../network/APIKey.h"
#include "iostream"
#include "string"
#include "vector"
#include "cmath"

namespace emb {
    using namespace std;
    using namespace nlohmann;
    using namespace api;

    double cosine_similarity(const vector<float>& vec_a, const vector<float>& vec_b);
    pair<shared_ptr<vector<float>>, APIKeyStatus> get_embeddings(const string& text, const string& api_key);
} // emb

#endif //GPT3BOT_EMBEDDING_H
