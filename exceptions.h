#include <exception>
#include <iostream>
#include <vector>
#include "memdb.h"

namespace memdb{
    class BadQuery : public std::exception {
    private:
        std::string message;
    public:

        explicit BadQuery(std::string msg) : message(std::move(msg)) {}

        [[nodiscard]] const char *what() const noexcept override {
            return message.c_str();
        }
    };


    void check_syntax(std::vector<memdb::Token> tokens);
}
