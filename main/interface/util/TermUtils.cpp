//
// Created by v2ray on 2023/3/21.
//

#include "TermUtils.h"

namespace util {

    unique_ptr<Term::Terminal> initialize_or_throw(const bool& clear_screen = false,
                                                   const bool& disable_signal_keys = false, const bool& hide_cursor = false) {
        if (!Term::stdin_connected() || !Term::stdout_connected()) {
            throw Term::Exception("The terminal is not attached to a TTY and therefore can't catch user input.");
        }
        return make_unique<Term::Terminal>(clear_screen, disable_signal_keys, hide_cursor);
    }

    /**
     * Get multiple lines of input from the user.
     * Press Ctrl + N to enter a new line.
     * Note: Remember to do error handling by catching std::exception.
     * @param history Input history.
     * @param prompt_string The text that's displayed before the user's input.
     * @return The user's input.
     */
    string get_multi_lines(vector<string>& history, const string& prompt_string) {
        auto t = initialize_or_throw();
        return Term::prompt_multiline(prompt_string, history, [](const auto& s){
            if (s.size() > 1 && s.substr(s.size() - 2, 1) == "\\") {
                return false;
            }
            return true;
        });
    }

    void ignore_line() {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    /**
     * Initialize the terminal and print a string that has color codes embedded in it.
     */
    void print_cs(const string& s, const bool& new_line, const bool& reset_color) {
        auto t = initialize_or_throw();
        cout << s;
        if (new_line) {
            cout << endl;
        }
        if (reset_color) {
            cout << Term::color_fg(Color::Name::Default);
        }
    }

    /**
     * Print a string with a color.
     */
    void print_clr(const string& s, const Color& color, const bool& new_line) {
        auto t = initialize_or_throw();
        print_cs(Term::color_fg(color) + s, new_line);
    }

    /**
     * Print a string with a color and a new line.
     */
    void println_clr(const string& s, const Color& color) {
        print_clr(s, color, true);
    }

    /**
     * Print a string as an info(Gray).
     */
    void println_info(const string& s, const bool& new_line) {
        print_clr(s, Color::Name::Gray, new_line);
    }

    /**
     * Print a string as a warning(Yellow).
     */
    void println_warn(const string& s, const bool& new_line) {
        print_clr(s, Color::Name::Yellow, new_line);
    }

    /**
     * Print a string as an error(Red).
     */
    void println_err(const string& s, const bool& new_line) {
        print_clr(s, Color::Name::Red, new_line);
    }

    /**
     * Perform a linear interpolation between two colors.
     */
    Color linear_interp(const Color& start, const Color& end, float t) {
        array<uint8_t, 3> start_rgb = start.to24bits();
        array<uint8_t, 3> end_rgb = end.to24bits();
        uint8_t start_r = start_rgb[0]; uint8_t start_g = start_rgb[1]; uint8_t start_b = start_rgb[2];
        return {static_cast<uint8_t>(static_cast<float>(start_r) + t * static_cast<float>(end_rgb[0] - start_r)),
                static_cast<uint8_t>(static_cast<float>(start_g) + t * static_cast<float>(end_rgb[1] - start_g)),
                static_cast<uint8_t>(static_cast<float>(start_b) + t * static_cast<float>(end_rgb[2] - start_b))};
    }

    /**
     * Calculate a gradual change of colors.
     * @param colors The colors to interpolate between,
     * the size of the vector must be greater than 1, or there will be a division by zero error.
     * @param steps The number of color changes.
     * @return A vector of colors that gradually changes.
     */
    vector<Color> multi_clr_gradual_change(const vector<Color>& colors, const size_t& steps) {
        vector<Color> gradual_change;
        gradual_change.reserve(steps);
        size_t color_count = colors.size() - 1;
        size_t steps_per_color = steps / color_count;
        for (size_t i = 0; i < steps; ++i) {
            size_t current_color_index = i / steps_per_color;
            if (current_color_index >= color_count) {
                current_color_index = color_count - 1;
            }
            size_t next_color_index = current_color_index + 1;
            float t = static_cast<float>(i % steps_per_color) / static_cast<float>(steps_per_color - 1);
            gradual_change.push_back(linear_interp(colors[current_color_index], colors[next_color_index], t));
        }
        return gradual_change;
    }

    /**
     * Print a string gradually changing colors.
     * @param colors The colors to interpolate between.
     * @param by_line Whether to print the color change by line or by character.
     */
    void print_m_clr(const string& s, const vector<Color>& colors, const bool& by_line) {
        //Prevent division by zero.
        if (colors.empty() || s.empty()) {
            cout << s;
            return;
        } else if (colors.size() == 1) {
            print_clr(s, colors[0]);
            return;
        } else if (s.size() / (colors.size() - 1) <= 1) {
            for (int i = 0; i < s.size() - 1; i++) {
                print_clr(s.substr(i, 1), colors[i < colors.size() ? i : colors.size() - 1]);
            }
            print_clr(s.substr(s.size() - 1, 1), colors.back());
            return;
        }
        if (by_line) {
            vector<string> lines = Term::split(s);
            //Prevent division by zero.
            if (lines.empty()) {
                return;
            } else if (lines.size() == 1) {
                print_clr(lines[0], colors[0]);
                return;
            } else if (lines.size() / (colors.size() - 1) <= 1) {
                for (int i = 0; i < lines.size() - 1; i++) {
                    println_clr(lines[i], colors[i < colors.size() ? i : colors.size() - 1]);
                }
                print_clr(lines.back(), colors.back());
                return;
            }
            vector<Color> g_clr = multi_clr_gradual_change(colors, lines.size());
            for (int i = 0; i < lines.size() - 1; i++) {
                println_clr(lines[i], g_clr[i]);
            }
            print_clr(lines.back(), g_clr.back());
        } else {
            vector<Color> g_clr = multi_clr_gradual_change(colors, s.size());
            for (int i = 0; i < s.size(); i++) {
                print_clr(string(1, s[i]), g_clr[i]);
            }
        }
    }
} // util