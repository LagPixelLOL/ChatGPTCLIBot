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
    std::string get_key_from_console(const std::string& api_base_url) {
        std::string api_key;
        while (true) {
            std::cout << "Please enter your OpenAI API key: ";
            getline(std::cin, api_key);
            APIKeyStatus status = check_key(api_key, api_base_url);
            if (status == APIKeyStatus::VALID) {
                util::println_info("API key is valid.");
                break;
            } else if (status == APIKeyStatus::EMPTY) {
                util::println_warn("API key cannot be empty, please try again.");
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

    const std::vector<std::string>& get_keys() {
        return api_keys_;
    }

    /**
     * Set the API key, will clear all previous keys.
     */
    void set_key(const std::string& api_key) {
        api_keys_.clear();
        api_keys_.emplace_back(api_key);
    }

    /**
     * Set the API key, will clear all previous keys.
     */
    void set_key(const std::vector<std::string>& api_keys) {
        api_keys_ = api_keys;
    }

    APIKeyStatus check_key(const std::string& api_key, const std::string& api_base_url) {
        util::println_info("Checking API key...");
        if (api_key.empty()) {
            return APIKeyStatus::EMPTY;
        }
        std::vector<std::string> headers = {"Content-Type: application/json"};
        std::string auth = "Authorization: Bearer ";
        headers.push_back(auth.append(api_key));
        nlohmann::json payload = nlohmann::json::object();
        payload.emplace("model", "text-embedding-ada-002");
        payload.emplace("input", nlohmann::json{"Test"});
        APIKeyStatus status = APIKeyStatus::API_REQUEST_FAILED;
        std::string response;
        try {
            curl::http_post(api_base_url + "/v1/embeddings", [&](const std::vector<char>& vec, CURL*){
                response.append(vec.begin(), vec.end());
            }, payload.dump(), headers);
        } catch (const std::exception& e) {
            util::println_err("API request failed: " + std::string(e.what()));
            return status;
        }
        if (boost::ends_with(response, "\n")) {
            response.pop_back();
        }
        try {
            check_err_obj(nlohmann::json::parse(response), status, false);
        } catch (const nlohmann::json::parse_error& e) {
            util::println_err("API returned string is not a valid json. String: " + response);
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
        auto it_err = json_to_check.find("error");
        nlohmann::json::const_iterator it_status;
        if (it_err != json_to_check.end() && it_err->is_object()) {
            status_in = APIKeyStatus::API_REQUEST_FAILED;
            std::optional<std::string> message;
            auto it_message = it_err->find("message");
            if (it_message != it_err->end() && it_message->is_string()) {
                message = it_message->get<std::string>();
            }
            auto it_code = it_err->find("code");
            nlohmann::json::const_iterator it_type;
            nlohmann::json::const_iterator it_discord;
            if (it_code != it_err->end() && it_code->is_string()) {
                std::string error_code = it_code->get<std::string>();
                if (error_code == "invalid_api_key" || error_code == "account_deactivated") {
                    status_in = APIKeyStatus::INVALID_KEY;
                } else if (!print_err_msg) {
                    util::println_err("API returned error code. Code: " + error_code);
                }
            } else if ((it_type = it_err->find("type")) != it_err->end() && it_type->is_string()) {
                std::string error_type = it_type->get<std::string>();
                if (error_type == "insufficient_quota") {
                    status_in = APIKeyStatus::QUOTA_EXCEEDED;
                } else if (error_type == "tokens" || error_type == "requests") {
                    status_in = APIKeyStatus::RATE_LIMIT_REACHED;
                } else if (!print_err_msg) {
                    util::println_err("API returned error type. Type: " + error_type);
                }
            } else if ((it_discord = it_err->find("discord")) != it_err->end() && it_discord->is_string() && message) {
                if (boost::starts_with(*message, "CATTO: Your API KEY is INVALID")) {
                    status_in = APIKeyStatus::INVALID_KEY;
                }
            } else {
                util::println_err("API returned unknown error. Json: " + json_to_check.dump(2));
            }
            if (print_err_msg && message) {
                util::println_err("\nAPI returned error: " + (status_in != APIKeyStatus::INVALID_KEY ? *message : "The API key is invalid."));
            }
            return true;
        } else if ((it_status = json_to_check.find("status")) != json_to_check.end() && it_status->is_boolean() && !it_status->get<bool>()) {
            status_in = APIKeyStatus::API_REQUEST_FAILED;
            if (print_err_msg) {
                auto it_response = json_to_check.find("response");
                if (it_response != json_to_check.end() && it_response->is_string()) {
                    util::println_err("\nAPI returned error: " + it_response->get<std::string>());
                }
            }
            return true;
        }
        return false;
    }
} // api