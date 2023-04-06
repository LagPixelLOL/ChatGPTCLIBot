//
// Created by v2ray on 2023/3/13.
//

#ifndef GPT3BOT_NETWORK_H
#define GPT3BOT_NETWORK_H

#include "boost/algorithm/string/regex.hpp"
#include "../util/PromptUtils.h"
#include "../util/TermUtils.h"

namespace api {
    using namespace std;
    using namespace boost;
    using namespace nlohmann;

    bool call_api(const string& initial_prompt, const vector<std::shared_ptr<chat::Exchange>>& chat_exchanges,
                  const string& api_key, const string& model, const float& temperature, const int& max_tokens,
                  const float& top_p, const float& frequency_penalty, const float& presence_penalty,
                  const unordered_map<uint16_t, float>& logit_bias,
                  const unsigned int& max_short_memory_length, const unsigned int& max_reference_length,
                  const string& me_id, const string& bot_id, std::function<void(const string& streamed_response)> callback,
                  const bool& debug_reference = false, const bool& pause_when_showing_reference = true);
    bool is_new_api(const string& model_name);
} // api

#endif //GPT3BOT_NETWORK_H
