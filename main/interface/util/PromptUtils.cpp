//
// Created by v2ray on 2023/3/2.
//

#include "PromptUtils.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "tbb/tbb.h"

#define USER_COLOR Term::color_fg(175, 200, 255)
#define BOT_COLOR Term::color_fg(175, 255, 225)

namespace prompt {

    void print_prompt(const std::string& initial_prompt, const std::shared_ptr<chat::ExchangeHistory>& chat_history,
                      const std::string& me_id, const std::string& bot_id, const unsigned int& max_length,
                      const bool& is_new_api, const bool& space_between_exchanges) {
        util::print_cs(to_string(initial_prompt, chat_history, me_id, bot_id, max_length, true, space_between_exchanges), true);
        if (!chat_history->empty() && !chat_history->back()->hasResponse()) {
            util::print_cs(BOT_COLOR + bot_id + (is_new_api ? ": " : ":"), false, false);
        }
    }

    template<typename T>
    void erase_except_back(T& s, const unsigned int& keep_back_count) {
        if (s.size() > keep_back_count) {
            s.erase(s.begin(), std::prev(s.end(), keep_back_count));
        }
    }

    std::string to_string(std::string initial_prompt, const chat::Messages& messages, const std::string& me_id,
                          const std::string& bot_id, const bool& add_color, const bool& space_between_exchanges) {
        for (const auto& message : messages) {
            switch (message.second) {
                case chat::Messages::Role::ASSISTANT:
                    initial_prompt.append((add_color ? BOT_COLOR : "") + (boost::format("\n%1%: %2%") % bot_id % message.first).str());
                    if (space_between_exchanges) {
                        initial_prompt.append("\n");
                    }
                    break;
                default:
                    initial_prompt.append((add_color ? USER_COLOR : "") + (boost::format("\n%1%: %2%") % me_id % message.first).str());
                    break;
            }
        }
        return initial_prompt;
    }

    std::string to_string(const std::string& initial_prompt, const std::shared_ptr<chat::ExchangeHistory>& chat_history,
                          const std::string& me_id, const std::string& bot_id, const unsigned int& max_length,
                          const bool& add_color, const bool& space_between_exchanges) {
        chat::ExchangeHistory tmp = *chat_history;
        erase_except_back(tmp, max_length);
        return to_string(initial_prompt, tmp.to_messages(), me_id, bot_id, add_color, space_between_exchanges);
    }

    inline void append_time(std::string& s) {
        s.append("\nCurrent time: " + util::currentTimeFormatted() + "\n");
    }

    inline std::vector<std::pair<std::shared_ptr<chat::Exchange>, double>> async_find_similar(
            const std::vector<float>& input_embeddings, const std::vector<std::shared_ptr<chat::Exchange>>& chat_history,
            const bool& search_response) {
        tbb::concurrent_vector<std::pair<std::shared_ptr<chat::Exchange>, double>> computed_exchanges;
        tbb::parallel_for(tbb::blocked_range<size_t>(0, chat_history.size()), [&](const tbb::blocked_range<size_t>& r){
            for (size_t i = r.begin(); i != r.end(); i++) {
                const std::shared_ptr<chat::Exchange>& exchange = chat_history[i];
                double similarity = emb::cosine_similarity(exchange->getInputEmbeddings(), input_embeddings);
                if (search_response && exchange->hasResponse() && exchange->hasResponseEmbeddings()) {
                    double response_similarity = emb::cosine_similarity(exchange->getResponseEmbeddings(), input_embeddings);
                    if (response_similarity > similarity) {
                        similarity = response_similarity;
                    }
                }
                if (similarity >= 0.8) { //Similarity >= 0.8 is considered as a match.
                    computed_exchanges.emplace_back(exchange, similarity);
                }
            }
        });
        return {computed_exchanges.begin(), computed_exchanges.end()};
    }

    inline std::vector<std::tuple<doc::Document, double, size_t>> async_find_similar(const std::vector<float>& input_embeddings,
                                                                                     const std::vector<doc::Document>& documents) {
        tbb::concurrent_vector<std::tuple<doc::Document, double, size_t>> computed_documents;
        tbb::parallel_for(tbb::blocked_range<size_t>(0, documents.size()), [&](const tbb::blocked_range<size_t>& r){
            for (size_t i = r.begin(); i != r.end(); i++) {
                const doc::Document& document = documents[i];
                double similarity = emb::cosine_similarity(document.getEmbeddings(), input_embeddings);
                if (similarity >= 0.8) {
                    computed_documents.emplace_back(document, similarity, i);
                }
            }
        });
        return {computed_documents.begin(), computed_documents.end()};
    }

    /**
     * Construct long-term memory reference for general chatting.
     * @return The constructed initial prompt.
     */
    std::string construct_reference(std::string initial_prompt, const std::vector<float>& input_embeddings,
                                    const std::shared_ptr<chat::ExchangeHistory>& chat_history, const bool& search_response,
                                    const unsigned int& max_reference_length, const unsigned int& max_short_memory_length,
                                    const std::string& me_id, const std::string& bot_id) {
        append_time(initial_prompt);
        if (max_reference_length == 0) {
            return initial_prompt;
        }
        std::vector<std::shared_ptr<chat::Exchange>> chat_history_vec(chat_history->begin(), chat_history->end());
        if (chat_history_vec.size() > max_short_memory_length) {
            chat_history_vec.erase(chat_history_vec.end() - max_short_memory_length, chat_history_vec.end());
        } else {
            chat_history_vec.clear();
        }
        auto computed_exchanges = async_find_similar(input_embeddings, chat_history_vec, search_response);
        if (computed_exchanges.empty()) {
            return initial_prompt;
        }
        std::sort(computed_exchanges.begin(), computed_exchanges.end(), [](const auto& a, const auto& b){
            return a.second < b.second;
        });
        erase_except_back(computed_exchanges, max_reference_length);
        std::sort(computed_exchanges.begin(), computed_exchanges.end(), [](const auto& a, const auto& b){
            return a.first->getTimeMS() < b.first->getTimeMS();
        });
        initial_prompt.append("\nChat exchanges for reference:\n\"\"\"\n");
        std::string ref;
        for (const auto& reference : computed_exchanges) {
            std::shared_ptr<chat::Exchange> exchange = reference.first;
            ref.append("\n\n" + util::ms_to_formatted_time(exchange->getTimeMS()));
            ref.append("\n" + me_id + ": " + exchange->getInput());
            if (exchange->hasResponse()) {
                ref.append("\n" + bot_id + ": " + exchange->getResponse());
            }
        }
        if (boost::starts_with(ref, "\n\n")) {
            ref.erase(0, 2);
        }
        return initial_prompt.append(ref + "\n\"\"\"\n");
    }

    /**
     * Construct reference for document Q&A.
     * @return The constructed initial prompt.
     */
    std::string construct_reference(std::string initial_prompt, const std::vector<float>& input_embeddings,
                                    const std::vector<doc::Document>& documents, const unsigned int& max_reference_length) {
        append_time(initial_prompt);
        if (max_reference_length == 0) {
            return initial_prompt;
        }
        //tuple<document, similarity, index>
        auto computed_documents = async_find_similar(input_embeddings, documents);
        if (computed_documents.empty()) {
            return initial_prompt;
        }
        std::sort(computed_documents.begin(), computed_documents.end(), [](const auto& a, const auto& b){
            return std::get<1>(a) < std::get<1>(b);
        });
        erase_except_back(computed_documents, max_reference_length);
        std::sort(computed_documents.begin(), computed_documents.end(), [](const auto& a, const auto& b){
            return std::get<2>(a) < std::get<2>(b);
        });
        initial_prompt.append("\nDocuments for reference:\n\"\"\"\n");
        std::string ref;
        size_t index = 0;
        for (const auto& reference : computed_documents) {
            ref.append("\n\nDocument snippet " + std::to_string(++index) + ": \"" + std::get<0>(reference).getText() + "\"");
        }
        if (boost::starts_with(ref, "\n\n")) {
            ref.erase(0, 2);
        }
        return initial_prompt.append(ref + "\n\"\"\"\n");
    }
} // prompt

namespace GPT {

    inline std::string get_pre_prompt(const std::string& me_id, const std::string& bot_id) {
        return (boost::format(
                "The following conversation is set to:\n"
                "%1%: is the prefix of the user, texts start with it are the user input\n"
                "%2%: is the prefix of your response, texts start with it are your response\n") % me_id % bot_id).str();
    }

    std::string to_payload(std::string initial_prompt, const chat::Messages& messages, const std::string& me_id, const std::string& bot_id) {
        const std::string& pre_prompt = get_pre_prompt(me_id, bot_id);
        if (!boost::starts_with(initial_prompt, pre_prompt)) {
            initial_prompt.insert(0, pre_prompt);
        }
        return prompt::to_string(initial_prompt, messages, me_id, bot_id) + "\n" + bot_id + ":";
    }
} // GPT

namespace ChatGPT {

    /**
     * This function converts the variables into a json format.
     * Json format:
     * [
     *     {"role": "system", "content": "<initial prompt>"},
     *     {"role": "user", "content": "<user input history 1>"},
     *     {"role": "assistant", "content": "<bot response history 1>"},
     *     {"role": "user", "content": "<user input history 2>"},
     *     {"role": "assistant", "content": "<bot response history 2>"},
     *     {"role": "user", "content": "<user current input>"}
     * ]
     */
    nlohmann::json to_payload(std::string initial_prompt, const chat::Messages& messages, const std::string& model, const std::string& me_id,
                              const std::string& bot_id) {
        boost::replace_all(initial_prompt, GPT::get_pre_prompt(me_id, bot_id), "");
        nlohmann::json payload = nlohmann::json::array();
        //For GPT-3.5, using "user" role is better for initial prompt, if OpenAI ever updates the model, I will change it to "system".
        payload.push_back({{"role", boost::starts_with(model, "gpt-3.5") ? "user" : "system"}, {"content", initial_prompt}});
        for (const auto& message : messages) {
            nlohmann::json j = nlohmann::json::object();
            switch (message.second) {
                case chat::Messages::Role::SYSTEM:
                    j["role"] = "system";
                    break;
                case chat::Messages::Role::ASSISTANT:
                    j["role"] = "assistant";
                    break;
                default:
                    j["role"] = "user";
                    break;
            }
            j["content"] = message.first;
            payload.push_back(j);
        }
        return payload;
    }
} // ChatGPT