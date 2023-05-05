#pragma once

#include "terminal.hpp"
#include "window.hpp"
#include "functional"
#include "optional"

namespace Term {
    /* Basic prompt */

    // indicates the results of prompt_blocking() and prompt_non_blocking
    enum class Result {
        // returned if the user chose yes
        YES,
        // returned if the user chose no
        NO,
        // returned of no terminal is attached to the program
        ERR,
        // returned of the enter key was pressed without additional input
        NONE,
        // returned if CTRL+C was pressed
        ABORT,
        // returned if the given input did not match the case 'yes' of 'no'
        INVALID
    };
    // indicates the results of prompt_simple()
    enum class Result_simple {
        // returned if the user chose yes
        YES,
        // returned if the user chose no or invalid / no input or if no terminal is
        // attached
        NO,
        // returned if CTRL+C was pressed
        ABORT
    };

    // A simple yes/no prompt, requires the user to press the ENTER key to continue
    // The arguments are used like this: 1 [2/3]4 <user Input>
    // the immediate switch indicates toggles whether pressing enter for
    // confirming the input is required or not
    Result prompt(const std::string& message, const std::string& first_option, const std::string& second_option,
                  const std::string& prompt_indicator, bool);

    // The most simple prompt possible, requires the user to press enter to continue
    // The arguments are used like this: 1 [y/N]:
    // Invalid input, errors (like no attached terminal) all result in 'no' as
    // default
    Result_simple prompt_simple(const std::string& message);

    /* Multiline prompt */

    // This model contains all the information about the state of the prompt in an
    // abstract way, irrespective of where or how it is rendered.
    class Model {
    public:
        std::string              prompt_string;  // The string to show as the prompt
        std::vector<std::string> lines{""};      // The current input string in the prompt as a vector of lines.
        // The current cursor position in the "input" string, starting from (1,1)
        std::size_t              cursor_col{1};
        std::size_t              cursor_row{1};
    };

    std::string concat(const std::vector<std::string>&);
    std::vector<std::string> split(std::string);
    char32_t UU(const std::string&);
    void print_left_curly_bracket(Term::Window& scr, const size_t& x, const size_t& y1, const size_t& y2,
                                  const std::vector<std::string>& display_vec);
    std::vector<std::vector<std::string>> pre_process(const Window& w, const Model& m, size_t& cursor_x, size_t& cursor_y);
    std::pair<size_t, size_t> render(Window& scr, const Model& m, const size_t& cols);
    void replace_all(std::string& str, const std::string& from, const std::string& to);
    long long calc_cursor_move(const std::string& str, const size_t& cursor_col, const long long& shift_amount);
    std::string prompt_multiline(const std::string& prompt_string, std::vector<std::string>& m_history,
                                 const std::optional<std::function<bool(const std::string&)>>& is_complete = std::nullopt,
                                 const std::optional<std::function<bool(Model&)>>& ctrl_c_callback = std::nullopt);
}  // namespace Term
