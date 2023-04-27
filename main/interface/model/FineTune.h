//
// Created by v2ray on 2023/4/26.
//

#ifndef GPT3BOT_FINETUNE_H
#define GPT3BOT_FINETUNE_H

#include "../cpp-tiktoken/pcre2_regex.h"
#include "nlohmann/json.hpp"

namespace ft {

    enum class SourceType : uint32_t {
        NKPR_SCN, UNKNOWN
    };

    nlohmann::json convert(const std::string& source, const SourceType& source_type,
                           const std::string& prompt_prefix = "", const std::string& completion_prefix = "");
    std::string to_string(const SourceType& source_type);
    SourceType from_string(const std::string& source_type);
} // ft

#endif //GPT3BOT_FINETUNE_H
