#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace Term {

    class Exception : public std::exception {
        std::string m_what;

    public:
        explicit Exception(std::string what) : m_what(std::move(what)) {};

        [[nodiscard]] const char* what() const noexcept override {
            return m_what.c_str();
        }
    };
}  // namespace Term
