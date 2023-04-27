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

    size_t write_callback(char* char_ptr, size_t size, size_t mem, const std::function<size_t(char*, size_t, size_t)>* callback_function) {
        return (*callback_function)(char_ptr, size, mem);
    }

    void http_request(const http_method& method, const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                      const std::vector<std::string>& headers, const std::optional<std::string>& post_data, const int& timeout_ms,
                      curl_mime* mime = nullptr) {
        CURLcode res;
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw request_failed("Failed to initialize cURL.");
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
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
        std::function<size_t(char*, size_t, size_t)> callback_lambda = [&](char* char_ptr, size_t size, size_t mem){
            size_t length = size * mem;
            try {
                callback(std::string(char_ptr, char_ptr + length), curl);
            } catch (const std::exception& e) {
                exception_ptr = std::current_exception();
            }
            return length;
        };
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_lambda);
        curl_slist* c_headers = nullptr;
        for (const auto& header : headers) {
            c_headers = curl_slist_append(c_headers, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, c_headers);
        res = curl_easy_perform(curl);
        util::curl_cleanup(curl, c_headers);
        if (res != CURLE_OK) {
            throw request_failed(curl_easy_strerror(res));
        } else if (exception_ptr) {
            std::rethrow_exception(exception_ptr);
        }
    }

    void http_get(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                  const std::vector<std::string>& headers, const int& timeout_ms) {
        http_request(http_method::GET, url, callback, headers, std::nullopt, timeout_ms);
    }

    void http_post(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                   const std::string& post_data, const std::vector<std::string>& headers, const int& timeout_ms) {
        http_request(http_method::POST, url, callback, headers, post_data, timeout_ms);
    }

    void http_delete(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                     const std::vector<std::string>& headers, const int& timeout_ms) {
        http_request(http_method::DEL, url, callback, headers, std::nullopt, timeout_ms);
    }

    void upload_binary(const std::string& url, const std::function<void(const std::string&, CURL*)>& callback,
                       const std::string& post_data_field, const std::string& post_data,
                       const std::string& binary_field, const std::vector<char>& binary_to_upload,
                       const std::string& binary_filename, const std::string& binary_content_type,
                       const std::vector<std::string>& headers, const int& timeout_ms) {
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
            http_request(http_method::POST, url, callback, headers, std::nullopt, timeout_ms, mime);
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