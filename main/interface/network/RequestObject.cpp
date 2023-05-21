//
// Created by v2ray on 2023/5/21.
//

#include "RequestObject.h"

namespace curl {

    //class request_failed start:
    request_failed::request_failed() noexcept : std::runtime_error("") {}
    request_failed::request_failed(const std::string& message) noexcept : runtime_error(message) {}
    request_failed::request_failed(const char* message) noexcept : runtime_error(message) {}
    request_failed::~request_failed() = default;
    //class request_failed end.

    size_t write_callback(char* char_ptr, size_t batch, size_t size, const std::function<size_t(char*, size_t, size_t)>* callback) {
        return (*callback)(char_ptr, batch, size);
    }

    int progress_callback(const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>* callback,
                          curl_off_t dl_total, curl_off_t dl_now, curl_off_t ul_total, curl_off_t ul_now) {
        return (*callback)(dl_total, dl_now, ul_total, ul_now);
    }

    //class RequestObject start:
    RequestObject::RequestObject() {
        curl = curl_easy_init();
        if (!curl) {
            throw request_failed("Failed to initialize cURL.");
        }
    }

    RequestObject::RequestObject(RequestObject&& other) noexcept {
        curl = other.curl;
        other.curl = nullptr;
        headers_c = other.headers_c;
        other.headers_c = nullptr;
        mime = other.mime;
        other.mime = nullptr;
        method = other.method;
        url = std::move(other.url);
        callback = std::move(other.callback);
        headers = std::move(other.headers);
        post_data = std::move(other.post_data);
        timeout_s = other.timeout_s;
        proxy = std::move(other.proxy);
        progress = std::move(other.progress);
        constructed = other.constructed;
    }

    RequestObject::RequestObject(const RequestObject& other) : RequestObject() {
        method = other.method;
        url = other.url;
        callback = other.callback;
        headers = other.headers;
        post_data = other.post_data;
        timeout_s = other.timeout_s;
        proxy = other.proxy;
        progress = other.progress;
    }

    RequestObject::~RequestObject() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
        if (headers_c) {
            curl_slist_free_all(headers_c);
        }
        if (mime) {
            curl_mime_free(mime);
        }
    }

    CURL* RequestObject::construct() {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout_s);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, timeout_s);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1);
        util::set_curl_proxy(curl, proxy);
        util::set_curl_ssl_cert(curl);
        if (method == Method::POST) {
            if (mime) {
                curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
            } else if (post_data) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data->c_str());
            }
        } else if (method == Method::DEL) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_lambda);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
        if (headers_c) {
            curl_slist_free_all(headers_c);
        }
        for (const auto& header : headers) {
            headers_c = curl_slist_append(headers_c, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_c);
        constructed = true;
        return curl;
    }

    void RequestObject::perform() {
        if (!constructed) {
            construct();
        }
        exception_ptr = nullptr;
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_ABORTED_BY_CALLBACK) {
            set_exception(std::make_exception_ptr(request_failed("Request cancelled manually.")));
        } else if (res != CURLE_OK) {
            set_exception(std::make_exception_ptr(request_failed(curl_easy_strerror(res))));
        }
        get_exception();
    }

    RequestObject RequestObject::construct_http_get(
            const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
            const std::vector<std::string>& headers, const int& timeout_s,
            const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        RequestObject request;
        request.method = Method::GET;
        request.url = url;
        request.callback = callback;
        request.headers = headers;
        request.timeout_s = timeout_s;
        request.progress = progress_callback;
        return request;
    }

    RequestObject RequestObject::construct_http_post(
            const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
            const std::string& post_data, const std::vector<std::string>& headers, const int& timeout_s,
            const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        RequestObject request;
        request.method = Method::POST;
        request.url = url;
        request.callback = callback;
        request.post_data = post_data;
        request.headers = headers;
        request.timeout_s = timeout_s;
        request.progress = progress_callback;
        return request;
    }

    RequestObject RequestObject::construct_http_delete(
            const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
            const std::vector<std::string>& headers, const int& timeout_s,
            const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        RequestObject request;
        request.method = Method::DEL;
        request.url = url;
        request.callback = callback;
        request.headers = headers;
        request.timeout_s = timeout_s;
        request.progress = progress_callback;
        return request;
    }

    void RequestObject::get_exception() const {
        if (exception_ptr) {
            std::rethrow_exception(exception_ptr);
        }
    }

    void RequestObject::set_exception(const std::exception_ptr& exceptionPtr) {
        exception_ptr = exceptionPtr;
    }
    //class RequestObject end.
} // curl