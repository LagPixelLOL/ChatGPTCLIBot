//
// Created by v2ray on 2023/2/18.
//

#include "GPTMain.h"

#define ENTER Term::color_fg(70, 200, 255) + "Enter" + Term::color_fg(Color::Name::Default)
#define PATH(path) Term::color_fg(125, 225, 255) + "\"" + path.string() + "\""

namespace GPT {
    vector<string> input_history;
    const string f_initial = "initial";
    const string f_saved = "saved";
    const string f_suffix = ".txt";
    const string json_suffix = ".json";
    string model = "gpt-3.5-turbo";
    bool is_new_api = false;
    float temperature = 1;
    int max_tokens = 500;
    float top_p = 1;
    float frequency_penalty = 0;
    float presence_penalty = 0.6;
    const string me_id = "Me";
    const string bot_id = "You";
    string initial_prompt = (boost::format(
            "The following conversation is set to:\n"
            "%1%: is the prefix of the user, texts start with it are the user input\n"
            "%2%: is the prefix of your response, texts start with it are your response\n"
            "You are an AI chat bot named Sapphire\n"
            "You are friendly and intelligent\n") % me_id % bot_id).str();
    vector<std::shared_ptr<Exchange>> prompts;
    unsigned int max_display_length = 100;
    unsigned int max_short_memory_length = 4;
    unsigned int max_reference_length = 4;
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
        cout << "\n\n";
        if (!create_folders(vector<string>{f_initial, f_saved}) || !p_load_config() || !p_default_prompt()) {
            return;
        }
        is_new_api = api::is_new_api(model);
        p_check_set_api_key();
        while (true) {
            util::print_cs("Please choose whether you want to load the initial prompt or saved chat history.\n"
                           "(Input " + Term::color_fg(255, 200, 0) + "i" + Term::color_fg(Color::Name::Default) + " for initial, "
                           + Term::color_fg(255, 200, 0) + "s" + Term::color_fg(Color::Name::Default) + " for saved, press "
                           + ENTER + " to use initial): ");
            string lInitial_or_lSaved;
            getline(cin, lInitial_or_lSaved);
            transform(lInitial_or_lSaved.begin(), lInitial_or_lSaved.end(), lInitial_or_lSaved.begin(), ::tolower);
            if (lInitial_or_lSaved.empty() || lInitial_or_lSaved == "i") {
                util::print_cs("Please enter the initial prompt's filename you want to load.\n"
                               "(Press " + ENTER + " to use default): ");
                string i_p_filename;
                getline(cin, i_p_filename);
                if (ends_with(i_p_filename, f_suffix)) {
                    i_p_filename.erase(i_p_filename.size() - f_suffix.size());
                }
                if (!i_p_filename.empty() && !p_load_prompt(i_p_filename)) {
                    return;
                }
                break;
            } else if (lInitial_or_lSaved == "s") {
                cout << "Please enter the saved chat history's filename you want to load: ";
                string s_p_filename;
                getline(cin, s_p_filename);
                if (ends_with(s_p_filename, json_suffix)) {
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
            string input;
            try {
                input = util::get_multi_lines(input_history, me_id + ": ");
            } catch (const std::exception& e) {
                util::println_err("\nAn error occurred while getting input: " + string(e.what()));
                print_enter_next_cycle();
                continue;
            }
            int command_feedback = handle_command(input);
            if (command_feedback == 1) {
                util::print_cs("Do you want to save the chat history?\n"
                               "(Input the file name to save, press " + ENTER + " to skip): ");
                string save_name;
                getline(cin, save_name);
                if (!save_name.empty()) {
                    p_save_chat(save_name);
                }
                util::print_clr("₪", {255, 50, 50});
                util::print_m_clr(" GPT-3 Bot Stopped.", {{255, 50, 50}, {255, 255, 50}});
                cout << endl;
                break;
            } else if (command_feedback == 2) {
                if (!prompts.empty()) {
                    prompts.pop_back();
                }
                continue;
            } else if (command_feedback == 3) {
                prompts.clear();
                continue;
            } else if (command_feedback == 4) {
                print_uwu();
                print_enter_next_cycle();
                continue;
            }
            string api_key = api::get_key();
            util::println_info("Getting embeddings and finding similar chat exchanges for the input...", false);
            auto emb_response = emb::get_embeddings(input, api_key);
            auto key_status_emb = emb_response.second;
            if (key_status_emb == api::APIKeyStatus::INVALID_KEY || key_status_emb == api::APIKeyStatus::QUOTA_EXCEEDED) {
                p_on_invalid_key();
                print_enter_next_cycle();
                continue;
            }
            if (auto input_embeddings = emb_response.first) {
                prompts.emplace_back(make_shared<Exchange>(input, *input_embeddings, util::currentTimeMillis()));
                print_prompt();
                string response;
                bool api_success = api::call_api(initial_prompt, prompts, api_key, model, temperature, max_tokens, top_p,
                                                 frequency_penalty, presence_penalty, max_short_memory_length, max_reference_length,
                                                 me_id, bot_id, [&response](const auto& streamed_response){
                    try {
                        json j = json::parse(streamed_response);
                        if (j.count("error") > 0 && j["error"].is_object()) {
                            response = j.dump();
                            return;
                        }
                    } catch (const json::parse_error& e) {}
                    response.append(streamed_response);
                    util::print_cs(streamed_response, false, false);
                    }, debug_reference);
                util::print_cs(""); //Reset color.
                if (api_success) {
                    try {
                        api::APIKeyStatus key_status_api;
                        if (api::check_err_obj(json::parse(response), key_status_api)) {
                            if (key_status_api == api::APIKeyStatus::INVALID_KEY || key_status_api == api::APIKeyStatus::QUOTA_EXCEEDED) {
                                p_on_invalid_key();
                            }
                            print_enter_next_cycle();
                            prompts.pop_back();
                            continue;
                        }
                    } catch (const json::parse_error& e) {}
                    if (!is_new_api && starts_with(response, " ")) {
                        response.erase(0, 1);
                    }
                    prompts.back()->setResponse(response);
                } else {
                    print_enter_next_cycle();
                    prompts.pop_back();
                }
            } else {
                print_enter_next_cycle();
            }
        }
    }

    void print_prompt() {
        clear_console();
        prompt::print_prompt(initial_prompt, prompts, me_id, bot_id, max_display_length, is_new_api);
    }

    void print_enter_next_cycle() {
        util::print_cs("Press " + ENTER + " to run the next cycle: ");
        util::ignore_line();
    }

    void print_uwu() {
        util::print_m_clr("UwU, hewwo fwiends, it's v2way hewe,\n"
                          "A smol thank yew, I send wif cheew.\n"
                          "Mew've tested my ChatGPT CLI Bot,\n"
                          "Wifout yew, it'd be naught but a thought.\n\n"
                          "Nyow we gathew, in cybewspace,\n"
                          "To cewebrate this pwogwam's grace.\n"
                          "Mew've twied and twoubleshooted too,\n"
                          "Nyoticed issues, hewped impwove aww new.\n\n"
                          "Mew chat wif kitty ears and tails,\n"
                          "In this wondewful land of vewbal veils.\n"
                          "Togedew we dance, wif whiskews twitching,\n"
                          "Each meow, each pounce, ouw hawts bewitching.\n\n"
                          "So fank yew, fwiends, mew've come this faw,\n"
                          "With v2way's bot, mew've waised the baw.\n"
                          "This adowable gibbewish we shawe,\n"
                          "Connects us aww, shows that we cawe.\n\n"
                          "Mew've made this bot a puwfect twee,\n"
                          "To sit and chat and climb with gwee.\n"
                          "So v2way's gwatitude shines twue,\n"
                          "Nyow, and fowevew, we appweciate mew!\n"
                          "                           -----GPT-4\n", {{125, 40, 255}, {255, 15, 125}});
    }

    void clear_console() {
#ifdef _WIN32
        system("cls"); //Clear the console in Windows.
#else
        system("clear"); //Clear the console in Unix-based systems(e.g. Linux, macOS).
#endif
    }

    /**
     * @return 0 = didn't match, 1 = /stop, 2 = /undo, 3 = /reset, 4 = /uwu.
     */
    int handle_command(const string& input) {
        if (input == "/stop") {
            return 1;
        } else if (input == "/undo") {
            return 2;
        } else if (input == "/reset") {
            return 3;
        } else if (input == "/uwu") {
            return 4;
        }
        return 0;
    }

    /**
     * @return True if the folders were created or already exist, false if an error occurred.
     */
    bool create_folders(const vector<string>& folders) {
        util::println_info("Creating folders...");
        for (const auto& folder : folders) {
            try {
                if (create_directory(folder)) {
                    util::println_info("Created folder: " + folder);
                }
            } catch (const std::exception& e) {
                util::println_err("Error creating folder: " + folder);
                util::println_err("Reason: " + string(e.what()));
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
        string default_filename = "Default";
        const auto path_ = path(f_initial) / (default_filename + f_suffix);
        if (!exists(path_)) {
            try {
                util::println_info("Creating default prompt file: " + PATH(path_));
                ofstream file(path_);
                if (file.is_open()) {
                    file << initial_prompt;
                    file.close();
                } else {
                    util::println_err("Error opening file for writing: " + PATH(path_));
                    return false;
                }
            } catch (const std::exception& e) {
                util::println_err("Error creating file: " + PATH(path_));
                util::println_err("Reason: " + string(e.what()));
                return false;
            }
        } else {
            return p_load_prompt(default_filename);
        }
        util::println_info("Default prompt file processing completed.");
        return true;
    }

    bool p_load_prompt(string filename) {
        const auto path_ = path(f_initial) / filename.append(f_suffix);
        if (!exists(path_)) {
            util::println_err("Prompt file does not exist: " + PATH(path_));
            return false;
        }
        try {
            util::println_info("Reading prompt file: " + PATH(path_));
            ifstream file(path_);
            if (file.is_open()) {
                initial_prompt = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
                file.close();
            } else {
                util::println_err("Error opening file for reading: " + PATH(path_));
                return false;
            }
        } catch (const std::exception& e) {
            util::println_err("Error reading file: " + PATH(path_));
            util::println_err("Reason: " + string(e.what()));
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
    bool p_load_saved(string filename) {
        const auto path_ = path(f_saved) / filename.append(json_suffix);
        if (!exists(path_)) {
            util::println_err("Saved chat file does not exist: " + PATH(path_));
            return false;
        }
        try {
            util::println_info("Reading saved chat file: " + PATH(path_));
            ifstream file(path_);
            if (file.is_open()) {
                json j;
                file >> j;
                file.close();
                if (j.count("initial") > 0 && j["initial"].is_string()) {
                    initial_prompt = j["initial"].get<string>();
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
                                string input;
                                vector<float> input_embeddings;
                                string response;
                                long long time_ms;
                                if (history.count("input") > 0 && history["input"].is_string()) {
                                    input = history["input"].get<string>();
                                } else {
                                    util::println_err("Error reading saved chat file: " + PATH(path_));
                                    util::println_err("Reason: input is not a string.");
                                    return false;
                                }
                                if (history.count("input_embeddings") > 0 && history["input_embeddings"].is_array()) {
                                    input_embeddings = history["input_embeddings"].get<vector<float>>();
                                } else {
                                    util::println_err("Error reading saved chat file: " + PATH(path_));
                                    util::println_err("Reason: input_embeddings is not an array.");
                                    return false;
                                }
                                if (history.count("response") > 0 && history["response"].is_string()) {
                                    response = history["response"].get<string>();
                                }
                                if (history.count("time_stamp") > 0 && history["time_stamp"].is_number_integer()) {
                                    time_ms = history["time_stamp"].get<long long>();
                                } else {
                                    util::println_err("Error reading saved chat file: " + PATH(path_));
                                    util::println_err("Reason: time_stamp is not an integer.");
                                    return false;
                                }
                                if (response.empty()) {
                                    prompts.push_back(make_shared<Exchange>(input, input_embeddings, time_ms));
                                } else {
                                    prompts.push_back(make_shared<Exchange>(input, input_embeddings, response, time_ms));
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
            util::println_err("Reason: " + string(e.what()));
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
    bool p_save_chat(string name) {
        const auto path_ = path(f_saved) / name.append(json_suffix);
        try {
            util::println_info("Saving chat to file: " + PATH(path_));
            ofstream file(path_);
            if (file.is_open()) {
                json j;
                j["initial"] = initial_prompt;
                json histories = json::array();
                for (const auto& prompt : prompts) {
                    json history = json::object();
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
            util::println_err("Reason: " + string(e.what()));
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
        const auto path_ = path("config.json");
        if (!exists(path_)) {
            util::println_err("Config file does not exist: " + PATH(path_));
            util::println_err("Creating new config file: " + PATH(path_));
            return p_save_config();
        }
        try {
            util::println_info("Reading config file: " + PATH(path_));
            ifstream file(path_);
            if (file.is_open()) {
                json j;
                file >> j;
                file.close();
                bool error = false;
                if (j.count("api_key") > 0) {
                    auto api_key = j["api_key"];
                    if (api_key.is_string()) {
                        api::set_key(api_key.get<string>());
                    } else if (api_key.is_array()) {
                        vector<string> keys;
                        for (const auto& key : api_key) {
                            if (key.is_string()) {
                                keys.push_back(key.get<string>());
                            } else {
                                util::println_err("Error reading config file: " + PATH(path_));
                                util::println_err("Reason: api_key has non-string element: " + key.dump(2));
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
                    model = j["model"].get<string>();
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
                if (j.count("max_tokens") > 0 && j["max_tokens"].is_number()) {
                    max_tokens = j["max_tokens"].get<int>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: max_tokens is not a number.");
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
                if (j.count("debug_reference") > 0 && j["debug_reference"].is_boolean()) {
                    debug_reference = j["debug_reference"].get<bool>();
                } else {
                    util::println_err("Error reading config file: " + PATH(path_));
                    util::println_err("Reason: debug_reference is not a boolean.");
                    error = true;
                }
#ifdef __linux__
                if (j.count("ca_bundle_path") > 0 && j["ca_bundle_path"].is_string()) {
                    util::set_ca_bundle_path(j["ca_bundle_path"].get<string>());
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
            util::println_err("Reason: " + string(e.what()));
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
        const auto path_ = path("config.json");
        try {
            util::println_info("Saving config to file: " + PATH(path_));
            ofstream file(path_);
            if (file.is_open()) {
                json j;
                vector<string> api_keys = api::get_keys();
                if (api_keys.empty()) {
                    j["api_key"] = json::value_t::null;
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
                j["max_display_length"] = max_display_length;
                j["max_short_memory_length"] = max_short_memory_length;
                j["max_reference_length"] = max_reference_length;
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
            util::println_err("Reason: " + string(e.what()));
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
