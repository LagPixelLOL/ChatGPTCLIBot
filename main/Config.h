//
// Created by v2ray on 2023/5/10.
//

#ifndef GPT3BOT_CONFIG_H
#define GPT3BOT_CONFIG_H

#include "interface/data/ExchangeHistory.h"
#include "interface/data/Document.h"

namespace config {

    enum log_level {
        INFO, ERR
    };

    struct log {
        log_level level = INFO;
        std::string message;
        std::optional<std::filesystem::path> path = std::nullopt;
    };

    class Config {
    public:
        const std::filesystem::path config_path;
        std::vector<std::string> input_history;
        const std::string default_initial_prompt_filename = "Default";
        const std::string f_initial = "initial";
        const std::string f_saved = "saved";
        const std::string f_documentQA = "documentQA";
        std::string model = "gpt-3.5-turbo";
        bool is_new_api = false;
        float temperature = 1;
        int max_tokens = 500;
        float top_p = 1;
        float frequency_penalty = 0;
        float presence_penalty = 0.6;
        std::unordered_map<std::string, float> logit_bias;
        std::string initial_prompt = "You are an AI chat bot named Sapphire\n"
                                     "You are friendly and intelligent\n"
                                     "Your backend is OpenAI's ChatGPT API\n";
        std::shared_ptr<chat::ExchangeHistory> chat_history = std::make_shared<chat::ExchangeHistory>();
        unsigned int max_display_length = 100;
        unsigned int max_short_memory_length = 4;
        unsigned int max_reference_length = 4;
        std::vector<doc::Document> documents;
        bool documentQA_mode = false;
        bool search_response = true;
        bool space_between_exchanges = false;
        bool debug_reference = false;

        Config();
        explicit Config(std::filesystem::path config_path);
        virtual ~Config();

        void load_config(const std::function<void(const log& msg)>& log_callback = [](const auto&){});
        void save_config(const std::function<void(const log& msg)>& log_callback = [](const auto&){});
    };
} // config

#endif //GPT3BOT_CONFIG_H
