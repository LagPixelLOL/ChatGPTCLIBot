//
// Created by v2ray on 2023/4/5.
//

#include "APIKey.h"

namespace api {
    std::vector<std::string> api_keys_;

    /**
     * Get the API key from console.
     * @return The API key.
     */
    std::string get_key_from_console() {
        std::string api_key;
        while (true) {
            std::cout << "Please enter your OpenAI API key: ";
            getline(std::cin, api_key);
            APIKeyStatus status = check_key(api_key);
            if (status == APIKeyStatus::VALID) {
                util::println_info("API key is valid.");
                break;
            } else if (status == APIKeyStatus::EMPTY) {
                util::println_warn("API key cannot be empty, please try again.");
            } else if (status == APIKeyStatus::INVALID_PREFIX) {
                util::println_warn("API key must start with \"sk-\", please try again.");
            } else if (status == APIKeyStatus::INVALID_LENGTH) {
                util::println_warn("API key must be 51 characters long, please try again.");
            } else if (status == APIKeyStatus::QUOTA_EXCEEDED) {
                util::println_warn("This API key has exceeded its quota, please try again.");
            } else if (status == APIKeyStatus::INVALID_KEY) {
                util::println_warn("API key is invalid, please try again.");
            } else if (status == APIKeyStatus::API_REQUEST_FAILED) {
                util::println_warn("API request failed, please try again.");
            }
        }
        return api_key;
    }

    size_t get_key_count() {
        return api_keys_.size();
    }

    void remove_first_key() {
        if (!api_keys_.empty()) {
            api_keys_.erase(api_keys_.begin());
        }
    }

    /**
     * Check if the API key has at least one key.
     * @return True if there is at least one key present, false otherwise.
     */
    bool has_key() {
        return !api_keys_.empty();
    }

    /**
     * Get the first API key from api_keys.
     * @return The API key, could be an empty string.
     */
    std::string get_key() {
        if (api_keys_.empty()) {
            return "";
        }
        return api_keys_.front();
    }

    std::vector<std::string> get_keys() {
        return api_keys_;
    }

    /**
     * Set the API key, will clear all previous keys.
     */
    void set_key(const std::string& api_key) {
        api_keys_.clear();
        api_keys_.push_back(api_key);
    }

    /**
     * Set the API key, will clear all previous keys.
     */
    void set_key(const std::vector<std::string>& api_keys) {
        api_keys_ = api_keys;
    }

    size_t write_callback(char* char_ptr, size_t size, size_t mem, std::string* base_str) {
        size_t length = size * mem;
        std::string s(char_ptr, length);
        base_str->append(s);
        return length;
    }

    APIKeyStatus check_key(const std::string& api_key) {
        util::println_info("Checking API key...");
        if (api_key.empty()) {
            return APIKeyStatus::EMPTY;
        } else if (!boost::starts_with(api_key, "sk-")) {
            return APIKeyStatus::INVALID_PREFIX;
        } else if (api_key.length() != 51) {
            return APIKeyStatus::INVALID_LENGTH;
        }
        CURL* curl;
        CURLcode res;
        std::string response;
        APIKeyStatus status;
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/embeddings");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
            util::set_curl_proxy(curl, util::system_proxy());
            util::set_curl_ssl_cert(curl);
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            std::string auth = "Authorization: Bearer ";
            headers = curl_slist_append(headers, auth.append(api_key).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            nlohmann::json payload = {{"model", "text-embedding-ada-002"}, {"input", ""}};
            std::string payload_str = payload.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                util::println_err("API request failed: " + std::string(curl_easy_strerror(res)));
            } else {
                if (boost::ends_with(response, "\n")) {
                    response.pop_back();
                }
                try {
                    check_err_obj(nlohmann::json::parse(response), status, false);
                } catch (const nlohmann::json::parse_error& e) {
                    util::println_err("API returned string is not a valid json. String: " + response);
                }
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        return status;
    }

    /**
     * Check if the json object has an error object, if so, print the error message and set the status.
     * @param json_to_check The json object to check.
     * @param status_in The status object to modify.
     * @param print_err_msg Whether to print the error message.
     * @return True if the json object has an error object, false otherwise.
     */
    bool check_err_obj(const nlohmann::json& json_to_check, APIKeyStatus& status_in, const bool& print_err_msg) {
        status_in = APIKeyStatus::VALID;
        if (json_to_check.count("error") > 0) {
            auto error_obj = json_to_check["error"];
            if (error_obj.is_object()) {
                status_in = APIKeyStatus::API_REQUEST_FAILED;
                if (error_obj.count("code") > 0 && error_obj["code"].is_string()) {
                    std::string error_code = error_obj["code"].get<std::string>();
                    if (error_code == "invalid_api_key") {
                        status_in = APIKeyStatus::INVALID_KEY;
                    } else if (!print_err_msg) {
                        util::println_err("API returned error code. Code: " + error_code);
                    }
                } else if (error_obj.count("type") > 0 && error_obj["type"].is_string()) {
                    std::string error_type = error_obj["type"].get<std::string>();
                    if (error_type == "insufficient_quota") {
                        status_in = APIKeyStatus::QUOTA_EXCEEDED;
                    } else if (!print_err_msg) {
                        util::println_err("API returned error type. Type: " + error_type);
                    }
                } else {
                    util::println_err("API returned unknown error. Json: " + json_to_check.dump(2));
                }
                if (print_err_msg) {
                    if (error_obj.count("message") > 0 && error_obj["message"].is_string()) {
                        util::println_err("\nAPI returned error: " + error_obj["message"].get<std::string>());
                    }
                }
                return true;
            }
        }
        return false;
    }
} // api