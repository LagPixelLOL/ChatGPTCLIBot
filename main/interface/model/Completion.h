//
// Created by v2ray on 2023/5/12.
//

#ifndef GPT3BOT_COMPLETION_H
#define GPT3BOT_COMPLETION_H

#include "../network/Network.h"

namespace chat {

    class Completion {
        std::string api_key;
        std::string model;
        std::string initial_prompt;
        std::string constructed_initial;
        std::shared_ptr<chat::ExchangeHistory> chat_history = std::make_shared<chat::ExchangeHistory>();
        float temperature = 1;
        float top_p = 1;
        int max_tokens = 500;
        float presence_penalty = 0.6;
        float frequency_penalty = 0;
        std::vector<std::pair<std::string, float>> logit_bias;
        unsigned int max_short_memory_length = 4;
        unsigned int max_reference_length = 4;
        std::shared_ptr<std::vector<std::shared_ptr<doc::Document>>> documents;
        bool search_response = true;
        std::string me_id = "Me";
        std::string bot_id = "You";
        std::function<void(const std::string& streamed_response)> stream_callback = [](const auto&){};
        std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)> progress_callback = [](auto, auto, auto, auto){return 0;};

    public:
        Completion();
        virtual ~Completion();

        void construct_initial();
        void call_api() const;

        [[nodiscard]] const std::string& getApiKey() const;
        void setAPIKey(const std::string& apiKey);
        [[nodiscard]] const std::string& getModel() const;
        void setModel(const std::string& model);
        [[nodiscard]] const std::string& getInitialPrompt() const;
        void setInitialPrompt(const std::string& initialPrompt);
        [[nodiscard]] const std::string& getConstructedInitial() const;
        [[nodiscard]] const std::shared_ptr<chat::ExchangeHistory>& getChatHistory() const;
        void setChatHistory(const std::shared_ptr<chat::ExchangeHistory>& chatHistory);
        [[nodiscard]] float getTemperature() const;
        void setTemperature(float temperature);
        [[nodiscard]] float getTopP() const;
        void setTopP(float topP);
        [[nodiscard]] int getMaxTokens() const;
        void setMaxTokens(int maxTokens);
        [[nodiscard]] float getPresencePenalty() const;
        void setPresencePenalty(float presencePenalty);
        [[nodiscard]] float getFrequencyPenalty() const;
        void setFrequencyPenalty(float frequencyPenalty);
        [[nodiscard]] const std::vector<std::pair<std::string, float>>& getLogitBias() const;
        void setLogitBias(const std::vector<std::pair<std::string, float>>& logitBias);
        [[nodiscard]] unsigned int getMaxShortMemoryLength() const;
        void setMaxShortMemoryLength(unsigned int maxShortMemoryLength);
        [[nodiscard]] unsigned int getMaxReferenceLength() const;
        void setMaxReferenceLength(unsigned int maxReferenceLength);
        [[nodiscard]] const std::shared_ptr<std::vector<std::shared_ptr<doc::Document>>>& getDocuments() const;
        void setDocuments(const std::shared_ptr<std::vector<std::shared_ptr<doc::Document>>>& documents_);
        [[nodiscard]] bool shouldSearchResponse() const;
        void setSearchResponse(bool searchResponse);
        [[nodiscard]] const std::string& getMeID() const;
        void setMeID(const std::string& meID);
        [[nodiscard]] const std::string& getBotID() const;
        void setBotID(const std::string& botID);
        [[nodiscard]] const std::function<void(const std::string&)>& getStreamCallback() const;
        void setStreamCallback(const std::function<void(const std::string&)>& streamCallback);
        [[nodiscard]] const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& getProgressCallback() const;
        void setProgressCallback(const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progressCallback);
    };
} // chat

#endif //GPT3BOT_COMPLETION_H
