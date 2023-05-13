//
// Created by v2ray on 2023/5/13.
//

#include "Messages.h"

namespace chat {
    //class Messages start:
    Messages::Messages() = default;
    Messages::Messages(const std::list<std::pair<std::string, Role>>& messages) : messages(messages) {}
    Messages::~Messages() = default;

    bool Messages::operator==(const Messages& other) const {
        return messages == other.messages;
    }

    bool Messages::operator!=(const Messages& other) const {
        return !(other == *this);
    }

    std::pair<std::string, Messages::Role>& Messages::front() {
        return messages.front();
    }

    std::pair<std::string, Messages::Role>& Messages::back() {
        return messages.back();
    }

    const std::pair<std::string, Messages::Role>& Messages::front() const {
        return messages.front();
    }

    const std::pair<std::string, Messages::Role>& Messages::back() const {
        return messages.back();
    }

    void Messages::push_front(const std::pair<std::string, Role>& message) {
        messages.push_front(message);
    }

    void Messages::push_back(const std::pair<std::string, Role>& message) {
        messages.push_back(message);
    }

    void Messages::emplace_front(const std::string& message, const Messages::Role& role) {
        messages.emplace_front(message, role);
    }

    void Messages::emplace_back(const std::string& message, const Messages::Role& role) {
        messages.emplace_back(message, role);
    }

    void Messages::pop_front() {
        messages.pop_front();
    }

    void Messages::pop_back() {
        messages.pop_back();
    }

    void Messages::clear() noexcept {
        messages.clear();
    }

    size_t Messages::size() const noexcept {
        return messages.size();
    }

    bool Messages::empty() const noexcept {
        return messages.empty();
    }

    std::list<std::pair<std::string, Messages::Role>>::iterator Messages::begin() noexcept {
        return messages.begin();
    }

    std::list<std::pair<std::string, Messages::Role>>::iterator Messages::end() noexcept {
        return messages.end();
    }

    std::list<std::pair<std::string, Messages::Role>>::const_iterator Messages::cbegin() const noexcept {
        return messages.cbegin();
    }

    std::list<std::pair<std::string, Messages::Role>>::const_iterator Messages::cend() const noexcept {
        return messages.cend();
    }

    std::list<std::pair<std::string, Messages::Role>>::const_iterator Messages::begin() const noexcept {
        return cbegin();
    }

    std::list<std::pair<std::string, Messages::Role>>::const_iterator Messages::end() const noexcept {
        return cend();
    }

    std::list<std::pair<std::string, Messages::Role>>::iterator Messages::erase(
            const std::list<std::pair<std::string, Messages::Role>>::iterator& iterator) {
        return messages.erase(iterator);
    }

    std::list<std::pair<std::string, Messages::Role>>::const_iterator Messages::erase(
            const std::list<std::pair<std::string, Messages::Role>>::const_iterator& iterator) {
        return messages.erase(iterator);
    }

    std::list<std::pair<std::string, Messages::Role>>::iterator Messages::erase(
            const std::list<std::pair<std::string, Messages::Role>>::iterator& begin,
            const std::list<std::pair<std::string, Messages::Role>>::iterator& end) {
        return messages.erase(begin, end);
    }

    std::list<std::pair<std::string, Messages::Role>>::const_iterator Messages::erase(
            const std::list<std::pair<std::string, Messages::Role>>::const_iterator& begin,
            const std::list<std::pair<std::string, Messages::Role>>::const_iterator& end) {
        return messages.erase(begin, end);
    }
    //class Messages end.
} // chat