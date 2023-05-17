//
// Created by v2ray on 2023/5/17.
//

#include "Base64.h"

namespace base64 {
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

    static inline bool is_base64(unsigned char c) {
        return isalnum(c) || c == '+' || c == '/';
    }

    std::string base64_encode(const unsigned char* buf, unsigned int buf_length) {
        std::string return_str;
        long long i = 0;
        long long j; //No need to initialize.
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        while (buf_length--) {
            char_array_3[i++] = *(buf++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                for (i = 0; i < 4; i++) {
                    return_str += base64_chars[char_array_4[i]];
                }
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 3; j++) {
                char_array_3[j] = '\0';
            }
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (j = 0; j < i + 1; j++) {
                return_str += base64_chars[char_array_4[j]];
            }
            while (i++ < 3) {
                return_str += '=';
            }
        }
        return return_str;
    }

    std::vector<unsigned char> base64_decode(const std::string& encoded_str) {
        auto in_len = static_cast<long long>(encoded_str.size());
        long long i = 0;
        long long j; //No need to initialize.
        long long in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> return_vec;
        while (in_len-- && encoded_str[in_] != '=' && is_base64(encoded_str[in_])) {
            char_array_4[i++] = encoded_str[in_];
            in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++) {
                    char_array_4[i] = base64_chars.find(static_cast<char>(char_array_4[i]));
                }
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                for (i = 0; i < 3; i++) {
                    return_vec.push_back(char_array_3[i]);
                }
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 4; j++) {
                char_array_4[j] = 0;
            }
            for (j = 0; j < 4; j++) {
                char_array_4[j] = base64_chars.find(static_cast<char>(char_array_4[j]));
            }
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (j = 0; j < i - 1; j++) {
                return_vec.push_back(char_array_3[j]);
            }
        }
        return return_vec;
    }
} // base64