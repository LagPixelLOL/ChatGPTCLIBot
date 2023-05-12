//
// Created by v2ray on 2023/5/10.
//

#ifndef GPT3BOT_CONFIG_H
#define GPT3BOT_CONFIG_H

#include "interface/model/Completion.h"
#include "interface/log/LogMsg.h"

namespace config {

    class Config {
    public:
        const std::filesystem::path config_path;
        std::vector<std::string> input_history;
        const std::string default_initial_prompt_filename = "Default";
        const std::string f_initial = "initial";
        const std::string f_saved = "saved";
        const std::string f_documentQA = "documentQA";
    private:
        std::string model = "gpt-3.5-turbo";
        bool is_new_api_ = true;
    public:
        float temperature = 1;
        int max_tokens = 500;
        float top_p = 1;
        float frequency_penalty = 0;
        float presence_penalty = 0.6;
        std::vector<std::pair<std::string, float>> logit_bias;
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

        [[nodiscard]] chat::Completion to_completion() const;

        void load_config(const std::function<void(const Log::LogMsg<std::filesystem::path>& msg)>& log_callback = [](const auto&){});
        void save_config(const std::function<void(const Log::LogMsg<std::filesystem::path>& msg)>& log_callback = [](const auto&){});
        void load_documents(const std::string& filename,
                            const std::function<void(const Log::LogMsg<std::filesystem::path>& msg)>& log_callback = [](const auto&){});

        [[maybe_unused]] [[nodiscard]] std::string get_model() const;
        [[maybe_unused]] void set_model(const std::string& model_);
        [[nodiscard]] bool is_new_api() const;
    };
} // config

#endif //GPT3BOT_CONFIG_H
