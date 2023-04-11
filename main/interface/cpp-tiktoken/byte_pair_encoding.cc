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
#include <sstream>
#include <optional>
#include <limits>

BytePairEncodingCore::BytePairEncodingCore(const std::unordered_map<std::vector<uint8_t>, int, VectorHash>& byte_pair_ranks,
                                           const std::unordered_map<std::string, int>& special_token_mappings,
                                           const std::shared_ptr<PCRERegex> &pattern_string) :
                                           byte_pair_ranks_(byte_pair_ranks), special_token_mappings_(special_token_mappings),
                                           pattern_string_(pattern_string) {}

template <typename T>
std::vector<T> BytePairEncodingCore::byte_pair_merge(const std::vector<uint8_t>& piece,
                                                     const std::unordered_map<std::vector<uint8_t>, int, VectorHash>& ranks,
                                                     std::function<T(int, int)> f) {
    std::vector<std::pair<int, int>> partitions(piece.size() + 1);

    // Initialize partitions with (i, max_int) values
    for (size_t i = 0; i <= piece.size(); ++i) {
        partitions[i] = {static_cast<int>(i), std::numeric_limits<int>::max()};
    }

    // Helper function to get rank
    auto get_rank = [&piece, &partitions, &ranks](size_t idx, int skip) -> std::optional<int> {
        if (idx + skip + 2 >= partitions.size()) {
            return std::nullopt;
        }
        std::vector<uint8_t> key(piece.begin() + partitions[idx].first, piece.begin() + partitions[idx + skip + 2].first);
        auto rank_iter = ranks.find(key);
        return (rank_iter != ranks.end()) ? std::optional<int>(rank_iter->second) : std::nullopt;
    };

    for (size_t i = 0; i < partitions.size() - 2; ++i) {
        auto rank = get_rank(i, 0);
        if (rank.has_value()) {
            partitions[i].second = rank.value();
        }
    }

    while (partitions.size() > 1) {
        int min_rank = std::numeric_limits<int>::max();
        size_t min_rank_idx = 0;

        for (size_t i = 0; i < partitions.size() - 1; ++i) {
            if (partitions[i].second < min_rank) {
                min_rank = partitions[i].second;
                min_rank_idx = i;
            }
        }

        if (min_rank != std::numeric_limits<int>::max()) {
            partitions[min_rank_idx].second = get_rank(min_rank_idx, 1).value_or(std::numeric_limits<int>::max());

            if (min_rank_idx > 0) {
                partitions[min_rank_idx - 1].second = get_rank(min_rank_idx - 1, 1).value_or(std::numeric_limits<int>::max());
            }

            partitions.erase(partitions.begin() + min_rank_idx + 1);
        } else {
            break;
        }
    }

    std::vector<T> output;
    output.reserve(partitions.size() - 1);
    for (size_t i = 0; i < partitions.size() - 1; ++i) {
        output.push_back(f(partitions[i].first, partitions[i + 1].first));
    }

    return output;
}

std::pair<std::vector<int>, std::vector<int>> BytePairEncodingCore::encode_native(const std::string& line_to_encode,
                                                                                  const std::unordered_set<std::string>& allowed_special) {
    std::vector<int> tokens;
    std::vector<int> segment_ids;
    auto matches = pattern_string_->all_matches(line_to_encode);
    for (auto token : matches) {
        auto special_mapping = special_token_mappings_.find(token);
        if (special_mapping != special_token_mappings_.end() && allowed_special.count(token) > 0) {
            tokens.push_back(special_mapping->second);
            segment_ids.push_back(0);
        } else {
            std::vector<uint8_t> utf8_encoded(token.begin(), token.end());
            if (utf8_encoded.size() == 1) {
                auto rank_iter = byte_pair_ranks_.find(utf8_encoded);
                if (rank_iter != byte_pair_ranks_.end()) {
                    tokens.push_back(rank_iter->second);
                    segment_ids.push_back(0);
                }
            } else {
                auto byte_pairs = byte_pair_merge<int>(utf8_encoded, byte_pair_ranks_, [&](int start, int end){
                    std::vector<uint8_t> key(utf8_encoded.begin() + start, utf8_encoded.begin() + end);
                    return byte_pair_ranks_[key];
                });
                tokens.insert(tokens.end(), byte_pairs.begin(), byte_pairs.end());
                segment_ids.insert(segment_ids.end(), byte_pairs.size(), 0);
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
