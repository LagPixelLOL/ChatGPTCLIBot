//
// Created by v2ray on 2023/5/21.
//

#ifndef GPT3BOT_REQUESTOBJECT_H
#define GPT3BOT_REQUESTOBJECT_H

#include "../util/CURLUtils.h"
#include "../util/SystemUtils.h"

namespace curl {

    class request_failed : public std::runtime_error {
    public:
        request_failed() noexcept;
        explicit request_failed(const std::string& message) noexcept;
        explicit request_failed(const char* message) noexcept;
        ~request_failed() override;
    };

    enum class Method {GET, POST, DEL};

    class RequestObject {
    public:
        CURL* curl;
        curl_slist* headers_c = nullptr;
        curl_mime* mime = nullptr;
        Method method = Method::GET;
        std::string url;
        std::function<void(const std::vector<char>&, CURL*)> callback = [](const auto&, auto*){};
        std::vector<std::string> headers;
        std::optional<std::string> post_data = std::nullopt;
        int timeout_s = 10;
        std::string proxy = util::system_proxy();
        std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)> progress = [](auto, auto, auto, auto){return 0;};
    private:
        bool constructed = false;
        std::exception_ptr exception_ptr;
        const std::function<size_t(char*, size_t, size_t)> callback_lambda = [&](char* char_ptr, size_t batch, size_t size){
            size_t length = batch * size;
            try {
                callback(std::vector<char>(char_ptr, char_ptr + length), curl);
            } catch (const std::exception& e) {
                exception_ptr = std::current_exception();
            }
            return length;
        };

    public:
        RequestObject();
        RequestObject(RequestObject&& other) noexcept;
        RequestObject(const RequestObject& other);
        virtual ~RequestObject();

        CURL* construct();
        void perform();

        static RequestObject construct_http_get(
                const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback
                = [](const auto&, const auto*){}, const std::vector<std::string>& headers = {}, const int& timeout_s = 10,
                const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                = [](auto, auto, auto, auto){return 0;});
        static RequestObject construct_http_post(
                const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback
                = [](const auto&, const auto*){}, const std::string& post_data = "", const std::vector<std::string>& headers = {},
                const int& timeout_s = 10, const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                = [](auto, auto, auto, auto){return 0;});
        static RequestObject construct_http_delete(
                const std::string& url, const std::function<void(const std::vector<char>&, CURL*)>& callback
                = [](const auto&, const auto*){}, const std::vector<std::string>& headers = {}, const int& timeout_s = 10,
                const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback
                = [](auto, auto, auto, auto){return 0;});

        void get_exception() const;
        void set_exception(const std::exception_ptr& exceptionPtr);
    };
} // curl

#endif //GPT3BOT_REQUESTOBJECT_H
