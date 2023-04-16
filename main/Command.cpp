//
// Created by v2ray on 2023/4/15.
//

#include "Command.h"

#define GOLDEN_TEXT(n) Term::color_fg(255, 200, 0) + n + Term::color_fg(Color::Name::Default)

namespace cmd {
    inline void print_colored_tokenized_text(const std::vector<int>& tokens, const std::shared_ptr<GptEncoding>& tokenizer_p);
    inline void print_colored_token_numbers(const std::vector<int>& tokens);
    inline void print_uwu();

    class ColorIterator {
        uint32_t i = 0;

    public:
        std::string next_color_str() {
            switch (i++ % 5) {
                case 0:
                    return Term::color_bg(204, 191, 238);
                case 1:
                    return Term::color_bg(190, 237, 198);
                case 2:
                    return Term::color_bg(246, 217, 171);
                case 3:
                    return Term::color_bg(244, 174, 177);
                case 4:
                    return Term::color_bg(164, 220, 243);
                default:
                    return "";
            }
        }
    };

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
            return Commands::TOKENIZE;
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
            case Commands::TOKENIZE:
                try {
                    static std::vector<std::string> input_history;
                    std::string input_tc;
                    input_tc = util::get_multiline(input_history);
                    util::print_cs(
                            "[" + GOLDEN_TEXT("1") + " = " + GOLDEN_TEXT("CL100K_BASE")
                            + ", " + GOLDEN_TEXT("2") + " = " + GOLDEN_TEXT("P50K_BASE")
                            + ", " + GOLDEN_TEXT("3") + " = " + GOLDEN_TEXT("P50K_EDIT")
                            + ", " + GOLDEN_TEXT("4") + " = " + GOLDEN_TEXT("R50K_BASE")
                            + "]\nChoose the tokenizer(" + GOLDEN_TEXT("1") + "/" + GOLDEN_TEXT("2")
                            + "/" + GOLDEN_TEXT("3") + "/" + GOLDEN_TEXT("4") + "), press " + ENTER
                            + " directly to use " + GOLDEN_TEXT("1") + ": ");
                    std::string tokenizer;
                    getline(std::cin, tokenizer);
                    LanguageModel tokenizer_;
                    if (tokenizer == "2") {
                        tokenizer_ = LanguageModel::P50K_BASE;
                    } else if (tokenizer == "3") {
                        tokenizer_ = LanguageModel::P50K_EDIT;
                    } else if (tokenizer == "4") {
                        tokenizer_ = LanguageModel::R50K_BASE;
                    } else {
                        tokenizer_ = LanguageModel::CL100K_BASE;
                    }
                    long long prev_time = util::currentTimeMillis();
                    std::shared_ptr<GptEncoding> tokenizer_p = util::get_enc_cache(tokenizer_);
                    std::vector<int> tokens = tokenizer_p->encode(input_tc);
                    size_t token_count = tokens.size();
                    long long time_spent = util::currentTimeMillis() - prev_time;
                    print_colored_tokenized_text(tokens, tokenizer_p);
                    std::cout << "Token count: " << token_count << "\nTime used: " << time_spent << "ms" << std::endl;
                    util::print_cs("Please choose whether you want to show the token numbers or run the next cycle.\n"
                                   "(Input " + GOLDEN_TEXT("y") + " to show the token numbers, " + GOLDEN_TEXT("n")
                                   + " to run the next cycle, press " + ENTER + " directly to run the next cycle): ");
                    std::string show_token;
                    getline(std::cin, show_token);
                    transform(show_token.begin(), show_token.end(), show_token.begin(), tolower);
                    if (show_token == "y") {
                        print_colored_token_numbers(tokens);
                    } else {
                        return ReturnOpCode::CONTINUE;
                    }
                } catch (const std::exception& e) {
                    util::println_err("\nAn error occurred while getting input: " + std::string(e.what()));
                }
                GPT::print_enter_next_cycle();
                return ReturnOpCode::CONTINUE;
            default:
                return ReturnOpCode::NONE;
        }
    }

    inline void print_colored_tokenized_text(const std::vector<int>& tokens, const std::shared_ptr<GptEncoding>& tokenizer_p) {
        std::cout << "Text:\n----------\n";
        util::print_cs(Term::color_fg(0, 0, 0), false, false);
        ColorIterator color_iterator;
        for (const auto& token : tokens) {
            std::string decoded = tokenizer_p->decode({token});
            try {
                Term::Private::utf8_to_utf32(decoded);
            } catch (const Term::Exception& e) {
                decoded = "�";
            }
            std::string background_color_str = color_iterator.next_color_str();
            util::print_cs(background_color_str, false, false);
            if (boost::contains(decoded, "\n")) {
                boost::replace_all(decoded, "\n", " " + Term::color_bg(Color::Name::Default) + "\n" + background_color_str);
                util::print_cs(decoded, false, false);
            } else {
                std::cout << decoded;
            }
        }
        util::print_cs("", true); //Reset color.
        std::cout << "----------" << std::endl;
    }

    inline void print_colored_token_numbers(const std::vector<int>& tokens) {
        std::cout << "Token numbers:\n----------\n";
        util::print_cs(Term::color_fg(0, 0, 0), false, false);
        ColorIterator color_iterator;
        for (const auto& token : tokens) {
            std::string background_color_str = color_iterator.next_color_str();
            util::print_cs(background_color_str + std::to_string(token) + Term::color_bg(Color::Name::Default) + " ", false, false);
        }
        util::print_cs("", true); //Reset color.
        std::cout << "----------" << std::endl;
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