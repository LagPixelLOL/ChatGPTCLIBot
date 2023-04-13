#include "prompt.hpp"

#include "input.hpp"
#include "platforms/conversion.hpp"
#include "terminal.hpp"

#include "algorithm"
#include "iostream"

Term::Result Term::prompt(const std::string& message, const std::string& first_option, const std::string& second_option,
                          const std::string& prompt_indicator, bool immediate)
{
  Terminal term(false, true, false);
  std::cout << message << " [" << first_option << '/' << second_option << ']' << prompt_indicator << ' ' << std::flush;

  if(!Term::stdin_connected())
  {
    std::cout << '\n' << std::flush;
    return Result::ERR;
  }

  int key;

  if(immediate)
  {
    while(true)
    {
      key = Term::read_key();
      if(key == 'y' || key == 'Y')
      {
        std::cout << '\n' << std::flush;
        return Result::YES;
      }
      else if(key == 'n' || key == 'N')
      {
        std::cout << '\n' << std::flush;
        return Result::NO;
      }
      else if(key == Term::Key::CTRL_C || key == Term::Key::CTRL_D)
      {
        std::cout << '\n' << std::flush;
        return Result::ABORT;
      }
      else if(key == Term::Key::ENTER)
      {
        std::cout << '\n' << std::flush;
        return Result::NONE;
      }
      else
      {
        std::cout << '\n' << std::flush;
        return Result::INVALID;
      }
    }
  }
  else
  {
    std::vector<char>  input;
    unsigned short int length = 0;
    while(true)
    {
      key = Term::read_key();
      if(key >= 'a' && key <= 'z')
      {
        std::cout << (char)key << std::flush;
        length++;
        input.push_back(static_cast<char>(key));
      }
      else if(key >= 'A' && key <= 'Z')
      {
        std::cout << (char)key << std::flush;
        length++;
        input.push_back(static_cast<char>(key + 32));  // convert upper case to lowercase
      }
      else if(key == Term::Key::CTRL_C || key == Term::Key::CTRL_D)
      {
        std::cout << '\n';
        return Result::ABORT;
      }
      else if(key == Term::Key::BACKSPACE)
      {
        if(length != 0)
        {
          std::cout << "\033[D \033[D" << std::flush;  // erase last line and move the cursor back
          length--;
          input.pop_back();
        }
      }
      else if(key == Term::Key::ENTER)
      {
        if(Private::vector_to_string(input) == "y" || Private::vector_to_string(input) == "yes")
        {
          std::cout << '\n' << std::flush;
          return Result::YES;
        }
        else if(Private::vector_to_string(input) == "n" || Private::vector_to_string(input) == "no")
        {
          std::cout << '\n' << std::flush;
          return Result::NO;
        }
        else if(length == 0)
        {
          std::cout << '\n' << std::flush;
          return Result::NONE;
        }
        else
        {
          std::cout << '\n' << std::flush;
          return Result::INVALID;
        }
      }
    }
  }
}

Term::Result_simple Term::prompt_simple(const std::string& message)
{
  switch(prompt(message, "y", "N", ":", false))
  {
    case Result::YES: return Result_simple::YES;
    case Result::ABORT: return Result_simple::ABORT;
    case Result::NO:     // falls through
    case Result::ERR:  // falls through
    case Result::NONE:   // falls through
    case Result::INVALID: return Result_simple::NO;
  }
  // shouldn't be reached
  return Result_simple::NO;
}

std::string Term::concat(const std::vector<std::string>& lines) {
    std::string s;
    for (const auto& line : lines) {
        s.append(line + "\n");
    }
    if (!s.empty() && s.back() == '\n') {
        s.pop_back();
    }
    return s;
}

std::vector<std::string> Term::split(std::string s) {
    s.append("\n");
    std::size_t j = 0;
    std::vector<std::string> lines;
    lines.emplace_back("");
    for (std::size_t i = 0; i < s.size() - 1; i++) {
        if (s[i] == '\n') {
            j++;
            lines.emplace_back("");
        } else {
            lines[j].push_back(s[i]);
        }
    }
    return lines;
}

char32_t Term::UU(const std::string& s) {
    std::u32string s2 = Private::utf8_to_utf32(s);
    if (s2.size() != 1) {
        throw Term::Exception("U(s): s not a codepoint.");
    }
    return s2[0];
}

void Term::print_left_curly_bracket(Term::Window& scr, const size_t& x, const size_t& y1, const size_t& y2,
                                    const std::vector<std::string>& display_vec) {
    size_t h = y2 - y1 + 1;
    std::u32string u32s_f = Private::utf8_to_utf32(display_vec[0]);
    auto x_c = static_cast<long long>(x);
    long long pos_f = std::max(x_c - static_cast<long long>(display_length(u32s_f) - u32s_f.size()), 1LL);
    if (h == 1) {
        scr.set_char(pos_f, y1, UU("]"));
    } else {
        scr.set_char(pos_f, y1, UU("┐"));
        for (size_t i = y1 + 1; i <= y2 - 1; i++) {
            std::u32string u32s_m = Private::utf8_to_utf32(display_vec[i - y1]);
            scr.set_char(std::max(x_c - static_cast<long long>(display_length(u32s_m) - u32s_m.size()), 1LL), i, UU("│"));
        }
        std::u32string u32s_b = Private::utf8_to_utf32(display_vec.back());
        scr.set_char(std::max(x_c - static_cast<long long>(display_length(u32s_b) - u32s_b.size()), 1LL), y2, UU("┘"));
    }
}

/**
 * Calculate the display length of a string, i.e. the number of columns it occupies on the terminal.
 * Some characters occupy 2 columns, e.g. Chinese characters.
 */
inline size_t Term::display_length(const std::u32string& u32str) {
    size_t len = 0;
    for (const auto& c : u32str) {
        short width = Private::c32_display_width(c);
        len += width >= 0 ? width : 0;
    }
    return len;
}

inline size_t Term::u8_to_u32_cursor_col(const std::string& str, const size_t& cursor_col) {
    return Private::utf8_to_utf32(str.substr(0, cursor_col - 1)).size() + 1;
}

inline size_t Term::u32_to_display_col(const std::u32string& u32str, const size_t& cursor_col) {
    return display_length(u32str.substr(0, cursor_col)) + 1;
}

/**
 * Pre-process the input vector of strings, if a line of string is too long to fit in the window, split it into multiple lines.
 * @param w Terminal window.
 * @param m Model that contains the input vector of strings.
 * @param cursor_x The cursor x position to modify, 1-indexed.
 * @param cursor_y The cursor y position to modify, 1-indexed.
 * @return The pre-processed vector of vector of strings, the inner vector is the split lines of each line,
 * the outer vector contains all the lines.
 */
std::vector<std::vector<std::string>> Term::pre_process(const Window& w, const Model& m, size_t& cursor_x, size_t& cursor_y) {
    std::vector<std::u32string> u32strings;
    for (const auto& s : m.lines) {
        u32strings.push_back(Private::utf8_to_utf32(s));
    }
    size_t skip_col = m.prompt_string.size() + 1;
    std::vector<std::vector<std::u32string>> u32result;
    for (const auto& u32s : u32strings) {
        std::vector<std::u32string> u32s_split;
        std::u32string u32s_tmp;
        size_t col = skip_col;
        for (const auto& c32 : u32s) {
            if (col < w.get_w()) {
                u32s_tmp.push_back(c32);
            } else {
                u32s_split.push_back(u32s_tmp);
                u32s_tmp.clear();
                u32s_tmp.push_back(c32);
                col = skip_col;
            }
            short width = Private::c32_display_width(c32);
            col += width >= 0 ? width : 0;
        }
        u32s_split.push_back(u32s_tmp);
        u32result.push_back(u32s_split);
    }
    std::vector<std::vector<std::string>> result;
    for (const auto& u32s_split : u32result) {
        std::vector<std::string> s_split;
        for (const auto& u32s : u32s_split) {
            s_split.push_back(Private::utf32_to_utf8(u32s));
        }
        result.push_back(s_split);
    }
    size_t total_lines = 1;
    size_t current_line = 1;
    size_t current_col = 1;
    cursor_x = u8_to_u32_cursor_col(m.lines[cursor_y - 1], cursor_x);
    for (const auto& u32s_split : u32result) {
        for (const auto& u32s : u32s_split) {
            if (current_line == cursor_y) {
                if (current_col + u32s.size() >= cursor_x) {
                    cursor_y = total_lines;
                    cursor_x = u32_to_display_col(u32s, cursor_x - current_col) + skip_col - 1;
                    goto end_loops;
                }
                current_col += u32s.size();
            }
            ++total_lines;
        }
        ++current_line;
        current_col = 1;
    } end_loops:
    return result;
}

inline std::vector<std::string> simplify(const std::vector<std::vector<std::string>>& v) {
    std::vector<std::string> simplified;
    for (const auto& e : v) {
        for (const auto& e2 : e) {
            simplified.push_back(e2);
        }
    }
    return simplified;
}

std::pair<size_t, size_t> Term::render(Window& scr, const Model& m, const size_t& cols) {
    scr.clear();
    size_t cursor_x = m.cursor_col;
    size_t cursor_y = m.cursor_row;
    auto processed_lines = pre_process(scr, m, cursor_x, cursor_y);
    auto simplified = simplify(processed_lines);
    auto s_size = simplified.size();
    if (s_size > scr.get_h()) {
        scr.set_h(s_size);
    }
    print_left_curly_bracket(scr, cols, 1, simplified.size(), simplified);
    size_t line_count = 1;
    scr.fill_fg(1, line_count, m.prompt_string.size(), s_size, Term::Color::Name::BrightGreen);
    scr.fill_style(1, line_count, m.prompt_string.size(), s_size, Term::Style::BOLD);
    scr.print_str(1, line_count, m.prompt_string);
    for (const auto& line_splits : processed_lines) {
        size_t line_splits_size = line_splits.size();
        for (size_t i = 0; i < line_splits_size; ++i) {
            size_t ps_size_m1 = m.prompt_string.size() - 1;
            if (line_count != 1) {
                if (i == 0) {
                    for (size_t j = 0; j < ps_size_m1; j++) {
                        scr.set_char(j + 1, line_count, '.');
                    }
                } else {
                    for (size_t j = 0; j < ps_size_m1; j++) {
                        scr.set_char(j + 1, line_count, j == 0 ? UU(i == line_splits_size - 1 ? "└" : "├") : '-');
                    }
                }
            }
            scr.print_str(ps_size_m1 + 2, line_count, line_splits[i]);
            ++line_count;
        }
    }
    scr.set_cursor_pos(cursor_x, cursor_y);
    return {s_size, cursor_y};
}

void Term::replace_all(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return;
    }
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length(); //In case 'to' contains 'from', like replacing 'x' with 'yx'.
    }
}

/**
 * Calculate how many chars the cursor should shift because utf-8 encoding might have multiple chars to represent 1 utf-8 char.
 * @param str The utf-8 string.
 * @param cursor_col Is the one-indexed current position of the cursor within the utf-8 string,
 * it can be +1 position of the last char, just like when you type in a terminal,
 * it's been converted to the correspond u32string position first.
 * @param shift_amount Is how many chars should be shifted within the u32string,
 * it could be negative to shift to the left and positive to the right.
 * @return The number of chars the cursor should shift within the utf-8 string,
 * it could be negative to shift to the left and positive to the right,
 * the return value is snapped to the nearest valid utf-8 position to make sure it's not inside the characters of a single utf-8 character.
 */
long long Term::calc_cursor_move(const std::string& str, const size_t& cursor_col, const long long& shift_amount) {
    std::u32string u32s = Private::utf8_to_utf32(str);
    if (cursor_col < 1 || cursor_col > str.size() + 1) { //cursor_col is 1-indexed, +1 for the cursor being after the last character.
        throw Term::Exception("calc_cursor_move: cursor position out of range.");
    }
    long long zero_indexed_cursor_col = static_cast<long long>(cursor_col) - 1; //-1 to convert to 0-indexed.
    //Snap zero_indexed_cursor_col to the nearest valid position if it's inside an utf-8 character.
    long long snapped_zero_indexed_cursor_col = zero_indexed_cursor_col; //Copy the value.
    while (snapped_zero_indexed_cursor_col > 0 && (str[snapped_zero_indexed_cursor_col] & 0xC0) == 0x80) {
        --snapped_zero_indexed_cursor_col;
    }
    long long snap_shift_amount = snapped_zero_indexed_cursor_col - zero_indexed_cursor_col;
    //Check if the cursor will move out of the string.
    if (snapped_zero_indexed_cursor_col + shift_amount < 0 || snapped_zero_indexed_cursor_col + shift_amount > str.size()) {
        return snap_shift_amount;
    }
    //Calculate the number of columns the cursor will move.
    size_t u32_cursor_col = Private::utf8_to_utf32(str.substr(0, snapped_zero_indexed_cursor_col)).size();
    long long u32_target_col = static_cast<long long>(u32_cursor_col) + shift_amount;
    if (u32_target_col < 0 || u32_target_col > u32s.size()) {
        return snap_shift_amount;
    }
    std::string utf8_initial = Private::utf32_to_utf8(u32s.substr(0, u32_cursor_col));
    std::string utf8_target = Private::utf32_to_utf8(u32s.substr(0, u32_target_col));
    long long utf8_shift_amount = static_cast<long long>(utf8_target.size()) - static_cast<long long>(utf8_initial.size());
    //Account for the snapped zero_indexed_cursor_col.
    utf8_shift_amount += snap_shift_amount;
    return utf8_shift_amount;
}
