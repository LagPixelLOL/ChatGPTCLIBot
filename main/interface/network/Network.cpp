//
// Created by v2ray on 2023/3/13.
//

#include "Network.h"

namespace api {

    /**
     * Call the OpenAI API with a custom lambda function as callback.
     */
    void call_api(const std::string& constructed_initial, const chat::Messages& chat_history,
                  const std::string& api_key, const std::string& model, const float& temperature, const int& max_tokens,
                  const float& top_p, const float& frequency_penalty, const float& presence_penalty,
                  const std::vector<std::pair<std::string, float>>& logit_bias, const std::string& me_id, const std::string& bot_id,
                  const std::function<void(const std::string& streamed_response)>& stream_callback,
                  const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progress_callback,
                  const std::string& api_base_url) {
        bool is_new_api_ = is_new_api(model);
        std::string url = api_base_url + (is_new_api_ ? "/v1/chat/completions" : "/v1/completions");
        std::vector<std::string> headers = {"Content-Type: application/json"};
        std::string auth = "Authorization: Bearer ";
        headers.emplace_back(auth.append(api_key));
        try {
            curl::http_post(url, [&](const std::vector<char>& vec, CURL* curl){
                handle_streamed_response(vec, is_new_api_, stream_callback);
            }, to_payload(constructed_initial, chat_history, model, is_new_api_, temperature, max_tokens, top_p, frequency_penalty,
                          presence_penalty, logit_bias, me_id, bot_id).dump(), headers, 20, progress_callback);
        } catch (const std::exception& e) {
            throw curl::request_failed(e.what());
        }
    }

    bool is_new_api(const std::string& model_name) {
        return boost::starts_with(model_name, "gpt-3.5") || boost::starts_with(model_name, "gpt-4");
    }

    void handle_streamed_response(const std::vector<char>& raw_vec, const bool& is_new_api,
                                  const std::function<void(const std::string& streamed_response)>& stream_callback) {
        std::vector<std::string> split_str;
        boost::split_regex(split_str, std::string(raw_vec.begin(), raw_vec.end()), boost::regex("\\n\\ndata: "));
        for (auto& str : split_str) {
            if (boost::starts_with(str, "data: ")) {
                str.erase(0, 6);
            }
            if (boost::ends_with(str, "\n\n")) {
                str.erase(str.size() - 2);
            }
            if (boost::starts_with(str, "[DONE]")) {
                break;
            }
            try {
                nlohmann::json j = nlohmann::json::parse(str);
                std::string response;
                auto it_choices = j.find("choices");
                if (it_choices != j.end() && it_choices->is_array()) {
                    if (it_choices->empty()) {
                        continue;
                    }
                    const auto& choice = (*it_choices)[0];
                    if (!choice.is_object()) {
                        continue;
                    }
                    if (!is_new_api) {
                        auto it_text = choice.find("text");
                        if (it_text != choice.end() && it_text->is_string()) {
                            response = it_text->get<std::string>();
                        }
                    } else {
                        auto it_delta = choice.find("delta");
                        if (it_delta != choice.end() && it_delta->is_object()) {
                            auto it_content = (*it_delta).find("content");
                            if (it_content != (*it_delta).end() && it_content->is_string()) {
                                response = it_content->get<std::string>();
                            }
                        }
                    }
                } else {
                    response = j.dump();
                }
                stream_callback(response);
            } catch (const nlohmann::json::parse_error& e) {
                throw curl::request_failed("Error parsing JSON: " + std::string(e.what()));
            }
        }
    }

    nlohmann::json to_payload(const std::string& constructed_initial, const chat::Messages& chat_history, const std::string& model,
                              const bool& is_new_api_, const float& temperature, const int& max_tokens, const float& top_p,
                              const float& frequency_penalty, const float& presence_penalty,
                              const std::vector<std::pair<std::string, float>>& logit_bias, const std::string& me_id,
                              const std::string& bot_id) {
        nlohmann::json payload = {{"model", model}, {"temperature", temperature}, {"top_p", top_p}, {"frequency_penalty", frequency_penalty},
                                  {"presence_penalty", presence_penalty}, {"stream", true}};
        unsigned int model_max_tokens = util::get_max_tokens(model);
        unsigned int token_count; //No need to initialize.
        if (!is_new_api_) {
            std::string prompt = GPT::to_payload(constructed_initial, chat_history, me_id, bot_id);
            if ((token_count = util::get_token_count(prompt, model)) >= model_max_tokens) {
                throw util::max_tokens_exceeded(
                        "Max tokens exceeded in prompt: " + std::to_string(token_count) + " >= " + std::to_string(model_max_tokens));
            }
            payload["prompt"] = prompt;
            static const std::string suffix = ": ";
            payload["stop"] = {me_id + suffix, bot_id + suffix};
        } else {
            nlohmann::json messages = ChatGPT::to_payload(constructed_initial, chat_history, model, me_id, bot_id);
            if ((token_count = util::get_token_count(messages, model)) >= model_max_tokens) {
                throw util::max_tokens_exceeded(
                        "Max tokens exceeded in messages: " + std::to_string(token_count) + " >= " + std::to_string(model_max_tokens));
            }
            payload["messages"] = messages;
        }
        unsigned int max_tokens_p = model_max_tokens - token_count;
        payload["max_tokens"] = max_tokens_p < max_tokens ? max_tokens_p : max_tokens;
        if (!logit_bias.empty()) {
            nlohmann::json logit_bias_json = nlohmann::json::object();
            std::shared_ptr<GptEncoding> tokenizer = util::get_enc_cache(util::get_tokenizer(model));
            for (const auto& pair : logit_bias) {
                if (pair.first.empty()) {
                    continue;
                }
                std::vector<int> token_ids = tokenizer->encode(pair.first);
                for (const auto& token_id : token_ids) {
                    logit_bias_json[std::to_string(token_id)] = pair.second;
                }
            }
            payload["logit_bias"] = logit_bias_json;
        }
        return payload;
    }
} // api