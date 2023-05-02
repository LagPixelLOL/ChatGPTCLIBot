//
// Created by v2ray on 2023/4/20.
//

#include "Document.h"

namespace doc {

    //class Document start:
    Document::Document(std::string text, std::vector<float> embeddings) : text_(std::move(text)), embeddings_(std::move(embeddings)) {}

    /**
     * Construct a Document object from a json object.
     * @param j A json object containing "text" and "embeddings" fields.
     * @throw std::invalid_argument If the json object is not valid.
     */
    Document::Document(const nlohmann::json& j) {
        if (!j.is_object()) {
            throw std::invalid_argument("Argument j must be a json object.");
        }
        if (!j.contains("text") || !j.contains("embeddings")) {
            throw std::invalid_argument("Argument j must contain 'text' and 'embeddings' fields.");
        }
        if (!j["text"].is_string() || !j["embeddings"].is_array()) {
            throw std::invalid_argument("Argument j must contain 'text' and 'embeddings' fields of type string and array.");
        }
        text_ = j["text"].get<std::string>();
        for (const nlohmann::json& e : j["embeddings"]) {
            if (!e.is_number()) {
                throw std::invalid_argument("Argument j must contain 'text' and 'embeddings' fields of type string and array.");
            }
            embeddings_.emplace_back(e.get<float>());
        }
    }

    Document::~Document() = default;

    /**
     * Convert the Document object to a json object.
     * @return A json object containing "text" and "embeddings" fields.
     */
    nlohmann::json Document::to_json() const {
        nlohmann::json j = nlohmann::json::object();
        j["text"] = text_;
        j["embeddings"] = embeddings_;
        return j;
    }

    const std::string& Document::getText() const {
        return text_;
    }

    const std::vector<float>& Document::getEmbeddings() const {
        return embeddings_;
    }

    void Document::setContent(const std::string& text, const std::vector<float>& embeddings) {
        text_ = text;
        embeddings_ = embeddings;
    }
    //class Document end.

    /**
     * Construct a list of Document objects from a list of texts and a list of embeddings.
     * The two lists must have the same size.
     * @throw std::invalid_argument If the two lists have different sizes.
     * @return A list of Document objects.
     */
    std::vector<Document> from_raw(const std::vector<std::string>& texts, const std::vector<std::vector<float>>& embeddings) {
        if (texts.size() != embeddings.size()) {
            throw std::invalid_argument("Texts and embeddings must have the same size.");
        }
        std::vector<Document> result;
        result.reserve(texts.size());
        for (int i = 0; i < texts.size(); ++i) {
            result.emplace_back(texts[i], embeddings[i]);
        }
        return result;
    }

    /**
     * Construct a list of Document objects from a json array.
     * @param j A json array containing json objects of type Document.
     * @throw std::invalid_argument If the json array is not valid.
     * @return A list of Document objects.
     */
    std::vector<Document> from_json(const nlohmann::json& j) {
        if (!j.is_array()) {
            throw std::invalid_argument("Argument j must be a json array.");
        }
        std::vector<Document> result;
        result.reserve(j.size());
        for (const nlohmann::json& e : j) {
            result.emplace_back(e);
        }
        return result;
    }

    /**
     * Convert a list of Document objects to a json array.
     * @param documents A list of Document objects.
     * @return A json array containing json objects of type Document.
     */
    nlohmann::json to_json(const std::vector<Document>& documents) {
        nlohmann::json j = nlohmann::json::array();
        for (const Document& e : documents) {
            j.push_back(e.to_json());
        }
        return j;
    }

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
            size_t last_punctuation = std::max({chunk_text.rfind('.'), chunk_text.rfind('?'), chunk_text.rfind('!'), chunk_text.rfind('\n')});
            if (last_punctuation != std::string::npos) {
                chunk_text.erase(last_punctuation + 1);
            }
            std::string chunk_text_to_append = chunk_text;
            PCRERegex("\n+").replace_all(chunk_text_to_append, " "); //Match one or more newlines and replace them with a single space.
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