//
// Created by v2ray on 2023/4/15.
//

#include "Command.h"

namespace cmd {
    inline void print_uwu();

    Commands match_command(const std::string& input) {
        if (input.empty() || !input.starts_with("/")) {
            return Commands::NONE;
        }
        if (input == "/stop") {
            return Commands::STOP;
        } else if (input == "/undo") {
            return Commands::UNDO;
        } else if (input == "/reset") {
            return Commands::RESET;
        } else if (input == "/uwu") {
            return Commands::UWU;
        } else if (input == "/tc") {
            return Commands::TOKENIZER;
        }
        return Commands::NONE;
    }

    ReturnOpCode handle_command(const std::string& input, std::vector<std::shared_ptr<chat::Exchange>>& prompts) {
        const auto& cmd = match_command(input);
        switch (cmd) {
            case Commands::STOP: {
                util::print_cs("Do you want to save the chat history?\n"
                               "(Input the file name to save, press " + ENTER + " to skip): ");
                std::string save_name;
                getline(std::cin, save_name);
                if (!save_name.empty()) {
                    GPT::p_save_chat(save_name);
                }
                util::print_clr("₪ ", {255, 50, 50});
                util::print_m_clr("GPT-3 Bot Stopped.", {{255, 50, 50}, {255, 255, 50}});
                std::cout << std::endl;
                return ReturnOpCode::STOP;
            }
            case Commands::UNDO:
                if (!prompts.empty()) {
                    prompts.pop_back();
                }
                return ReturnOpCode::CONTINUE;
            case Commands::RESET:
                prompts.clear();
                return ReturnOpCode::CONTINUE;
            case Commands::UWU:
                print_uwu();
                GPT::print_enter_next_cycle();
                return ReturnOpCode::CONTINUE;
            case Commands::TOKENIZER:
                try {
                    static std::vector<std::string> input_history;
                    std::string input_tc;
                    input_tc = util::get_multi_lines(input_history, "> ");
                    std::cout << "Choose the tokenizer(1/2/3/4): ";
                    std::string tokenizer;
                    getline(std::cin, tokenizer);
                    std::string tokenizer_;
                    if (tokenizer == "2") {
                        tokenizer_ = "text-davinci-001";
                    } else if (tokenizer == "3") {
                        tokenizer_ = "text-davinci-003";
                    } else if (tokenizer == "4") {
                        tokenizer_ = "text-davinci-edit-001";
                    } else {
                        tokenizer_ = "gpt-4";
                    }
                    auto prev_time = util::currentTimeMillis();
                    std::cout << "Token count: " << util::get_token_count(input_tc, tokenizer_) << "\n"
                    << "Time used: " << util::currentTimeMillis() - prev_time << "ms" << std::endl;
                } catch (const std::exception& e) {
                    util::println_err("\nAn error occurred while getting input: " + std::string(e.what()));
                }
                GPT::print_enter_next_cycle();
                return ReturnOpCode::CONTINUE;
            default:
                return ReturnOpCode::NONE;
        }
    }

    inline void print_uwu() {
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
} // cmd