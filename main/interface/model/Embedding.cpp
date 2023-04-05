//
// Created by v2ray on 2023/3/3.
//

#include "Embedding.h"

namespace emb {

    double cosine_similarity(const vector<float>& vec_a, const vector<float>& vec_b) {
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

    size_t write_callback(char* char_ptr, size_t size, size_t mem, string* base_str) {
        size_t length = size * mem;
        string s(char_ptr, length);
        base_str->append(s);
        return length;
    }

    /**
     * Get embeddings of a text.
     * Important: Do null check on the returned pointer.
     * @param text Text to get embeddings.
     * @return A shared pointer that points to a list of embeddings, or nullptr if failed.
     */
    pair<shared_ptr<vector<float>>, APIKeyStatus> get_embeddings(const string& text, const string& api_key) {
        CURL* curl;
        CURLcode res;
        string response;
        shared_ptr<vector<float>> embeddings = nullptr;
        APIKeyStatus key_status = APIKeyStatus::VALID;
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
            string auth = "Authorization: Bearer ";
            headers = curl_slist_append(headers, auth.append(api_key).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            json payload = {{"model", "text-embedding-ada-002"}, {"input", text}};
            string payload_str = payload.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                util::println_err("\nAPI request failed: " + string(curl_easy_strerror(res)));
            } else {
                try {
                    json j = json::parse(response);
                    if (!check_err_obj(j, key_status)) {
                        if (j.count("data") > 0 && j["data"].is_array()) {
                            auto data = j["data"];
                            if (!data.empty() && data[0].is_object()) {
                                auto data_first = data[0];
                                if (data_first.count("embedding") > 0 && data_first["embedding"].is_array()) {
                                    auto j_embeddings = data_first["embedding"];
                                    shared_ptr<vector<float>> p_embeddings = make_shared<vector<float>>();
                                    for (const auto& embedding : j_embeddings) {
                                        p_embeddings->push_back(embedding.get<float>());
                                    }
                                    if (!p_embeddings->empty()) {
                                        embeddings = p_embeddings;
                                    }
                                }
                            }
                        } else {
                            util::println_err("\nAPI returned unknown response. Json: " + response);
                        }
                    }
                } catch (const json::parse_error& e) {
                    util::println_err("\nAPI returned string is not a valid json. String: " + response);
                }
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        return {embeddings, key_status};
    }
} // emb