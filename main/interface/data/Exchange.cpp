//
// Created by v2ray on 2023/3/4.
//

#include "Exchange.h"

namespace chat {

    //class Exchange start:
    Exchange::Exchange(std::string input, std::vector<float> input_embeddings, long long time_ms) :
    input_(std::move(input)), input_embeddings_(std::move(input_embeddings)), time_ms_(time_ms) {}

    Exchange::Exchange(std::string input, std::vector<float> input_embeddings, std::string response, long long time_ms) :
    Exchange(std::move(input), std::move(input_embeddings), time_ms) {
        response_ = std::move(response);
    }

    Exchange::Exchange(std::string input, std::vector<float> input_embeddings, std::string response,
                       std::vector<float> response_embeddings, long long int time_ms) :
                       Exchange(std::move(input), std::move(input_embeddings), std::move(response), time_ms) {
        response_embeddings_ = std::move(response_embeddings);
    }

    /**
     * @throw std::invalid_argument If j is not valid.
     */
    Exchange::Exchange(const nlohmann::json& j) {
        if (!j.is_object()) {
            throw std::invalid_argument("Argument is not an object.");
        }
        auto it_input = j.find("input");
        if (it_input == j.end() || !it_input->is_string()) {
            throw std::invalid_argument("Key input is not a string.");
        }
        auto it_input_embeddings = j.find("input_embeddings");
        if (it_input_embeddings == j.end() || !it_input_embeddings->is_array()) {
            throw std::invalid_argument("Key input_embeddings is not an array.");
        }
        auto it_time_stamp = j.find("time_stamp");
        if (it_time_stamp == j.end() || !it_time_stamp->is_number_integer()) {
            throw std::invalid_argument("Key time_stamp is not an integer.");
        }
        input_ = it_input->get<std::string>();
        input_embeddings_ = it_input_embeddings->get<std::vector<float>>();
        time_ms_ = it_time_stamp->get<long long>();
        auto it_response = j.find("response");
        if (it_response != j.end() && it_response->is_string()) {
            response_ = it_response->get<std::string>();
            auto it_response_embeddings = j.find("response_embeddings");
            if (it_response_embeddings != j.end() && it_response_embeddings->is_array()) {
                response_embeddings_ = it_response_embeddings->get<std::vector<float>>();
            }
        }
    }

    Exchange::~Exchange() = default;

    bool Exchange::operator==(const Exchange& rhs) const {
        return input_ == rhs.input_ && input_embeddings_ == rhs.input_embeddings_ && response_ == rhs.response_
        && response_embeddings_ == rhs.response_embeddings_ && time_ms_ == rhs.time_ms_;
    }

    bool Exchange::operator!=(const Exchange& rhs) const {
        return !(rhs == *this);
    }

    size_t hash_value(const Exchange& instance) {
        size_t seed = 0;
        seed ^= std::hash<std::string>{}(instance.input_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<std::string>{}(instance.response_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<long long>{}(instance.time_ms_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    nlohmann::json Exchange::to_json() const {
        nlohmann::json j = nlohmann::json::object();
        j["input"] = getInput();
        j["input_embeddings"] = getInputEmbeddings();
        j["time_stamp"] = getTimeMS();
        if (hasResponse()) {
            j["response"] = getResponse();
            if (hasResponseEmbeddings()) {
                j["response_embeddings"] = getResponseEmbeddings();
            }
        }
        return j;
    }

    const std::string& Exchange::getInput() const {
        return input_;
    }

    const std::vector<float>& Exchange::getInputEmbeddings() const {
        return input_embeddings_;
    }

    const std::string& Exchange::getResponse() const {
        return response_;
    }

    bool Exchange::hasResponse() const {
        return !response_.empty();
    }

    void Exchange::setResponse(const std::string& response) {
        response_ = response;
    }

    const std::vector<float>& Exchange::getResponseEmbeddings() const {
        return response_embeddings_;
    }

    bool Exchange::hasResponseEmbeddings() const {
        return !response_embeddings_.empty();
    }

    void Exchange::setResponseEmbeddings(const std::vector<float>& response_embeddings) {
        response_embeddings_ = response_embeddings;
    }

    const long long& Exchange::getTimeMS() const {
        return time_ms_;
    }
    //class Exchange end.
} // chat

namespace std {

    size_t hash<chat::Exchange>::operator()(const chat::Exchange& instance) const {
        return chat::hash_value(instance);
    }
}