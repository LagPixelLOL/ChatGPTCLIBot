//
// Created by v2ray on 2023/3/3.
//

#include "Embedding.h"

namespace emb {

    double cosine_similarity(const std::vector<float>& vec_a, const std::vector<float>& vec_b) {
        if (vec_a.empty() || vec_a.size() != vec_b.size()) { //No need to check vec_b.empty(), the size check will cover it.
            throw std::invalid_argument("Vectors for cosine similarity must be non-empty and of the same size.");
        }
        double dot_product = 0;
        double norm_a = 0;
        double norm_b = 0;
        for (size_t i = 0; i < vec_a.size(); i++) {
            dot_product += vec_a[i] * vec_b[i];
            norm_a += pow(vec_a[i], 2);
            norm_b += pow(vec_b[i], 2);
        }
        //Prevent division by 0.
        return norm_a == 0 || norm_b == 0 ? 0 : dot_product / (sqrt(norm_a) * sqrt(norm_b));
    }

    /**
     * Get embeddings for a list of text.
     * @param texts The texts to get embeddings for.
     * @param api_key The API key to use.
     * @param key_status_in The API key status to update.
     * @return A list of embeddings, one for each text, the order is preserved.
     */
    std::vector<std::vector<float>> get_embeddings(const std::vector<std::string>& texts, const std::string& api_key,
                                                   api::APIKeyStatus& key_status_in, const std::string& api_base_url,
                                                   const std::function<int(curl_off_t, curl_off_t,
                                                           curl_off_t, curl_off_t)>& progress_callback) {
        static const std::string model = "text-embedding-ada-002";
        if (texts.empty()) {
            throw std::invalid_argument("Texts cannot be empty.");
        }
        unsigned int model_max_tokens = util::get_max_tokens(model);
        for (const auto& text : texts) {
            unsigned int token_count = util::get_token_count(text, model);
            if (token_count >= model_max_tokens) {
                throw util::max_tokens_exceeded("Max tokens exceeded in text: "
                + std::to_string(token_count) + " >= " + std::to_string(model_max_tokens));
            }
        }
        std::vector<std::string> headers = {"Content-Type: application/json"};
        std::string auth = "Authorization: Bearer ";
        headers.emplace_back(auth.append(api_key));
        nlohmann::json inputs = nlohmann::json::array();
        for (const auto& text : texts) {
            inputs.emplace_back(text);
        }
        nlohmann::json payload = nlohmann::json::object();
        payload.emplace("model", model);
        payload.emplace("input", inputs);
        std::string response;
        curl::http_post(api_base_url + "/v1/embeddings", [&](const std::vector<char>& vec, CURL*){
            response.append(vec.begin(), vec.end());
        }, payload.dump(), headers, 10, progress_callback);
        try {
            nlohmann::json j = nlohmann::json::parse(response);
            if (!api::check_err_obj(j, key_status_in)) {
                if (j.count("data") > 0 && j["data"].is_array()) {
                    auto data = j["data"];
                    std::vector<std::vector<float>> embeddings;
                    for (const auto& e : data) {
                        if (!e.is_object() || e.count("embedding") <= 0 || !e["embedding"].is_array()) {
                            throw curl::request_failed("API returned json does not contain embedding. Json: " + response);
                        }
                        std::vector<float> embedding;
                        for (const auto& j_embedding : e["embedding"]) {
                            if (!j_embedding.is_number()) {
                                throw curl::request_failed("Embeddings have non-numeric values. Json: " + response);
                            }
                            embedding.push_back(j_embedding.get<float>());
                        }
                        embeddings.push_back(embedding);
                    }
                    const size_t& embeddings_size = embeddings.size();
                    const size_t& texts_size = texts.size();
                    if (embeddings_size != texts_size) {
                        throw curl::request_failed("API returned embeddings size does not match texts size. Embeddings size: "
                        + std::to_string(embeddings_size) + " Texts size: " + std::to_string(texts_size));
                    }
                    return embeddings;
                } else {
                    throw curl::request_failed("API returned json does not contain data. Json: " + response);
                }
            } else {
                throw curl::request_failed("");
            }
        } catch (const nlohmann::json::parse_error& e) {
            throw curl::request_failed("API returned string is not a valid json. String: " + response);
        }
    }
} // emb