//
// Created by v2ray on 2023/4/20.
//

#ifndef GPT3BOT_DOCUMENT_H
#define GPT3BOT_DOCUMENT_H

#include "../cpp-terminal/platforms/conversion.hpp"
#include "../cpp-tiktoken/pcre2_regex.h"
#include "../util/TokenUtils.h"
#include "../util/FileUtils.h"
#include "../util/Base64.h"

namespace doc {

    class Document {
        std::string text_;
        std::vector<float> embeddings_;

    public:
        Document(std::string text, std::vector<float> embeddings);
        explicit Document(const nlohmann::json& j);
        virtual ~Document();

        [[nodiscard]] nlohmann::json to_json() const;

        [[nodiscard]] const std::string& getText() const;
        [[nodiscard]] const std::vector<float>& getEmbeddings() const;
        [[maybe_unused]] void setContent(const std::string& text, const std::vector<float>& embeddings);
    };

    std::vector<Document> from_raw(const std::vector<std::string>& texts, const std::vector<std::vector<float>>& embeddings);
    std::vector<Document> from_json(const nlohmann::json& j);
    nlohmann::json to_json(const std::vector<Document>& documents);
    std::vector<std::string> split_text(const std::string& text, const unsigned int& tokens_per_chunk, const bool& remove_new_lines = true);
    [[maybe_unused]] void test_split_text();
} // doc

#endif //GPT3BOT_DOCUMENT_H
