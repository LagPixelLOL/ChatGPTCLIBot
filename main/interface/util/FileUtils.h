//
// Created by v2ray on 2023/4/20.
//

#ifndef GPT3BOT_FILEUTILS_H
#define GPT3BOT_FILEUTILS_H

#include "boost/algorithm/string.hpp"
#include "filesystem"
#include "fstream"
#include "vector"

namespace file {

    class file_error : public std::exception {
        const std::string message;
        const std::filesystem::path path;

    public:
        file_error() = delete;
        file_error(std::string message, std::filesystem::path path);
        ~file_error() override;

        [[nodiscard]] const char* what() const noexcept override;
        [[nodiscard]] const std::filesystem::path& get_path() const noexcept;
    };

    std::string read_text_file(const std::filesystem::path& path);
    void write_text_file(const std::string& content, const std::filesystem::path& path);
    std::vector<char> read_binary_file(const std::filesystem::path& path);
    void write_binary_file(const std::vector<char>& content, const std::filesystem::path& path);
    bool create_folder(const std::filesystem::path& folder);
    std::vector<std::filesystem::path> create_folders(const std::vector<std::filesystem::path>& folders);
} // file

#endif //GPT3BOT_FILEUTILS_H
