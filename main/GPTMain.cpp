//
// Created by v2ray on 2023/2/18.
//

#include "GPTMain.h"

namespace GPT {
    config::Config config("config.json");

    std::atomic_bool ctrl_c_flag = false;
    const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)> progress_callback = [](auto, auto, auto, auto){
        bool return_value = ctrl_c_flag;
        ctrl_c_flag = false;
        return return_value;
    };

    void on_ctrl_c_interrupt(int) {
        ctrl_c_flag = true;
        signal(SIGINT, on_ctrl_c_interrupt);
    }

    /**
     * The main function for GPT3Bot.
     */
    void pre_settings() {
        util::print_m_clr(
                "   ********    *******    **********            ****        ******                  **  \n"
                "  **//////**  /**////**  /////**///            */// *      /*////**                /**  \n"
                " **      //   /**   /**      /**              /    /*      /*   /**     ******    ******\n"
                "/**           /*******       /**       *****     ***       /******     **////**  ///**/ \n"
                "/**    *****  /**////        /**      /////     /// *      /*//// **  /**   /**    /**  \n"
                "//**  ////**  /**            /**               *   /*      /*    /**  /**   /**    /**  \n"
                " //********   /**            /**              / ****       /*******   //******     //** \n"
                "  ////////    //             //                ////        ///////     //////       //  ",
                {{255, 15, 125}, {125, 40, 255}}, true);
        std::cout << "\n\n";
        try {
            config.load_config([](const Log::LogMsg<std::filesystem::path>& log){
                switch (log.get_level()) {
                    case Log::Level::INFO:
                        util::println_info(log.get_message());
                        if (log.get_payload()) {
                            util::println_info("Config file: " + PATH_S(*log.get_payload()));
                        }
                        break;
                    case Log::Level::ERR:
                        util::println_err("Error loading config file: " + log.get_message());
                        if (log.get_payload()) {
                            util::println_err("Config file: " + PATH_S(*log.get_payload()));
                        }
                        break;
                    default:
                        break;
                }
            });
        } catch (const std::exception& e) {
            util::println_err("Error loading config file: " + std::string(e.what()));
            return;
        }
        if (!create_folders({config.f_initial, config.f_saved, config.f_documentQA}) || !p_default_prompt()) {
            return;
        }
        p_check_set_api_key();
        while (true) {
            util::print_cs("Please choose whether you want to load the initial prompt,\nload the saved chat history, "
                           "load the saved document Q&A,\ncreate a new document Q&A, enter fine tune helper,\nor translate texts.\n"
                           "(Input " + GOLDEN_TEXT("i") + " for initial, " + GOLDEN_TEXT("s") + " for saved, "
                           + GOLDEN_TEXT("d") + " for load doc Q&A,\n " + GOLDEN_TEXT("c") + " for create new doc Q&A, "
                           + GOLDEN_TEXT("f") + " for fine tune helper, " + GOLDEN_TEXT("t") + " for translator,\n "
                           + "or press " + ENTER + " directly to use initial): ");
            std::string chose_mode;
            getline(std::cin, chose_mode);
            std::transform(chose_mode.begin(), chose_mode.end(), chose_mode.begin(), tolower);
            if (chose_mode.empty() || chose_mode == "i") {
                util::print_cs("Please enter the initial prompt's filename you want to load.\n(Press " + ENTER + " to use default): ");
                std::string i_p_filename;
                getline(std::cin, i_p_filename);
                if (boost::ends_with(i_p_filename, f_suffix)) {
                    i_p_filename.erase(i_p_filename.size() - f_suffix.size());
                }
                if (!i_p_filename.empty() && !p_load_prompt(i_p_filename)) {
                    return;
                }
                break;
            } else if (chose_mode == "s") {
                std::cout << "Please enter the saved chat history's filename you want to load: ";
                std::string s_filename;
                getline(std::cin, s_filename);
                if (boost::ends_with(s_filename, json_suffix)) {
                    s_filename.erase(s_filename.size() - json_suffix.size());
                }
                if (!p_load_saved(s_filename)) {
                    return;
                }
                break;
            } else if (chose_mode == "d") {
                std::cout << "Please enter the document Q&A's filename you want to load: ";
                std::string d_filename;
                getline(std::cin, d_filename);
                if (boost::ends_with(d_filename, json_suffix)) {
                    d_filename.erase(d_filename.size() - json_suffix.size());
                }
                try {
                    config.load_documents(d_filename, [](const Log::LogMsg<std::filesystem::path>& log){
                        util::println_info(log.get_message());
                        if (log.get_payload()) {
                            util::println_info("Document file: " + PATH_S(*log.get_payload()));
                        }
                    });
                } catch (const std::exception& e) {
                    util::println_err("Error loading document Q&A file: " + std::string(e.what()));
                    return;
                }
                break;
            } else if (chose_mode == "c") {
                if (!p_create_docQA()) {
                    return;
                }
                break;
            } else if (chose_mode == "f") {
                fth::fine_tune_helper_main(config);
                return;
            } else if (chose_mode == "t") {
                translator::translator_main(config);
                return;
            } else {
                util::println_warn("Invalid input, please try again.");
            }
        }
        signal(SIGINT, on_ctrl_c_interrupt); //Setup SIGINT handler.
        start_loop();
    }

    /**
     * The main loop for GPT3Bot.
     */
    void start_loop() {
        while (true) {
            print_prompt();
            std::string input;
            try {
                input = util::get_multiline(config.input_history, me_id + ": ", [](Term::Model& m){
                    m.lines = {"/stop"};
                    m.cursor_col = 1;
                    m.cursor_row = 1;
                    return true;
                });
            } catch (const std::exception& e) {
                util::println_err("\nAn error occurred while getting input: " + std::string(e.what()));
                print_enter_next_cycle();
                continue;
            }
            cmd::ReturnOpCode rc = cmd::handle_command(input, config.initial_prompt, config.chat_history, me_id, bot_id,
                                                       config.max_display_length, config.space_between_exchanges,
                                                       config.documents.operator bool());
            if (rc == cmd::ReturnOpCode::CONTINUE) {
                continue;
            } else if (rc == cmd::ReturnOpCode::STOP) {
                break;
            }
            std::string api_key = api::get_key();
            util::println_info("Getting embeddings and finding similar chat exchanges for the input...", false);
            api::APIKeyStatus key_status = api::APIKeyStatus::VALID;
            std::vector<std::vector<float>> emb_response;
            std::vector<std::string> texts{input};
            bool get_response_embeddings = false;
            if (config.search_response && !config.documents && !config.chat_history->empty()) {
                const chat::Exchange& last_exchange = *config.chat_history->back();
                if (!last_exchange.hasResponseEmbeddings() && last_exchange.hasResponse()) {
                    get_response_embeddings = true;
                    texts.emplace_back(last_exchange.getResponse());
                }
            }
            try {
                ctrl_c_flag = false;
                emb_response = emb::get_embeddings(texts, api_key, key_status, config.api_base_url, progress_callback);
            } catch (const std::exception& e) {
                std::string err_msg(e.what());
                if (!err_msg.empty()) {
                    util::println_err("\nError when getting embeddings: " + err_msg);
                }
                if (key_status == api::APIKeyStatus::INVALID_KEY || key_status == api::APIKeyStatus::QUOTA_EXCEEDED) {
                    p_on_invalid_key();
                }
                print_enter_next_cycle();
                continue;
            }
            if (get_response_embeddings) {
                config.chat_history->back()->setResponseEmbeddings(emb_response[1]);
            }
            /* No need to do prompts.pop_back() before this line. */
            config.chat_history->push_back(std::make_shared<chat::Exchange>(input, emb_response[0], util::currentTimeMillis()));
            /* Need to do prompts.pop_back() after this line before continue;. */
            print_prompt();
            std::string response;
            try {
                ctrl_c_flag = false;
                chat::Completion completion = config.to_completion();
                completion.setMeID(me_id);
                completion.setBotID(bot_id);
                completion.construct_initial();
                if (config.debug_reference) {
                    std::string dr_prefix = Term::color_fg(255, 200, 0) + "<Debug Reference> " + Term::color_fg(Term::Color::Name::Default);
                    util::print_cs("\n" + dr_prefix + Term::color_fg(255, 225, 0)
                    + "Constructed initial prompt:\n----------\n" + completion.getConstructedInitial() + "\n----------", true);
                    util::print_cs(dr_prefix + "Press " + Term::color_fg(70, 200, 255) + "Enter"
                    + Term::color_fg(Term::Color::Name::Default) + " to continue: ");
                    util::ignore_line();
                    util::print_cs(Term::color_fg(175, 255, 225) + bot_id + (config.is_new_api() ? ": " : ":"), false, false);
                }
                completion.setStreamCallback([&response](const auto& streamed_response){
                    try {
                        nlohmann::json j = nlohmann::json::parse(streamed_response);
                        auto it_error = j.find("error");
                        if (it_error != j.end() && it_error->is_object()) {
                            response = j.dump();
                            return;
                        }
                    } catch (const nlohmann::json::parse_error& e) {}
                    response.append(streamed_response);
                    util::print_cs(streamed_response, false, false);
                });
                completion.setProgressCallback(progress_callback);
                completion.call_api();
                util::print_cs(""); //Reset color.
            } catch (const std::exception& e) {
                util::println_err("\nError when calling API: " + std::string(e.what()));
                print_enter_next_cycle();
                config.chat_history->pop_back();
                continue;
            }
            try {
                api::APIKeyStatus key_status_api;
                if (api::check_err_obj(nlohmann::json::parse(response), key_status_api)) {
                    if (key_status_api == api::APIKeyStatus::INVALID_KEY || key_status_api == api::APIKeyStatus::QUOTA_EXCEEDED) {
                        p_on_invalid_key();
                    }
                    print_enter_next_cycle();
                    config.chat_history->pop_back();
                    continue;
                }
            } catch (const nlohmann::json::parse_error& e) {}
            if (!config.is_new_api() && boost::starts_with(response, " ")) {
                response.erase(0, 1);
            }
            while (boost::ends_with(response, "\n")) {
                response.pop_back();
            }
            config.chat_history->back()->setResponse(response);
        }
    }

    void print_prompt() {
        clear_console();
        prompt::print_prompt(config.initial_prompt, config.chat_history, me_id, bot_id, config.max_display_length, config.is_new_api(),
                             config.space_between_exchanges);
    }

    void print_enter_next_cycle() {
        util::print_cs("Press " + ENTER + " to run the next cycle: ");
        util::ignore_line();
    }

    void clear_console() {
#ifdef _WIN32
        system("cls"); //Clear the console in Windows.
#else
        system("clear"); //Clear the console in Unix-based systems(e.g. Linux, macOS).
#endif
    }

    /**
     * @return True if the folders were created or already exist, false if an error occurred.
     */
    bool create_folders(const std::vector<std::string>& folders) {
        util::println_info("Creating folders...");
        try {
            std::vector<std::filesystem::path> paths;
            for (const auto& folder : folders) {
                paths.emplace_back(folder);
            }
            for (const auto& p : file::create_folders(paths)) {
                util::println_info("Created folder: " + PATH_S(p));
            }
        } catch (const std::exception& e) {
            util::println_err("Error creating folders: " + std::string(e.what()));
            return false;
        }
        util::println_info("Creating folders completed.");
        return true;
    }

    /**
     * @return True if the file was created or already exists and read, false if an error occurred.
     */
    bool p_default_prompt() {
        const auto& path_ = std::filesystem::path(config.f_initial) / (config.default_initial_prompt_filename + f_suffix);
        if (!file::exists(path_)) {
            try {
                util::println_info("Creating default prompt file: " + PATH_S(path_));
                file::write_text_file(config.initial_prompt, path_);
            } catch (const file::file_error& e) {
                util::println_err("Error creating file: " + PATH_S(e.get_path()));
                util::println_err("Reason: " + std::string(e.what()));
                return false;
            }
        } else {
            return p_load_prompt(config.default_initial_prompt_filename);
        }
        util::println_info("Default prompt file processing completed.");
        return true;
    }

    bool p_load_prompt(std::string filename) {
        const auto& path_ = std::filesystem::path(config.f_initial) / filename.append(f_suffix);
        util::println_info("Loading prompt file: " + PATH_S(path_));
        try {
            config.initial_prompt = file::read_text_file(path_);
        } catch (const file::file_error& e) {
            util::println_err("Error reading prompt file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        util::println_info("Prompt file loading completed.");
        return true;
    }

    /**
     * Load the saved chat history from the file(.json), and also does error handling.
     * It uses nlohmann::json library.
     * If the json is not in the correct format, it will return false.
     * @return True if the file already exists and read, false if an error occurred.
     */
    bool p_load_saved(std::string filename) {
        const auto& path_ = std::filesystem::path(config.f_saved) / filename.append(json_suffix);
        if (!file::exists(path_)) {
            util::println_err("Saved chat file does not exist: " + PATH_S(path_));
            return false;
        }
        nlohmann::json j;
        try {
            util::println_info("Reading saved chat file: " + PATH_S(path_));
            j = nlohmann::json::parse(file::read_text_file(path_));
        } catch (const file::file_error& e) {
            util::println_err("Error reading saved chat file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        } catch (const nlohmann::json::parse_error& e) {
            util::println_err("Error parsing saved chat file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        if (!j.contains("initial") || !j["initial"].is_string()) {
            util::println_err("Error reading saved chat file: " + PATH_S(path_));
            util::println_err("Reason: initial is not a string.");
            return false;
        }
        config.initial_prompt = j["initial"].get<std::string>();
        if (!j.contains("histories")) {
            util::println_err("Error reading saved chat file: " + PATH_S(path_));
            util::println_err("Reason: histories is not found.");
            return false;
        }
        auto histories = j["histories"];
        if (!histories.is_null()) {
            try {
                config.chat_history = std::make_shared<chat::ExchangeHistory>(histories);
            } catch (const std::exception& e) {
                util::println_err("Error reading saved chat file: " + PATH_S(path_));
                util::println_err("Reason: " + std::string(e.what()));
                return false;
            }
        }
        util::println_info("Saved chat file reading completed.");
        return true;
    }

    /**
     * Save the chat history to the file(name.json), and also does error handling.
     * It uses nlohmann::json library.
     * @return True if the file is correctly saved, false if an error occurred.
     */
    bool p_save_chat(std::string name) {
        const auto& path_ = std::filesystem::path(config.f_saved) / name.append(json_suffix);
        util::println_info("Saving chat to file: " + PATH_S(path_));
        nlohmann::json j = nlohmann::json::object();
        j["initial"] = config.initial_prompt;
        j["histories"] = config.chat_history->to_json();
        try {
            file::write_text_file(j.dump(2), path_);
        } catch (const file::file_error& e) {
            util::println_err("Error saving chat to file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        util::println_info("Chat saved to file: " + PATH_S(path_));
        return true;
    }

    /**
     * Create the document Q&A from the file(name.txt), and also does error handling.
     * Turn the document .txt file and save it as a json file, then load it using p_load_docQA().
     * @return True if the function is correctly executed, false if an error occurred.
     */
    bool p_create_docQA() {
        std::string filename;
        while (true) {
            std::cout << "Please enter the filename of the document .txt file you want to convert: ";
            getline(std::cin, filename);
            if (!filename.empty()) {
                if (boost::ends_with(filename, f_suffix)) {
                    filename.erase(filename.size() - f_suffix.size());
                }
                break;
            }
            util::println_warn("The filename cannot be empty, please try again.");
        }
        const auto& path_ = std::filesystem::path(config.f_documentQA) / (filename + f_suffix);
        std::string content;
        try {
            content = file::read_text_file(path_);
        } catch (const file::file_error& e) {
            util::println_err("Error reading document .txt file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        int max_tokens_per_chunk; //No need to initialize.
        while (true) {
            std::cout << "Please enter the maximum number of tokens per chunk.\n(It's recommended to be around 300): ";
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
        util::println_info("Splitting document .txt file into chunks...");
        std::vector<std::string> chunks;
        try {
            chunks = doc::split_text(content, max_tokens_per_chunk);
        } catch (const std::exception& e) {
            util::println_err("Error splitting document .txt file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        //Get the embeddings for each chunk.
        util::println_info("Getting embeddings for each chunk...", false);
        api::APIKeyStatus key_status = api::APIKeyStatus::VALID;
        std::vector<std::vector<float>> emb_response;
        try {
            emb_response = emb::get_embeddings(chunks, api::get_key(), key_status, config.api_base_url);
        } catch (const std::exception& e) {
            std::string err_msg(e.what());
            if (!err_msg.empty()) {
                util::println_err("\nError when getting embeddings: " + err_msg);
            }
            if (key_status == api::APIKeyStatus::INVALID_KEY || key_status == api::APIKeyStatus::QUOTA_EXCEEDED) {
                p_on_invalid_key();
            }
            return false;
        }
        std::cout << std::endl;
        nlohmann::json documents_j = doc::to_json(doc::from_raw(chunks, emb_response));
        std::string initial_prompt_;
        while (true) {
            util::print_cs("Please enter the initial prompt's filename you want to use for this document Q&A.\n"
                           "(Press " + ENTER + " to use default): ");
            std::string i_p_filename;
            getline(std::cin, i_p_filename);
            if (i_p_filename.empty()) {
                i_p_filename = config.default_initial_prompt_filename;
            } else if (boost::ends_with(i_p_filename, f_suffix)) {
                i_p_filename.erase(i_p_filename.size() - f_suffix.size());
            }
            const auto& path_i = std::filesystem::path(config.f_initial) / i_p_filename.append(f_suffix);
            try {
                initial_prompt_ = file::read_text_file(path_i);
                break;
            } catch (const file::file_error& e) {
                util::println_err("Error reading initial prompt file: " + PATH_S(e.get_path()));
                util::println_err("Reason: " + std::string(e.what()));
            }
            util::println_warn("An error occurred when reading the prompt file, please try again.");
        }
        nlohmann::json j = nlohmann::json::object();
        j["initial"] = initial_prompt_;
        j["documents"] = documents_j;
        std::string name_to_save;
        while (true) {
            util::print_cs("Please enter the filename you want to save the document Q&A as.\n"
                           "(Press " + ENTER + " to use the same name as the original file): ");
            getline(std::cin, name_to_save);
            if (name_to_save.empty()) {
                name_to_save = filename;
            } else if (boost::ends_with(name_to_save, json_suffix)) {
                name_to_save.erase(name_to_save.size() - json_suffix.size());
            }
            const auto& path_s = std::filesystem::path(config.f_documentQA) / (name_to_save + json_suffix);
            try {
                file::write_text_file(j.dump(2), path_s);
                util::println_info("Document Q&A file saved successfully: " + PATH_S(path_s));
                break;
            } catch (const file::file_error& e) {
                util::println_err("Error saving document Q&A file: " + PATH_S(e.get_path()));
                util::println_err("Reason: " + std::string(e.what()));
            }
            util::println_warn("An error occurred when saving the document Q&A file, please try again.");
        }
        try {
            config.load_documents(name_to_save, [](const Log::LogMsg<std::filesystem::path>& log){
                util::println_info(log.get_message());
                if (log.get_payload()) {
                    util::println_info("Document file: " + PATH_S(*log.get_payload()));
                }
            });
        } catch (const std::exception& e) {
            util::println_err("Error loading document Q&A file: " + std::string(e.what()));
            return false;
        }
        return true;
    }

    inline bool p_save_config() {
        try {
            config.save_config([](const Log::LogMsg<std::filesystem::path>& log){
                util::println_info(log.get_message());
                if (log.get_payload()) {
                    util::println_info("Config file: " + PATH_S(*log.get_payload()));
                }
            });
        } catch (const std::exception& e) {
            util::println_err("Error saving config file: " + std::string(e.what()));
            return false;
        }
        return true;
    }

    /**
     * Check if the api key is set, if not, ask the user to set it.
     * @return True if the api key is present or set, false if an error occurred.
     */
    bool p_check_set_api_key() {
        if (!api::has_key()) {
            api::set_key(api::get_key_from_console(config.api_base_url));
            return p_save_config();
        }
        return true;
    }

    void p_on_invalid_key() {
        if (api::get_key_count() > 1) {
            api::remove_first_key();
            util::println_warn("Unusable api key detected, removed it.");
            p_save_config();
        }
    }
}
