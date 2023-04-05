//
// Created by v2ray on 2023/3/6.
//

#ifndef GPT3BOT_CURLUTILS_H
#define GPT3BOT_CURLUTILS_H

#include "curl/curl.h"
#include "string"

namespace util {
    using namespace std;

    void set_curl_proxy(CURL* curl, const string& proxy);
    void set_curl_ssl_cert(CURL* curl);
    void set_ca_bundle_path(const string& path);
    string get_ca_bundle_path();
} // util

#endif //GPT3BOT_CURLUTILS_H
