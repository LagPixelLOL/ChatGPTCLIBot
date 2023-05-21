#include "GPTMain.h"
//Big thanks to https://github.com/jupyter-xeus/cpp-terminal for cross-platform terminal support.

void setup_console_encoding();
void shutdown_hook();

int main() {
    try {
        atexit(shutdown_hook); //Setup shutdown hook.
        curl_global_init(CURL_GLOBAL_ALL); //CURL global init.
        setup_console_encoding(); //Setup console encoding.
//Set to 1 to run tests.
#if 0
#define GPT_MAIN_TEST_ENABLED
#endif
#ifndef GPT_MAIN_TEST_ENABLED
        //Main
        GPT::pre_settings(); //Start the bot.
#else
        //Test
        std::cout << "Running test..." << std::endl;
        //doc::test_split_text();

        std::vector<std::shared_ptr<curl::RequestObject>> requests;
        for (uint32_t i = 0; i < 100; i++) {
            auto request = std::make_shared<curl::RequestObject>(curl::RequestObject::construct_http_get( //Potential exception throw.
                    "https://httpbin.org/ip", [i](const std::vector<char>& vec, CURL*){
                std::cout << "#" << i << ": " << std::string(vec.begin(), vec.end()) << std::endl;
            }, {}, 5));
            requests.push_back(request);
        }
        curl::multi_perform(requests); //Potential exception throw.
        for (const auto& request : requests) {
            try {
                request->get_exception(); //Potential exception throw.
            } catch (const std::exception& e) {
                std::cout << "Exception: " << e.what() << std::endl;
            }
        }

        std::cout << "Test finished." << std::endl;
#endif
    } catch (const std::exception& e) {
#ifndef GPT_MAIN_TEST_ENABLED
        util::println_err("An uncaught error occurred: " + std::string(e.what()));
#else
        std::cout << "An uncaught error occurred: " << e.what() << std::endl;
#endif
    } catch (...) {
#ifndef GPT_MAIN_TEST_ENABLED
        util::println_err("An unknown error occurred.");
#else
        std::cout << "An unknown error occurred." << std::endl;
#endif
    }
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
#ifndef GPT_MAIN_TEST_ENABLED
    util::initialize_or_throw();
#endif
}

void shutdown_hook() {
    curl_global_cleanup();
    util::free_proxy_factory();
}
