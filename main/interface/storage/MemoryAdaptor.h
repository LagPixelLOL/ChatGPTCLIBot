//
// Created by v2ray on 2023/5/10.
//

#ifndef GPT3BOT_MEMORYADAPTOR_H
#define GPT3BOT_MEMORYADAPTOR_H

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

        [[nodiscard]] size_t size() const {
            return memory.size();
        }

        [[nodiscard]] bool empty() const {
            return memory.empty();
        }

        Memory::iterator begin() {
            return memory.begin();
        }

        Memory::iterator end() {
            return memory.end();
        }

        Memory::const_iterator cbegin() const {
            return memory.cbegin();
        }

        Memory::const_iterator cend() const {
            return memory.cend();
        }

        Memory::const_iterator begin() const {
            return cbegin();
        }

        Memory::const_iterator end() const {
            return cend();
        }
    };
} // db

#endif //GPT3BOT_MEMORYADAPTOR_H
