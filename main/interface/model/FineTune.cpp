//
// Created by v2ray on 2023/4/26.
//

#include "FineTune.h"

namespace ft {

    inline nlohmann::json convert_nkpr_scn(const std::string& source, const std::string& prompt_prefix, const std::string& completion_prefix) {
        nlohmann::json source_j;
        try {
            source_j = nlohmann::json::parse(source);
        } catch (const std::exception& e) {
            throw std::invalid_argument("Invalid source format: " + std::string(e.what()));
        }
        if (!source_j.contains("scenes") || !source_j["scenes"].is_array()) {
            throw std::invalid_argument("Invalid source format: missing scenes array.");
        }
        std::vector<std::pair<std::string, std::string>> character_text_pairs;
        for (const auto& scene : source_j["scenes"]) {
            if (!scene.is_object() || !scene.contains("texts") || !scene["texts"].is_array()) {
                continue;
            }
            for (const auto& text_info : scene["texts"]) {
                if (!text_info.is_array() || text_info.size() < 3) {
                    continue;
                }
                nlohmann::json messages = text_info[2];
                if (!messages.is_array() || messages.size() < 2) {
                    continue;
                }
                nlohmann::json message = messages[1];
                if (!message.is_array() || message.size() < 2 || !message[0].is_string() || !message[1].is_string()) {
                    continue;
                }
                character_text_pairs.emplace_back(message[0].get<std::string>(), message[1].get<std::string>());
            }
        }
        PCRERegex regex("(?:%f[^;]*;|「|」)"); //Matches "%f...;", "「", and "」".
        static const std::string protagonist = "Kashou";
        std::string last_protagonist_text;
        nlohmann::json result;
        for (auto& [character, text] : character_text_pairs) {
            regex.replace_all(text, "");
            if (character == protagonist) {
                last_protagonist_text = text;
                continue;
            }
            text.insert(0, " ");
            result.push_back({{"prompt", prompt_prefix + last_protagonist_text
            + completion_prefix}, {"completion", text}}); //NOLINT(performance-inefficient-string-concatenation)
        }
        return result;
    }

    /**
     * @return The converted json for fine tuning.
     * Json format:
     * [
     *   {
     *     "prompt": "Raw prompt1",
     *     "completion": "Raw completion1"
     *   },
     *   {
     *     "prompt": "Raw prompt2",
     *     "completion": "Raw completion2"
     *   }
     * ]
     */
    nlohmann::json convert(const std::string& source, const SourceType& source_type,
                           const std::string& prompt_prefix, const std::string& completion_prefix) {
        switch (source_type) {
            case SourceType::NKPR_SCN:
                return convert_nkpr_scn(source, prompt_prefix, completion_prefix);
            default:
                throw std::invalid_argument("Unknown source type: " + to_string(source_type));
        }
    }

    std::string to_string(const SourceType& source_type) {
        switch (source_type) {
            case SourceType::NKPR_SCN:
                return "NKPR_SCN";
            default:
                return "UNKNOWN";
        }
    }

    SourceType from_string(const std::string& source_type) {
        if (source_type == "NKPR_SCN") {
            return SourceType::NKPR_SCN;
        }
        return SourceType::UNKNOWN;
    }
} // ft