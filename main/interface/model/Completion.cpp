//
// Created by v2ray on 2023/5/12.
//

#include "Completion.h"

namespace chat {
    //class Completion start:
    Completion::Completion() = default;
    Completion::~Completion() = default;

    /**
     * Construct the initial prompt with embeddings searching.
     * @throw std::invalid_argument When chat_history is empty.
     */
    void Completion::construct_initial() {
        if (!chat_history->empty()) {
            if (documents) {
                constructed_initial = prompt::construct_reference(initial_prompt, chat_history->back()->getInputEmbeddings(),
                                                                  documents, max_reference_length);
            } else {
                constructed_initial = prompt::construct_reference(initial_prompt, chat_history->back()->getInputEmbeddings(),
                                                                  chat_history, search_response, max_reference_length,
                                                                  max_short_memory_length, me_id, bot_id);
            }
        } else {
            throw std::invalid_argument("Field chat_history is empty.");
        }
    }

    /**
     * Call the OpenAI API.
     * Note: Blocking IO operation.
     * @throw std::exception When an error occurred.
     */
    void Completion::call_api() const {
        api::call_api(constructed_initial, prompt::construct_from_back(*chat_history, max_short_memory_length).to_messages(), api_key,
                      model, temperature, max_tokens, top_p, frequency_penalty, presence_penalty, logit_bias, me_id, bot_id,
                      stream_callback, progress_callback);
    }

    const std::string& Completion::getApiKey() const {
        return api_key;
    }

    void Completion::setAPIKey(const std::string& apiKey) {
        api_key = apiKey;
    }

    const std::string& Completion::getModel() const {
        return model;
    }

    void Completion::setModel(const std::string& model_) {
        model = model_;
    }

    const std::string& Completion::getInitialPrompt() const {
        return initial_prompt;
    }

    void Completion::setInitialPrompt(const std::string& initialPrompt) {
        initial_prompt = initialPrompt;
        constructed_initial = initialPrompt;
    }

    const std::string& Completion::getConstructedInitial() const {
        return constructed_initial;
    }

    const std::shared_ptr<chat::ExchangeHistory>& Completion::getChatHistory() const {
        return chat_history;
    }

    void Completion::setChatHistory(const std::shared_ptr<chat::ExchangeHistory>& chatHistory) {
        chat_history = chatHistory;
    }

    float Completion::getTemperature() const {
        return temperature;
    }

    void Completion::setTemperature(float temperature_) {
        temperature = temperature_;
    }

    float Completion::getTopP() const {
        return top_p;
    }

    void Completion::setTopP(float topP) {
        top_p = topP;
    }

    int Completion::getMaxTokens() const {
        return max_tokens;
    }

    void Completion::setMaxTokens(int maxTokens) {
        max_tokens = maxTokens;
    }

    float Completion::getPresencePenalty() const {
        return presence_penalty;
    }

    void Completion::setPresencePenalty(float presencePenalty) {
        presence_penalty = presencePenalty;
    }

    float Completion::getFrequencyPenalty() const {
        return frequency_penalty;
    }

    void Completion::setFrequencyPenalty(float frequencyPenalty) {
        frequency_penalty = frequencyPenalty;
    }

    const std::vector<std::pair<std::string, float>>& Completion::getLogitBias() const {
        return logit_bias;
    }

    void Completion::setLogitBias(const std::vector<std::pair<std::string, float>>& logitBias) {
        logit_bias = logitBias;
    }

    unsigned int Completion::getMaxShortMemoryLength() const {
        return max_short_memory_length;
    }

    void Completion::setMaxShortMemoryLength(unsigned int maxShortMemoryLength) {
        max_short_memory_length = maxShortMemoryLength;
    }

    unsigned int Completion::getMaxReferenceLength() const {
        return max_reference_length;
    }

    void Completion::setMaxReferenceLength(unsigned int maxReferenceLength) {
        max_reference_length = maxReferenceLength;
    }

    const std::shared_ptr<std::vector<std::shared_ptr<doc::Document>>>& Completion::getDocuments() const {
        return documents;
    }

    void Completion::setDocuments(const std::shared_ptr<std::vector<std::shared_ptr<doc::Document>>>& documents_) {
        documents = documents_;
    }

    bool Completion::shouldSearchResponse() const {
        return search_response;
    }

    void Completion::setSearchResponse(bool searchResponse) {
        search_response = searchResponse;
    }

    const std::string& Completion::getMeID() const {
        return me_id;
    }

    void Completion::setMeID(const std::string& meID) {
        me_id = meID;
    }

    const std::string& Completion::getBotID() const {
        return bot_id;
    }

    void Completion::setBotID(const std::string& botID) {
        bot_id = botID;
    }

    const std::function<void(const std::string&)>& Completion::getStreamCallback() const {
        return stream_callback;
    }

    void Completion::setStreamCallback(const std::function<void(const std::string&)>& streamCallback) {
        stream_callback = streamCallback;
    }

    const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& Completion::getProgressCallback() const {
        return progress_callback;
    }

    void Completion::setProgressCallback(const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)>& progressCallback) {
        progress_callback = progressCallback;
    }
    //class Completion end.
} // chat