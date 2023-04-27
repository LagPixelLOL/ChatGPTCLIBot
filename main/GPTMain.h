//
// Created by v2ray on 2023/2/18.
//

#ifndef GPT3BOT_GPTMAIN_H
#define GPT3BOT_GPTMAIN_H

#include "interface/network/Network.h"
#include "interface/util/DocUtils.h"
#include "Command.h"
#include "FineTuneHelper.h"

#define PATH_S(path) Term::color_fg(125, 225, 255) + "\"" + path.string() + "\""
#define ENTER Term::color_fg(70, 200, 255) + "Enter" + Term::color_fg(Term::Color::Name::Default)
#define GOLDEN_TEXT(n) Term::color_fg(255, 200, 0) + n + Term::color_fg(Term::Color::Name::Default)

namespace GPT {
    using Color = Term::Color;

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
    bool p_load_docQA(std::string filename);
    bool p_create_docQA();
    bool p_load_config();
    bool p_save_config();
    bool p_check_set_api_key();
    void p_on_invalid_key();
    const std::string& get_f_suffix();
    const std::string& get_json_suffix();
    const std::string& get_me_id();
    const std::string& get_bot_id();
}

#endif //GPT3BOT_GPTMAIN_H
