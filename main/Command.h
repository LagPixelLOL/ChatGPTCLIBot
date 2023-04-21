//
// Created by v2ray on 2023/4/15.
//

#ifndef GPT3BOT_COMMAND_H
#define GPT3BOT_COMMAND_H

#include "GPTMain.h"
#include "interface/cpp-terminal/platforms/conversion.hpp"

namespace cmd {
    using Color = Term::Color;

    enum class Commands : uint16_t {
        NONE = 0,
        STOP,
        UNDO,
        RESET,
        UWU,
        TOKENIZE,
        DUMP
    };

    enum class ReturnOpCode : uint16_t {
        NONE = 0,
        CONTINUE,
        STOP
    };

    ReturnOpCode handle_command(const std::string& input, const std::string& initial_prompt,
                                std::vector<std::shared_ptr<chat::Exchange>>& prompts, const std::string& me_id, const std::string& bot_id,
                                const unsigned int& max_display_length, const bool& space_between_exchanges, const bool& documentQA_mode);
} // cmd

#endif //GPT3BOT_COMMAND_H
