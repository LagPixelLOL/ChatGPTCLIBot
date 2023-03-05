//
// Created by v2ray on 2023/3/2.
//

#include "PromptUtils.h"

namespace prompt {

    void print_prompt(const string& initial_prompt, const vector<std::shared_ptr<chat::Exchange>>& prompts,
                      const string& me_id, const string& bot_id, const unsigned int& max_length, const bool& is_new_api) {
        cout << to_string(initial_prompt, prompts, me_id, bot_id, max_length);
        if (prompts.empty() || prompts.back()->hasResponse()) {
            cout << "\n" << me_id << ": ";
        } else {
            cout << "\n" << bot_id << (is_new_api ? ": " : ":");
        }
    }

    template<typename T>
    void delete_front_keep_back(vector<T>& vec, const unsigned int& keep_back_count) {
        if (vec.size() > keep_back_count) {
            vec.erase(vec.begin(), vec.end() - keep_back_count);
        }
    }

    string to_string(string initial_prompt, vector<std::shared_ptr<chat::Exchange>> prompts,
                     const string& me_id, const string& bot_id, const unsigned int& max_length) {
        delete_front_keep_back(prompts, max_length);
        for (const auto& exchange : prompts) {
            initial_prompt.append((format("\n%1%: %2%") % me_id % exchange->getInput()).str());
            if (exchange->hasResponse()) {
                initial_prompt.append((format("\n%1%: %2%") % bot_id % exchange->getResponse()).str());
            }
        }
        return initial_prompt;
    }

    string construct_reference(string initial_prompt, const vector<float>& input_embeddings,
                               vector<std::shared_ptr<chat::Exchange>> chat_exchanges,
                               const unsigned int& max_reference_length, const unsigned int& max_short_memory_length,
                               const string& me_id, const string& bot_id) {
        initial_prompt.append("\nCurrent time: " + util::currentTimeFormatted() + "\n");
        if (max_reference_length == 0) {
            return initial_prompt;
        }
        if (chat_exchanges.size() > max_short_memory_length) {
            chat_exchanges.erase(chat_exchanges.end() - max_short_memory_length, chat_exchanges.end());
        } else {
            chat_exchanges.clear();
        }
        vector<pair<chat::Exchange, double>> computed_exchanges;
        for (const auto& exchange : chat_exchanges) {
            double similarity = exchange->compare_similarity(input_embeddings);
            if (similarity >= 0.8) {
                computed_exchanges.emplace_back(*exchange, similarity);
            }
        }
        if (computed_exchanges.empty()) {
            return initial_prompt;
        }
        sort(computed_exchanges.begin(), computed_exchanges.end(),
             [](const auto& a, const auto& b){
            return a.second < b.second;
        });
        delete_front_keep_back(computed_exchanges, max_reference_length);
        sort(computed_exchanges.begin(), computed_exchanges.end(),
             [](const auto& a, const auto& b){
            return a.first.getTimeMS() < b.first.getTimeMS();
        });
        initial_prompt.append("\nChat exchanges for reference:\n\"");
        string ref;
        for (const auto& reference : computed_exchanges) {
            chat::Exchange exchange = reference.first;
            ref.append("\n\n" + util::ms_to_formatted_time(exchange.getTimeMS()));
            ref.append("\n" + me_id + ": " + exchange.getInput());
            if (exchange.hasResponse()) {
                ref.append("\n" + bot_id + ": " + exchange.getResponse());
            }
        }
        if (starts_with(ref, "\n\n")) {
            ref.erase(0, 2);
        }
        return initial_prompt.append(ref + "\"\n");
    }
} // prompt

namespace GPT {

    string to_payload(const string& initial_prompt, const vector<std::shared_ptr<chat::Exchange>>& prompts,
                      const string& me_id, const string& bot_id, const unsigned int& max_length) {
        return prompt::to_string(initial_prompt, prompts, me_id, bot_id, max_length) + "\n" + bot_id + ":";
    }
} // GPT

namespace ChatGPT {

    /**
     * This function converts the variables into a json format.
     * Json format:
     * [
     *     {"role": "system", "content": "<initial prompt>"},
     *     {"role": "user", "content": "<user input 1>"},
     *     {"role": "assistant", "content": "<bot response 1>"},
     *     {"role": "user", "content": "<user input 2>"},
     *     {"role": "assistant", "content": "<bot response 2>"},
     *     {"role": "user", "content": "<user input last>"}
     * ]
     */
    json to_payload(string initial_prompt, vector<std::shared_ptr<chat::Exchange>> prompts,
                    const string& me_id, const string& bot_id, const unsigned int& max_length) {
        prompt::delete_front_keep_back(prompts, max_length);
        replace_all(initial_prompt, (format(
                "The following conversation is set to:\n"
                "%1%: is the prefix of the user, texts start with it are the user input\n"
                "%2%: is the prefix of your response, texts start with it are your response\n")
                % me_id % bot_id).str(), "");
        json payload = json::array();
        payload.push_back({{"role", "user"}, {"content", initial_prompt}}); // system/user
        for (const auto& exchange : prompts) {
            payload.push_back({{"role", "user"}, {"content", exchange->getInput()}});
            if (exchange->hasResponse()) {
                payload.push_back({{"role", "assistant"}, {"content", exchange->getResponse()}});
            }
        }
        return payload;
    }
} // ChatGPT