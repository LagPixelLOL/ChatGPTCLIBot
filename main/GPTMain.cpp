//
// Created by v2ray on 2023/2/18.
//

#include "GPTMain.h"

namespace GPT {
    const string f_initial = "initial";
    const string f_saved = "saved";
    const string f_suffix = ".txt";
    const string json_suffix = ".json";
    string api_key;
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
        cout << "   ********    *******    **********            ****        ******                  **  \n"
                "  **//////**  /**////**  /////**///            */// *      /*////**                /**  \n"
                " **      //   /**   /**      /**              /    /*      /*   /**     ******    ******\n"
                "/**           /*******       /**       *****     ***       /******     **////**  ///**/ \n"
                "/**    *****  /**////        /**      /////     /// *      /*//// **  /**   /**    /**  \n"
                "//**  ////**  /**            /**               *   /*      /*    /**  /**   /**    /**  \n"
                " //********   /**            /**              / ****       /*******   //******     //** \n"
                "  ////////    //             //                ////        ///////     //////       //  \n\n";
        if (!create_folders(vector<string>{f_initial, f_saved}) || !p_load_config() || !p_default_prompt()) {
            return;
        }
        is_new_api = starts_with(model, "gpt-3.5");
        if (api_key.empty()) {
            while (true) {
                cout << "Please enter your OpenAI API key: ";
                getline(cin, api_key);
                if (api_key.empty()) {
                    cout << "API key cannot be empty, please try again.\n";
                    continue;
                }
                break;
            }
            if (!p_save_config()) {
                return;
            }
        }
        while (true) {
            cout << "Please choose whether you want to load the initial prompt or saved chat history.\n"
                    "(Input i for initial, s for saved, press Enter to use initial): ";
            string lInitial_or_lSaved;
            getline(cin, lInitial_or_lSaved);
            transform(lInitial_or_lSaved.begin(), lInitial_or_lSaved.end(), lInitial_or_lSaved.begin(), ::tolower);
            if (lInitial_or_lSaved.empty() || lInitial_or_lSaved == "i") {
                cout << "Please enter the initial prompt's filename you want to load.\n(Press Enter to use default): ";
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
                cout << "Invalid input, please try again." << endl;
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
            getline(cin, input);
            int command_feedback = handle_command(input);
            if (command_feedback == 1) {
                cout << "Do you want to save the chat history?\n"
                        "(Input the file name to save, press Enter to skip): ";
                string save_name;
                getline(cin, save_name);
                if (!save_name.empty()) {
                    p_save_chat(save_name);
                }
                cout << "₪ GPT-3 Bot Stopped." << endl;
                break;
            } else if (command_feedback == 2) {
                if (!prompts.empty()) {
                    prompts.pop_back();
                }
                continue;
            } else if (command_feedback == 3) {
                prompts.clear();
                continue;
            }
            replace_all(input, "\\n", "\n");
            cout << "Getting embeddings and finding similar chat exchanges for the input...";
            auto input_embeddings = emb::get_embeddings(input, api_key);
            if (input_embeddings) {
                prompts.emplace_back(make_shared<Exchange>(input, *input_embeddings, util::currentTimeMillis()));
                print_prompt();
                call_api();
            } else {
                print_enter_next_cycle();
            }
        }
    }

    void call_api() {
        CURL* curl;
        CURLcode res;
        string response;
        curl = curl_easy_init();
        if (curl) {
            string url = is_new_api ? "https://api.openai.com/v1/chat/completions" : "https://api.openai.com/v1/completions";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
            const string& proxy = util::system_proxy();
            if (!proxy.empty()) {
                curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
                curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
            }
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            string auth = "Authorization: Bearer ";
            headers = curl_slist_append(headers, auth.append(api_key).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            string suffix = ": ";
            json payload = {{"model", model},
                            {"temperature", temperature},
                            {"max_tokens", max_tokens},
                            {"top_p", top_p},
                            {"frequency_penalty", frequency_penalty},
                            {"presence_penalty", presence_penalty},
                            {"stream", true}};
            string constructed_initial = prompt::construct_reference(
                    initial_prompt, prompts.back()->getInputEmbeddings(),
                    prompts, max_reference_length, max_short_memory_length, me_id, bot_id);
            if (debug_reference) {
                string dr_prefix = "<Debug Reference> ";
                cout << "\n" << dr_prefix << "Constructed initial prompt:\n----------\n" << constructed_initial << "\n----------\n";
                cout << dr_prefix << "Press Enter to continue: ";
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << bot_id << (is_new_api ? suffix : ":");
            }
            if (!is_new_api) {
                payload["prompt"] = to_payload(
                        constructed_initial, prompts, me_id, bot_id, max_short_memory_length);
                payload["stop"] = {me_id + suffix, bot_id + suffix};
            } else {
                payload["messages"] = ChatGPT::to_payload(
                        constructed_initial, prompts, me_id, bot_id, max_short_memory_length);
            }
            string payload_str = payload.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
            res = curl_easy_perform(curl);
            bool error = false;
            if (res != CURLE_OK) {
                error = true;
                cerr << "\nAPI request failed: " << curl_easy_strerror(res) << "\n";
            } else {
                try {
                    json j = json::parse(response);
                    if (j.count("error") > 0 && j["error"].is_object()) {
                        auto error_obj = j["error"];
                        error = true;
                        if (error_obj.count("message") > 0 && error_obj["message"].is_string()) {
                            cerr << "\nAPI returned error: " << error_obj["message"].get<string>() << "\n";
                        } else {
                            cerr << "\nAPI returned unknown error. Json: " << response << "\n";
                        }
                    }
                } catch (const json::parse_error& e) {}
            }
            if (error) {
                print_enter_next_cycle();
                prompts.pop_back();
            } else {
                if (!is_new_api) {
                    if (starts_with(response, " ")) {
                        response.erase(0, 1);
                    }
                }
                prompts.back()->setResponse(response);
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
    }

    size_t write_callback(char* char_ptr, size_t size, size_t mem, string* base_str) {
        size_t length = size * mem;
        string s(char_ptr, length);
        vector<string> split_str;
        split_regex(split_str, s, regex("[\n][\n][d][a][t][a][:][ ]"));
        for (auto& str : split_str) {
            if (starts_with(str, "data: ")) {
                str.erase(0, 6);
            }
            if (ends_with(str, "\n\n")) {
                str.erase(str.size() - 2);
            }
            if (str != "[DONE]") {
                try {
                    json j = json::parse(str);
                    if (j.count("choices") > 0 && j["choices"].is_array()) {
                        auto choices = j["choices"];
                        if (!is_new_api) {
                            for (const auto& choice : choices) {
                                if (choice.count("text") > 0 && choice["text"].is_string()) {
                                    string text = choice["text"].get<string>();
                                    base_str->append(text);
                                    cout << text;
                                }
                            }
                        } else {
                            for (const auto& choice : choices) {
                                if (choice.count("delta") > 0 && choice["delta"].is_object()) {
                                    auto delta = choice["delta"];
                                    if (delta.count("content") > 0 && delta["content"].is_string()) {
                                        string content = delta["content"].get<string>();
                                        base_str->append(content);
                                        cout << content;
                                    }
                                }
                            }
                        }
                    } else {
                        *base_str = j.dump();
                    }
                } catch (const json::parse_error& e) {
                    cerr << "Error parsing JSON: " << e.what() << endl;
                }
            } else {
                cout << endl;
            }
        }
        return length;
    }

    void print_prompt() {
        clear_console();
        prompt::print_prompt(initial_prompt, prompts, me_id, bot_id, max_display_length, is_new_api);
    }

    void print_enter_next_cycle() {
        cout << "Press Enter to run the next cycle: ";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    void clear_console() {
#ifdef _WIN32
        system("cls"); //Clear the console in Windows
#else
        system("clear"); //Clear the console in Unix-based systems (e.g. Linux, macOS)
#endif
    }

    /**
     * @return 0 = didn't match, 1 = /stop, 2 = /undo, 3 = /reset.
     */
    int handle_command(const string& input) {
        if (input == "/stop") {
            return 1;
        } else if (input == "/undo") {
            return 2;
        } else if (input == "/reset") {
            return 3;
        }
        return 0;
    }

    /**
     * @return True if the folders were created or already exist, false if an error occurred.
     */
    bool create_folders(const vector<string>& folders) {
        cout << "Creating folders...\n";
        for (const auto& folder : folders) {
            try {
                if (!exists(folder)) {
                    create_directory(folder);
                    cout << "Created folder: " << folder << '\n';
                }
            } catch (const std::exception& e) {
                cerr << "Error creating folder: " << folder << '\n';
                cerr << "Reason: " << e.what() << '\n';
                return false;
            }
        }
        cout << "Creating folders completed.\n";
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
                cout << "Creating default prompt file: " << path_ << "\n";
                ofstream file(path_);
                if (file.is_open()) {
                    file << initial_prompt;
                    file.close();
                } else {
                    cerr << "Error opening file for writing: " << path_ << endl;
                    return false;
                }
            } catch (const std::exception& e) {
                cerr << "Error creating file: " << path_ << endl;
                cerr << "Reason: " << e.what() << endl;
                return false;
            }
        } else {
            return p_load_prompt(default_filename);
        }
        cout << "Default prompt file processing completed.\n";
        return true;
    }

    bool p_load_prompt(string filename) {
        const auto path_ = path(f_initial) / filename.append(f_suffix);
        if (!exists(path_)) {
            cerr << "Prompt file does not exist: " << path_ << endl;
            return false;
        }
        try {
            cout << "Reading prompt file: " << path_ << "\n";
            ifstream file(path_);
            if (file.is_open()) {
                initial_prompt = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
                file.close();
            } else {
                cerr << "Error opening file for reading: " << path_ << endl;
                return false;
            }
        } catch (const std::exception& e) {
            cerr << "Error reading file: " << path_ << endl;
            cerr << "Reason: " << e.what() << endl;
            return false;
        }
        cout << "Prompt file processing completed.\n";
        return true;
    }

    /**
     * This function loads the saved chat history from the file(.json), and also does error handling.
     * It uses nlohmann::json library.
     * If the json is not in the correct format, it will return false.
     * @return True if the file already exists and read, false if an error occurred.
     */
    bool p_load_saved(string filename) {
        const auto path_ = path(f_saved) / filename.append(json_suffix);
        if (!exists(path_)) {
            cerr << "Saved chat file does not exist: " << path_ << endl;
            return false;
        }
        try {
            cout << "Reading saved chat file: " << path_ << "\n";
            ifstream file(path_);
            if (file.is_open()) {
                json j;
                file >> j;
                file.close();
                if (j.count("initial") > 0 && j["initial"].is_string()) {
                    initial_prompt = j["initial"].get<string>();
                } else {
                    cerr << "Error reading saved chat file: " << path_ << endl;
                    cerr << "Reason: initial is not a string." << endl;
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
                                    cerr << "Error reading saved chat file: " << path_ << endl;
                                    cerr << "Reason: input is not a string." << endl;
                                    return false;
                                }
                                if (history.count("input_embeddings") > 0 && history["input_embeddings"].is_array()) {
                                    input_embeddings = history["input_embeddings"].get<vector<float>>();
                                } else {
                                    cerr << "Error reading saved chat file: " << path_ << endl;
                                    cerr << "Reason: input_embeddings is not an array." << endl;
                                    return false;
                                }
                                if (history.count("response") > 0 && history["response"].is_string()) {
                                    response = history["response"].get<string>();
                                }
                                if (history.count("time_stamp") > 0 && history["time_stamp"].is_number_integer()) {
                                    time_ms = history["time_stamp"].get<long long>();
                                } else {
                                    cerr << "Error reading saved chat file: " << path_ << endl;
                                    cerr << "Reason: time_stamp is not an integer." << endl;
                                    return false;
                                }
                                if (response.empty()) {
                                    prompts.push_back(make_shared<Exchange>(input, input_embeddings, time_ms));
                                } else {
                                    prompts.push_back(make_shared<Exchange>(input, input_embeddings, response, time_ms));
                                }
                            } else {
                                cerr << "Error reading saved chat file: " << path_ << endl;
                                cerr << "Reason: history is not an object." << endl;
                                return false;
                            }
                        }
                    } else if (histories.is_null()) {} else {
                        cerr << "Error reading saved chat file: " << path_ << endl;
                        cerr << "Reason: histories is not an array." << endl;
                        return false;
                    }
                } else {
                    cerr << "Error reading saved chat file: " << path_ << endl;
                    cerr << "Reason: histories is not found." << endl;
                    return false;
                }
            } else {
                cerr << "Error opening saved chat file for reading: " << path_ << endl;
                return false;
            }
        } catch (const std::exception& e) {
            cerr << "Error reading saved chat file: " << path_ << endl;
            cerr << "Reason: " << e.what() << endl;
            return false;
        }
        cout << "Saved chat file reading completed.\n";
        return true;
    }

    /**
     * This function saves the chat history to the file(name.json), and also does error handling.
     * It uses nlohmann::json library.
     * @return True if the file is correctly saved, false if an error occurred.
     */
    bool p_save_chat(string name) {
        const auto path_ = path(f_saved) / name.append(json_suffix);
        try {
            cout << "Saving chat to file: " << path_ << "\n";
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
                file << j.dump(4);
                file.close();
            } else {
                cerr << "Error opening file for writing: " << path_ << endl;
                return false;
            }
        } catch (const std::exception& e) {
            cerr << "Error creating file: " << path_ << endl;
            cerr << "Reason: " << e.what() << endl;
            return false;
        }
        cout << "Chat saved to file: " << path_ << "\n";
        return true;
    }

    /**
     * This function loads the config from the file(config.json), and also does error handling.
     * It uses nlohmann::json library.
     * If config.json does not exist or is not in the correct format, it will create a new config.json file.
     * @return True if the config is correctly loaded, false if an error occurred.
     */
    bool p_load_config() {
        const auto path_ = path("config.json");
        if (!exists(path_)) {
            cerr << "Config file does not exist: " << path_ << endl;
            cerr << "Creating new config file: " << path_ << endl;
            return p_save_config();
        }
        try {
            cout << "Reading config file: " << path_ << "\n";
            ifstream file(path_);
            if (file.is_open()) {
                json j;
                file >> j;
                file.close();
                bool error = false;
                if (j.count("api_key") > 0 && j["api_key"].is_string()) {
                    api_key = j["api_key"].get<string>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: api_key is not a string." << endl;
                    error = true;
                }
                if (j.count("model") > 0 && j["model"].is_string()) {
                    model = j["model"].get<string>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: model is not a string." << endl;
                    error = true;
                }
                if (j.count("temperature") > 0 && j["temperature"].is_number()) {
                    temperature = j["temperature"].get<float>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: temperature is not a number." << endl;
                    error = true;
                }
                if (j.count("max_tokens") > 0 && j["max_tokens"].is_number()) {
                    max_tokens = j["max_tokens"].get<int>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: max_tokens is not a number." << endl;
                    error = true;
                }
                if (j.count("top_p") > 0 && j["top_p"].is_number()) {
                    top_p = j["top_p"].get<float>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: top_p is not a number." << endl;
                    error = true;
                }
                if (j.count("frequency_penalty") > 0 && j["frequency_penalty"].is_number()) {
                    frequency_penalty = j["frequency_penalty"].get<float>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: frequency_penalty is not a number." << endl;
                    error = true;
                }
                if (j.count("presence_penalty") > 0 && j["presence_penalty"].is_number()) {
                    presence_penalty = j["presence_penalty"].get<float>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: presence_penalty is not a number." << endl;
                    error = true;
                }
                if (j.count("max_display_length") > 0 && j["max_display_length"].is_number_unsigned()) {
                    max_display_length = j["max_display_length"].get<unsigned int>();
                    if (max_display_length == 0) {
                        cerr << "Error reading config file: " << path_ << endl;
                        cerr << "Reason: max_display_length cannot be 0." << endl;
                        max_display_length = 1;
                        error = true;
                    }
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: max_display_length is not an unsigned integer." << endl;
                    error = true;
                }
                if (j.count("max_short_memory_length") > 0 && j["max_short_memory_length"].is_number_unsigned()) {
                    max_short_memory_length = j["max_short_memory_length"].get<unsigned int>();
                    if (max_short_memory_length == 0) {
                        cerr << "Error reading config file: " << path_ << endl;
                        cerr << "Reason: max_short_memory_length cannot be 0." << endl;
                        max_short_memory_length = 1;
                        error = true;
                    }
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: max_short_memory_length is not an unsigned integer." << endl;
                    error = true;
                }
                if (j.count("max_reference_length") > 0 && j["max_reference_length"].is_number_unsigned()) {
                    max_reference_length = j["max_reference_length"].get<unsigned int>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: max_reference_length is not an unsigned integer." << endl;
                    error = true;
                }
                if (j.count("debug_reference") > 0 && j["debug_reference"].is_boolean()) {
                    debug_reference = j["debug_reference"].get<bool>();
                } else {
                    cerr << "Error reading config file: " << path_ << endl;
                    cerr << "Reason: debug_reference is not a boolean." << endl;
                    error = true;
                }
                if (j.count("me_id") > 0 && j["me_id"].is_string()) {
                    cerr << "Outdated element in config file: me_id" << endl;
                    error = true;
                }
                if (j.count("bot_id") > 0 && j["bot_id"].is_string()) {
                    cerr << "Outdated element in config file: bot_id" << endl;
                    error = true;
                }
                if (error) {
                    cerr << "Error detected, creating new config file: " << path_ << endl;
                    return p_save_config();
                }
            }
        } catch (const std::exception& e) {
            cerr << "Error reading config file: " << path_ << endl;
            cerr << "Reason: " << e.what() << endl;
            return false;
        }
        cout << "Config loaded from file: " << path_ << "\n";
        return true;
    }

    /**
     * This function save the config to the file(config.json), and also does error handling.
     * It uses nlohmann::json library.
     * @return True if the config is correctly saved, false if an error occurred.
     */
    bool p_save_config() {
        const auto path_ = path("config.json");
        try {
            cout << "Saving config to file: " << path_ << "\n";
            ofstream file(path_);
            if (file.is_open()) {
                json j;
                j["api_key"] = api_key;
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
                file << j.dump(4);
                file.close();
            } else {
                cerr << "Error opening file for writing: " << path_ << endl;
                return false;
            }
        } catch (const std::exception& e) {
            cerr << "Error creating file: " << path_ << endl;
            cerr << "Reason: " << e.what() << endl;
            return false;
        }
        cout << "Config saved to file: " << path_ << "\n";
        return true;
    }
}
