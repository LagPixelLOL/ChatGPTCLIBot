//
// Created by v2ray on 2023/3/3.
//

#include "SystemUtils.h"

namespace util {
    pxProxyFactory* pf = px_proxy_factory_new();

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

    /**
     * Get the current system proxy using libproxy.
     * @return Proxy string if found, empty string otherwise.
     */
    string system_proxy() {
        string proxy;
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
        return proxy;
    }

    void free_proxy_factory() {
        px_proxy_factory_free(pf);
    }
} // util