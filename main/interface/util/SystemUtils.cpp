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

    tm get_time_info(long long time_ms) {
        time_t time = time_ms / 1000;
        tm time_info{};
#ifdef _MSC_VER
        localtime_s(&time_info, &time);
#else
        localtime_r(&time, &time_info);
#endif
        return time_info;
    }

    std::string ms_to_formatted_time(long long time_ms) {
        tm time_info = get_time_info(time_ms);
        char buffer[80];
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &time_info);
        try {
            short offset_minutes = timezone_offset_minutes(time_ms);
            auto offset_hours = static_cast<short>(offset_minutes / 60);
            auto remaining_minutes = static_cast<short>(offset_minutes % 60);
            std::stringstream ss;
            ss << "UTC";
            if (offset_hours >= 0) {
                ss << '+';
            }
            ss << offset_hours;
            if (remaining_minutes > 0) {
                ss << ':' << remaining_minutes;
            }
            ss << ' ' << buffer;
            return ss.str();
        } catch (const boost::bad_lexical_cast& e) {}
        return buffer;
    }

    short timezone_offset_minutes(long long time_ms) {
        tm time_info = get_time_info(time_ms);
        char buffer[10];
        strftime(buffer, 10, "%z", &time_info);
        auto offset_hours = boost::lexical_cast<short>(std::string(buffer, 3));
        auto offset_minutes = boost::lexical_cast<short>(std::string(buffer + 3, 2));
        if (buffer[0] == '-') {
            offset_minutes = static_cast<short>(-offset_minutes);
        }
        auto total_offset_minutes = static_cast<short>(offset_hours * 60 + offset_minutes);
        return total_offset_minutes;
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