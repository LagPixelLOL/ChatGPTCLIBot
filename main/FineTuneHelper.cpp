//
// Created by v2ray on 2023/4/26.
//

#include "FineTuneHelper.h"

namespace fth {
    const std::filesystem::path f_finetune = "finetune";
    const std::filesystem::path f_download = "download";
    const std::string& json_suffix = GPT::get_json_suffix();
    const std::string url_files = "https://api.openai.com/v1/files";
    const std::string url_fine_tunes = "https://api.openai.com/v1/fine-tunes";
    const std::string url_models = "https://api.openai.com/v1/models";
    
    inline void print_api_response(const std::string& response) {
        std::cout << "API response:\n" << response << (response.ends_with('\n') ? "" : "\n");
    }

    inline bool create_folder(const std::filesystem::path& path) {
        try {
            if (file::create_folder(path)) {
                util::println_info("Created folder: " + PATH_S(path));
            }
        } catch (const file::file_error& e) {
            util::println_err("Error creating folder: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return false;
        }
        return true;
    }

    inline void convert() {
        if (!create_folder(f_finetune)) {
            return;
        }
        std::cout << "Please enter the source's filename you want to load: ";
        std::string s_filename;
        getline(std::cin, s_filename);
        if (boost::ends_with(s_filename, json_suffix)) {
            s_filename.erase(s_filename.size() - json_suffix.size());
        }
        const auto& path_ = f_finetune / (s_filename + json_suffix);
        util::println_info("Loading source from file: " + PATH_S(path_));
        std::string content;
        try {
            content = file::read_text_file(path_);
        } catch (const file::file_error& e) {
            util::println_err("Error reading source file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        std::cout << "Please enter the source type.\n(";
        for (uint32_t i = 0; i < static_cast<uint32_t>(ft::SourceType::UNKNOWN); i++) {
            util::print_cs((i == 0 ? "" : ", ") + GOLDEN_TEXT(ft::to_string(static_cast<ft::SourceType>(i))));
        }
        std::cout << "): ";
        std::string source_type_in;
        getline(std::cin, source_type_in);
        static const std::string me_id = GPT::get_me_id() + ": ";
        static const std::string bot_id = GPT::get_bot_id() + ":"; //bot_id doesn't end with space because of tokenization reasons.
        std::cout << "Do you want to use the chat format(" << me_id << "and " << bot_id << ")? (y/n): ";
        std::string use_chat_format_yn;
        getline(std::cin, use_chat_format_yn);
        std::transform(use_chat_format_yn.begin(), use_chat_format_yn.end(), use_chat_format_yn.begin(), tolower);
        bool use_chat_format = use_chat_format_yn != "n";
        util::println_info("Converting source...");
        nlohmann::json result;
        try {
            result = use_chat_format ? ft::convert(content, ft::from_string(source_type_in), me_id, "\n" + bot_id)
                    : ft::convert(content, ft::from_string(source_type_in));
        } catch (const std::exception& e) {
            util::println_err("Error converting source: " + std::string(e.what()));
            return;
        }
        try {
            const auto& w_path = f_finetune / (s_filename + "_converted" + json_suffix);
            util::println_info("Writing converted source to file: " + PATH_S(w_path));
            file::write_text_file(result.dump(2), w_path);
        } catch (const file::file_error& e) {
            util::println_err("Error writing converted source file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        util::println_info("Conversion completed.");
    }

    inline void upload() {
        std::cout << "Please enter the converted source's filename you want to upload: ";
        std::string cs_filename;
        getline(std::cin, cs_filename);
        if (boost::ends_with(cs_filename, json_suffix)) {
            cs_filename.erase(cs_filename.size() - json_suffix.size());
        }
        const auto& path_ = f_finetune / (cs_filename + json_suffix);
        util::println_info("Loading converted source from file: " + PATH_S(path_));
        std::string content;
        try {
            content = file::read_text_file(path_);
        } catch (const file::file_error& e) {
            util::println_err("Error reading converted source file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        util::println_info("Converting converted source to JSONL format...");
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(content);
        } catch (const nlohmann::json::parse_error& e) {
            util::println_err("Error parsing converted source: " + std::string(e.what()));
            return;
        }
        if (!j.is_array()) {
            util::println_err("Error parsing converted source: Converted source is not an array.");
            return;
        }
        std::string jsonl;
        for (const auto& e : j) {
            if (!e.is_object()) {
                util::println_err("Error parsing converted source: Converted source contains non-object element.");
                return;
            }
            if (!e.contains("prompt") || !e.contains("completion")) {
                util::println_err("Error parsing converted source: Converted source contains element without prompt or completion.");
                return;
            }
            if (!e["prompt"].is_string() || !e["completion"].is_string()) {
                util::println_err("Error parsing converted source: Converted source contains element with non-string prompt or completion.");
                return;
            }
            jsonl.append(e.dump() + "\n");
        }
        if (boost::ends_with(jsonl, "\n")) {
            jsonl.erase(jsonl.size() - 1);
        }
        util::println_info("Uploading converted source...");
        std::string response;
        try {
            curl::upload_binary(url_files, [&response](const std::string& s, CURL*){
                response.append(s);
            }, "purpose", "fine-tune", "file", std::vector<char>(jsonl.begin(), jsonl.end()), cs_filename
            + ".jsonl", "application/json", {"Authorization: Bearer " + api::get_key()}, 300000);
        } catch (const std::exception& e) {
            util::println_err("Error uploading converted source: " + std::string(e.what()));
            return;
        }
        print_api_response(response);
        util::println_info("Upload completed.");
    }

    inline void upload_binary() {
        std::cout << "Please enter the full filename you want to upload.\n"
                     "(Note: You need to enter the file extension as well): ";
        std::string filename;
        getline(std::cin, filename);
        if (filename.empty()) {
            util::println_err("Filename cannot be empty.");
            return;
        }
        const auto& path_ = f_finetune / filename;
        util::println_info("Loading file: " + PATH_S(path_));
        std::vector<char> content;
        try {
            content = file::read_binary_file(path_);
        } catch (const file::file_error& e) {
            util::println_err("Error reading file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        util::println_info("Uploading file...");
        std::string response;
        try {
            curl::upload_binary(url_files, [&response](const std::string& s, CURL*){
                response.append(s);
            }, "purpose", "fine-tune", "file", content, filename, "", {"Authorization: Bearer " + api::get_key()}, 300000);
        } catch (const std::exception& e) {
            util::println_err("Error uploading file: " + std::string(e.what()));
            return;
        }
        print_api_response(response);
        util::println_info("Upload completed.");
    }

    inline void view() {
        util::println_info("Getting file list from API...");
        std::string response;
        try {
            curl::http_get(url_files, [&response](const std::string& s, CURL*){
                response.append(s);
            }, {"Authorization: Bearer " + api::get_key()});
        } catch (const std::exception& e) {
            util::println_err("Error getting file list from API: " + std::string(e.what()));
            return;
        }
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(response);
        } catch (const nlohmann::json::parse_error& e) {
            util::println_err("Error parsing API response: " + std::string(e.what()));
            return;
        }
        if (!j.contains("data") || !j["data"].is_array()) {
            util::println_err("Error: API response's data field is not an array.");
            return;
        }
        nlohmann::json data = j["data"];
        if (data.empty()) {
            util::println_warn("You don't have any files.");
        } else {
            std::cout << "Your files:\n";
            for (const auto& e : data) {
                if (!e.is_object()) {
                    util::println_err("Error: API response's data field contains non-object element.");
                    continue;
                }
                if (!e.contains("id") || !e.contains("filename") || !e.contains("bytes")
                || !e.contains("created_at") || !e.contains("status")) {
                    util::println_err("Error: API response's data field contains element without id, filename, bytes, "
                                      "created_at or status.");
                    continue;
                }
                if (!e["id"].is_string() || !e["filename"].is_string() || !e["bytes"].is_number_unsigned()
                || !e["created_at"].is_number() || !e["status"].is_string()) {
                    util::println_err("Error: API response's data field contains element with non-string id, filename, "
                                      "non-unsigned integer bytes, non-number created_at or non-string status.");
                    continue;
                }
                double size_num;
                std::string size_unit;
                auto bytes = static_cast<double>(e["bytes"].get<uint64_t>());
                if (bytes < 1024) {
                    size_num = bytes;
                    size_unit = "B";
                } else if (bytes < 1024 * 1024) {
                    size_num = bytes / 1024;
                    size_unit = "KB";
                } else if (bytes < 1024 * 1024 * 1024) {
                    size_num = bytes / (1024 * 1024);
                    size_unit = "MB";
                } else {
                    size_num = bytes / (1024 * 1024 * 1024);
                    size_unit = "GB";
                }
                std::stringstream size_num_ss;
                size_num_ss << std::fixed << std::setprecision(2) << size_num;
                util::print_cs(GOLDEN_TEXT(e["id"].get<std::string>()) + ": " + e["filename"].get<std::string>() + " "
                + size_num_ss.str() + size_unit + "  " + util::ms_to_formatted_time(e["created_at"].get<long long>() * 1000)
                + "  " + e["status"].get<std::string>(), true);
            }
        }
        util::println_info("Getting file list completed.");
    }

    inline void delete_() {
        std::cout << "Please enter the file's ID you want to delete.\n(Note: It's the ID, NOT the filename): ";
        std::string file_id;
        getline(std::cin, file_id);
        if (file_id.empty()) {
            util::println_err("Error: File ID cannot be empty.");
            return;
        }
        util::println_info("Deleting file...");
        std::string response;
        try {
            curl::http_delete(url_files + '/' + file_id, [&response](const std::string& s, CURL*){
                response.append(s);
            }, {"Authorization: Bearer " + api::get_key()});
        } catch (const std::exception& e) {
            util::println_err("Error deleting file: " + std::string(e.what()));
            return;
        }
        print_api_response(response);
        util::println_info("Deletion completed.");
    }

    inline void download() {
        if (!create_folder(f_download)) {
            return;
        }
        std::cout << "Please enter the file's ID you want to download.\n(Note: It's the ID, NOT the filename): ";
        std::string file_id;
        getline(std::cin, file_id);
        if (file_id.empty()) {
            util::println_err("Error: File ID cannot be empty.");
            return;
        }
        std::cout << "Please enter the filename you want to save as: ";
        std::string filename;
        getline(std::cin, filename);
        if (filename.empty()) {
            util::println_err("Error: Filename cannot be empty.");
            return;
        }
        util::println_info("Downloading file...", false);
        std::string response;
        try {
            curl::http_get(url_files + '/' + file_id + "/content", [&response](const std::string& s, CURL*){
                response.append(s);
            }, {"Authorization: Bearer " + api::get_key()}, 300000);
        } catch (const std::exception& e) {
            util::println_err("\nError downloading file: " + std::string(e.what()));
            return;
        }
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(response);
            api::APIKeyStatus status = api::APIKeyStatus::VALID;
            if (api::check_err_obj(j, status)) {
                return;
            }
        } catch (const nlohmann::json::parse_error& e) {}
        std::vector<char> bin_data(response.begin(), response.end());
        try {
            const auto& w_path = f_download / filename;
            file::write_binary_file(bin_data, w_path);
        } catch (const file::file_error& e) {
            util::println_err("\nError saving file: " + PATH_S(e.get_path()));
            util::println_err("Reason: " + std::string(e.what()));
            return;
        }
        util::println_info("\nDownload completed.");
    }

    inline void create_fine_tune() {
        std::cout << "Please enter the file ID you want to use for this fine tune task.\n"
                     "(Note: It's the ID, NOT the filename): ";
        std::string file_id;
        getline(std::cin, file_id);
        if (file_id.empty()) {
            util::println_err("Error: File ID cannot be empty.");
            return;
        }
        std::cout << "Please enter the model name you want to fine tune: ";
        std::string model_name;
        getline(std::cin, model_name);
        if (model_name.empty()) {
            util::println_err("Error: Model name cannot be empty.");
            return;
        }
        std::cout << "Please enter the custom suffix you want to set for your fine tuned model.\n"
                     "(Leave empty if you don't want to set a custom suffix): ";
        std::string custom_suffix;
        getline(std::cin, custom_suffix);
        std::vector<std::string> headers = {"Content-Type: application/json", "Authorization: Bearer " + api::get_key()};
        nlohmann::json payload = nlohmann::json::object();
        payload["training_file"] = file_id;
        payload["model"] = model_name;
        if (!custom_suffix.empty()) {
            payload["suffix"] = custom_suffix;
        }
        util::println_info("Creating fine tune task...");
        std::string response;
        try {
            curl::http_post(url_fine_tunes, [&response](const std::string& s, CURL*){
                response.append(s);
            }, payload.dump(), headers);
        } catch (const std::exception& e) {
            util::println_err("Error creating fine tune task: " + std::string(e.what()));
            return;
        }
        print_api_response(response);
        util::println_info("Fine tune task creation completed.");
    }

    inline void list_fine_tunes() {
        util::println_info("Getting fine tune tasks from API...");
        std::string response;
        try {
            curl::http_get(url_fine_tunes, [&response](const std::string& s, CURL*){
                response.append(s);
            }, {"Authorization: Bearer " + api::get_key()});
        } catch (const std::exception& e) {
            util::println_err("Error getting fine tune tasks from API: " + std::string(e.what()));
            return;
        }
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(response);
        } catch (const nlohmann::json::parse_error& e) {
            util::println_err("Error parsing API response: " + std::string(e.what()));
            return;
        }
        if (!j.contains("data") || !j["data"].is_array()) {
            util::println_err("Error: API response's data field is not an array.");
            return;
        }
        nlohmann::json data = j["data"];
        if (data.empty()) {
            util::println_warn("You don't have any fine tune tasks.");
        } else {
            size_t task_count = data.size();
            std::cout << "You have " << task_count << " fine tune task" << (task_count > 1 ? "s" : "") << ".\n"
            "Please enter the index(1-indexed) of the fine tune task you want to see.\n"
            "(Placed in creation time order, earliest first): ";
            std::string index_str;
            getline(std::cin, index_str);
            if (index_str.empty()) {
                util::println_err("Error: Index cannot be empty.");
                return;
            }
            long long index;
            try {
                index = std::stoll(index_str);
            } catch (const std::exception& e) {
                util::println_err("Error: Invalid index: " + std::string(e.what()));
                return;
            }
            if (index <= 0) {
                util::println_err("Error: Index must be positive.");
                return;
            }
            if (index > task_count) {
                util::println_err("Error: Index out of range.");
                return;
            }
            std::cout << "Fine tune task " << index << " data:\n" << data[index - 1].dump(2) << std::endl;
        }
        util::println_info("Getting fine tune tasks completed.");
    }

    inline void retrieve_fine_tune() {
        std::cout << "Please enter the fine tune ID you want to retrieve.\n(Note: It's the ID, NOT the model name): ";
        std::string fine_tune_id;
        getline(std::cin, fine_tune_id);
        if (fine_tune_id.empty()) {
            util::println_err("Error: Fine tune ID cannot be empty.");
            return;
        }
        util::println_info("Getting fine tune task...");
        std::string response;
        try {
            curl::http_get(url_fine_tunes + '/' + fine_tune_id, [&response](const std::string& s, CURL*){
                response.append(s);
            }, {"Authorization: Bearer " + api::get_key()});
        } catch (const std::exception& e) {
            util::println_err("Error getting fine tune task: " + std::string(e.what()));
            return;
        }
        print_api_response(response);
        util::println_info("Getting fine tune task completed.");
    }

    inline void cancel_fine_tune() {
        std::cout << "Please enter the fine tune ID you want to cancel.\n"
                     "(Note: It's the ID, NOT the model name): ";
        std::string fine_tune_id;
        getline(std::cin, fine_tune_id);
        if (fine_tune_id.empty()) {
            util::println_err("Error: Fine tune ID cannot be empty.");
            return;
        }
        util::println_info("Canceling fine tune task...");
        std::string response;
        try {
            curl::http_post(url_fine_tunes + '/' + fine_tune_id + "/cancel", [&response](const std::string& s, CURL*){
                response.append(s);
            }, "", {"Authorization: Bearer " + api::get_key()});
        } catch (const std::exception& e) {
            util::println_err("Error canceling fine tune task: " + std::string(e.what()));
            return;
        }
        print_api_response(response);
        util::println_info("Fine tune task cancellation completed.");
    }

    inline void delete_model() {
        std::cout << "Please enter the model name you want to delete: ";
        std::string model_name;
        getline(std::cin, model_name);
        if (model_name.empty()) {
            util::println_err("Error: Model name cannot be empty.");
            return;
        }
        util::println_info("Deleting model...");
        std::string response;
        try {
            curl::http_delete(url_models + '/' + model_name, [&response](const std::string& s, CURL*){
                response.append(s);
            }, {"Authorization: Bearer " + api::get_key()});
        } catch (const std::exception& e) {
            util::println_err("Error deleting model: " + std::string(e.what()));
            return;
        }
        print_api_response(response);
        util::println_info("Model deletion completed.");
    }

    void fine_tune_helper_main() {
        while (true) {
            util::print_cs("Please choose whether you want to convert source, upload file,\n"
                           "view files, delete files, download file, create a new fine tune task, \n"
                           "list fine tune tasks, retrieve a fine tune task, \n"
                           "cancel a fine tune task, or delete a fine tuned model.\n"
                           "(Input " + GOLDEN_TEXT("c") + " for convert, " + GOLDEN_TEXT("u") + " for upload, "
                           + GOLDEN_TEXT("v") + " for view,\n " + GOLDEN_TEXT("d") + " for delete, "
                           + GOLDEN_TEXT("dl") + " for download file,\n " + GOLDEN_TEXT("cf") + " for create fine tune, "
                           + GOLDEN_TEXT("lf") + " for list fine tunes,\n " + GOLDEN_TEXT("rf") + " for retrieve fine tune, "
                           + GOLDEN_TEXT("cn") + " for cancel fine tune,\n " + GOLDEN_TEXT("dm") + " for delete model, "
                           + GOLDEN_TEXT("q") + " for quit): ");
            std::string chose_mode;
            getline(std::cin, chose_mode);
            std::transform(chose_mode.begin(), chose_mode.end(), chose_mode.begin(), tolower);
            if (chose_mode == "c") {
                convert();
            } else if (chose_mode == "u") {
                upload();
            } else if (chose_mode == "ub") {
                //For testing purpose.
                upload_binary();
            } else if (chose_mode == "v") {
                view();
            } else if (chose_mode == "d") {
                delete_();
            } else if (chose_mode == "dl") {
                download();
            } else if (chose_mode == "cf") {
                create_fine_tune();
            } else if (chose_mode == "lf") {
                list_fine_tunes();
            } else if (chose_mode == "rf") {
                retrieve_fine_tune();
            } else if (chose_mode == "cn") {
                cancel_fine_tune();
            } else if (chose_mode == "dm") {
                delete_model();
            } else if (chose_mode == "q") {
                break;
            } else {
                util::println_warn("Invalid input, please try again.");
            }
        }
        util::print_clr("₪ ", {255, 50, 50});
        util::print_m_clr("Fine tune helper stopped.", {{255, 50, 50}, {255, 255, 50}});
    }
} // fth