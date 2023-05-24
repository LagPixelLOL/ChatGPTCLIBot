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
    public:
        using Memory = std::list<std::shared_ptr<MemoryType>>;
        using iterator = typename Memory::iterator;
        using const_iterator = typename Memory::const_iterator;
    private:
        Memory memory;

    public:
        MemoryAdaptor() = default;
        explicit MemoryAdaptor(const Memory& memory) : memory(memory) {}
        template<typename IteratorType> MemoryAdaptor(IteratorType first_i, IteratorType last_e) : memory(first_i, last_e) {}
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

        iterator begin() noexcept {
            return memory.begin();
        }

        iterator end() noexcept {
            return memory.end();
        }

        const_iterator cbegin() const noexcept {
            return memory.cbegin();
        }

        const_iterator cend() const noexcept {
            return memory.cend();
        }

        const_iterator begin() const noexcept {
            return cbegin();
        }

        const_iterator end() const noexcept {
            return cend();
        }

        iterator erase(typename Memory::iterator pos) {
            return memory.erase(pos);
        }

        const_iterator erase(typename Memory::const_iterator pos) {
            return memory.erase(pos);
        }

        iterator erase(typename Memory::iterator begin, typename Memory::iterator end) {
            return memory.erase(begin, end);
        }

        const_iterator erase(typename Memory::const_iterator begin, typename Memory::const_iterator end) {
            return memory.erase(begin, end);
        }
    };
} // db

#endif //GPT3BOT_MEMORYADAPTOR_H
