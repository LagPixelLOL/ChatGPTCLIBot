//
// Created by v2ray on 2023/2/18.
//

#ifndef GPT3BOT_GPTMAIN_H
#define GPT3BOT_GPTMAIN_H

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/regex.hpp"
#include "boost/format.hpp"
#include "util/PromptUtils.h"
#include "chat/Exchange.h"
#include "fstream"

namespace GPT {
    using namespace std;
    using namespace boost;
    using namespace filesystem;
    using namespace nlohmann;
    using namespace chat;

    void pre_settings();
    void start_loop();
    void call_api();
    size_t write_callback(char* char_ptr, size_t size, size_t mem, string* base_str);
    void print_prompt();
    void print_enter_next_cycle();
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
