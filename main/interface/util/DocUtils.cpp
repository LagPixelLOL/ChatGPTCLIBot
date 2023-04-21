//
// Created by v2ray on 2023/4/20.
//

#include "DocUtils.h"

namespace doc {

    /**
     * Split a text into chunks.
     * @param text The text to split.
     * @param tokens_per_chunk The maximum number of tokens per chunk, tokenizer is CL100K_BASE.
     * @return A list of split text chunks.
     */
    std::vector<std::string> split_text(const std::string& text, const unsigned int& tokens_per_chunk) {
        if (text.empty() || text.find_first_not_of(' ') == std::string::npos || tokens_per_chunk <= 0) {
            return {};
        }
        try { //Pre-check for invalid UTF-8 encoding.
            Term::Private::utf8_to_utf32(text);
        } catch (const Term::Exception& e) {
            throw std::invalid_argument("Invalid UTF-8 encoding in text.[1]");
        }
        std::shared_ptr<GptEncoding> tokenizer = util::get_enc_cache(LanguageModel::CL100K_BASE);
        std::vector<int> tokens = tokenizer->encode(text);
        std::vector<std::string> chunks;
        while (!tokens.empty()) {
            long long chunk_sub_size = std::min(static_cast<long long>(tokens_per_chunk), static_cast<long long>(tokens.size()));
            std::vector<int> chunk(tokens.begin(), tokens.begin() + chunk_sub_size);
            std::string chunk_text = tokenizer->decode(chunk);
            if (chunk_text.empty() || chunk_text.find_first_not_of(' ') == std::string::npos) {
                tokens.erase(tokens.begin(), tokens.begin() + static_cast<long long>(chunk.size()));
                continue;
            }
            long long additional_tokens = 0;
            while (true) { //UTF-8 characters may be split into multiple tokens, so we need to check if the chunk is valid.
                try {
                    Term::Private::utf8_to_utf32(chunk_text); //Check if the chunk's encoding is valid by converting it to UTF-32.
                    break; //If it doesn't throw an exception, the chunk is valid.
                } catch (const Term::Exception& e) { //If it throws an exception, the chunk is invalid.
                    long long new_index = chunk_sub_size + additional_tokens++; //The index of the next token.
                    if (tokens.size() <= new_index) { //If we have reached the end of the text, we can't add any more tokens.
                        throw std::invalid_argument("Invalid UTF-8 encoding in text.[2]"); //And this will only appear if the text is invalid.
                    }
                    chunk.emplace_back(tokens[new_index]); //Append the next token to the chunk.
                    chunk_text = tokenizer->decode(chunk); //Decode the chunk again.
                }
            }
            size_t last_punctuation = std::max({chunk_text.rfind('.'), chunk_text.rfind('?'),
                                                chunk_text.rfind('!'), chunk_text.rfind('\n')});
            if (last_punctuation != std::string::npos) {
                chunk_text = chunk_text.substr(0, last_punctuation + 1);
            }
            std::string chunk_text_to_append = chunk_text;
            chunk_text_to_append.erase(std::remove(chunk_text_to_append.begin(), chunk_text_to_append.end(), '\n'),
                                       chunk_text_to_append.end());
            chunk_text_to_append.erase(chunk_text_to_append.begin(), std::find_if(chunk_text_to_append.begin(), chunk_text_to_append.end(),
                                                                                  [](unsigned char ch){return !std::isspace(ch);}));
            chunk_text_to_append.erase(std::find_if(chunk_text_to_append.rbegin(), chunk_text_to_append.rend(),
                                                    [](unsigned char ch){return !std::isspace(ch);}).base(), chunk_text_to_append.end());
            chunks.push_back(chunk_text_to_append);
            std::vector<int> tokenized_chunk_text = tokenizer->encode(chunk_text);
            tokens.erase(tokens.begin(), tokens.begin() + static_cast<long long>(tokenized_chunk_text.size()));
        }
        if (!tokens.empty()) {
            std::string remaining_text = tokenizer->decode(tokens);
            remaining_text.erase(std::remove(remaining_text.begin(), remaining_text.end(), '\n'), remaining_text.end());
            remaining_text.erase(remaining_text.begin(), std::find_if(remaining_text.begin(), remaining_text.end(),
                                                                      [](unsigned char ch){return !std::isspace(ch);}));
            remaining_text.erase(std::find_if(remaining_text.rbegin(), remaining_text.rend(),
                                              [](unsigned char ch){return !std::isspace(ch);}).base(), remaining_text.end());

            chunks.push_back(remaining_text);
        }
        return chunks;
    }
} // doc