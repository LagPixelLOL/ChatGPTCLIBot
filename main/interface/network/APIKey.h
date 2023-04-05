//
// Created by v2ray on 2023/4/5.
//

#ifndef GPT3BOT_APIKEY_H
#define GPT3BOT_APIKEY_H

#include "nlohmann/json.hpp"
#include "../util/TermUtils.h"
#include "../util/CURLUtils.h"
#include "../util/SystemUtils.h"

namespace api {
    using namespace std;
    using namespace nlohmann;

    enum class APIKeyStatus {
        VALID = 0,
        EMPTY,
        INVALID_PREFIX,
        INVALID_LENGTH,
        QUOTA_EXCEEDED,
        INVALID_KEY,
        API_REQUEST_FAILED
    };

    string get_key_from_console();
    size_t get_key_count();
    void remove_first_key();
    bool has_key();
    string get_key();
    vector<string> get_keys();
    void set_key(const string& api_key);
    void set_key(const vector<string>& api_keys);
    APIKeyStatus check_key(const string& api_key);
    bool check_err_obj(const json& json_to_check, APIKeyStatus& status_in, const bool& print_err_msg = true);
} // api

#endif //GPT3BOT_APIKEY_H
