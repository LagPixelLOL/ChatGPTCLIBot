//
// Created by v2ray on 2023/5/17.
//

#ifndef GPT3BOT_BASE64_H
#define GPT3BOT_BASE64_H

#include "nlohmann/json.hpp"

namespace base64 {
    inline const unsigned int float_size = sizeof(float);

    std::string base64_encode(const unsigned char* buf, unsigned int buf_length);
    std::vector<unsigned char> base64_decode(const std::string& encoded_str);

    inline nlohmann::json embeddings_to_b64_json_array(const std::vector<float>& vec) {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& e : vec) {
            j.push_back(base64_encode(reinterpret_cast<const unsigned char*>(&e), float_size));
        }
        return j;
    }

    inline float b64_str_to_float(const std::string& base64_str) {
        std::vector<unsigned char> bytes = base64_decode(base64_str);
        if (bytes.size() != float_size) {
            throw std::invalid_argument("Base64 string has invalid length.");
        }
        return reinterpret_cast<float&>(bytes[0]);
    }
} // base64

#endif //GPT3BOT_BASE64_H
