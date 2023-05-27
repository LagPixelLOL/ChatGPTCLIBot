//
// Created by v2ray on 2023/4/20.
//

#include "Document.h"
#include "iostream"

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
            throw std::invalid_argument("Argument must be a json object.");
        }
        auto it_text = j.find("text");
        auto it_embeddings = j.find("embeddings");
        if (it_text == j.end() || it_embeddings == j.end()) {
            throw std::invalid_argument("Argument must contain 'text' and 'embeddings' fields.");
        }
        if (!it_text->is_string() || !it_embeddings->is_array()) {
            throw std::invalid_argument("Argument must contain 'text' and 'embeddings' fields of type string and array.");
        }
        text_ = it_text->get<std::string>();
        for (const nlohmann::json& e : *it_embeddings) {
            if (e.is_string()) {
                embeddings_.push_back(base64::b64_str_to_float(e.get<std::string>()));
            } else if (e.is_number()) {
                embeddings_.push_back(e.get<float>());
            } else {
                throw std::invalid_argument("Argument must contain 'embeddings' field of type string or number.");
            }
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
        j["embeddings"] = base64::embeddings_to_b64_json_array(embeddings_);
        return j;
    }

    const std::string& Document::getText() const {
        return text_;
    }

    const std::vector<float>& Document::getEmbeddings() const {
        return embeddings_;
    }

    [[maybe_unused]] void Document::setContent(const std::string& text, const std::vector<float>& embeddings) {
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
    std::vector<std::shared_ptr<Document>> from_raw(const std::vector<std::string>& texts,
                                                    const std::vector<std::vector<float>>& embeddings) {
        if (texts.size() != embeddings.size()) {
            throw std::invalid_argument("Texts and embeddings must have the same size.");
        }
        std::vector<std::shared_ptr<Document>> result;
        result.reserve(texts.size());
        for (size_t i = 0; i < texts.size(); i++) {
            result.push_back(std::make_shared<Document>(texts[i], embeddings[i]));
        }
        return result;
    }

    /**
     * Construct a list of Document objects from a json array.
     * @param j A json array containing json objects of type Document.
     * @throw std::invalid_argument If the json array is not valid.
     * @return A list of Document objects.
     */
    std::vector<std::shared_ptr<Document>> from_json(const nlohmann::json& j) {
        if (!j.is_array()) {
            throw std::invalid_argument("Argument j must be a json array.");
        }
        std::vector<std::shared_ptr<Document>> result;
        result.reserve(j.size());
        for (const nlohmann::json& e : j) {
            result.push_back(std::make_shared<Document>(e));
        }
        return result;
    }

    /**
     * Convert a list of Document objects to a json array.
     * @param documents A list of Document objects.
     * @return A json array containing json objects of type Document.
     */
    nlohmann::json to_json(const std::vector<std::shared_ptr<Document>>& documents) {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& e : documents) {
            j.push_back(e->to_json());
        }
        return j;
    }

    inline std::unordered_set<int> get_punctuation_token_set(const std::shared_ptr<GptEncoding>& tokenizer) {
        static const PCRERegex regex("^(?:.|\\n)*[.!?\\n] *$");
        std::unordered_set<int> result;
        for (const auto& [byte_pair, token] : tokenizer->get_byte_pair_token_map()) {
            std::string s(byte_pair.begin(), byte_pair.end());
            if (regex.contains(s)) {
                result.insert(token);
            }
        }
        return result;
    }

    inline void process_append_str(std::string& s) {
        static const PCRERegex regex("\\n+");
        regex.replace_all(s, " "); //Match one or more new lines and replace them with a single space.
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){return !std::isspace(ch);}));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){return !std::isspace(ch);}).base(), s.end());
    }

    /**
     * Split a text into chunks.
     * @param text The text to split.
     * @param tokens_per_chunk The maximum number of tokens per chunk, tokenizer is CL100K_BASE.
     * @return A list of split text chunks.
     */
    std::vector<std::string> split_text(const std::string& text, const unsigned int& tokens_per_chunk, const bool& remove_new_lines) {
        if (text.empty() || text.find_first_not_of(' ') == std::string::npos || tokens_per_chunk <= 0) {
            return {};
        }
        try { //Pre-check for invalid UTF-8 encoding.
            Term::Private::utf8_to_utf32(text);
        } catch (const Term::Exception& e) {
            throw std::invalid_argument("Invalid UTF-8 encoding in text.[1]");
        }
        static const std::shared_ptr<GptEncoding> tokenizer = util::get_enc_cache(LanguageModel::CL100K_BASE);
        static const std::unordered_set<int> punctuation_token_set = get_punctuation_token_set(tokenizer);
        std::vector<int> tokens = tokenizer->encode(text);
        std::vector<std::string> chunks;
        while (!tokens.empty()) {
            long long chunk_sub_size = std::min(static_cast<long long>(tokens_per_chunk), static_cast<long long>(tokens.size()));
            std::vector<int> chunk(tokens.begin(), tokens.begin() + chunk_sub_size);
            if (chunk.empty()) {
                tokens.erase(tokens.begin(), tokens.begin() + static_cast<long long>(chunk.size()));
                continue;
            }
            long long additional_tokens = 0;
            while (true) { //UTF-8 characters may be split into multiple tokens, so we need to check if the chunk is valid.
                try {
                    //Check if the chunk's encoding is valid by converting it to UTF-32.
                    Term::Private::utf8_to_utf32(tokenizer->decode(chunk));
                    break; //If it doesn't throw an exception, the chunk is valid.
                } catch (const Term::Exception& e) {
                    long long new_index = chunk_sub_size + additional_tokens++; //The index of the next token.
                    if (tokens.size() <= new_index) { //If the next token doesn't exist, the chunk is invalid.
                        throw std::invalid_argument("Invalid UTF-8 encoding in text.[2]");
                    }
                    chunk.emplace_back(tokens[new_index]); //Otherwise, add the next token to the chunk.
                }
            }
            for (size_t i = chunk.size(); i-- > 0;) { //Remove until the last punctuation token.
                if (punctuation_token_set.contains(chunk[i])) {
                    size_t erase_start_index = i + 1;
                    if (erase_start_index >= tokens_per_chunk / 2) { //If the chunk is too small, don't perform the removal.
                        chunk.erase(chunk.begin() + static_cast<long long>(erase_start_index), chunk.end());
                    }
                    break;
                }
            }
            std::string text_to_append = tokenizer->decode(chunk);
            if (text_to_append.empty() || text_to_append.find_first_not_of(' ') == std::string::npos) {
                tokens.erase(tokens.begin(), tokens.begin() + static_cast<long long>(chunk.size()));
                continue;
            }
            if (remove_new_lines) {
                //Remove leading and trailing whitespaces and replace multiple new lines with a single space.
                process_append_str(text_to_append);
            }
            chunks.emplace_back(text_to_append);
            tokens.erase(tokens.begin(), tokens.begin() + static_cast<long long>(chunk.size()));
        }
        if (!tokens.empty()) {
            std::string remaining_text = tokenizer->decode(tokens);
            if (remove_new_lines) {
                process_append_str(remaining_text);
            }
            chunks.emplace_back(remaining_text);
        }
        return chunks;
    }

    /**
     * Test function for split_text, do not use this function in production.
     */
    [[maybe_unused]] void test_split_text() {
        static const std::filesystem::path dir("documentQA");
        std::string text = file::read_text_file(dir / "testdoc.txt");
        std::vector<std::string> chunks = split_text(text, 50);
        std::string to_write = "----------\n";
        for (const std::string& chunk : chunks) {
            to_write += chunk + "\n----------\n";
        }
        if (to_write.ends_with('\n')) {
            to_write.erase(to_write.size() - 1);
        }
        file::write_text_file(to_write, dir / "test_result.txt");
    }
} // doc