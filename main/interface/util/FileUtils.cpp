//
// Created by v2ray on 2023/4/20.
//

#include "FileUtils.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace file {

    std::filesystem::path get_exe_parent_path_intern() {
        std::filesystem::path path;
#ifdef _WIN32
        wchar_t result[MAX_PATH] = {0};
        GetModuleFileNameW(nullptr, result, MAX_PATH);
        path = std::filesystem::path(result);
#else
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        path = std::filesystem::path(std::string(result, count > 0 ? count : 0));
#endif
        return path.parent_path();
    }

    const std::filesystem::path exe_parent_path = get_exe_parent_path_intern();

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
            std::filesystem::path exe_base_path = path.is_absolute() ? path : exe_parent_path / path;
            if (!std::filesystem::exists(exe_base_path) || !std::filesystem::is_regular_file(exe_base_path)) {
                throw file_error("File does not exist or is not a regular file.", path);
            }
            std::ifstream file(exe_base_path);
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
            std::ofstream file(path.is_absolute() ? path : exe_parent_path / path);
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
     * Read binary data from a file.
     * @param path Path to the file.
     * @throw file::file_error If the file cannot be opened or read.
     * @return Binary data as a vector of chars.
     */
    std::vector<char> read_binary_file(const std::filesystem::path& path) {
        try {
            std::filesystem::path exe_base_path = path.is_absolute() ? path : exe_parent_path / path;
            if (!std::filesystem::exists(exe_base_path) || !std::filesystem::is_regular_file(exe_base_path)) {
                throw file_error("File does not exist or is not a regular file.", path);
            }
            std::ifstream file(exe_base_path, std::ios::binary);
            if (!file.is_open() || file.bad()) {
                throw file_error("File cannot be opened.", path);
            }
            std::vector<char> content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (file.bad()) {
                throw file_error("File cannot be read.", path);
            }
            file.close();
            return content;
        } catch (const file_error& e) {
            throw;
        } catch (const std::exception& e) {
            throw file_error(e.what(), path);
        }
    }

    /**
     * Write binary data to a file.
     * @param content The data to write.
     * @param path Path to the file.
     * @throw file::file_error If the file cannot be opened or written.
     */
    void write_binary_file(const std::vector<char>& content, const std::filesystem::path& path) {
        try {
            std::ofstream file(path.is_absolute() ? path : exe_parent_path / path, std::ios::binary);
            if (!file.is_open() || file.bad()) {
                throw file_error("File cannot be opened.", path);
            }
            file.write(content.data(), static_cast<std::streamsize>(content.size()));
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
            return std::filesystem::create_directories(folder.is_absolute() ? folder : exe_parent_path / folder);
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

    /**
     * List all files in a folder, doesn't contain subfolders.
     * @param folder Path to the folder.
     * @throw file::file_error If the folder does not exist, is not a directory, or an error occurred.
     * @return Vector of paths to the files.
     */
    std::unordered_set<std::filesystem::path, path_hash> list_files(const std::filesystem::path& folder) {
        try {
            std::filesystem::path exe_base_path = folder.is_absolute() ? folder : exe_parent_path / folder;
            if (!std::filesystem::exists(exe_base_path) || !std::filesystem::is_directory(exe_base_path)) {
                throw file_error("Folder does not exist or is not a directory.", folder);
            }
            std::unordered_set<std::filesystem::path, path_hash> files;
            for (const auto& entry : std::filesystem::directory_iterator(exe_base_path)) {
                if (std::filesystem::is_regular_file(entry)) {
                    files.insert(entry.path());
                }
            }
            return files;
        } catch (const file_error& e) {
            throw;
        } catch (const std::exception& e) {
            throw file_error(e.what(), folder);
        }
    }

    bool exists(const std::filesystem::path& path) {
        return std::filesystem::exists(path.is_absolute() ? path : exe_parent_path / path);
    }
} // file