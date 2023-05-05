# ChatGPT CLI Bot
Run `gpt-3.5-turbo` or any other GPT models(`text-davinci-003`) with this program! \
Use `gpt-4` or `gpt-4-32k` to use the new GPT-4 models if you have access. \
You can switch models in the `config.json` file. \
It's like https://chat.openai.com/ but in your CMD and better(in terms of memory). \
You can add custom initial prompts and save/load your chat history! \
Download and double-click the `GPT3Bot.exe` or `run.bat` to run the program! \
In Linux and macOS, you can run `./GPT3Bot` to run the program. \
\
Please check the Wiki for more info: [Click Me](https://github.com/LagPixelLOL/ChatGPTCLIBot/wiki)

# Features/Manual:
1. **Long term memory support!** Keep hitting the 4096 tokens context limit? Worry no more with this CLI Bot. It has nearly INFINITE context memory(If you have infinite disk space lol), all thanks to Embeddings! If you want to see how this program handles embeddings internally, set `debug_reference` to `true` in `config.json`!
2. **Q&A with custom documents support!** You can load custom documents, and perform Q&A with them, please check the Wiki for more info.
3. You can use `/stop` to end and save the chat.
4. You can use `/undo` to undo your last prompt.
5. You can use `/reset` to reset your entire chat.
6. You can use `/dump` to dump your chat history to a .txt file inside the `dump` folder.
7. You can place .txt files in the "initial" folder to set different initial prompts, and you can use the filename to load it when you open the program. Simply directly press enter after you open the program, then enter the initial prompt file's name and press enter to load it.
8. After you execute `/stop`, the program will ask you to input the filename to save. You can press enter directly to skip this and not save the chat. If you input any other text and then press enter, the chat will be saved into a json in the "saved" folder. When you open the program next time, you can simply input "s"(which means saved), press enter, then type the saved chat's json file's name to load your saved chat.
9. Easy config file in `config.json`, can be easily modified.
10. Unlike other bots, this one actually streams. This means it will display the output as soon as a token is sent from the API(Just like what ChatGPT's website is doing), no need to wait until the entire response is generated!
11. When the response is being streamed, you can press Ctrl+C to cancel the stream.
12. Automatically use the system proxy. Note: This feature is only supported on Windows, because there's a bug in my proxy library that causes it fail to compile on Linux and macOS.
13. Multiline input support, you need to press Ctrl+N or Alt+Enter to enter a new line.
14. Ctrl+V pasting support, you can paste text from your clipboard by pressing Ctrl+V.
15. Full UTF-8 support, you can type in any language you want!
16. Full of colors(If your terminal supports it)!
17. Fine tune helper, you can fine tune base models with this program(Only for professional users).

Written in C++ (Libraries used:
[Boost](https://www.boost.org/),
[cURL](https://curl.se/),
[nlohmann/json](https://github.com/nlohmann/json),
[libproxy](https://libproxy.github.io/libproxy/),
[cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal),
[ftxui](https://github.com/ArthurSonzogni/FTXUI),
[oneTBB](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html),
[clip](https://github.com/dacap/clip),
[cpp-tiktoken](https://github.com/gh-markt/tiktoken),
[pcre2](https://www.pcre.org/),
[utf8proc](https://juliastrings.github.io/utf8proc/))

# Supported OS:
* Windows 10/11 64-bit
* Linux 64-bit (Tested on Ubuntu 20.04 & CentOS 8) (Won't work on Ubuntu 18.04, CentOS 7 and lower, because they don't support C++17)
* macOS 64-bit (Didn't test, but it should work on macOS 12 and higher)