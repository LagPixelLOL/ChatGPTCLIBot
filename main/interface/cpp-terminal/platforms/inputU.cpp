//
// Created by v2ray on 2023/4/13.
//

#include "../inputU.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#endif

#include "../exception.hpp"
#include "../tty.hpp"
#include "../prompt.hpp"
#include "conversion.hpp"

#include "../../clip/clip.h"
#include "thread"

namespace Term {

    std::string prompt_multiline(const std::string& prompt_string, std::vector<std::string>& m_history,
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
        Window scr(cols, 1);
        KeyU key; //No need to initialize.
        std::pair<std::size_t, std::size_t> skip_info = render(scr, m, cols);
        std::cout << scr.render(1, row, term_attached) << std::flush;
        bool not_complete = true;
        while (not_complete) {
            key = static_cast<KeyU>(Platform::read_key_u());
            if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')
            || (Platform::is_character_u(key) && !Platform::is_control_char(key))) {
                std::string before = m.lines[m.cursor_row - 1].substr(0, m.cursor_col - 1);
                std::string after = m.lines[m.cursor_row - 1].substr(m.cursor_col - 1);
                std::string new_char;
                Private::codepoint_to_utf8(new_char, key);
                m.lines[m.cursor_row - 1] = before += new_char += after;
                m.cursor_col += calc_cursor_move(m.lines[m.cursor_row - 1], m.cursor_col, 1);
            } else if (key == KeyU::CTRL_D) {
                if (m.lines.size() == 1 && m.lines[m.cursor_row - 1].empty()) {
                    m.lines[m.cursor_row - 1].push_back(KeyU::CTRL_D);
                    std::cout << "\n" << std::flush;
                    m_history.push_back(m.lines[0]);
                    return m.lines[0];
                }
            } else {
                switch (key) {
                    case KeyU::ENTER:
                        not_complete = !is_complete(concat(m.lines));
                        if (!not_complete) {
                            break;
                        }
                        [[fallthrough]];
                    case KeyU::BACKSPACE:
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
                    case KeyU::DEL: {
                        std::string& line = m.lines[m.cursor_row - 1];
                        if (m.cursor_col <= line.size()) {
                            std::string before = line.substr(0, m.cursor_col - 1);
                            std::string after = line.substr(m.cursor_col - 1 + calc_cursor_move(line, m.cursor_col, 1));
                            line = before + after;
                        }
                        break;
                    }
                    case KeyU::ARROW_LEFT:
                        m.cursor_col += calc_cursor_move(m.lines[m.cursor_row - 1], m.cursor_col, -1);
                        break;
                    case KeyU::ARROW_RIGHT:
                        m.cursor_col += calc_cursor_move(m.lines[m.cursor_row - 1], m.cursor_col, 1);
                        break;
                    case KeyU::HOME:
                        m.cursor_col = 1;
                        break;
                    case KeyU::END:
                        m.cursor_col = m.lines[m.cursor_row - 1].size() + 1;
                        break;
                    case KeyU::ARROW_UP:
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
                    case KeyU::ARROW_DOWN:
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
                    case KeyU::CTRL_N:
                    case KeyU::ALT_ENTER: {
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
                    case KeyU::CTRL_V: {
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
            skip_info = render(scr, m, cols);
            std::cout << scr.render(1, row, term_attached) << std::flush;
            if (row + static_cast<int>(scr.get_h()) - 1 > rows) {
                row = rows - (static_cast<int>(scr.get_h()) - 1);
                std::cout << scr.render(1, row, term_attached) << std::flush;
            }
        }
        std::string line_skips;
        for (std::size_t i = 0; i <= skip_info.first - skip_info.second; i++) {
            line_skips += "\n";
        }
        std::cout << line_skips << std::flush;
        std::string final_str = concat(m.lines);
        if (m_history.empty() || m_history.back() != final_str) {
            m_history.push_back(final_str);
        }
        return final_str;
    }
    
    namespace Platform {

        bool read_raw_u(char32_t* c32) {
            //Do nothing when TTY is not connected.
            if (!is_stdin_a_tty()) {
                return false;
            }
#ifdef _WIN32
            DWORD n_read = 0;
            GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &n_read);
            if (n_read >= 1) {
                INPUT_RECORD buf;
                if (!ReadConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &buf, 1, &n_read)) {
                    throw Exception("ReadConsoleInput() failed.");
                }
                if (n_read == 1) {
                    switch (buf.EventType) {
                        case KEY_EVENT: {
                            WORD skip = buf.Event.KeyEvent.wVirtualKeyCode; //Skip them for now.
                            if (skip == VK_SHIFT || skip == VK_LWIN || skip == VK_RWIN || skip == VK_APPS
                            || skip == VK_CONTROL || skip == VK_MENU || skip == VK_CAPITAL) {
                                return false;
                            }
                            if (buf.Event.KeyEvent.bKeyDown) {
                                *c32 = buf.Event.KeyEvent.uChar.UnicodeChar;
                                return true;
                            } else {
                                return false;
                            }
                        }
                        case FOCUS_EVENT:
                        case MENU_EVENT:
                        case MOUSE_EVENT:
                        case WINDOW_BUFFER_SIZE_EVENT:
                        default:
                            return false;
                    }
                } else {
                    throw Exception("kbhit() and ReadConsoleInput() inconsistent.");
                }
            } else {
                return false;
            }
#else
            std::vector<char> buf(1, 0); //Maximum 4 bytes for a single UTF-8 encoded character.
            ::ssize_t n_read_u = ::read(0, &buf[0], 1);
            if (n_read_u == -1 && errno != EAGAIN) {
                throw Exception("read() failed.");
            }
            //Check the number of bytes needed to complete the UTF-8 character.
            auto first_byte = static_cast<unsigned char>(buf[0]);
            int bytes_to_read = 0;
            if (first_byte >> 7 == 0) {
                bytes_to_read = 0;
            } else if (first_byte >> 5 == 0b110) {
                bytes_to_read = 1;
            } else if (first_byte >> 4 == 0b1110) {
                bytes_to_read = 2;
            } else if (first_byte >> 3 == 0b11110) {
                bytes_to_read = 3;
            } else {
                return false;
            }
            //Read the remaining bytes.
            for (int i = 0; i < bytes_to_read; i++) {
                char c = 0;
                n_read_u = ::read(0, &c, 1);
                if (n_read_u == -1 && errno != EAGAIN) {
                    throw Exception("read() failed.");
                }
                buf.push_back(c);
            }
            std::string s8(buf.begin(), buf.end());
            std::u32string s32 = Private::utf8_to_utf32(s8);
            if (s32.empty()) {
                return false;
            }
            *c32 = s32[0];
            return n_read_u > 0;
#endif
        }

        bool is_character_u(const KeyU& key) {
            if (key >= 0 && key <= 0x10FFFF) {
                return true;
            }
            return false;
        }

        bool is_CTRL_u(const KeyU& key) {
            //Need to suppress the TAB etc...
            if (key > 0 && key <= 31 && key != BACKSPACE && key != TAB
            && key != ESC && /* the two mapped to ENTER */ key != LF && key != CR) {
                return true;
            }
            return false;
        }

        bool is_control_char(const char32_t& c32) {
            //Control characters are in the ranges U+0000 to U+001F and U+007F to U+009F.
            if ((c32 >= 0x00 && c32 <= 0x1F) || (c32 >= 0x7F && c32 <= 0x9F)) {
                return true;
            }
            return false;
        }

        int32_t read_key_u() {
            int32_t key;
            while ((key = read_key0_u()) == NO_KEY) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            return key;
        }

        int32_t read_key0_u() {
            char32_t c32{};
            if (!read_raw_u(&c32)) {
                return NO_KEY;
            }
            if (is_CTRL_u(static_cast<KeyU>(c32))) {
                return static_cast<int32_t>(c32);
            } else if (c32 == ESC) {
                char32_t seq[4]{0, 0, 0, 0};
                if (!read_raw_u(&seq[0])) {
                    return ESC;
                }
                if (!read_raw_u(&seq[1])) {
                    if (seq[0] >= U'a' && seq[0] <= U'z') {
                        // gnome-term, Windows Console
                        return static_cast<int32_t>(ALT + seq[0]);
                    }
                    if (seq[0] == U'\x0d') {
                        // gnome-term
                        return ALT_ENTER;
                    }
                    return -1;
                }
                if (seq[0] == U'[') {
                    if (seq[1] >= U'0' && seq[1] <= U'9') {
                        if (!read_raw_u(&seq[2])) {
                            return -2;
                        }
                        if (seq[2] == U'~') {
                            switch (seq[1]) {
                                case U'1': return HOME;
                                case U'2': return INSERT;
                                case U'3': return DEL;
                                case U'4': return END;
                                case U'5': return PAGE_UP;
                                case U'6': return PAGE_DOWN;
                                case U'7': return HOME;
                                case U'8': return END;
                            }
                        } else if (seq[2] == U';') {
                            if (seq[1] == U'1') {
                                if (!read_raw_u(&seq[2])) {
                                    return -10;
                                }
                                if (!read_raw_u(&seq[3])) {
                                    return -11;
                                }
                                if (seq[2] == U'5') {
                                    switch (seq[3]) {
                                        case U'A': return CTRL_UP;
                                        case U'B': return CTRL_DOWN;
                                        case U'C': return CTRL_RIGHT;
                                        case U'D': return CTRL_LEFT;
                                    }
                                }
                                return -12;
                            }
                        } else {
                            if (seq[2] >= U'0' && seq[2] <= U'9') {
                                if (!read_raw_u(&seq[3])) {
                                    return -3;
                                }
                                if (seq[3] == U'~') {
                                    if (seq[1] == U'1') {
                                        switch (seq[2]) {
                                            case U'5': return F5;
                                            case U'7': return F6;
                                            case U'8': return F7;
                                            case U'9': return F8;
                                        }
                                    } else if (seq[1] == U'2') {
                                        switch (seq[2]) {
                                            case U'0': return F9;
                                            case U'1': return F10;
                                            case U'3': return F11;
                                            case U'4': return F12;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        switch (seq[1]) {
                            case U'A': return ARROW_UP;
                            case U'B': return ARROW_DOWN;
                            case U'C': return ARROW_RIGHT;
                            case U'D': return ARROW_LEFT;
                            case U'E': return NUMERIC_5;
                            case U'H': return HOME;
                            case U'F': return END;
                        }
                    }
                } else if (seq[0] == U'O') {
                    switch (seq[1]) {
                        case U'F': return END;
                        case U'H': return HOME;
                        case U'P': return F1;
                        case U'Q': return F2;
                        case U'R': return F3;
                        case U'S': return F4;
                    }
                }
                return -4;
            } else {
                switch (c32) {
                    case DEL: return BACKSPACE;
                    case LF:
                    case CR: return ENTER;
                    default: break;
                } if (c32 == U'\xc3') {
                    if (!read_raw_u(&c32)) {
                        return -8;
                    } else {
                        if (c32 >= U'\xa1' && c32 <= U'\xba') {
                            // xterm
                            return static_cast<int32_t>(ALT + (c32 + U'a' - U'\xa1'));
                        }
                        return -9;
                    }
                } else if (c32 == U'\xc2') {
                    if (!read_raw_u(&c32)) {
                        return -10;
                    } else {
                        if (c32 == U'\x8d') {
                            // xterm
                            return ALT_ENTER;
                        }
                        return -11;
                    }
                }
                return static_cast<int32_t>(c32);
            }
        }
    } // namespace Platform
} // namespace Term
