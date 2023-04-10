//
// Created by v2ray on 2023/3/21.
//

#ifndef GPT3BOT_TERMUTILS_H
#define GPT3BOT_TERMUTILS_H

#include "../cpp-terminal/prompt.hpp"
#include "../cpp-terminal/exception.hpp"

namespace util {
    using Color = Term::Color;

    std::string get_multi_lines(std::vector<std::string>& history, const std::string& prompt_string = "> ");
    void ignore_line();
    void print_cs(const std::string& s, const bool& new_line = false, const bool& reset_color = true);
    void print_clr(const std::string& s, const Color& color = Color::Name::Default, const bool& new_line = false);
    void println_clr(const std::string& s, const Color& color = Color::Name::Default);
    void println_info(const std::string& s, const bool& new_line = true);
    void println_warn(const std::string& s, const bool& new_line = true);
    void println_err(const std::string& s, const bool& new_line = true);
    void print_m_clr(const std::string& s, const std::vector<Color>& colors = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}},
                     const bool& by_line = false);
} // util

#endif //GPT3BOT_TERMUTILS_H
