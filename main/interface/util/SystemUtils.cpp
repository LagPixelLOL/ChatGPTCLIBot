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
        return duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::string currentTimeFormatted() {
        return ms_to_formatted_time(currentTimeMillis());
    }

    std::string ms_to_formatted_time(long long timeMillis) {
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
     * Get the current system proxy using libproxy.
     * @return Proxy string if found, empty string otherwise.
     */
    std::string system_proxy() {
        std::string proxy;
#ifdef _WIN32
        if (pf) {
            char** p_proxy = px_proxy_factory_get_proxies(pf, "https://www.google.com");
            if (p_proxy != nullptr) {
                proxy = *p_proxy;
            }
            px_proxy_factory_free_proxies(p_proxy);
            if (boost::starts_with(proxy, "direct://")) {
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