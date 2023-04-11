#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <sys/ioctl.h>
#include <unistd.h>
#include "utf8proc.h"
#include "../tty.hpp"
#endif

#include "../exception.hpp"
#include "../platforms/platform.hpp"

std::pair<std::size_t, std::size_t> Term::Private::get_term_size() {
#ifdef _WIN32
    if (GetStdHandle(STD_OUTPUT_HANDLE) == INVALID_HANDLE_VALUE) {
        throw Term::Exception("GetStdHandle(STD_OUTPUT_HANDLE) failed");
    }
    CONSOLE_SCREEN_BUFFER_INFO inf;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &inf)) {
        std::size_t cols = inf.srWindow.Right - inf.srWindow.Left + 1;
        std::size_t rows = inf.srWindow.Bottom - inf.srWindow.Top + 1;
        return {rows, cols};
    } else {
        // This happens when we are not connected to a terminal
        throw Term::Exception("Couldn't get terminal size. Is it connected to a TTY?");
    }
#else
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // This happens when we are not connected to a terminal
        throw Term::Exception("Couldn't get terminal size. Is it connected to a TTY?");
    } else {
        return std::pair<std::size_t, std::size_t>{ws.ws_row, ws.ws_col};
    }
#endif
}

unsigned int Term::Private::c32_display_width(const char32_t& c32) {
#ifdef _WIN32
    if (0x20 <= c32 && c32 <= 0x7e)
        /* ASCII */
        return 1;
    else if (0x3041 <= c32 && c32 <= 0x3094)
        /* Hiragana */
        return 2;
    else if (0x30a1 <= c32 && c32 <= 0x30f6)
        /* Katakana */
        return 2;
    else if (0x3105 <= c32 && c32 <= 0x312c)
        /* Bopomofo */
        return 2;
    else if (0x3131 <= c32 && c32 <= 0x318e)
        /* Hangul Elements */
        return 2;
    else if (0xac00 <= c32 && c32 <= 0xd7a3)
        /* Korean Hangul Syllables */
        return 2;
    else if (0xff01 <= c32 && c32 <= 0xff5e)
        /* Fullwidth ASCII variants */
        return 2;
    else if (0xff61 <= c32 && c32 <= 0xff9f)
        /* Halfwidth Katakana variants */
        return 1;
    else if (
            (0xffa0 <= c32 && c32 <= 0xffbe) ||
            (0xffc2 <= c32 && c32 <= 0xffc7) ||
            (0xffca <= c32 && c32 <= 0xffcf) ||
            (0xffd2 <= c32 && c32 <= 0xffd7) ||
            (0xffda <= c32 && c32 <= 0xffdc))
        /* Halfwidth Hangul variants */
        return 1;
    else if (0xffe0 <= c32 && c32 <= 0xffe6)
        /* Fullwidth symbol variants */
        return 2;
    else if (0x4e00 <= c32 && c32 <= 0x9fa5)
        /* Han Ideographic */
        return 2;
    else if (0xf900 <= c32 && c32 <= 0xfa2d)
        /* Han Ideographic Compatibility */
        return 2;
    else if (0x2000 <= c32 && c32 <= 0x206f)
        /* General Punctuation */
        return 1;
    else if (0x2070 <= c32 && c32 <= 0x209f)
        /* Superscripts and Subscripts */
        return 1;
    else if (0x20a0 <= c32 && c32 <= 0x20cf)
        /* Currency Symbols */
        return 1;
    else if (0x20d0 <= c32 && c32 <= 0x20ff)
        /* Combining Diacritical Marks for Symbols */
        return 0;
    else if (0x2100 <= c32 && c32 <= 0x214f)
        /* Letterlike Symbols */
        return 1;
    else if (
            (0x00C0 <= c32 && c32 <= 0x00FF) || // Latin-1 Supplement (covers French accented letters)
            (0x0100 <= c32 && c32 <= 0x017F))   // Latin Extended-A (additional French accented letters)
        /* French letters */
        return 2;
    else if (0x0250 <= c32 && c32 <= 0x02AF)
        /* IPA Extensions */
        return 1;
    else if (0x0442 == c32)
        /* Cyrillic small letters */
        return 1;
    else if (0x0400 <= c32 && c32 <= 0x04FF)
        /* Cyrillic (covers Russian letters) */
        return 2;
    //Add more Unicode ranges if necessary.

    return 2;
#else
    return utf8proc_charwidth(static_cast<wchar_t>(c32));
#endif
}
