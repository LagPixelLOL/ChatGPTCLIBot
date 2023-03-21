#include "GPTMain.h"
//Big thanks to https://github.com/jupyter-xeus/cpp-terminal for cross-platform terminal support.

void setup_console_encoding();
void shutdown_hook();

int main() {
    atexit(shutdown_hook); //Setup shutdown hook.
    curl_global_init(CURL_GLOBAL_ALL); //CURL global init.

    setup_console_encoding(); //Setup console encoding.

    //Main
    GPT::pre_settings(); //Start the bot.
    return 0;
}

/**
 * Setup console encoding to correctly display UTF-8 characters.
 */
void setup_console_encoding() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    CONSOLE_FONT_INFOEX info = {0};
    info.cbSize = sizeof(info);
    info.dwFontSize.Y = 16;
    info.FontWeight = FW_NORMAL;
    wcscpy_s(info.FaceName, L"SimSun-ExtB");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), 0, &info);
#endif
}

void shutdown_hook() {
    curl_global_cleanup();
    util::free_proxy_factory();
}
