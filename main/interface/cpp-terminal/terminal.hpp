#pragma once

#include "terminfo.hpp"

namespace Term {

    /* Note: the code that uses Terminal must be inside try/catch block, otherwise
     * the destructors will not be called when an exception happens and the
     * terminal will not be left in a good state. Terminal uses exceptions when
     * something goes wrong.
     */
    class Terminal {
        bool clear_screen{};
        bool disable_signal_keys{true};
        bool hide_cursor{};
        Term::Terminfo m_terminfo;

        void store_and_restore();
        void setRawMode();

    public:
        explicit Terminal(const bool& _clear_screen = false, const bool& _disable_signal_keys = true, const bool& _hide_cursor = false);
        ~Terminal();
    };
} // namespace Term
