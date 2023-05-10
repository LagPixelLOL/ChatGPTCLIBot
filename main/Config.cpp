//
// Created by v2ray on 2023/5/10.
//

#include "Config.h"

namespace config {

    //class Config start:
    Config::Config() : config_path("config.json") {}
    Config::Config(std::filesystem::path config_path) : config_path(std::move(config_path)) {}
    Config::~Config() = default;

    /**
     * @throw std::exception If an error occurred when loading the config file.
     */
    void Config::load_config(const std::function<void(const Log::LogMsg<std::filesystem::path>& msg)>& log_callback) {
        log_callback({Log::Level::INFO, "Loading config file...", config_path});
        if (!file::exists(config_path)) {
            log_callback({Log::Level::ERR, "Config file does not exist, creating new config file..."});
            save_config();
            return;
        }
        nlohmann::json j = nlohmann::json::parse(file::read_text_file(config_path));
        bool error = false;
        auto it_api_key = j.find("api_key");
        if (it_api_key != j.end()) {
            if (!it_api_key->is_null()) {
                if (it_api_key->is_string()) {
                    api::set_key(it_api_key->get<std::string>());
                } else if (it_api_key->is_array()) {
                    std::vector<std::string> keys;
                    for (const auto& key : *it_api_key) {
                        if (key.is_string()) {
                            keys.push_back(key.get<std::string>());
                        } else {
                            log_callback({Log::Level::ERR, "Argument api_key has non-string element. Element: " + key.dump(2)});
                            error = true;
                        }
                    }
                    api::set_key(keys);
                } else {
                    log_callback({Log::Level::ERR, "Argument api_key is not a string, array or null."});
                    error = true;
                }
            }
        } else {
            log_callback({Log::Level::ERR, "Argument api_key is not found."});
            error = true;
        }
        auto it_model = j.find("model");
        if (it_model != j.end() && it_model->is_string()) {
            model = it_model->get<std::string>();
        } else {
            log_callback({Log::Level::ERR, "Argument model is not a string."});
            error = true;
        }
        auto it_temperature = j.find("temperature");
        if (it_temperature != j.end() && it_temperature->is_number()) {
            temperature = it_temperature->get<float>();
        } else {
            log_callback({Log::Level::ERR, "Argument temperature is not a number."});
            error = true;
        }
        auto it_max_tokens = j.find("max_tokens");
        if (it_max_tokens != j.end() && it_max_tokens->is_number_integer()) {
            max_tokens = it_max_tokens->get<int>();
            if (max_tokens <= 0) {
                log_callback({Log::Level::ERR, "Argument max_tokens is less or equal to 0."});
                max_tokens = 1;
                error = true;
            }
        } else {
            log_callback({Log::Level::ERR, "Argument max_tokens is not an integer number."});
            error = true;
        }
        auto it_top_p = j.find("top_p");
        if (it_top_p != j.end() && it_top_p->is_number()) {
            top_p = it_top_p->get<float>();
        } else {
            log_callback({Log::Level::ERR, "Argument top_p is not a number."});
            error = true;
        }
        auto it_frequency_penalty = j.find("frequency_penalty");
        if (it_frequency_penalty != j.end() && it_frequency_penalty->is_number()) {
            frequency_penalty = it_frequency_penalty->get<float>();
        } else {
            log_callback({Log::Level::ERR, "Argument frequency_penalty is not a number."});
            error = true;
        }
        auto it_presence_penalty = j.find("presence_penalty");
        if (it_presence_penalty != j.end() && it_presence_penalty->is_number()) {
            presence_penalty = it_presence_penalty->get<float>();
        } else {
            log_callback({Log::Level::ERR, "Argument presence_penalty is not a number."});
            error = true;
        }
        auto it_logit_bias = j.find("logit_bias");
        if (it_logit_bias != j.end() && it_logit_bias->is_object()) {
            logit_bias.clear();
            for (const auto& [key, value] : it_logit_bias->items()) {
                if (!value.is_number()) {
                    log_callback({Log::Level::ERR, "Argument logit_bias has non-number bias. Value: " + value.dump(2)});
                    error = true;
                    continue;
                }
                float bias = value.get<float>();
                if (bias < -100 || bias > 100) {
                    log_callback({Log::Level::ERR, "Argument logit_bias's bias is out of range, it must be between -100 and 100. Bias: "
                    + std::to_string(bias)});
                    error = true;
                    continue;
                }
                logit_bias[key] = bias;
            }
        } else {
            log_callback({Log::Level::ERR, "Argument logit_bias is not an object."});
            error = true;
        }
        auto it_max_display_length = j.find("max_display_length");
        if (it_max_display_length != j.end() && it_max_display_length->is_number_unsigned()) {
            max_display_length = it_max_display_length->get<unsigned int>();
            if (max_display_length == 0) {
                log_callback({Log::Level::ERR, "Argument max_display_length cannot be 0."});
                max_display_length = 1;
                error = true;
            }
        } else {
            log_callback({Log::Level::ERR, "Argument max_display_length is not an unsigned integer."});
            error = true;
        }
        auto it_max_short_memory_length = j.find("max_short_memory_length");
        if (it_max_short_memory_length != j.end() && it_max_short_memory_length->is_number_unsigned()) {
            max_short_memory_length = it_max_short_memory_length->get<unsigned int>();
            if (max_short_memory_length == 0) {
                log_callback({Log::Level::ERR, "Argument max_short_memory_length cannot be 0."});
                max_short_memory_length = 1;
                error = true;
            }
        } else {
            log_callback({Log::Level::ERR, "Argument max_short_memory_length is not an unsigned integer."});
            error = true;
        }
        auto it_max_reference_length = j.find("max_reference_length");
        if (it_max_reference_length != j.end() && it_max_reference_length->is_number_unsigned()) {
            max_reference_length = it_max_reference_length->get<unsigned int>();
        } else {
            log_callback({Log::Level::ERR, "Argument max_reference_length is not an unsigned integer."});
            error = true;
        }
        auto it_search_response = j.find("search_response");
        if (it_search_response != j.end() && it_search_response->is_boolean()) {
            search_response = it_search_response->get<bool>();
        } else {
            log_callback({Log::Level::ERR, "Argument search_response is not a boolean."});
            error = true;
        }
        auto it_space_between_exchanges = j.find("space_between_exchanges");
        if (it_space_between_exchanges != j.end() && it_space_between_exchanges->is_boolean()) {
            space_between_exchanges = it_space_between_exchanges->get<bool>();
        } else {
            log_callback({Log::Level::ERR, "Argument space_between_exchanges is not a boolean."});
            error = true;
        }
        auto it_debug_reference = j.find("debug_reference");
        if (it_debug_reference != j.end() && it_debug_reference->is_boolean()) {
            debug_reference = it_debug_reference->get<bool>();
        } else {
            log_callback({Log::Level::ERR, "Argument debug_reference is not a boolean."});
            error = true;
        }
#ifdef __linux__
        auto it_ca_bundle_path = j.find("ca_bundle_path");
        if (it_ca_bundle_path != j.end() && it_ca_bundle_path->is_string()) {
            util::set_ca_bundle_path(it_ca_bundle_path->get<std::string>());
        } else {
            log_callback({Log::Level::ERR, "Argument ca_bundle_path is not a string."});
            error = true;
        }
#endif
        if (error) {
            log_callback({Log::Level::INFO, "Error detected, creating new config file..."});
            save_config(log_callback);
            return;
        }
        log_callback({Log::Level::INFO, "Config loaded."});
    }

    /**
     * @throw std::exception If an error occurred when saving the config file.
     */
    void Config::save_config(const std::function<void(const Log::LogMsg<std::filesystem::path>& msg)>& log_callback) {
        log_callback({Log::Level::INFO, "Saving config to file...", config_path});
        nlohmann::json j;
        std::vector<std::string> api_keys = api::get_keys();
        if (api_keys.empty()) {
            j["api_key"] = nlohmann::json::value_t::null;
        } else if (api_keys.size() == 1) {
            j["api_key"] = api_keys[0];
        } else {
            j["api_key"] = api_keys;
        }
        j["model"] = model;
        j["temperature"] = temperature;
        j["max_tokens"] = max_tokens;
        j["top_p"] = top_p;
        j["frequency_penalty"] = frequency_penalty;
        j["presence_penalty"] = presence_penalty;
        nlohmann::json pair_json_object = nlohmann::json::object();
        for (const auto& pair : logit_bias) {
            pair_json_object[pair.first] = pair.second;
        }
        j["logit_bias"] = pair_json_object;
        j["max_display_length"] = max_display_length;
        j["max_short_memory_length"] = max_short_memory_length;
        j["max_reference_length"] = max_reference_length;
        j["search_response"] = search_response;
        j["space_between_exchanges"] = space_between_exchanges;
        j["debug_reference"] = debug_reference;
#ifdef __linux__
        j["ca_bundle_path"] = util::get_ca_bundle_path();
#endif
        file::write_text_file(j.dump(2), config_path);
        log_callback({Log::Level::INFO, "Config saved."});
    }

    /**
     * @throw std::exception If an error occurred when loading the documents.
     */
    void Config::load_documents(const std::string& filename,
                                const std::function<void(const Log::LogMsg<std::filesystem::path>& msg)>& log_callback) {
        const auto& doc_path = std::filesystem::path(f_documentQA) / (filename + ".json");
        log_callback({Log::Level::INFO, "Loading document Q&A from file...", doc_path});
        nlohmann::json j = nlohmann::json::parse(file::read_text_file(doc_path));
        auto it_documents = j.find("documents");
        if (it_documents == j.end() || !it_documents->is_array()) {
            throw std::invalid_argument("Argument documents is not an array.");
        }
        auto it_initial = j.find("initial");
        if (it_initial == j.end() || !it_initial->is_string()) {
            throw std::invalid_argument("Argument initial is not a string.");
        }
        documents = doc::from_json(*it_documents);
        initial_prompt = it_initial->get<std::string>();
        documentQA_mode = true;
        log_callback({Log::Level::INFO, "Document Q&A loaded."});
    }
    //class Config end.
} // config