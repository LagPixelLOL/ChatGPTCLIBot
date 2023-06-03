//
// Created by v2ray on 2023/5/22.
//

#include "Translator.h"

#ifdef max
#undef max
#endif

namespace translator {
    const std::string translator_initial_prompt = "After this message you will translate every user messages to %1%\n"
                                                  "You need to keep the original markdown and format\n%2%"
                                                  "Respond with ONLY the translated texts with no other texts nor explanations\n";

    inline std::vector<std::string> translate(const std::vector<std::string>& chunks, const std::string& target_language,
                                              const std::string& additional_instructions, const unsigned int& max_connections,
                                              const config::Config& config) {
        if (chunks.empty()) {
            return {};
        }
        std::unordered_map<size_t, std::string> chunk_pool;
        for (size_t i = 0; i < chunks.size(); i++) {
            chunk_pool[i] = chunks[i];
        }
        std::unordered_map<size_t, std::string> translated_chunks;
        std::string initial_prompt = (boost::format(translator_initial_prompt) % target_language % (additional_instructions.empty() ? "" :
                "Additional instructions:\n\"\"\"\n" + additional_instructions + "\n\"\"\"\n")).str();
        bool is_new_api_ = api::is_new_api(config.get_model());
        std::string url = config.api_base_url + (is_new_api_ ? "/v1/chat/completions" : "/v1/completions");
        std::vector<std::string> headers = {"Content-Type: application/json"};
        std::string auth = "Authorization: Bearer ";
        headers.emplace_back(auth.append(api::get_key()));
        bool rate_limit_reached = false;
        while (!chunk_pool.empty()) {
            std::vector<std::shared_ptr<curl::RequestObject>> requests;
            std::vector<std::pair<size_t, std::string>> responses;
            std::vector<std::string> original_chunks;
            for (unsigned int i = 0; i < max_connections; i++) {
                if (chunk_pool.empty()) {
                    break;
                }
                std::pair<size_t, std::string> text_to_translate = *chunk_pool.begin();
                chunk_pool.erase(chunk_pool.begin());
                chat::Messages message({{text_to_translate.second, chat::Messages::Role::USER}});
                responses.emplace_back(text_to_translate.first, "");
                original_chunks.emplace_back(text_to_translate.second);
                requests.push_back(std::make_shared<curl::RequestObject>(curl::RequestObject::construct_http_post(
                        url, [i, &is_new_api_, &responses](const std::vector<char>& vec, CURL*){
                    api::handle_streamed_response(vec, is_new_api_, [&i, &responses](const std::string& streamed_response){
                        std::string& response = responses[i].second;
                        try {
                            nlohmann::json j = nlohmann::json::parse(streamed_response);
                            auto it_error = j.find("error");
                            nlohmann::json::const_iterator it_status;
                            if (it_error != j.end() && it_error->is_object() || (it_status = j.find("status")) != j.end()
                            && it_status->is_boolean() && !it_status->get<bool>()) {
                                response = j.dump();
                                return;
                            }
                        } catch (const nlohmann::json::parse_error& e) {}
                        response.append(streamed_response);
                    });
                }, api::to_payload(initial_prompt, message, config.get_model(), is_new_api_, 0, std::numeric_limits<int>::max(),
                                   1, 0, 0, {}, GPT::me_id, GPT::bot_id).dump(), headers, 40)));
            }
            if (rate_limit_reached) {
                util::println_info("Rate limit reached, waiting for 60 seconds...");
                std::this_thread::sleep_for(std::chrono::seconds(60));
                util::println_info("Resuming...");
            }
            rate_limit_reached = false;
            curl::multi_perform(requests);
            bool not_displayed_error = true;
            for (size_t i = 0; i < requests.size(); i++) {
                const size_t& index = responses[i].first;
                std::string& response = responses[i].second;
                const std::string& original_text = original_chunks[i];
                try {
                    requests[i]->get_exception();
                } catch (const std::exception& e) {
                    if (not_displayed_error) {
                        util::println_err("\nError when calling API: " + std::string(e.what()));
                        not_displayed_error = false;
                    }
                    chunk_pool.emplace(index, original_text);
                    continue;
                }
                try {
                    api::APIKeyStatus key_status_api;
                    if (api::check_err_obj(nlohmann::json::parse(response), key_status_api, not_displayed_error)) {
                        not_displayed_error = false;
                        if (key_status_api == api::APIKeyStatus::RATE_LIMIT_REACHED) {
                            rate_limit_reached = true;
                        }
                        chunk_pool.emplace(index, original_text);
                        continue;
                    }
                } catch (const nlohmann::json::parse_error& e) {}
                if (!config.is_new_api() && boost::starts_with(response, " ")) {
                    response.erase(0, 1);
                }
                while (boost::ends_with(response, "\n")) {
                    response.pop_back();
                }
                translated_chunks.emplace(index, response);
            }
            const size_t& chunks_left = chunk_pool.size();
            util::println_info(chunks_left > 0 ? std::to_string(chunks_left) + " chunks left." :
            "All chunks translated, translation completed.");
        }
        std::vector<std::string> translated_texts(translated_chunks.size());
        for (const auto& pair : translated_chunks) {
            if (pair.first >= translated_texts.size()) {
                throw std::invalid_argument("Translated chunk index out of bounds: " + std::to_string(pair.first)
                + " >= " + std::to_string(translated_texts.size()));
            }
            translated_texts[pair.first] = pair.second;
        }
        return translated_texts;
    }

    void translator_main(const config::Config& config) {
        std::string filename;
        while (true) {
            std::cout << "Please enter the filename of the text file you want to translate: ";
            getline(std::cin, filename);
            if (!filename.empty()) {
                if (boost::ends_with(filename, GPT::f_suffix)) {
                    filename.erase(filename.size() - GPT::f_suffix.size());
                }
                break;
            }
            util::println_warn("The filename cannot be empty, please try again.");
        }
        static const std::filesystem::path folder(config.f_documentQA);
        const auto& path_ = folder / (filename + GPT::f_suffix);
        std::string content;
        try {
            content = file::read_text_file(path_);
        } catch (const file::file_error& e) {
            util::println_err("Error reading text file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        std::string target_language;
        while (true) {
            std::cout << "Please enter the target language: ";
            getline(std::cin, target_language);
            if (!target_language.empty()) {
                break;
            }
            util::println_warn("The target language cannot be empty, please try again.");
        }
        std::cout << "Please enter the additional instructions(Leave empty if there's none)." << std::endl;
        static std::vector<std::string> input_history;
        std::string additional_instructions = util::get_multiline(input_history);
        int max_tokens_per_chunk; //No need to initialize.
        while (true) {
            std::cout << "Please enter the maximum number of tokens per chunk.\n(It's recommended to be around 1500): ";
            std::string input;
            getline(std::cin, input);
            if (input.empty()) {
                util::println_warn("The input cannot be empty, please try again.");
                continue;
            }
            try {
                max_tokens_per_chunk = std::stoi(input);
                if (max_tokens_per_chunk > 0) {
                    break;
                }
                util::println_warn("The maximum number of tokens per chunk must be greater than 0, please try again.");
            } catch (const std::exception& e) {
                util::println_warn("The input is not a valid number or it's too large, please try again.");
            }
        }
        util::println_info("Splitting file into chunks...");
        std::vector<std::string> chunks;
        try {
            chunks = doc::split_text(content, max_tokens_per_chunk, false);
        } catch (const std::exception& e) {
            util::println_err("Error splitting text file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        int max_connections; //No need to initialize.
        while (true) {
            std::cout << "Please enter the maximum number of connections.\n(It's recommended to be around 500): ";
            std::string input;
            getline(std::cin, input);
            if (input.empty()) {
                util::println_warn("The input cannot be empty, please try again.");
                continue;
            }
            try {
                max_connections = std::stoi(input);
                if (max_connections > 0) {
                    break;
                }
                util::println_warn("The maximum number of connections must be greater than 0, please try again.");
            } catch (const std::exception& e) {
                util::println_warn("The input is not a valid number or it's too large, please try again.");
            }
        }
        util::println_info("Translating...");
        std::vector<std::string> translated_chunks;
        try {
            translated_chunks = translate(chunks, target_language, additional_instructions, max_connections, config);
        } catch (const std::exception& e) {
            util::println_err("Error translating text file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        std::string translated_text;
        for (const std::string& translated_chunk : translated_chunks) {
            translated_text.append(translated_chunk).append(" ");
        }
        if (boost::ends_with(translated_text, " ")) {
            translated_text.erase(translated_text.size() - 1);
        }
        while (true) {
            util::print_cs("Please enter the filename you want to save the translated text as.\n"
                           "(Press " + ENTER + " to use original_name_target_language.txt): ");
            std::string name_to_save;
            getline(std::cin, name_to_save);
            if (name_to_save.empty()) {
                name_to_save.append(filename).append("_").append(target_language);
            } else if (boost::ends_with(name_to_save, GPT::f_suffix)) {
                name_to_save.erase(name_to_save.size() - GPT::f_suffix.size());
            }
            const auto& path_s = folder / name_to_save.append(GPT::f_suffix);
            try {
                file::write_text_file(translated_text, path_s);
                util::println_info("Translated text saved successfully: " + PATH_S(path_s));
                break;
            } catch (const file::file_error& e) {
                util::println_err("Error saving translated text: " + PATH_S(e.get_path()));
                util::println_err("Reason: " + std::string(e.what()));
            }
            util::println_warn("An error occurred when saving the translated text, please try again.");
        }
        util::println_info("Translator finished.");
    }
} // translator