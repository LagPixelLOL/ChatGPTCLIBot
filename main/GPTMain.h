//
// Created by v2ray on 2023/2/18.
//

#ifndef GPT3BOT_GPTMAIN_H
#define GPT3BOT_GPTMAIN_H

#include "interface/network/Network.h"
#include "fstream"

namespace GPT {
    using namespace std;
    using namespace boost;
    using namespace filesystem;
    using namespace nlohmann;
    using namespace chat;

    void pre_settings();
    void start_loop();
    void print_prompt();
    void print_enter_next_cycle();
    void print_uwu();
    void clear_console();
    int handle_command(const string& input);
    bool create_folders(const vector<string>& folders);
    bool p_default_prompt();
    bool p_load_prompt(string filename);
    bool p_load_saved(string filename);
    bool p_save_chat(string name);
    bool p_load_config();
    bool p_save_config();
}

#endif //GPT3BOT_GPTMAIN_H
