# NOTICE: NO LONGER MAINTAINED
Goodbye everyone!

As you might have noticed, the landscape of LLMs has changed dramatically in recent months. With the introduction of models boasting a whopping 100,000+ tokens of context memory and the big players like OpenAI, Google, and Anthropic integrating RAG directly into their official websites, I feel like this repo has served its purpose.

I've also been focusing on other projects and prioritizing my personal life and mental well-being. As a result, I haven't been able to dedicate time to updating this repo, which is evident from the lack of commits in the past few months.

Looking back, this repo was initially created to address the limitations of the 4096 context GPT-3, as that context size was simply too small. However, with the advancements in LLMs and the emergence of better GUIs, I believe that higher context is now preferable to using embeddings for RAG.

If you're looking for some awesome LLM GUIs, here are my top recommendations:
- [SillyTavern](https://github.com/SillyTavern/SillyTavern) - Perfect for ~~bot fooking~~ roleplaying and even has RAG support!
- [LobeChat](https://github.com/lobehub/lobe-chat) - Great for general-purpose LLM usage, although it doesn't have RAG.
- [BetterChatGPT](https://github.com/ztjhz/BetterChatGPT) - Another fantastic option for general-purpose LLM usage, specifically designed for OpenAI. While it's not actively maintained, the UI closely resembles the original ChatGPT website.

(Plz don't unstar this repo🥺)

# ChatGPT CLI Bot
Run `gpt-3.5-turbo` or any other GPT models(`text-davinci-003`) with this program! \
Use `gpt-4` or `gpt-4-32k` to use the new GPT-4 models if you have access. \
You can switch models in the `config.json` file. \
It's like https://chat.openai.com/ but in your CMD and better(in terms of memory). \
You can add custom initial prompts and save/load your chat history! \
Download and double-click the `GPT3Bot.exe` or `run.bat` to run the program! \
In Linux and macOS, you can run `./GPT3Bot` to run the program. \
\
Click to download: [Stable Release](https://github.com/LagPixelLOL/ChatGPTCLIBot/releases) | [Development Build](https://github.com/LagPixelLOL/ChatGPTCLIBot/actions) \
Please check the Wiki for more information: [Click Me](https://github.com/LagPixelLOL/ChatGPTCLIBot/wiki)

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
17. Fine tune helper, you can fine tune base models more easily(Only for professional users).
18. Auto translator, you can translate text files automatically.

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
