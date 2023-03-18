# ChatGPT CLI Bot
Run `gpt-3.5-turbo` or any other GPT-3 models(`text-davinci-003`) with this program! \
Use `gpt-4` or `gpt-4-32k` to use the new GPT-4 models if you have access. \
You can switch models in the `config.json` file. \
It's like https://chat.openai.com/ but in your CMD and better(in terms of memory). \
You can add custom initial prompts and save/load your chat history! \
Download and double-click the `GPT3Bot.exe` or `run.bat` to run the program! \
In Linux and macOS, you can run `./GPT3Bot` to run the program.

# Features/Manual:
1. **Long term memory support!** Keep hitting the 4096 tokens context limit? Worry no more with this CLI Bot. It has nearly INFINITE context memory(If you have infinite disk space lol), all thanks to Embeddings! If you want to see how this program handles embeddings internally, set `debug_reference` to `true` in `config.json`!
2. You can use `/stop` to end and save the chat.
3. You can use `/undo` to undo your last prompt.
4. You can use `/reset` to reset your entire chat.
5. You can place .txt files in the "initial" folder to set different initial prompts, and you can use the filename to load it when you open the program. Simply directly press enter after you open the program, then enter the initial prompt file's name and press enter to load it.
6. After you execute `/stop`, the program will ask you to input the filename to save. You can press enter directly to skip this and not save the chat. If you input any other text and then press enter, the chat will be saved into a json in the "saved" folder. When you open the program next time, you can simply input "s"(which means saved), press enter, then type the saved chat's json file's name to load your saved chat.
7. Easy config file in `config.json`, can be easily modified.
8. Unlike other bots, this one actually streams. This means it will display the output as soon as a token is sent from the API(Just like what ChatGPT's website is doing), no need to wait until the entire response is generated!
9. Automatically use the system proxy. Note: This feature is only supported on Windows, because there's a bug in my proxy library that causes it fail to compile on Linux and macOS.
10. Multiline input support, you need to press Ctrl+N or Alt+Enter to enter a new line.
11. Ctrl+V pasting support, you can paste text from your clipboard by pressing Ctrl+V.

Written in C++ (Libraries used: Boost, cURL, nlohmann/json, libproxy, cpp-terminal, ftxui, clip)

# Supported OS:
* Windows 10/11 64-bit
* Linux 64-bit (Tested on Ubuntu 20.04 & CentOS 8) (Will not work on Ubuntu 18.04, CentOS 7 and lower, because they don't support C++17)
* macOS 64-bit (Didn't test, but it should work on macOS 12 and higher)