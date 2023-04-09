//
// Created by v2ray on 2023/3/6.
//

#include "CURLUtils.h"

namespace util {
    string ca_bundle_path = "/etc/ssl/certs/ca-bundle.crt";

    void set_curl_proxy(CURL* curl, const string& proxy) {
        if (!proxy.empty()) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
            curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
        }
    }

    void set_curl_ssl_cert([[maybe_unused]] CURL* curl) {
#ifdef __linux__
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_bundle_path.c_str());
#endif
    }

    void set_ca_bundle_path(const string& path) {
        ca_bundle_path = path;
    }

    string get_ca_bundle_path() {
        return ca_bundle_path;
    }

    void curl_cleanup(CURL* curl, curl_slist* headers) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
} // util