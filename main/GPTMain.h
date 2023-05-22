//
// Created by v2ray on 2023/2/18.
//

#ifndef GPT3BOT_GPTMAIN_H
#define GPT3BOT_GPTMAIN_H

#include "interface/network/Network.h"
#include "Config.h"
#include "Command.h"
#include "FineTuneHelper.h"
#include "Translator.h"
#include "csignal"

#define PATH_S(path) Term::color_fg(125, 225, 255) + "\"" + (path).string() + "\""
#define ENTER Term::color_fg(70, 200, 255) + "Enter" + Term::color_fg(Term::Color::Name::Default)
#define GOLDEN_TEXT(n) Term::color_fg(255, 200, 0) + (n) + Term::color_fg(Term::Color::Name::Default)

namespace GPT {
    using Color = Term::Color;

    inline const std::string f_suffix = ".txt";
    inline const std::string json_suffix = ".json";
    inline const std::string me_id = "Me";
    inline const std::string bot_id = "You";

    void pre_settings();
    void start_loop();
    void print_prompt();
    void print_enter_next_cycle();
    void clear_console();
    bool create_folders(const std::vector<std::string>& folders);
    bool p_default_prompt();
    bool p_load_prompt(std::string filename);
    bool p_load_saved(std::string filename);
    bool p_save_chat(std::string name);
    bool p_create_docQA();
    bool p_check_set_api_key();
    void p_on_invalid_key();
}

#endif //GPT3BOT_GPTMAIN_H
