//
// Created by v2ray on 2023/4/9.
//

#ifndef GPT3BOT_TOKENUTILS_H
#define GPT3BOT_TOKENUTILS_H

#include "../cpp-tiktoken/encoding.h"
#include "utility"
#include "boost/algorithm/string.hpp"
#include "nlohmann/json.hpp"

namespace util {

    class max_tokens_exceeded : public std::exception {
        const std::string message;

    public:
        max_tokens_exceeded();
        explicit max_tokens_exceeded(std::string message);
        ~max_tokens_exceeded() override;

        [[nodiscard]] const char* what() const noexcept override;
    };

    unsigned int get_token_count(const nlohmann::json& messages, const std::string& model_name = "gpt-3.5-turbo");
    unsigned int get_token_count(const std::string& text, const std::string& model_name = "gpt-3.5-turbo");
    LanguageModel get_tokenizer(const std::string& model_name);
    unsigned int get_max_tokens(const std::string& model_name);
    std::shared_ptr<GptEncoding> get_enc_cache(LanguageModel model);
} // util

#endif //GPT3BOT_TOKENUTILS_H
