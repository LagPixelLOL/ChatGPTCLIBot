//
// Created by v2ray on 2023/2/18.
//

#include "GPTMain.h"

namespace GPT {
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
    std::vector<std::shared_ptr<chat::Exchange>> prompts;
    unsigned int max_display_length = 100;
    unsigned int max_short_memory_length = 4;
    unsigned int max_reference_length = 4;
    std::vector<doc::Document> documents;
    bool documentQA_mode = false;
    bool search_response = true;
    bool space_between_exchanges = false;
    bool debug_reference = false;

    std::atomic_bool ctrl_c_flag = false;
    const std::function<int(curl_off_t, curl_off_t, curl_off_t, curl_off_t)> progress_callback = [](auto, auto, auto, auto){
        bool return_value = ctrl_c_flag;
        ctrl_c_flag = false;
        return return_value;
    };

    void on_ctrl_c_interrupt(int signal_) {
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
        if (!create_folders({f_initial, f_saved, f_documentQA}) || !p_load_config() || !p_default_prompt()) {
            return;
        }
        is_new_api = api::is_new_api(model);
        p_check_set_api_key();
        while (true) {
            util::print_cs("Please choose whether you want to load the initial prompt, load the saved chat history,\n"
                           "load the saved document Q&A, or create a new document Q&A.\n"
                           "(Input " + GOLDEN_TEXT("i") + " for initial, " + GOLDEN_TEXT("s") + " for saved,\n "
                           + GOLDEN_TEXT("d") + " for load doc Q&A, " + GOLDEN_TEXT("c") + " for create new doc Q&A,\n "
                           + GOLDEN_TEXT("f") + " for fine tune helper, or press " + ENTER + " directly to use initial): ");
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
                if (!p_load_docQA(d_filename)) {
                    return;
                }
                break;
            } else if (chose_mode == "c") {
                if (!p_create_docQA()) {
                    return;
                }
                break;
            } else if (chose_mode == "f") {
                fth::fine_tune_helper_main();
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
                input = util::get_multiline(input_history, me_id + ": ", [](Term::Model& m){
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
            cmd::ReturnOpCode rc = cmd::handle_command(input, initial_prompt, prompts, me_id, bot_id, max_display_length,
                                                       space_between_exchanges, documentQA_mode);
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
            bool need_to_get_response_embeddings = false;
            if (search_response && !documentQA_mode && !prompts.empty()) {
                const chat::Exchange& last_exchange = *prompts.back();
                if (!last_exchange.hasResponseEmbeddings() && last_exchange.hasResponse()) {
                    need_to_get_response_embeddings = true;
                    texts.emplace_back(last_exchange.getResponse());
                }
            }
            try {
                ctrl_c_flag = false;
                emb_response = emb::get_embeddings(texts, api_key, key_status, progress_callback);
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
            if (need_to_get_response_embeddings) {
                prompts.back()->setResponseEmbeddings(emb_response[1]);
            }
            /* No need to do prompts.pop_back() before this line. */
            prompts.emplace_back(std::make_shared<chat::Exchange>(input, emb_response[0], util::currentTimeMillis()));
            /* Need to do prompts.pop_back() after this line before continue;. */
            print_prompt();
            std::string response;
            try {
                std::optional<std::vector<doc::Document>> documents_opt = std::nullopt;
                if (documentQA_mode) {
                    documents_opt = documents;
                }
                ctrl_c_flag = false;
                api::call_api(initial_prompt, prompts, api_key, model, temperature, max_tokens, top_p,
                              frequency_penalty, presence_penalty, logit_bias, search_response && !documentQA_mode,
                              max_short_memory_length, max_reference_length, me_id, bot_id, [&response](const auto& streamed_response){
                    try {
                        nlohmann::json j = nlohmann::json::parse(streamed_response);
                        if (j.count("error") > 0 && j["error"].is_object()) {
                            response = j.dump();
                            return;
                        }
                    } catch (const nlohmann::json::parse_error& e) {}
                    response.append(streamed_response);
                    util::print_cs(streamed_response, false, false);
                }, debug_reference, true, documents_opt, progress_callback);
                util::print_cs(""); //Reset color.
            } catch (const std::exception& e) {
                util::println_err("\nError when calling API: " + std::string(e.what()));
                print_enter_next_cycle();
                prompts.pop_back();
                continue;
            }
            try {
                api::APIKeyStatus key_status_api;
                if (api::check_err_obj(nlohmann::json::parse(response), key_status_api)) {
                    if (key_status_api == api::APIKeyStatus::INVALID_KEY || key_status_api == api::APIKeyStatus::QUOTA_EXCEEDED) {
                        p_on_invalid_key();
                    }
                    print_enter_next_cycle();
                    prompts.pop_back();
                    continue;
                }
            } catch (const nlohmann::json::parse_error& e) {}
            if (!is_new_api && boost::starts_with(response, " ")) {
                response.erase(0, 1);
            }
            while (boost::ends_with(response, "\n")) {
                response.pop_back();
            }
            prompts.back()->setResponse(response);
        }
    }

    void print_prompt() {
        clear_console();
        prompt::print_prompt(initial_prompt, prompts, me_id, bot_id, max_display_length, is_new_api, space_between_exchanges);
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
        const auto& path_ = std::filesystem::path(f_initial) / (default_initial_prompt_filename + f_suffix);
        if (!exists(path_)) {
            try {
                util::println_info("Creating default prompt file: " + PATH_S(path_));
                file::write_text_file(initial_prompt, path_);
            } catch (const file::file_error& e) {
                util::println_err("Error creating file: " + PATH_S(e.get_path()));
                util::println_err("Reason: " + std::string(e.what()));
                return false;
            }
        } else {
            return p_load_prompt(default_initial_prompt_filename);
        }
        util::println_info("Default prompt file processing completed.");
        return true;
    }

    bool p_load_prompt(std::string filename) {
        const auto& path_ = std::filesystem::path(f_initial) / filename.append(f_suffix);
        util::println_info("Loading prompt file: " + PATH_S(path_));
        try {
            initial_prompt = file::read_text_file(path_);
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
        const auto& path_ = std::filesystem::path(f_saved) / filename.append(json_suffix);
        if (!exists(path_)) {
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
        initial_prompt = j["initial"].get<std::string>();
        if (!j.contains("histories")) {
            util::println_err("Error reading saved chat file: " + PATH_S(path_));
            util::println_err("Reason: histories is not found.");
            return false;
        }
        auto histories = j["histories"];
        if (!histories.is_null()) {
            if (!histories.is_array()) {
                util::println_err("Error reading saved chat file: " + PATH_S(path_));
                util::println_err("Reason: histories is not an array.");
                return false;
            }
            for (const auto& history : histories) {
                if (!history.is_object()) {
                    util::println_err("Error reading saved chat file: " + PATH_S(path_));
                    util::println_err("Reason: history is not an object.");
                    return false;
                }
                std::string input;
                std::vector<float> input_embeddings;
                std::string response;
                std::vector<float> response_embeddings;
                long long time_ms;
                if (!history.contains("input") || !history["input"].is_string()) {
                    util::println_err("Error reading saved chat file: " + PATH_S(path_));
                    util::println_err("Reason: input is not a string.");
                    return false;
                }
                if (!history.contains("input_embeddings") || !history["input_embeddings"].is_array()) {
                    util::println_err("Error reading saved chat file: " + PATH_S(path_));
                    util::println_err("Reason: input_embeddings is not an array.");
                    return false;
                }
                if (!history.contains("time_stamp") || !history["time_stamp"].is_number_integer()) {
                    util::println_err("Error reading saved chat file: " + PATH_S(path_));
                    util::println_err("Reason: time_stamp is not an integer.");
                    return false;
                }
                input = history["input"].get<std::string>();
                input_embeddings = history["input_embeddings"].get<std::vector<float>>();
                time_ms = history["time_stamp"].get<long long>();
                if (history.contains("response") && history["response"].is_string()) {
                    response = history["response"].get<std::string>();
                    if (history.contains("response_embeddings") && history["response_embeddings"].is_array()) {
                        response_embeddings = history["response_embeddings"].get<std::vector<float>>();
                    }
                }
                if (response.empty()) {
                    prompts.push_back(std::make_shared<chat::Exchange>(input, input_embeddings, time_ms));
                } else if (response_embeddings.empty()) {
                    prompts.push_back(std::make_shared<chat::Exchange>(input, input_embeddings, response, time_ms));
                } else {
                    prompts.push_back(std::make_shared<chat::Exchange>(input, input_embeddings, response, response_embeddings, time_ms));
                }
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
        const auto& path_ = std::filesystem::path(f_saved) / name.append(json_suffix);
        try {
            util::println_info("Saving chat to file: " + PATH_S(path_));
            nlohmann::json j = nlohmann::json::object();
            j["initial"] = initial_prompt;
            nlohmann::json histories = nlohmann::json::array();
            for (const auto& prompt : prompts) {
                nlohmann::json history = nlohmann::json::object();
                history["input"] = prompt->getInput();
                history["input_embeddings"] = prompt->getInputEmbeddings();
                if (prompt->hasResponse()) {
                    history["response"] = prompt->getResponse();
                    if (prompt->hasResponseEmbeddings()) {
                        history["response_embeddings"] = prompt->getResponseEmbeddings();
                    }
                }
                history["time_stamp"] = prompt->getTimeMS();
                histories.push_back(history);
            }
            j["histories"] = histories;
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
     * Load the document Q&A from the file(name.json), and also does error handling.
     * @return True if the file is correctly loaded, false if an error occurred.
     */
    bool p_load_docQA(std::string filename) {
        const auto& path_ = std::filesystem::path(f_documentQA) / filename.append(json_suffix);
        util::println_info("Loading document Q&A from file: " + PATH_S(path_));
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(file::read_text_file(path_));
        } catch (const file::file_error& e) {
            util::println_err("Error reading document Q&A file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        try {
        } catch (const nlohmann::json::parse_error& e) {
            util::println_err("Error parsing document Q&A file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        if (!j.contains("initial") || !j["initial"].is_string()) {
            util::println_err("Error parsing document Q&A file: " + PATH_S(path_));
            util::println_err("Reason: initial is not a string.");
            return false;
        }
        if (!j.contains("documents") || !j["documents"].is_array()) {
            util::println_err("Error parsing document Q&A file: " + PATH_S(path_));
            util::println_err("Reason: documents is not an array.");
            return false;
        }
        try {
            documents = doc::from_json(j["documents"]);
        } catch (const std::exception& e) {
            util::println_err("Error parsing document Q&A file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        initial_prompt = j["initial"].get<std::string>();
        documentQA_mode = true;
        util::println_info("Document Q&A loading completed.");
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
        const auto& path_ = std::filesystem::path(f_documentQA) / (filename + f_suffix);
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
            emb_response = emb::get_embeddings(chunks, api::get_key(), key_status);
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
                i_p_filename = default_initial_prompt_filename;
            } else if (boost::ends_with(i_p_filename, f_suffix)) {
                i_p_filename.erase(i_p_filename.size() - f_suffix.size());
            }
            const auto& path_i = std::filesystem::path(f_initial) / i_p_filename.append(f_suffix);
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
            const auto& path_s = std::filesystem::path(f_documentQA) / (name_to_save + json_suffix);
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
        return p_load_docQA(name_to_save);
    }

    /**
     * Load the config from the file(config.json), and also does error handling.
     * It uses nlohmann::json library.
     * If config.json does not exist or is not in the correct format, it will create a new config.json file.
     * @return True if the config is correctly loaded, false if an error occurred.
     */
    bool p_load_config() {
        const auto& path_ = std::filesystem::path("config.json");
        if (!exists(path_)) {
            util::println_err("Config file does not exist: " + PATH_S(path_));
            util::println_err("Creating new config file: " + PATH_S(path_));
            return p_save_config();
        }
        nlohmann::json j;
        try {
            util::println_info("Reading config file: " + PATH_S(path_));
            j = nlohmann::json::parse(file::read_text_file(path_));
        } catch (const file::file_error& e) {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        } catch (const nlohmann::json::parse_error& e) {
            util::println_err("Error parsing config file: " + PATH_S(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        bool error = false;
        if (j.contains("api_key")) {
            auto api_key = j["api_key"];
            if (!api_key.is_null()) {
                if (api_key.is_string()) {
                    api::set_key(api_key.get<std::string>());
                } else if (api_key.is_array()) {
                    std::vector<std::string> keys;
                    for (const auto& key : api_key) {
                        if (key.is_string()) {
                            keys.push_back(key.get<std::string>());
                        } else {
                            util::println_err("Error reading config file: " + PATH_S(path_));
                            util::println_err("Reason: api_key has non-string element. Element: " + key.dump(2));
                            error = true;
                        }
                    }
                    api::set_key(keys);
                } else {
                    util::println_err("Error reading config file: " + PATH_S(path_));
                    util::println_err("Reason: api_key is not a string, array or null.");
                    error = true;
                }
            }
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: api_key is not found.");
            error = true;
        }
        if (j.contains("model") && j["model"].is_string()) {
            model = j["model"].get<std::string>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: model is not a string.");
            error = true;
        }
        if (j.contains("temperature") && j["temperature"].is_number()) {
            temperature = j["temperature"].get<float>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: temperature is not a number.");
            error = true;
        }
        if (j.contains("max_tokens") && j["max_tokens"].is_number_integer()) {
            max_tokens = j["max_tokens"].get<int>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: max_tokens is not an integer number.");
            error = true;
        }
        if (j.contains("top_p") && j["top_p"].is_number()) {
            top_p = j["top_p"].get<float>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: top_p is not a number.");
            error = true;
        }
        if (j.contains("frequency_penalty") && j["frequency_penalty"].is_number()) {
            frequency_penalty = j["frequency_penalty"].get<float>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: frequency_penalty is not a number.");
            error = true;
        }
        if (j.contains("presence_penalty") && j["presence_penalty"].is_number()) {
            presence_penalty = j["presence_penalty"].get<float>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: presence_penalty is not a number.");
            error = true;
        }
        if (j.contains("logit_bias") && j["logit_bias"].is_object()) {
            auto logit_bias_obj = j["logit_bias"].get<nlohmann::json>();
            logit_bias.clear();
            for (const auto& [key, value] : logit_bias_obj.items()) {
                if (!value.is_number()) {
                    util::println_err("Error reading config file: " + PATH_S(path_));
                    util::println_err("Reason: logit_bias has non-number bias. Value: " + value.dump(2));
                    error = true;
                    continue;
                }
                float bias = value.get<float>();
                if (bias < -100 || bias > 100) {
                    util::println_err("Error reading config file: " + PATH_S(path_));
                    util::println_err("Reason: logit_bias's bias is out of range, it must be between -100 and 100. Bias: "
                    + std::to_string(bias));
                    error = true;
                    continue;
                }
                logit_bias[key] = bias;
            }
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: logit_bias is not an object.");
            error = true;
        }
        if (j.contains("max_display_length") && j["max_display_length"].is_number_unsigned()) {
            max_display_length = j["max_display_length"].get<unsigned int>();
            if (max_display_length == 0) {
                util::println_err("Error reading config file: " + PATH_S(path_));
                util::println_err("Reason: max_display_length cannot be 0.");
                max_display_length = 1;
                error = true;
            }
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: max_display_length is not an unsigned integer.");
            error = true;
        }
        if (j.contains("max_short_memory_length") && j["max_short_memory_length"].is_number_unsigned()) {
            max_short_memory_length = j["max_short_memory_length"].get<unsigned int>();
            if (max_short_memory_length == 0) {
                util::println_err("Error reading config file: " + PATH_S(path_));
                util::println_err("Reason: max_short_memory_length cannot be 0.");
                max_short_memory_length = 1;
                error = true;
            }
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: max_short_memory_length is not an unsigned integer.");
            error = true;
        }
        if (j.contains("max_reference_length") && j["max_reference_length"].is_number_unsigned()) {
            max_reference_length = j["max_reference_length"].get<unsigned int>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: max_reference_length is not an unsigned integer.");
            error = true;
        }
        if (j.contains("search_response") && j["search_response"].is_boolean()) {
            search_response = j["search_response"].get<bool>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: search_response is not a boolean.");
            error = true;
        }
        if (j.contains("space_between_exchanges") && j["space_between_exchanges"].is_boolean()) {
            space_between_exchanges = j["space_between_exchanges"].get<bool>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: space_between_exchanges is not a boolean.");
            error = true;
        }
        if (j.contains("debug_reference") && j["debug_reference"].is_boolean()) {
            debug_reference = j["debug_reference"].get<bool>();
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: debug_reference is not a boolean.");
            error = true;
        }
#ifdef __linux__
        if (j.contains("ca_bundle_path") && j["ca_bundle_path"].is_string()) {
            util::set_ca_bundle_path(j["ca_bundle_path"].get<std::string>());
        } else {
            util::println_err("Error reading config file: " + PATH_S(path_));
            util::println_err("Reason: ca_bundle_path is not a string.");
            error = true;
        }
#endif
        if (error) {
            util::println_err("Error detected, creating new config file: " + PATH_S(path_));
            return p_save_config();
        }
        util::println_info("Config loaded from file: " + PATH_S(path_));
        return true;
    }

    /**
     * Save the config to the file(config.json), and also does error handling.
     * It uses nlohmann::json library.
     * @return True if the config is correctly saved, false if an error occurred.
     */
    bool p_save_config() {
        const auto& path_ = std::filesystem::path("config.json");
        util::println_info("Saving config to file: " + PATH_S(path_));
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
        try {
            file::write_text_file(j.dump(2), path_);
        } catch (const file::file_error& e) {
            util::println_err("Error saving config file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        util::println_info("Config saved to file: " + PATH_S(path_));
        return true;
    }

    /**
     * Check if the api key is set, if not, ask the user to set it.
     * @return True if the api key is present or set, false if an error occurred.
     */
    bool p_check_set_api_key() {
        if (!api::has_key()) {
            api::set_key(api::get_key_from_console());
            if (!p_save_config()) {
                return false;
            }
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
