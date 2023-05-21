//
// Created by v2ray on 2023/4/25.
//

#ifndef GPT3BOT_REQUEST_H
#define GPT3BOT_REQUEST_H

#include "RequestObject.h"
#include "unordered_map"

namespace curl {

    void http_get(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback
                  = [](const auto&, const auto*){}, const std::vector<std::string>& headers = {}, const int& timeout_s = 10,
                  const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                  = [](auto, auto, auto, auto){return 0;});
    void http_post(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback
                   = [](const auto&, const auto*){}, const std::string& post_data = "", const std::vector<std::string>& headers = {},
                   const int& timeout_s = 10, const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                   = [](auto, auto, auto, auto){return 0;});
    void http_delete(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback
                     = [](const auto&, const auto*){}, const std::vector<std::string>& headers = {}, const int& timeout_s = 10,
                     const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                     = [](auto, auto, auto, auto){return 0;});
    void upload_binary(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
                       const std::string& post_data_field, const std::string& post_data,
                       const std::string& binary_field, const std::vector<char>& binary_to_upload,
                       const std::string& binary_filename, const std::string& binary_content_type,
                       const std::vector<std::string>& headers, const int& timeout_s,
                       const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                       = [](auto, auto, auto, auto){return 0;});
    void multi_perform(const std::vector<std::shared_ptr<RequestObject>>& requests);
} // curl

#endif //GPT3BOT_REQUEST_H
