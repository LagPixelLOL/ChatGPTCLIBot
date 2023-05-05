//
// Created by v2ray on 2023/4/25.
//

#include "Request.h"

namespace curl {

    //class request_failed start:
    request_failed::request_failed() noexcept : std::runtime_error("") {}
    request_failed::request_failed(const std::string& message) noexcept : runtime_error(message) {}
    request_failed::request_failed(const char* message) noexcept : runtime_error(message) {}
    request_failed::~request_failed() = default;
    //class request_failed end.

    enum class http_method {GET, POST, DEL};

    size_t write_callback(char* char_ptr, size_t batch, size_t size, const std::function<size_t(char*, size_t, size_t)>* callback) {
        return (*callback)(char_ptr, batch, size);
    }

    int progress_callback(const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>* callback,
                          curl_off_t dl_total, curl_off_t dl_now, curl_off_t ul_total, curl_off_t ul_now) {
        return (*callback)(dl_total, dl_now, ul_total, ul_now);
    }

    void http_request(const http_method& method, const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                      const std::vector<std::string>& headers, const std::optional<std::string>& post_data, const int& timeout_s,
                      curl_mime* mime = nullptr, const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress
                      = [](auto, auto, auto, auto){return 0;}) {
        CURLcode res;
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw request_failed("Failed to initialize cURL.");
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout_s);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, timeout_s);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1);
        util::set_curl_proxy(curl, util::system_proxy());
        util::set_curl_ssl_cert(curl);
        if (method == http_method::POST) {
            if (mime) {
                curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
            } else if (post_data) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data->c_str());
            }
        } else if (method == http_method::DEL) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        std::exception_ptr exception_ptr = nullptr;
        std::function<size_t(char*, size_t, size_t)> callback_lambda = [&](char* char_ptr, size_t batch, size_t size){
            size_t length = batch * size;
            try {
                callback(std::string(char_ptr, char_ptr + length), curl);
            } catch (const std::exception& e) {
                exception_ptr = std::current_exception();
            }
            return length;
        };
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_lambda);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
        curl_slist* c_headers = nullptr;
        for (const auto& header : headers) {
            c_headers = curl_slist_append(c_headers, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, c_headers);
        res = curl_easy_perform(curl);
        util::curl_cleanup(curl, c_headers);
        if (res == CURLE_ABORTED_BY_CALLBACK) {
            throw request_failed("Request cancelled manually.");
        } else if (res != CURLE_OK) {
            throw request_failed(curl_easy_strerror(res));
        } else if (exception_ptr) {
            std::rethrow_exception(exception_ptr);
        }
    }

    void http_get(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                  const std::vector<std::string>& headers, const int& timeout_s,
                  const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        http_request(http_method::GET, url, callback, headers, std::nullopt, timeout_s, nullptr, progress_callback);
    }

    void http_post(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                   const std::string& post_data, const std::vector<std::string>& headers, const int& timeout_s,
                   const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        http_request(http_method::POST, url, callback, headers, post_data, timeout_s, nullptr, progress_callback);
    }

    void http_delete(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                     const std::vector<std::string>& headers, const int& timeout_s,
                     const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        http_request(http_method::DEL, url, callback, headers, std::nullopt, timeout_s, nullptr, progress_callback);
    }

    void upload_binary(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                       const std::string& post_data_field, const std::string& post_data,
                       const std::string& binary_field, const std::vector<char>& binary_to_upload,
                       const std::string& binary_filename, const std::string& binary_content_type,
                       const std::vector<std::string>& headers, const int& timeout_s,
                       const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw curl::request_failed("Failed to initialize cURL.");
        }
        curl_mime* mime = curl_mime_init(curl);
        curl_mimepart* part;
        //Add post_data to mime.
        part = curl_mime_addpart(mime);
        curl_mime_name(part, post_data_field.c_str());
        curl_mime_data(part, post_data.c_str(), CURL_ZERO_TERMINATED);
        //Add binary_to_upload to mime.
        part = curl_mime_addpart(mime);
        curl_mime_name(part, binary_field.c_str());
        curl_mime_filename(part, binary_filename.c_str());
        curl_mime_type(part, binary_content_type.c_str());
        curl_mime_data(part, binary_to_upload.data(), binary_to_upload.size());
        std::exception_ptr exception_ptr = nullptr;
        try {
            http_request(http_method::POST, url, callback, headers, std::nullopt, timeout_s, mime, progress_callback);
        } catch (const std::exception& e) {
            exception_ptr = std::current_exception();
        }
        curl_mime_free(mime);
        curl_easy_cleanup(curl);
        if (exception_ptr) {
            std::rethrow_exception(exception_ptr);
        }
    }
} // curl