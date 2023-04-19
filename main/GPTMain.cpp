//
// Created by v2ray on 2023/2/18.
//

#include "GPTMain.h"

#define PATH(path) Term::color_fg(125, 225, 255) + "\"" + path.string() + "\""

namespace GPT {
    static std::vector<std::string> input_history;
    const std::string f_initial = "initial";
    const std::string f_saved = "saved";
    const std::string f_suffix = ".txt";
    const std::string json_suffix = ".json";
    std::string model = "gpt-3.5-turbo";
    bool is_new_api = false;
    float temperature = 1;
    int max_tokens = 500;
    float top_p = 1;
    float frequency_penalty = 0;
    float presence_penalty = 0.6;
    std::unordered_map<std::string, float> logit_bias;
    const std::string me_id = "Me";
    const std::string bot_id = "You";
    std::string initial_prompt = (boost::format(
            "The following conversation is set to:\n"
            "%1%: is the prefix of the user, texts start with it are the user input\n"
            "%2%: is the prefix of your response, texts start with it are your response\n"
            "You are an AI chat bot named Sapphire\n"
            "You are friendly and intelligent\n") % me_id % bot_id).str();
    std::vector<std::shared_ptr<chat::Exchange>> prompts;
    unsigned int max_display_length = 100;
    unsigned int max_short_memory_length = 4;
    unsigned int max_reference_length = 4;
    bool space_between_exchanges = false;
    bool debug_reference = false;

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
        if (!create_folders(std::vector<std::string>{f_initial, f_saved}) || !p_load_config() || !p_default_prompt()) {
            return;
        }
        is_new_api = api::is_new_api(model);
        p_check_set_api_key();
        while (true) {
            util::print_cs("Please choose whether you want to load the initial prompt or saved chat history.\n"
                           "(Input " + Term::color_fg(255, 200, 0) + "i" + Term::color_fg(Color::Name::Default) + " for initial, "
                           + Term::color_fg(255, 200, 0) + "s" + Term::color_fg(Color::Name::Default) + " for saved, press "
                           + ENTER + " to use initial): ");
            std::string lInitial_or_lSaved;
            getline(std::cin, lInitial_or_lSaved);
            transform(lInitial_or_lSaved.begin(), lInitial_or_lSaved.end(), lInitial_or_lSaved.begin(), tolower);
            if (lInitial_or_lSaved.empty() || lInitial_or_lSaved == "i") {
                util::print_cs("Please enter the initial prompt's filename you want to load.\n"
                               "(Press " + ENTER + " to use default): ");
                std::string i_p_filename;
                getline(std::cin, i_p_filename);
                if (boost::ends_with(i_p_filename, f_suffix)) {
                    i_p_filename.erase(i_p_filename.size() - f_suffix.size());
                }
                if (!i_p_filename.empty() && !p_load_prompt(i_p_filename)) {
                    return;
                }
                break;
            } else if (lInitial_or_lSaved == "s") {
                std::cout << "Please enter the saved chat history's filename you want to load: ";
                std::string s_p_filename;
                getline(std::cin, s_p_filename);
                if (boost::ends_with(s_p_filename, json_suffix)) {
                    s_p_filename.erase(s_p_filename.size() - json_suffix.size());
                }
                if (!p_load_saved(s_p_filename)) {
                    return;
                }
                break;
            } else {
                util::println_warn("Invalid input, please try again.");
            }
        }
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
                input = util::get_multiline(input_history, me_id + ": ");
            } catch (const std::exception& e) {
                util::println_err("\nAn error occurred while getting input: " + std::string(e.what()));
                print_enter_next_cycle();
                continue;
            }
            cmd::ReturnOpCode rc = cmd::handle_command(input, prompts);
            if (rc == cmd::ReturnOpCode::CONTINUE) {
                continue;
            } else if (rc == cmd::ReturnOpCode::STOP) {
                break;
            }
            std::string api_key = api::get_key();
            util::println_info("Getting embeddings and finding similar chat exchanges for the input...", false);
            std::pair<std::shared_ptr<std::vector<float>>, api::APIKeyStatus> emb_response;
            try {
                emb_response = emb::get_embeddings(input, api_key);
            } catch (const std::exception& e) {
                util::println_err("\nError when getting embeddings: " + std::string(e.what()));
                print_enter_next_cycle();
                continue;
            }
            auto key_status_emb = emb_response.second;
            if (key_status_emb == api::APIKeyStatus::INVALID_KEY || key_status_emb == api::APIKeyStatus::QUOTA_EXCEEDED) {
                p_on_invalid_key();
                print_enter_next_cycle();
                continue;
            }
            if (auto input_embeddings = emb_response.first) {
                prompts.emplace_back(make_shared<chat::Exchange>(input, *input_embeddings, util::currentTimeMillis()));
                print_prompt();
                std::string response;
                try {
                    bool api_success = api::call_api(initial_prompt, prompts, api_key, model, temperature, max_tokens, top_p,
                                                     frequency_penalty, presence_penalty, logit_bias, max_short_memory_length,
                                                     max_reference_length, me_id, bot_id, [&response](const auto& streamed_response){
                        try {
                            nlohmann::json j = nlohmann::json::parse(streamed_response);
                            if (j.count("error") > 0 && j["error"].is_object()) {
                                response = j.dump();
                                return;
                            }
                        } catch (const nlohmann::json::parse_error& e) {}
                        response.append(streamed_response);
                        util::print_cs(streamed_response, false, false);
                        }, debug_reference);
                    util::print_cs(""); //Reset color.
                    if (!api_success) {
                        print_enter_next_cycle();
                        prompts.pop_back();
                        continue;
                    }
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
                prompts.back()->setResponse(response);
            } else {
                print_enter_next_cycle();
            }
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
        for (const auto& folder : folders) {
            try {
                if (std::filesystem::create_directory(folder)) {
                    util::println_info("Created folder: " + folder);
                }
            } catch (const std::exception& e) {
                util::println_err("Error creating folder: " + folder);
                util::println_err("Reason: " + std::string(e.what()));
                return false;
            }
        }
        util::println_info("Creating folders completed.");
        return true;
    }

    /**
     * @return True if the file was created or already exists and read, false if an error occurred.
     */
    bool p_default_prompt() {
        std::string default_filename = "Default";
        const auto path_ = std::filesystem::path(f_initial) / (default_filename + f_suffix);
        if (!exists(path_)) {
            try {
                util::println_info("Creating default prompt file: " + PATH(path_));
                std::ofstream file(path_);
                if (file.is_open()) {
                    file << initial_prompt;
                    file.close();
                } else {
                    util::println_err("Error opening file for writing: " + PATH(path_));
                    return false;
                }
            } catch (const std::exception& e) {
                util::println_err("Error creating file: " + PATH(path_));
                util::println_err("Reason: " + std::string(e.what()));
                return false;
            }
        } else {
            return p_load_prompt(default_filename);
        }
        util::println_info("Default prompt file processing completed.");
        return true;
    }

    bool p_load_prompt(std::string filename) {
        const auto path_ = std::filesystem::path(f_initial) / filename.append(f_suffix);
        if (!exists(path_)) {
            util::println_err("Prompt file does not exist: " + PATH(path_));
            return false;
        }
        try {
            util::println_info("Reading prompt file: " + PATH(path_));
            std::ifstream file(path_);
            if (file.is_open()) {
                initial_prompt = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
                file.close();
            } else {
                util::println_err("Error opening file for reading: " + PATH(path_));
                return false;
            }
        } catch (const std::exception& e) {
            util::println_err("Error reading file: " + PATH(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        util::println_info("Prompt file processing completed.");
        return true;
    }

    /**
     * Load the saved chat history from the file(.json), and also does error handling.
     * It uses nlohmann::json library.
     * If the json is not in the correct format, it will return false.
     * @return True if the file already exists and read, false if an error occurred.
     */
    bool p_load_saved(std::string filename) {
        const auto path_ = std::filesystem::path(f_saved) / filename.append(json_suffix);
        if (!exists(path_)) {
            util::println_err("Saved chat file does not exist: " + PATH(path_));
            return false;
        }
        try {
            util::println_info("Reading saved chat file: " + PATH(path_));
            std::ifstream file(path_);
            if (file.is_open()) {
                nlohmann::json j;
                file >> j;
                file.close();
                if (j.count("initial") > 0 && j["initial"].is_string()) {
                    initial_prompt = j["initial"].get<std::string>();
                } else {
                    util::println_err("Error reading saved chat file: " + PATH(path_));
                    util::println_err("Reason: initial is not a string.");
                    return false;
                }
                if (j.count("histories") > 0) {
                    auto histories = j["histories"];
                    if (histories.is_array()) {
                        for (const auto& history : histories) {
                            if (history.is_object()) {
                                std::string input;
                                std::vector<float> input_embeddings;
                                std::string response;
                                long long time_ms;
                                if (history.count("input") > 0 && history["input"].is_string()) {
                                    input = history["input"].get<std::string>();
                                } else {
                                    util::println_err("Error reading saved chat file: " + PATH(path_));
                                    util::println_err("Reason: input is not a string.");
                                    return false;
                                }
                                if (history.count("input_embeddings") > 0 && history["input_embeddings"].is_array()) {
                                    input_embeddings = history["input_embeddings"].get<std::vector<float>>();
                                } else {
                                    util::println_err("Error reading saved chat file: " + PATH(path_));
                                    util::println_err("Reason: input_embeddings is not an array.");
                                    return false;
                                }
                                if (history.count("response") > 0 && history["response"].is_string()) {
                                    response = history["response"].get<std::string>();
                                }
                                if (history.count("time_stamp") > 0 && history["time_stamp"].is_number_integer()) {
                                    time_ms = history["time_stamp"].get<long long>();
                                } else {
                                    util::println_err("Error reading saved chat file: " + PATH(path_));
                                    util::println_err("Reason: time_stamp is not an integer.");
                                    return false;
                                }
                                if (response.empty()) {
                                    prompts.push_back(std::make_shared<chat::Exchange>(input, input_embeddings, time_ms));
                                } else {
                                    prompts.push_back(std::make_shared<chat::Exchange>(input, input_embeddings, response, time_ms));
                                }
                            } else {
                                util::println_err("Error reading saved chat file: " + PATH(path_));
                                util::println_err("Reason: history is not an object.");
                                return false;
                            }
                        }
                    } else if (histories.is_null()) {} else {
                        util::println_err("Error reading saved chat file: " + PATH(path_));
                        util::println_err("Reason: histories is not an array.");
                        return false;
                    }
                } else {
                    util::println_err("Error reading saved chat file: " + PATH(path_));
                    util::println_err("Reason: histories is not found.");
                    return false;
                }
            } else {
                util::println_err("Error opening saved chat file for reading: " + PATH(path_));
                return false;
            }
        } catch (const std::exception& e) {
            util::println_err("Error reading saved chat file: " + PATH(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
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
        const auto path_ = std::filesystem::path(f_saved) / name.append(json_suffix);
        try {
            util::println_info("Saving chat to file: " + PATH(path_));
            std::ofstream file(path_);
            if (file.is_open()) {
                nlohmann::json j;
                j["initial"] = initial_prompt;
                nlohmann::json histories = nlohmann::json::array();
                for (const auto& prompt : prompts) {
                    nlohmann::json history = nlohmann::json::object();
                    history["input"] = prompt->getInput();
                    history["input_embeddings"] = prompt->getInputEmbeddings();
                    if (prompt->hasResponse()) {
                        history["response"] = prompt->getResponse();
                    }
                    history["time_stamp"] = prompt->getTimeMS();
                    histories.push_back(history);
                }
                j["histories"] = histories;
                file << j.dump(2);
                file.close();
            } else {
                util::println_err("Error opening file for writing: " + PATH(path_));
                return false;
            }
        } catch (const std::exception& e) {
            util::println_err("Error creating file: " + PATH(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        util::println_info("Chat saved to file: " + PATH(path_));
        return true;
    }

    /**
     * Load the config from the file(config.json), and also does error handling.
     * It uses nlohmann::json library.
     * If config.json does not exist or is not in the correct format, it will create a new config.json file.
     * @return True if the config is correctly loaded, false if an error occurred.
     */
    bool p_load_config() {
        const auto path_ = std::filesystem::path("config.json");
        if (!exists(path_)) {
            util::println_err("Config file does not exist: " + PATH(path_));
            util::println_err("Creating new config file: " + PATH(path_));
            return p_save_config();
        }
        try {
            util::println_info("Reading config file: " + PATH(path_));
            std::ifstream file(path_);
            if (file.is_open()) {
                nlohmann::json j;
                file >> j;
                file.close();
                bool error = false;
                if (j.count("api_key") > 0) {
                    auto api_key = j["api_key"];
                    if (api_key.is_string()) {
                        api::set_key(api_key.get<std::string>());
                    } else if (api_key.is_array()) {
                        std::vector<std::string> keys;
                        for (const auto& key : api_key) {
                            if (key.is_string()) {
                                keys.push_back(key.get<std::string>());
                            } else {
                                util::println_err("Error reading config file: " + PATH(path_));
                                util::println_err("Reason: api_key has non-string element. Element: " + key.dump(2));
                                error = true;
                            }
                        }
                        api::set_key(keys);
                    } else if (!api_key.is_null()) {
                        util::println_err("Error reading config file: " + PATH(path_));
                        util::println_err("Reason: api_key is not a string, array or null.");
                        error = true;
                    }
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: api_key is not found.");
                    error = true;
                }
                if (j.count("model") > 0 && j["model"].is_string()) {
                    model = j["model"].get<std::string>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: model is not a string.");
                    error = true;
                }
                if (j.count("temperature") > 0 && j["temperature"].is_number()) {
                    temperature = j["temperature"].get<float>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: temperature is not a number.");
                    error = true;
                }
                if (j.count("max_tokens") > 0 && j["max_tokens"].is_number_integer()) {
                    max_tokens = j["max_tokens"].get<int>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: max_tokens is not an integer number.");
                    error = true;
                }
                if (j.count("top_p") > 0 && j["top_p"].is_number()) {
                    top_p = j["top_p"].get<float>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: top_p is not a number.");
                    error = true;
                }
                if (j.count("frequency_penalty") > 0 && j["frequency_penalty"].is_number()) {
                    frequency_penalty = j["frequency_penalty"].get<float>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: frequency_penalty is not a number.");
                    error = true;
                }
                if (j.count("presence_penalty") > 0 && j["presence_penalty"].is_number()) {
                    presence_penalty = j["presence_penalty"].get<float>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: presence_penalty is not a number.");
                    error = true;
                }
                if (j.count("logit_bias") > 0 && j["logit_bias"].is_object()) {
                    auto logit_bias_obj = j["logit_bias"].get<nlohmann::json>();
                    logit_bias.clear();
                    for (const auto& [key, value] : logit_bias_obj.items()) {
                        if (!value.is_number()) {
                            util::println_err("Error reading config file: " + PATH(path_));
                            util::println_err("Reason: logit_bias has non-number bias. Value: " + value.dump(2));
                            error = true;
                            continue;
                        }
                        float bias = value.get<float>();
                        if (bias < -100 || bias > 100) {
                            util::println_err("Error reading config file: " + PATH(path_));
                            util::println_err("Reason: logit_bias's bias is out of range, it must be between -100 and 100. Bias: "
                            + std::to_string(bias));
                            error = true;
                            continue;
                        }
                        logit_bias[key] = bias;
                    }
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: logit_bias is not an object.");
                    error = true;
                }
                if (j.count("max_display_length") > 0 && j["max_display_length"].is_number_unsigned()) {
                    max_display_length = j["max_display_length"].get<unsigned int>();
                    if (max_display_length == 0) {
                        util::println_err("Error reading config file: " + PATH(path_));
                        util::println_err("Reason: max_display_length cannot be 0.");
                        max_display_length = 1;
                        error = true;
                    }
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: max_display_length is not an unsigned integer.");
                    error = true;
                }
                if (j.count("max_short_memory_length") > 0 && j["max_short_memory_length"].is_number_unsigned()) {
                    max_short_memory_length = j["max_short_memory_length"].get<unsigned int>();
                    if (max_short_memory_length == 0) {
                        util::println_err("Error reading config file: " + PATH(path_));
                        util::println_err("Reason: max_short_memory_length cannot be 0.");
                        max_short_memory_length = 1;
                        error = true;
                    }
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: max_short_memory_length is not an unsigned integer.");
                    error = true;
                }
                if (j.count("max_reference_length") > 0 && j["max_reference_length"].is_number_unsigned()) {
                    max_reference_length = j["max_reference_length"].get<unsigned int>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: max_reference_length is not an unsigned integer.");
                    error = true;
                }
                if (j.count("space_between_exchanges") > 0 && j["space_between_exchanges"].is_boolean()) {
                    space_between_exchanges = j["space_between_exchanges"].get<bool>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: space_between_exchanges is not a boolean.");
                    error = true;
                }
                if (j.count("debug_reference") > 0 && j["debug_reference"].is_boolean()) {
                    debug_reference = j["debug_reference"].get<bool>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: debug_reference is not a boolean.");
                    error = true;
                }
#ifdef __linux__
                if (j.count("ca_bundle_path") > 0 && j["ca_bundle_path"].is_string()) {
                    util::set_ca_bundle_path(j["ca_bundle_path"].get<std::string>());
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: ca_bundle_path is not a string.");
                    error = true;
                }
#endif
                if (error) {
                    util::println_err("Error detected, creating new config file: " + PATH(path_));
                    return p_save_config();
                }
            }
        } catch (const std::exception& e) {
            util::println_err("Error reading config file: " + PATH(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        util::println_info("Config loaded from file: " + PATH(path_));
        return true;
    }

    /**
     * Save the config to the file(config.json), and also does error handling.
     * It uses nlohmann::json library.
     * @return True if the config is correctly saved, false if an error occurred.
     */
    bool p_save_config() {
        const auto path_ = std::filesystem::path("config.json");
        try {
            util::println_info("Saving config to file: " + PATH(path_));
            std::ofstream file(path_);
            if (file.is_open()) {
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
                j["space_between_exchanges"] = space_between_exchanges;
                j["debug_reference"] = debug_reference;
#ifdef __linux__
                j["ca_bundle_path"] = util::get_ca_bundle_path();
#endif
                file << j.dump(2);
                file.close();
            } else {
                util::println_err("Error opening file for writing: " + PATH(path_));
                return false;
            }
        } catch (const std::exception& e) {
            util::println_err("Error creating file: " + PATH(path_));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        util::println_info("Config saved to file: " + PATH(path_));
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
