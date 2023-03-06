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
        char buffer[80];
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&time));
        return buffer;
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