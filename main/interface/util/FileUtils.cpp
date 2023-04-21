//
// Created by v2ray on 2023/4/20.
//

#include "FileUtils.h"

namespace file {

    //class file_error start:
    file_error::file_error(std::string message, std::filesystem::path path) : message(std::move(message)), path(std::move(path)) {}
    file_error::~file_error() = default;

    const char* file_error::what() const noexcept {
        return message.c_str();
    }

    const std::filesystem::path& file_error::get_path() const noexcept {
        return path;
    }
    //class file_error end.

    /**
     * Read a file to a string.
     * @param path Path to the file.
     * @throw file::file_error If the file cannot be opened or read.
     * @return File content as a string.
     */
    std::string read_text_file(const std::filesystem::path& path) {
        try {
            if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
                throw file_error("File does not exist or is not a regular file.", path);
            }
            std::ifstream file(path);
            if (!file.is_open() || file.bad()) {
                throw file_error("File cannot be opened.", path);
            }
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (file.bad()) {
                throw file_error("File cannot be read.", path);
            }
            file.close();
            boost::replace_all(content, "\r\n", "\n");
            boost::replace_all(content, "\r", "\n");
            return content;
        } catch (const file_error& e) {
            throw;
        } catch (const std::exception& e) {
            throw file_error(e.what(), path);
        }
    }

    /**
     * Write a string to a file.
     * @param content The string to write.
     * @param path Path to the file.
     * @throw file::file_error If the file cannot be opened or written.
     */
    void write_text_file(const std::string& content, const std::filesystem::path& path) {
        try {
            std::ofstream file(path);
            if (!file.is_open() || file.bad()) {
                throw file_error("File cannot be opened.", path);
            }
            file << content;
            if (file.bad()) {
                throw file_error("File cannot be written.", path);
            }
            file.close();
        } catch (const file_error& e) {
            throw;
        } catch (const std::exception& e) {
            throw file_error(e.what(), path);
        }
    }

    /**
     * Create a folder, including all parent folders.
     * @param folder Path to the folder.
     * @throw file::file_error If an error occurred while creating the folder.
     * @return True if the folder was created, false if it already existed.
     */
    bool create_folder(const std::filesystem::path& folder) {
        try {
            return std::filesystem::create_directories(folder);
        } catch (const std::exception& e) {
            throw file_error(e.what(), folder);
        }
    }

    /**
     * Create multiple folders, including all parent folders.
     * @param folders Vector of paths to the folders.
     * @throw file::file_error If an error occurred while creating a folder.
     * @return Vector of paths to the folders that were created.
     */
    std::vector<std::filesystem::path> create_folders(const std::vector<std::filesystem::path>& folders) {
        std::vector<std::filesystem::path> created_folders;
        for (const auto& folder : folders) {
            if (create_folder(folder)) {
                created_folders.push_back(folder);
            }
        }
        return created_folders;
    }
} // file