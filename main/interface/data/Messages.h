//
// Created by v2ray on 2023/5/13.
//

#ifndef GPT3BOT_MESSAGES_H
#define GPT3BOT_MESSAGES_H

#include "list"
#include "string"

namespace chat {

    class Messages {
    public:
        enum class Role {
            SYSTEM, USER, ASSISTANT
        };

    private:
        std::list<std::pair<std::string, Role>> messages;

    public:
        Messages();
        explicit Messages(const std::list<std::pair<std::string, Role>>& messages);
        virtual ~Messages();

        bool operator==(const Messages& other) const;
        bool operator!=(const Messages& other) const;

        std::pair<std::string, Role>& front();
        std::pair<std::string, Role>& back();
        [[nodiscard]] const std::pair<std::string, Role>& front() const;
        [[nodiscard]] const std::pair<std::string, Role>& back() const;
        void push_front(const std::pair<std::string, Role>& message = {"", Role::USER});
        void push_back(const std::pair<std::string, Role>& message = {"", Role::USER});
        void emplace_front(const std::string& message = "", const Role& role = Role::USER);
        void emplace_back(const std::string& message = "", const Role& role = Role::USER);
        void pop_front();
        void pop_back();
        std::list<std::pair<std::string, Role>>::iterator erase(const std::list<std::pair<std::string, Role>>::iterator& iterator);
        std::list<std::pair<std::string, Role>>::const_iterator erase(
                const std::list<std::pair<std::string, Role>>::const_iterator& iterator);
        std::list<std::pair<std::string, Role>>::iterator erase(const std::list<std::pair<std::string, Role>>::iterator& begin,
                                                                const std::list<std::pair<std::string, Role>>::iterator& end);
        std::list<std::pair<std::string, Role>>::const_iterator erase(const std::list<std::pair<std::string, Role>>::const_iterator& begin,
                                                                      const std::list<std::pair<std::string, Role>>::const_iterator& end);
        void clear() noexcept;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;
        std::list<std::pair<std::string, Role>>::iterator begin() noexcept;
        std::list<std::pair<std::string, Role>>::iterator end() noexcept;
        [[nodiscard]] std::list<std::pair<std::string, Role>>::const_iterator cbegin() const noexcept;
        [[nodiscard]] std::list<std::pair<std::string, Role>>::const_iterator cend() const noexcept;
        [[nodiscard]] std::list<std::pair<std::string, Role>>::const_iterator begin() const noexcept;
        [[nodiscard]] std::list<std::pair<std::string, Role>>::const_iterator end() const noexcept;
    };
} // chat

#endif //GPT3BOT_MESSAGES_H
