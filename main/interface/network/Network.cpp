//
// Created by v2ray on 2023/3/13.
//

#include "Network.h"

namespace api {

    size_t write_callback(char* char_ptr, size_t size, size_t mem, std::function<size_t(char*, size_t, size_t)>* callback_function) {
        return (*callback_function)(char_ptr, size, mem);
    }

    /**
     * Call the OpenAI API with a custom lambda function as callback.
     * @return True if the call was successful, false otherwise.
     */
    bool call_api(const string& initial_prompt, const vector<std::shared_ptr<chat::Exchange>>& chat_exchanges,
                  const string& api_key, const string& model, const float& temperature, const int& max_tokens,
                  const float& top_p, const float& frequency_penalty, const float& presence_penalty,
                  const unsigned int& max_short_memory_length, const unsigned int& max_reference_length,
                  const string& me_id, const string& bot_id, std::function<void(const string& streamed_response)> callback,
                  const bool& debug_reference, const bool& pause_when_showing_reference) {
        bool error = false;
        CURL* curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
            bool is_new_api_ = is_new_api(model);
            string url = is_new_api_ ? "https://api.openai.com/v1/chat/completions" : "https://api.openai.com/v1/completions";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            std::function<size_t(char*, size_t, size_t)> callback_lambda = [&](char* char_ptr, size_t size, size_t mem){
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, max_tokens / 10);
                size_t length = size * mem;
                string s(char_ptr, length);
                vector<string> split_str;
                split_regex(split_str, s, regex("[\n][\n][d][a][t][a][:][ ]"));
                for (auto& str: split_str) {
                    if (starts_with(str, "data: ")) {
                        str.erase(0, 6);
                    }
                    if (ends_with(str, "\n\n")) {
                        str.erase(str.size() - 2);
                    }
                    if (str != "[DONE]") {
                        try {
                            json j = json::parse(str);
                            string response;
                            if (j.count("choices") > 0 && j["choices"].is_array()) {
                                auto choices = j["choices"];
                                if (!is_new_api_) {
                                    for (const auto& choice: choices) {
                                        if (choice.count("text") > 0 && choice["text"].is_string()) {
                                            response = choice["text"].get<string>();
                                        }
                                    }
                                } else {
                                    for (const auto& choice: choices) {
                                        if (choice.count("delta") > 0 && choice["delta"].is_object()) {
                                            auto delta = choice["delta"];
                                            if (delta.count("content") > 0 && delta["content"].is_string()) {
                                                response = delta["content"].get<string>();
                                            }
                                        }
                                    }
                                }
                            } else {
                                response = j.dump(4);
                            }
                            callback(response);
                        } catch (const json::parse_error& e) {
                            util::println_err("Error parsing JSON: " + string(e.what()));
                        }
                    }
                }
                return length;
            };
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_lambda);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
            util::set_curl_proxy(curl, util::system_proxy());
            util::set_curl_ssl_cert(curl);
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            string auth = "Authorization: Bearer ";
            headers = curl_slist_append(headers, auth.append(api_key).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            json payload = {{"model", model},
                            {"temperature", temperature},
                            {"max_tokens", max_tokens},
                            {"top_p", top_p},
                            {"frequency_penalty", frequency_penalty},
                            {"presence_penalty", presence_penalty},
                            {"stream", true}};
            string constructed_initial = prompt::construct_reference(
                    initial_prompt, chat_exchanges.back()->getInputEmbeddings(),
                    chat_exchanges, max_reference_length, max_short_memory_length, me_id, bot_id);
            string suffix = ": ";
            if (debug_reference) {
                string dr_prefix = Term::color_fg(255, 200, 0) + "<Debug Reference> " + Term::color_fg(Term::Color::Name::Default);
                util::print_cs("\n" + dr_prefix + Term::color_fg(255, 225, 0)
                + "Constructed initial prompt:\n----------\n" + constructed_initial + "\n----------", true);
                if (pause_when_showing_reference) {
                    util::print_cs(dr_prefix + "Press " + Term::color_fg(70, 200, 255) + "Enter"
                    + Term::color_fg(Term::Color::Name::Default) + " to continue: ");
                    util::ignore_line();
                }
                util::print_cs(Term::color_fg(175, 255, 225) + bot_id + (is_new_api_ ? suffix : ":"), false, false);
            }
            if (!is_new_api_) {
                payload["prompt"] = GPT::to_payload(
                        constructed_initial, chat_exchanges, me_id, bot_id, max_short_memory_length);
                payload["stop"] = {me_id + suffix, bot_id + suffix};
            } else {
                payload["messages"] = ChatGPT::to_payload(
                        constructed_initial, chat_exchanges, me_id, bot_id, max_short_memory_length);
            }
            string payload_str = payload.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                error = true;
                util::println_err("\nAPI request failed: " + string(curl_easy_strerror(res)));
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        return !error;
    }

    bool is_new_api(const string& model_name) {
        return starts_with(model_name, "gpt-3.5") || starts_with(model_name, "gpt-4");
    }
} // api