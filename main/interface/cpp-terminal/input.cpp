// Bug in some GCC
#if !defined(_GLIBCXX_USE_NANOSLEEP)
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include "input.hpp"

#include <thread>
#include <type_traits>

bool Term::is_ASCII(const Term::Key& key) {
    if (key >= 0 && key <= 127) {
        return true;
    }
    return false;
}

bool Term::is_extended_ASCII(const Term::Key& key) {
    if (key >= 0 && key <= 255) {
        return true;
    }
    return false;
}

bool Term::is_CTRL(const Term::Key& key) {
    // Need to suppress the TAB etc...
    if (key > 0 && key <= 31 && key != BACKSPACE && key != TAB
    && key != ESC && /* the two mapped to ENTER */ key != LF && key != CR) {
        return true;
    }
    return false;
}

bool Term::is_ALT(const Term::Key& key) {
    if ((key & ALT) == ALT) {
        return true;
    }
    return false;
}

int32_t Term::read_key() {
    int32_t key;
    while ((key = read_key0()) == NO_KEY) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return key;
}

int32_t Term::read_key0() {
    char c{};
    if (!Platform::read_raw(&c)) {
        return NO_KEY;
    }
    if (is_CTRL(static_cast<Term::Key>(c))) {
        return c;
    } else if (c == ESC) {
        char seq[4]{'\0', '\0', '\0', '\0'};
        if (!Platform::read_raw(&seq[0])) {
            return ESC;
        }
        if (!Platform::read_raw(&seq[1])) {
            if (seq[0] >= 'a' && seq[0] <= 'z') {
                // gnome-term, Windows Console
                return ALT + seq[0];
            }
            if (seq[0] == '\x0d') {
                // gnome-term
                return ALT_ENTER;
            }
            return -1;
        }
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (!Platform::read_raw(&seq[2])) {
                    return -2;
                }
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME;
                        case '2': return INSERT;
                        case '3': return DEL;
                        case '4': return END;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME;
                        case '8': return END;
                    }
                } else if (seq[2] == ';') {
                    if (seq[1] == '1') {
                        if (!Platform::read_raw(&seq[2])) {
                            return -10;
                        }
                        if (!Platform::read_raw(&seq[3])) {
                            return -11;
                        }
                        if (seq[2] == '5') {
                            switch (seq[3]) {
                                case 'A': return CTRL_UP;
                                case 'B': return CTRL_DOWN;
                                case 'C': return CTRL_RIGHT;
                                case 'D': return CTRL_LEFT;
                            }
                        }
                        return -12;
                    }
                } else {
                    if (seq[2] >= '0' && seq[2] <= '9') {
                        if (!Platform::read_raw(&seq[3])) {
                            return -3;
                        }
                        if (seq[3] == '~') {
                            if (seq[1] == '1') {
                                switch (seq[2]) {
                                    case '5': return F5;
                                    case '7': return F6;
                                    case '8': return F7;
                                    case '9': return F8;
                                }
                            } else if (seq[1] == '2') {
                                switch (seq[2]) {
                                    case '0': return F9;
                                    case '1': return F10;
                                    case '3': return F11;
                                    case '4': return F12;
                                }
                            }
                        }
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'E': return NUMERIC_5;
                    case 'H': return HOME;
                    case 'F': return END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'F': return END;
                case 'H': return HOME;
                case 'P': return F1;
                case 'Q': return F2;
                case 'R': return F3;
                case 'S': return F4;
            }
        }
        return -4;
    } else {
        switch (c) {
            case DEL: return BACKSPACE;
            case LF:
            case CR: return ENTER;
            default: break;
        } if (c == '\xc3') {
            if (!Platform::read_raw(&c)) {
                return -8;
            } else {
                if (c >= '\xa1' && c <= '\xba') {
                    // xterm
                    return ALT + (c + 'a' - '\xa1');
                }
                return -9;
            }
        } else if (c == '\xc2') {
            if (!Platform::read_raw(&c)) {
                return -10;
            } else {
                if (c == '\x8d') {
                    // xterm
                    return ALT_ENTER;
                }
                return -11;
            }
        }
        return c;
    }
}

//Returns the whole input from STDIN as string.
std::string Term::read_stdin() {
    std::string file;
    char c; //No need to initialize.
    while (true) {
        c = Platform::read_raw_stdin();
        if (c == 0x04) {
            return file;
        } else {
            file.push_back(c);
        }
    }
}
