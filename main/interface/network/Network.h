//
// Created by v2ray on 2023/3/13.
//

#ifndef GPT3BOT_NETWORK_H
#define GPT3BOT_NETWORK_H

#include "../util/PromptUtils.h"
#include "boost/algorithm/string/regex.hpp"

namespace api {

    void call_api(const std::string& constructed_initial, const std::shared_ptr<chat::ExchangeHistory>& chat_history,
                  const std::string& api_key, const std::string& model, const float& temperature, const int& max_tokens,
                  const float& top_p, const float& frequency_penalty, const float& presence_penalty,
                  const std::vector<std::pair<std::string, float>>& logit_bias, const unsigned int& max_short_memory_length,
                  const std::string& me_id, const std::string& bot_id,
                  const std::function<void(const std::string& streamed_response)>& stream_callback,
                  const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                  = [](auto, auto, auto, auto){return 0;});
    bool is_new_api(const std::string& model_name);
} // api

#endif //GPT3BOT_NETWORK_H
