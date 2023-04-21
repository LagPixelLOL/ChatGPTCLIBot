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
} // doc