//
// Created by v2ray on 2023/3/3.
//

#include "Embedding.h"

namespace emb {

    double cosine_similarity(const std::vector<float>& vec_a, const std::vector<float>& vec_b) {
        double dot_product = 0;
        double norm_a = 0;
        double norm_b = 0;
        for (int i = 0; i < vec_a.size(); i++) {
            dot_product += vec_a[i] * vec_b[i];
            norm_a += pow(vec_a[i], 2);
            norm_b += pow(vec_b[i], 2);
        }
        return dot_product / (sqrt(norm_a) * sqrt(norm_b));
    }

    size_t write_callback(char* char_ptr, size_t size, size_t mem, std::string* base_str) {
        size_t length = size * mem;
        base_str->append(char_ptr, length);
        return length;
    }

    inline void clean_then_throw(CURL* curl, curl_slist* headers, const std::string& message = "") {
        util::curl_cleanup(curl, headers);
        throw std::runtime_error(message);
    }

    /**
     * Get embeddings for a list of text.
     * @param texts The texts to get embeddings for.
     * @param api_key The API key to use.
     * @param key_status_in The API key status to update.
     * @return A list of embeddings, one for each text, the order is preserved.
     */
    std::vector<std::vector<float>> get_embeddings(const std::vector<std::string>& texts, const std::string& api_key,
                                                   api::APIKeyStatus& key_status_in) {
        static const std::string model = "text-embedding-ada-002";
        if (texts.empty()) {
            throw std::invalid_argument("Texts cannot be empty.");
        }
        auto model_max_tokens = util::get_max_tokens(model);
        for (const auto& text : texts) {
            auto token_count = util::get_token_count(text, model);
            if (token_count >= model_max_tokens) {
                throw util::max_tokens_exceeded("Max tokens exceeded in text: "
                + std::to_string(token_count) + " >= " + std::to_string(model_max_tokens));
            }
        }
        CURL* curl;
        CURLcode res;
        std::string response;
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
            nlohmann::json inputs = nlohmann::json::array();
            for (const auto& text : texts) {
                inputs.emplace_back(text);
            }
            nlohmann::json payload = {{"model", model}, {"input", inputs}};
            std::string payload_str = payload.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                clean_then_throw(curl, headers, "API request failed: " + std::string(curl_easy_strerror(res)));
            }
            try {
                nlohmann::json j = nlohmann::json::parse(response);
                if (!api::check_err_obj(j, key_status_in)) {
                    if (j.count("data") > 0 && j["data"].is_array()) {
                        auto data = j["data"];
                        std::vector<std::vector<float>> embeddings;
                        for (const auto& e : data) {
                            if (!e.is_object() || e.count("embedding") <= 0 || !e["embedding"].is_array()) {
                                clean_then_throw(curl, headers, "API returned unknown response. Json: " + response);
                            }
                            std::vector<float> embedding;
                            for (const auto& j_embedding : e["embedding"]) {
                                if (!j_embedding.is_number()) {
                                    clean_then_throw(curl, headers, "Embeddings have non-numeric values. Json: " + response);
                                }
                                embedding.push_back(j_embedding.get<float>());
                            }
                            embeddings.push_back(embedding);
                        }
                        util::curl_cleanup(curl, headers);
                        const size_t& embeddings_size = embeddings.size();
                        const size_t& texts_size = texts.size();
                        if (embeddings_size != texts_size) {
                            throw std::runtime_error("API returned embeddings size does not match texts size. Embeddings size: "
                            + std::to_string(embeddings_size) + " Texts size: " + std::to_string(texts_size));
                        }
                        return embeddings;
                    } else {
                        clean_then_throw(curl, headers, "API returned json does not contain data. Json: " + response);
                    }
                } else {
                    clean_then_throw(curl, headers);
                }
            } catch (const nlohmann::json::parse_error& e) {
                clean_then_throw(curl, headers, "API returned string is not a valid json. String: " + response);
            }
        } else {
            throw std::runtime_error("Failed to initialize cURL.");
        }
    }
} // emb