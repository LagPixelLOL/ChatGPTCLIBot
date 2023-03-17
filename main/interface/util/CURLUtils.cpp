//
// Created by v2ray on 2023/3/6.
//

#include "CURLUtils.h"

namespace util {

    void set_curl_proxy(CURL* curl, const string& proxy) {
        if (!proxy.empty()) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
            curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
        }
    }

    void set_curl_ssl_cert(CURL* curl) {
#ifdef __linux__
        curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-bundle.crt");
#endif
    }
} // util