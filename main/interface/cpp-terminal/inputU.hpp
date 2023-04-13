//
// Created by v2ray on 2023/4/13.
//

#ifndef GPT3BOT_INPUTU_HPP
#define GPT3BOT_INPUTU_HPP

#include "cstdint"

namespace Term {

    enum KeyU : int32_t {
        NO_KEY    = -1,
        //Begin Unicode (Basic Multilingual Plane).
        NUL       = 0x0000,
        CTRL_A    = 0x0001,
        CTRL_B    = 0x0002,
        CTRL_C    = 0x0003,
        CTRL_D    = 0x0004,
        CTRL_E    = 0x0005,
        CTRL_F    = 0x0006,
        CTRL_G    = 0x0007,
        BACKSPACE = 0x0008,
        TAB       = 0x0009,
        ENTER     = 0x000A,
        LF        = 0x000A,
        CTRL_K    = 0x000B,
        CTRL_L    = 0x000C,
        CR        = 0x000D,
        CTRL_N    = 0x000E,
        CTRL_O    = 0x000F,
        CTRL_P    = 0x0010,
        CTRL_Q    = 0x0011,
        CTRL_R    = 0x0012,
        CTRL_S    = 0x0013,
        CTRL_T    = 0x0014,
        CTRL_U    = 0x0015,
        CTRL_V    = 0x0016,
        CTRL_W    = 0x0017,
        CTRL_X    = 0x0018,
        CTRL_Y    = 0x0019,
        CTRL_Z    = 0x001A,
        ESC       = 0x001B,
        SPACE     = 0x0020,
        DEL       = 0x007F,
        //...(Other Unicode characters)

        //Special keys(Need to be greater than 0x10FFFF).
        ALT_ENTER = 0x110000,
        ARROW_LEFT,
        ARROW_RIGHT,
        ARROW_UP,
        ARROW_DOWN,
        CTRL_UP,
        CTRL_DOWN,
        CTRL_RIGHT,
        CTRL_LEFT,
        NUMERIC_5,
        HOME,
        INSERT,
        END,
        PAGE_UP,
        PAGE_DOWN,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        CTRL = -64,
        //Now use << to for detecting special key + key press.
        ALT = (1 << 21)
    };

    namespace Platform {

        bool read_raw_u(char32_t* c32);
        bool is_character_u(const KeyU& key);
        bool is_CTRL_u(const KeyU& key);
        int32_t read_key_u();
        int32_t read_key0_u();
    }
} // Term::Platform

#endif //GPT3BOT_INPUTU_HPP
