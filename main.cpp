#include "memdb.h"

int main() {
    memdb::Database db;

    std::string check_string = "CreATe TABLE users (id: int32, login: string[16])";
    std::vector<memdb::Token> tokens = memdb::tokenize(check_string);
    for (const auto& token: tokens) {
        std::cout << token.value << ", " << memdb::tokenTypeToString(token.type) << std::endl;
    }


}