//
// Created by v2ray on 2023/4/9.
//

#ifndef GPT3BOT_TOKENUTILS_H
#define GPT3BOT_TOKENUTILS_H

#include "utility"
#include "boost/algorithm/string.hpp"
#include "nlohmann/json.hpp"
#include "../cpp-tiktoken/encoding.h"

namespace util {
    using namespace std;
    using namespace nlohmann;

    class max_tokens_exceeded : public std::exception {
        const string message;
    public:
        max_tokens_exceeded();
        explicit max_tokens_exceeded(string message);
        ~max_tokens_exceeded() override;

        [[nodiscard]] const char* what() const noexcept override;
    };

    unsigned int get_token_count(const json& messages, const string& model_name = "gpt-3.5-turbo");
    unsigned int get_token_count(const string& text, const string& model_name = "gpt-3.5-turbo");
    LanguageModel get_tokenizer(const string& model_name);
    unsigned int get_max_tokens(const string& model_name);
} // util

#endif //GPT3BOT_TOKENUTILS_H
