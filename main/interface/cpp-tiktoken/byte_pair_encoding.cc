/*
 * Copyright (c) 2023 by Mark Tarrabain All rights reserved. Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the nor the names of its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "byte_pair_encoding.h"
#include <string>
#include <algorithm>
#include <sstream>

BytePairEncodingCore::BytePairEncodingCore(const std::unordered_map<std::vector<uint8_t>, int, VectorHash>& byte_pair_ranks,
                                           const std::unordered_map<std::string, int>& special_token_mappings,
                                           const std::shared_ptr<PCRERegex> &pattern_string)
        : byte_pair_ranks_(byte_pair_ranks), special_token_mappings_(special_token_mappings), pattern_string_(pattern_string) {}

std::pair<std::vector<int>, std::vector<int>> BytePairEncodingCore::encode_native(const std::string& line_to_encode,
                                                                                  const std::unordered_set<std::string>& allowed_special) {
    std::vector<int> tokens;
    std::vector<int> segment_ids;

    auto matches = pattern_string_->all_matches(line_to_encode);
    for(auto token : matches) {
        auto special_mapping = special_token_mappings_.find(token);
        if (special_mapping != special_token_mappings_.end() && allowed_special.count(token) > 0) {
            tokens.push_back(special_mapping->second);
            segment_ids.push_back(0);
        } else {
            std::vector<uint8_t> utf8_encoded(token.begin(), token.end());
            int prev_token_start = 0;
            for (size_t i = 0; i < utf8_encoded.size();) {
                int token_length = 0;
                int token_id = -1;
                auto lookup = std::vector<uint8_t>(utf8_encoded.begin() + prev_token_start, utf8_encoded.end());
                while(!lookup.empty()) {
                    auto byte_pair_range = byte_pair_ranks_.find(lookup);
                    if (byte_pair_range != byte_pair_ranks_.end()) {
                        token_id = byte_pair_range->second;
                        token_length = lookup.size();
                        break;
                    }
                    lookup.pop_back();
                }
                if (token_id != -1) {
                    tokens.push_back(token_id);
                    segment_ids.push_back(0);
                    i += token_length;
                    prev_token_start = i;
                } else {
                    throw std::logic_error("Critical error! No token found for " + std::string(utf8_encoded.begin() + prev_token_start, utf8_encoded.end()));
                }
            }
        }
    }

    return std::make_pair(tokens, segment_ids);
}

std::string BytePairEncodingCore::decode_native(const std::vector<int>& input_tokens_to_decode) {
    std::stringstream decoded_string;

    for (const int token_id : input_tokens_to_decode) {
        auto special_token = std::find_if(special_token_mappings_.begin(), special_token_mappings_.end(),
                                          [token_id](const auto& pair) { return pair.second == token_id; });

        if (special_token != special_token_mappings_.end()) {
            decoded_string << special_token->first;
        } else {
            for (const auto& byte_pair : byte_pair_ranks_) {
                if (byte_pair.second == token_id) {
                    decoded_string << std::string(byte_pair.first.begin(), byte_pair.first.end());
                    break;
                }
            }
        }
    }

    return decoded_string.str();
}
