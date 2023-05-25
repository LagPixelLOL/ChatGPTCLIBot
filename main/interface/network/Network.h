//
// Created by v2ray on 2023/3/13.
//

#ifndef GPT3BOT_NETWORK_H
#define GPT3BOT_NETWORK_H

#include "../util/PromptUtils.h"
#include "../data/Messages.h"
#include "boost/algorithm/string/regex.hpp"

namespace api {

    void call_api(const std::string& constructed_initial, const chat::Messages& chat_history, const std::string& api_key,
                  const std::string& model = "gpt-3.5-turbo", const float& temperature = 1, const int& max_tokens = 500,
                  const float& top_p = 1, const float& frequency_penalty = 0, const float& presence_penalty = 0.6,
                  const std::vector<std::pair<std::string, float>>& logit_bias = {}, const std::string& me_id = "Me",
                  const std::string& bot_id = "You", const std::function<void(const std::string& streamed_response)>& stream_callback
                  = [](const auto&){}, const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                  = [](auto, auto, auto, auto){return 0;});
    bool is_new_api(const std::string& model_name);
    void handle_streamed_response(const std::vector<char>& raw_vec, const bool& is_new_api,
                                  const std::function<void(const std::string& streamed_response)>& stream_callback);
    nlohmann::json to_payload(const std::string& constructed_initial, const chat::Messages& chat_history,
                              const std::string& model = "gpt-3.5-turbo", const bool& is_new_api_ = true, const float& temperature = 1,
                              const int& max_tokens = 500, const float& top_p = 1, const float& frequency_penalty = 0,
                              const float& presence_penalty = 0.6, const std::vector<std::pair<std::string, float>>& logit_bias = {},
                              const std::string& me_id = "Me", const std::string& bot_id = "You");
} // api

#endif //GPT3BOT_NETWORK_H
