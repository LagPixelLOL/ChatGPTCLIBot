//
// Created by v2ray on 2023/4/20.
//

#ifndef GPT3BOT_DOCUMENT_H
#define GPT3BOT_DOCUMENT_H

#include "string"
#include "vector"
#include "stdexcept"
#include "nlohmann/json.hpp"

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
        void setContent(const std::string& text, const std::vector<float>& embeddings);
    };

    std::vector<Document> from_raw(const std::vector<std::string>& texts, const std::vector<std::vector<float>>& embeddings);
    std::vector<Document> from_json(const nlohmann::json& j);
    nlohmann::json to_json(const std::vector<Document>& documents);
} // doc

#endif //GPT3BOT_DOCUMENT_H
