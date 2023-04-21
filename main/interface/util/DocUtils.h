//
// Created by v2ray on 2023/4/20.
//

#ifndef GPT3BOT_DOCUTILS_H
#define GPT3BOT_DOCUTILS_H

#include "TokenUtils.h"
#include "../cpp-terminal/platforms/conversion.hpp"

namespace doc {

    std::vector<std::string> split_text(const std::string& text, const unsigned int& tokens_per_chunk);
} // doc

#endif //GPT3BOT_DOCUTILS_H
