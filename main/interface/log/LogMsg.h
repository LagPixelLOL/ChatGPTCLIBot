//
// Created by v2ray on 2023/5/11.
//

#ifndef GPT3BOT_LOGMSG_H
#define GPT3BOT_LOGMSG_H

#include "string"
#include "optional"

namespace Log {

    enum class Level {
        INFO, WARN, ERR, FATAL, DEBUG
    };

    template<typename PayloadType>
    class LogMsg {
        const Level level = Level::INFO;
        const std::string message;
        const std::optional<PayloadType> payload = std::nullopt;

    public:
        LogMsg() = default;
        explicit LogMsg(std::string message) : message(std::move(message)) {}
        LogMsg(const Level& level, std::string message, const std::optional<PayloadType>& payload = std::nullopt) :
        level(level), message(std::move(message)), payload(payload) {}
        virtual ~LogMsg() = default;

        [[nodiscard]] const Level& get_level() const noexcept {
            return level;
        }

        [[nodiscard]] const std::string& get_message() const noexcept {
            return message;
        }

        [[nodiscard]] const std::optional<PayloadType>& get_payload() const noexcept {
            return payload;
        }
    };
} // Log

#endif //GPT3BOT_LOGMSG_H
