//
// Created by v2ray on 2023/4/25.
//

#include "Request.h"

#ifdef max
#undef max
#endif

namespace curl {

    void http_get(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
                  const std::vector<std::string>& headers, const int& timeout_s,
                  const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        RequestObject::construct_http_get(url, callback, headers, timeout_s, progress_callback).perform();
    }

    void http_post(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
                   const std::string& post_data, const std::vector<std::string>& headers, const int& timeout_s,
                   const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        RequestObject::construct_http_post(url, callback, post_data, headers, timeout_s, progress_callback).perform();
    }

    void http_delete(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
                     const std::vector<std::string>& headers, const int& timeout_s,
                     const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        RequestObject::construct_http_delete(url, callback, headers, timeout_s, progress_callback).perform();
    }

    void upload_binary(const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback,
                       const std::string& post_data_field, const std::string& post_data,
                       const std::string& binary_field, const std::vector<char>& binary_to_upload,
                       const std::string& binary_filename, const std::string& binary_content_type,
                       const std::vector<std::string>& headers, const int& timeout_s,
                       const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback) {
        RequestObject request = RequestObject::construct_http_post(url, callback, "", headers, timeout_s, progress_callback);
        request.mime = curl_mime_init(request.curl);
        //Add post_data to mime.
        curl_mimepart* part = curl_mime_addpart(request.mime);
        curl_mime_name(part, post_data_field.c_str());
        curl_mime_data(part, post_data.c_str(), CURL_ZERO_TERMINATED);
        //Add binary_to_upload to mime.
        part = curl_mime_addpart(request.mime);
        curl_mime_name(part, binary_field.c_str());
        curl_mime_filename(part, binary_filename.c_str());
        curl_mime_type(part, binary_content_type.c_str());
        curl_mime_data(part, binary_to_upload.data(), binary_to_upload.size());
        request.perform();
    }

    void multi_perform(const std::vector<std::shared_ptr<RequestObject>>& requests) {
        CURLM* multi_handle = curl_multi_init();
        std::unordered_map<CURL*, std::shared_ptr<RequestObject>> curl_to_request;
        for (const auto& request : requests) {
            request->construct();
            request->set_exception(nullptr);
            curl_multi_add_handle(multi_handle, request->curl);
            curl_to_request.insert({request->curl, request});
        }
        curl_multi_setopt(multi_handle, CURLMOPT_MAXCONNECTS, std::numeric_limits<int>::max());
        int still_running;
        std::exception_ptr exception_ptr;
        do {
            CURLMcode res = curl_multi_perform(multi_handle, &still_running);
            if (!res && still_running) {
                res = curl_multi_poll(multi_handle, nullptr, 0, 1000, nullptr);
            }
            if (res) {
                exception_ptr = std::make_exception_ptr(request_failed(curl_multi_strerror(res)));
                break;
            }
        } while (still_running);
        CURLMsg* curl_msg;
        int msgs_left;
        while ((curl_msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (curl_msg->msg == CURLMSG_DONE) {
                CURL* curl = curl_msg->easy_handle;
                CURLcode res = curl_msg->data.result;
                auto it_request = curl_to_request.find(curl);
                if (it_request != curl_to_request.end()) {
                    if (res == CURLE_ABORTED_BY_CALLBACK) {
                        it_request->second->set_exception(std::make_exception_ptr(request_failed("Request cancelled manually.")));
                    } else if (res != CURLE_OK) {
                        it_request->second->set_exception(std::make_exception_ptr(request_failed(curl_easy_strerror(res))));
                    }
                }
                curl_multi_remove_handle(multi_handle, curl);
            } else {
                if (!exception_ptr) {
                    exception_ptr = std::make_exception_ptr(request_failed("Unknown CURLMsg."));
                }
            }
        }
        curl_multi_cleanup(multi_handle);
        if (exception_ptr) {
            std::rethrow_exception(exception_ptr);
        }
    }
} // curl