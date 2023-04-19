//
// Created by v2ray on 2023/3/2.
//

#ifndef GPT3BOT_PROMPTUTILS_H
#define GPT3BOT_PROMPTUTILS_H

#include "../chat/Exchange.h"
#include "boost/format.hpp"
#include "string"
#include "vector"

namespace prompt {

    void print_prompt(const std::string& initial_prompt, const std::vector<std::shared_ptr<chat::Exchange>>& prompts,
                      const std::string& me_id, const std::string& bot_id, const unsigned int& max_length,
                      const bool& is_new_api, const bool& space_between_exchanges);
    std::string to_string(std::string initial_prompt, std::vector<std::shared_ptr<chat::Exchange>> prompts,
                          const std::string& me_id, const std::string& bot_id, const unsigned int& max_length,
                          const bool& add_color = false, const bool& space_between_exchanges = false);
    std::string construct_reference(std::string initial_prompt, const std::vector<float>& input_embeddings,
                                    std::vector<std::shared_ptr<chat::Exchange>> chat_exchanges, const bool& search_response,
                                    const unsigned int& max_reference_length, const unsigned int& max_short_memory_length,
                                    const std::string& me_id, const std::string& bot_id);
} // prompt

namespace GPT {

    std::string to_payload(const std::string& initial_prompt, const std::vector<std::shared_ptr<chat::Exchange>>& prompts,
                           const std::string& me_id, const std::string& bot_id, const unsigned int& max_length);
} // GPT

namespace ChatGPT {

    nlohmann::json to_payload(std::string initial_prompt, std::vector<std::shared_ptr<chat::Exchange>> prompts,
                              const std::string& me_id, const std::string& bot_id, const unsigned int& max_length);
} // ChatGPT

#endif //GPT3BOT_PROMPTUTILS_H
