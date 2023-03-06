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
    using namespace std;
    using namespace boost;
    using format = boost::format;

    void print_prompt(const string& initial_prompt, const vector<std::shared_ptr<chat::Exchange>>& prompts,
                      const string& me_id, const string& bot_id, const unsigned int& max_length, const bool& is_new_api);
    string to_string(string initial_prompt, vector<std::shared_ptr<chat::Exchange>> prompts,
                     const string& me_id, const string& bot_id, const unsigned int& max_length);
    string construct_reference(string initial_prompt, const vector<float>& input_embeddings,
                               vector<std::shared_ptr<chat::Exchange>> chat_exchanges,
                               const unsigned int& max_reference_length, const unsigned int& max_short_memory_length,
                               const string& me_id, const string& bot_id);
} // prompt

namespace GPT {
    using namespace std;

    string to_payload(const string& initial_prompt, const vector<std::shared_ptr<chat::Exchange>>& prompts,
                      const string& me_id, const string& bot_id, const unsigned int& max_length);
} // GPT

namespace ChatGPT {
    using namespace std;
    using namespace boost;
    using namespace nlohmann;

    json to_payload(string initial_prompt, vector<std::shared_ptr<chat::Exchange>> prompts,
                    const string& me_id, const string& bot_id, const unsigned int& max_length);
} // ChatGPT

#endif //GPT3BOT_PROMPTUTILS_H
