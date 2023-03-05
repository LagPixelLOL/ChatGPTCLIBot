//
// Created by v2ray on 2023/3/3.
//

#ifndef GPT3BOT_SYSTEMUTILS_H
#define GPT3BOT_SYSTEMUTILS_H

#include "boost/algorithm/string.hpp"
#include "proxy.h"
#include "string"
#include "vector"
#include "chrono"

namespace util {
    using namespace std;
    using namespace boost;
    using namespace chrono;

    long long currentTimeMillis();
    string currentTimeFormatted();
    string ms_to_formatted_time(long long timeMillis);
    string system_proxy();
    void free_proxy_factory();
} // util

#endif //GPT3BOT_SYSTEMUTILS_H
