//
// Created by v2ray on 2023/3/3.
//

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "SystemUtils.h"

#ifdef _WIN32
#include "proxy.h"
#endif

namespace util {
#ifdef _WIN32
    pxProxyFactory* pf = px_proxy_factory_new();
#endif

    long long currentTimeMillis() {
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    string currentTimeFormatted() {
        return ms_to_formatted_time(currentTimeMillis());
    }

    string ms_to_formatted_time(long long timeMillis) {
        time_t time = timeMillis / 1000;
        struct tm time_info{};
#ifdef _MSC_VER
        localtime_s(&time_info, &time);
#else
        localtime_r(&time, &time_info);
#endif
        char buffer[80];
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &time_info);
        return buffer;
    }

    /**
     * Get multiple lines of input from the user.
     * Press Ctrl + N to enter a new line.
     * Note: Remember to do error handling by catching std::exception.
     * @param history Input history.
     * @param prompt_string The text that's displayed before the user's input.
     * @return The user's input.
     */
    string get_multi_lines(vector<string>& history, const string& prompt_string) {
        if (!Term::stdin_connected()) {
            throw Term::Exception("The terminal is not attached to a TTY and therefore can't catch user input.");
        }
        Term::Terminal t(false, false, false); //Initialize the terminal.
        return Term::prompt_multiline(prompt_string, history, [](const auto& s){
            if (s.size() > 1 && s.substr(s.size() - 2, 1) == "\\") {
                return false;
            }
            return true;
        });
    }

    void ignore_line() {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    /**
     * Get the current system proxy using libproxy.
     * @return Proxy string if found, empty string otherwise.
     */
    string system_proxy() {
        string proxy;
#ifdef _WIN32
        if (pf) {
            char** p_proxy = px_proxy_factory_get_proxies(pf, "https://www.google.com");
            if (p_proxy != nullptr) {
                proxy = *p_proxy;
            }
            px_proxy_factory_free_proxies(p_proxy);
            if (starts_with(proxy, "direct://")) {
                proxy = "";
            }
        }
#endif
        return proxy;
    }

    void free_proxy_factory() {
#ifdef _WIN32
        px_proxy_factory_free(pf);
#endif
    }
} // util