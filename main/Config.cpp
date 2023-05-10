//
// Created by v2ray on 2023/5/10.
//

#include "Config.h"

namespace config {

    //class Config start:
    Config::Config() : config_path("config.json") {}
    Config::Config(std::filesystem::path config_path) : config_path(std::move(config_path)) {}
    Config::~Config() = default;

    void Config::load_config() {
    }

    void Config::save_config() {
    }
    //class Config end.
} // config