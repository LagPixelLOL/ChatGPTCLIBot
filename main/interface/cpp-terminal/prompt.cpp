#include "prompt.hpp"

#include "input.hpp"
#include "platforms/conversion.hpp"
#include "terminal.hpp"
#include "tty.hpp"

#include "../clip/clip.h"
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

void Term::print_left_curly_bracket(Term::Window& scr, const size_t& x, const size_t& y1, const size_t& y2, const Model& m) {
    size_t h = y2 - y1 + 1;
    std::u32string u32s_f = Private::utf8_to_utf32(m.lines[0]);
    auto x_c = static_cast<long long>(x);
    long long pos_f = std::max(x_c - static_cast<long long>(display_length(u32s_f) - u32s_f.size()), 1LL);
    if (h == 1) {
        scr.set_char(pos_f, y1, UU("]"));
    } else {
        scr.set_char(pos_f, y1, UU("┐"));
        for (size_t i = y1 + 1; i <= y2 - 1; i++) {
            std::u32string u32s_m = Private::utf8_to_utf32(m.lines[i - y1]);
            scr.set_char(std::max(x_c - static_cast<long long>(display_length(u32s_m) - u32s_m.size()), 1LL), i, UU("│"));
        }
        std::u32string u32s_b = Private::utf8_to_utf32(m.lines.back());
        scr.set_char(std::max(x_c - static_cast<long long>(display_length(u32s_b) - u32s_b.size()), 1LL), y2, UU("┘"));
    }
}

size_t Term::display_length(const std::u32string& u32str) {
    size_t len = 0;
    for (const auto& c : u32str) {
        len += Private::c32_display_width(c);
    }
    return len;
}

size_t Term::display_length(const std::string& str) {
    return display_length(Private::utf8_to_utf32(str));
}

/**
 * Calculate the column number of the cursor in the rendered string, an ascii char is 1 column, an utf-8 char is 2 columns.
 * @param str The string that's on the current line.
 * @param cursor_col The raw cursor column number, one-indexed.
 * @return The rendered cursor column number.
 */
size_t Term::cursor_render_col(const std::string& str, const size_t& cursor_col) {
    return display_length(str.substr(0, cursor_col - 1)) + 1;
}

void Term::render(Term::Window& scr, const Model& m, const std::size_t& cols) {
    scr.clear();
    print_left_curly_bracket(scr, cols, 1, m.lines.size(), m);
    std::u32string u32s_b = Private::utf8_to_utf32(m.lines.back());;
    scr.print_str(std::max(static_cast<long long>(cols) - 6 - static_cast<long long>(display_length(u32s_b) - u32s_b.size()), 1LL),
                  m.lines.size(), std::to_string(m.cursor_row) + "," + std::to_string(m.cursor_col));
    for (std::size_t j = 0; j < m.lines.size(); j++) {
        if (j == 0) {
            scr.fill_fg(1, j + 1, m.prompt_string.size(), m.lines.size(), Term::Color::Name::BrightGreen);
            scr.fill_style(1, j + 1, m.prompt_string.size(), m.lines.size(), Term::Style::BOLD);
            scr.print_str(1, j + 1, m.prompt_string);
        } else {
            for (std::size_t i = 0; i < m.prompt_string.size() - 1; i++) {
                scr.set_char(i + 1, j + 1, '.');
            }
        }
        scr.print_str(m.prompt_string.size() + 1, j + 1, m.lines[j]);
    }
    scr.set_cursor_pos(m.prompt_string.size() + cursor_render_col(m.lines[m.cursor_row - 1], m.cursor_col), m.cursor_row);
}

void replace_all(std::string& str, const std::string& from, const std::string& to) {
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

std::string Term::prompt_multiline(const std::string& prompt_string, std::vector<std::string>& m_history,
                                   const std::function<bool(std::string)>& is_complete) {
    std::size_t row{1}, col{1};
    std::size_t rows{25}, cols{80};
    bool term_attached = Term::is_stdin_a_tty();
    if (stdin_connected()) {
        std::tie(row, col) = cursor_position();
        std::tie(rows, cols) = get_size();
    }
    Model m;
    m.prompt_string = prompt_string;
    //Make a local copy of history that can be modified by the user. All changes will be forgotten once a command is submitted.
    std::vector<std::string> history = m_history;
    std::size_t history_pos = history.size();
    history.push_back(concat(m.lines)); //Push back empty input.
    Term::Window scr(cols, 1);
    Key key{NO_KEY};
    render(scr, m, cols);
    std::cout << scr.render(1, row, term_attached) << std::flush;
    bool not_complete = true;
    while (not_complete) {
        key = static_cast<Key>(Term::read_key());
        if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (Term::is_extended_ASCII(key) && !iscntrl(key))) {
            std::string before = m.lines[m.cursor_row - 1].substr(0, m.cursor_col - 1);
            std::string new_char;
            new_char.push_back(key);
            std::string after = m.lines[m.cursor_row - 1].substr(m.cursor_col - 1);
            m.lines[m.cursor_row - 1] = before += new_char += after;
            m.cursor_col++;
        } else if (key == Key::CTRL_D) {
            if (m.lines.size() == 1 && m.lines[m.cursor_row - 1].empty()) {
                m.lines[m.cursor_row - 1].push_back(Key::CTRL_D);
                std::cout << "\n" << std::flush;
                m_history.push_back(m.lines[0]);
                return m.lines[0];
            }
        } else {
            switch (key) {
                case Key::ENTER:
                    not_complete = !is_complete(concat(m.lines));
                    if (not_complete) {
                        key = Key::ALT_ENTER;
                    } else {
                        break;
                    }
                    CPP_TERMINAL_FALLTHROUGH;
                case Key::BACKSPACE:
                    if (m.cursor_col > 1) {
                        std::string& line = m.lines[m.cursor_row - 1];
                        long long chars_to_erase = calc_cursor_move(line, m.cursor_col, -1);
                        std::string before = line.substr(0, m.cursor_col - 1 + chars_to_erase);
                        std::string after = line.substr(m.cursor_col - 1);
                        line = before + after;
                        m.cursor_col += chars_to_erase;
                    } else if (m.cursor_col == 1 && m.cursor_row > 1) {
                        m.cursor_col = m.lines[m.cursor_row - 2].size() + 1;
                        m.lines[m.cursor_row - 2] += m.lines[m.cursor_row - 1];
                        m.lines.erase(m.lines.begin() + (long long)m.cursor_row - 1);
                        m.cursor_row--;
                    }
                    break;
                case Key::DEL: {
                    std::string& line = m.lines[m.cursor_row - 1];
                    if (m.cursor_col <= line.size()) {
                        std::string before = line.substr(0, m.cursor_col - 1);
                        std::string after = line.substr(m.cursor_col - 1 + calc_cursor_move(line, m.cursor_col, 1));
                        line = before + after;
                    }
                    break;
                }
                case Key::ARROW_LEFT:
                    m.cursor_col += calc_cursor_move(m.lines[m.cursor_row - 1], m.cursor_col, -1);
                    break;
                case Key::ARROW_RIGHT:
                    m.cursor_col += calc_cursor_move(m.lines[m.cursor_row - 1], m.cursor_col, 1);
                    break;
                case Key::HOME:
                    m.cursor_col = 1;
                    break;
                case Key::END:
                    m.cursor_col = m.lines[m.cursor_row - 1].size() + 1;
                    break;
                case Key::ARROW_UP:
                    if (m.cursor_row == 1) {
                        if (history_pos > 0) {
                            history[history_pos] = concat(m.lines);
                            history_pos--;
                            m.lines = split(history[history_pos]);
                            m.cursor_row = m.lines.size();
                            m.cursor_col = m.lines[m.cursor_row - 1].size() + 1;
                            if (m.lines.size() > scr.get_h()) {
                                scr.set_h(m.lines.size());
                            }
                        }
                    } else {
                        m.cursor_row--;
                        size_t col_ = m.lines[m.cursor_row - 1].size() + 1;
                        if (m.cursor_col > col_) {
                            m.cursor_col = col_;
                        } else {
                            m.cursor_col += calc_cursor_move(m.lines[m.cursor_row - 1], m.cursor_col, 0);
                        }
                    }
                    break;
                case Key::ARROW_DOWN:
                    if (m.cursor_row == m.lines.size()) {
                        if (history_pos < history.size() - 1) {
                            history[history_pos] = concat(m.lines);
                            history_pos++;
                            m.lines = split(history[history_pos]);
                            m.cursor_row = 1;
                            m.cursor_col = m.lines[m.cursor_row - 1].size() + 1;
                            if (m.lines.size() > scr.get_h()) {
                                scr.set_h(m.lines.size());
                            }
                        }
                    } else {
                        m.cursor_row++;
                        size_t col_ = m.lines[m.cursor_row - 1].size() + 1;
                        if (m.cursor_col > col_) {
                            m.cursor_col = col_;
                        } else {
                            m.cursor_col += calc_cursor_move(m.lines[m.cursor_row - 1], m.cursor_col, 0);
                        }
                    }
                    break;
                case Key::CTRL_N:
                case Key::ALT_ENTER: {
                    std::string before = m.lines[m.cursor_row - 1].substr(0, m.cursor_col - 1);
                    std::string after = m.lines[m.cursor_row - 1].substr(m.cursor_col - 1);
                    m.lines[m.cursor_row - 1] = before;
                    if (m.cursor_row < m.lines.size()) {
                        //Not at the bottom row, can't push back
                        m.lines.insert(m.lines.begin() + (long long)m.cursor_row, after);
                    } else {
                        m.lines.push_back(after);
                    }
                    m.cursor_col = 1;
                    m.cursor_row++;
                    if (m.lines.size() > scr.get_h()) {
                        scr.set_h(m.lines.size());
                    }
                    break;
                }
                case Key::CTRL_V: {
                    std::string paste_text;
                    clip::get_text(paste_text);
                    if (!paste_text.empty()) {
                        replace_all(paste_text, "\r\n", "\n");
                        replace_all(paste_text, "\r", "\n");
                        std::vector<std::string> split_str = split(paste_text);
                        std::string before = m.lines[m.cursor_row - 1].substr(0, m.cursor_col - 1);
                        std::string after = m.lines[m.cursor_row - 1].substr(m.cursor_col - 1);
                        m.lines[m.cursor_row - 1] = before + split_str[0];
                        for (size_t i = 1; i < split_str.size(); ++i) {
                            m.cursor_row++;
                            if (m.cursor_row <= m.lines.size()) {
                                m.lines.insert(m.lines.begin() + (long long)m.cursor_row - 1, split_str[i]);
                            } else {
                                m.lines.push_back(split_str[i]);
                            }
                        }
                        m.lines[m.cursor_row - 1] += after;
                        m.cursor_col = m.lines[m.cursor_row - 1].size() - after.size() + 1;
                        if (m.lines.size() > scr.get_h()) {
                            scr.set_h(m.lines.size());
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
        render(scr, m, cols);
        std::cout << scr.render(1, row, term_attached) << std::flush;
        if (row + (int)scr.get_h() - 1 > rows) {
            row = rows - ((int)scr.get_h() - 1);
            std::cout << scr.render(1, row, term_attached) << std::flush;
        }
    }
    std::string line_skips;
    for (std::size_t i = 0; i <= m.lines.size() - m.cursor_row; i++) {
        line_skips += "\n";
    }
    std::cout << line_skips << std::flush;
    std::string final_str = concat(m.lines);
    if (m_history.empty() || m_history.back() != final_str) {
        m_history.push_back(final_str);
    }
    return final_str;
}
