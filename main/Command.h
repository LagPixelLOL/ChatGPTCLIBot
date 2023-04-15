//
// Created by v2ray on 2023/4/15.
//

#ifndef GPT3BOT_COMMAND_H
#define GPT3BOT_COMMAND_H

#include "GPTMain.h"

namespace cmd {

    enum class Commands : uint16_t {
        NONE = 0,
        STOP,
        UNDO,
        RESET,
        UWU,
        TOKENIZER
    };

    enum class ReturnOpCode : uint16_t {
        NONE = 0,
        CONTINUE,
        STOP
    };

    ReturnOpCode handle_command(const std::string& input, std::vector<std::shared_ptr<chat::Exchange>>& prompts);
} // cmd

#endif //GPT3BOT_COMMAND_H
