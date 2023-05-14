//
// Created by v2ray on 2023/5/10.
//

#ifndef GPT3BOT_MEMORYADAPTOR_H
#define GPT3BOT_MEMORYADAPTOR_H

#include "../data/Messages.h"
#include "nlohmann/json.hpp"
#include "list"

namespace db {

    template<typename MemoryType>
    class MemoryAdaptor {
        using Memory = std::list<std::shared_ptr<MemoryType>>;
        Memory memory;

    public:
        MemoryAdaptor() = default;
        explicit MemoryAdaptor(const Memory& memory) : memory(memory) {}
        virtual ~MemoryAdaptor() = default;

        virtual MemoryAdaptor& operator=(const MemoryAdaptor& other) { //NOLINT(misc-unconventional-assign-operator)
            memory = other.memory;
            return *this;
        }

        virtual bool operator==(const MemoryAdaptor& other) const {
            return memory == other.memory;
        }

        bool operator!=(const MemoryAdaptor& other) const {
            return !other.operator==(*this);
        }

        [[nodiscard]] virtual chat::Messages to_messages() const = 0;
        [[nodiscard]] virtual nlohmann::json to_json() const = 0;

        std::shared_ptr<MemoryType>& front() {
            return memory.front();
        }

        std::shared_ptr<MemoryType>& back() {
            return memory.back();
        }

        const std::shared_ptr<MemoryType>& front() const {
            return memory.front();
        }

        const std::shared_ptr<MemoryType>& back() const {
            return memory.back();
        }

        void push_front(const std::shared_ptr<MemoryType>& memory_) {
            memory.push_front(memory_);
        }

        void push_back(const std::shared_ptr<MemoryType>& memory_) {
            memory.push_back(memory_);
        }

        void pop_front() {
            memory.pop_front();
        }

        void pop_back() {
            memory.pop_back();
        }

        void clear() noexcept {
            memory.clear();
        }

        [[nodiscard]] size_t size() const noexcept {
            return memory.size();
        }

        [[nodiscard]] bool empty() const noexcept {
            return memory.empty();
        }

        typename Memory::iterator begin() noexcept {
            return memory.begin();
        }

        typename Memory::iterator end() noexcept {
            return memory.end();
        }

        typename Memory::const_iterator cbegin() const noexcept {
            return memory.cbegin();
        }

        typename Memory::const_iterator cend() const noexcept {
            return memory.cend();
        }

        typename Memory::const_iterator begin() const noexcept {
            return cbegin();
        }

        typename Memory::const_iterator end() const noexcept {
            return cend();
        }

        typename Memory::iterator erase(typename Memory::iterator pos) {
            return memory.erase(pos);
        }

        typename Memory::const_iterator erase(typename Memory::const_iterator pos) {
            return memory.erase(pos);
        }

        typename Memory::iterator erase(typename Memory::iterator begin, typename Memory::iterator end) {
            return memory.erase(begin, end);
        }

        typename Memory::const_iterator erase(typename Memory::const_iterator begin, typename Memory::const_iterator end) {
            return memory.erase(begin, end);
        }
    };
} // db

#endif //GPT3BOT_MEMORYADAPTOR_H
