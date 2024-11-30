#include <iostream>
#include <vector>
#include <regex>
#include <string>
#include "memdb.h"

std::string memdb::tokenTypeToString(memdb::Token::token_type type) {
    switch (type) {
        case Token::KEYWORD:
            return "KEYWORD";
        case Token::FIELD_NAME:
            return "FIELD_NAME";
        case Token::TABLE_NAME:
            return "TABLE_NAME";
        case Token::TYPE_NAME:
            return "TYPE_NAME";
        case Token::OPERATOR:
            return "OPERATOR";
        case Token::DEFAULT_VALUE:
            return "DEFAULT_VALUE";
        case Token::VALUE:
            return "VALUE";
        case Token::SYMBOL:
            return "SYMBOL";
        case Token::ATTRIBUTE:
            return "ATTRIBUTE";
        default:
            return "UNDEFINED";
    }
}

std::string memdb::trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");
    return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}


std::vector<std::string> memdb::splitIntoTokens(const std::string &str) {
    std::vector<std::string> tokens;
    std::string current_token;

    bool inside_string = false;

    for (size_t i = 0; i < str.size(); ++i) {
        char ch = str[i];

        if (inside_string) {
            current_token += ch;
            if (ch == '\"') {
                tokens.emplace_back(current_token); // Добавляем строку как токен
                current_token.clear();
                inside_string = false;
            }
        } else if (ch == '\"') {
            if (!current_token.empty()) {
                tokens.emplace_back(trim(current_token));
                current_token.clear();
            }
            inside_string = true;
            current_token += ch; // Добавляем открывающую кавычку
        } else if (std::isspace(ch)) {
            if (!current_token.empty()) {
                tokens.emplace_back(trim(current_token));
                current_token.clear();
            }
        } else if (std::string("{}(),:;").find(ch) != std::string::npos) {
            if (!current_token.empty()) {
                tokens.emplace_back(trim(current_token));
                current_token.clear();
            }
            tokens.emplace_back(1, ch);
        } else if ((ch == '<' || ch == '>' || ch == '=' || ch == '!') && i + 1 < str.size() && str[i + 1] == '=') {
            if (!current_token.empty()) {
                tokens.push_back(trim(current_token));
                current_token.clear();
            }
            tokens.push_back(std::string(1, ch) + str[i + 1]);
            ++i;
        } else if (ch == '|' && i + 1 < str.size() && str[i + 1] == '|') {
            if (!current_token.empty()) {
                tokens.push_back(trim(current_token));
                current_token.clear();
            }
            tokens.emplace_back("||");
            ++i;

        } else if (ch == '|') {
            if (!current_token.empty()) {
                tokens.emplace_back(trim(current_token));
                current_token.clear();
            }
            tokens.emplace_back("|");
        } else if (ch == '&' && i + 1 < str.size() && str[i + 1] == '&') {
            if (!current_token.empty()) {
                tokens.push_back(trim(current_token));
                current_token.clear();
            }
            tokens.emplace_back("&&");
            ++i;
        } else {
            current_token += ch;
        }
    }

    if (!current_token.empty()) {
        tokens.emplace_back(trim(current_token));
    }
    return tokens;
}

std::string memdb::to_lower(const std::string &str) {
    std::string res;
    for (auto ch: str) {
        res += std::tolower(static_cast<unsigned char>(ch));
    }
    return res;
}

std::vector<memdb::Token> memdb::tokenize(const std::string &str) {
    std::vector<Token> tokens;
    std::vector<std::string> raw_tokens = splitIntoTokens(str);

    const std::regex keyword_regex("^(create|table|insert|select|from|where|to|delete)$", std::regex_constants::icase);
    const std::regex field_name_regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    const std::regex type_name_regex(R"(^int32$|^bool$|^string\[\d+\]$|^bytes\[\d+\]$)");
    const std::regex table_name_regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    const std::regex operator_regex(R"(^(%|\|\||<|>|<=|>=|==|!=|\\|&&)$)");
    const std::regex value_regex(R"(^(0x[0-9a-fA-F]+|\d+|true|false|"(?:[^"\\]|\\.)*")$)");
    const std::regex default_value_regex(R"(^(0x[0-9a-fA-F]+|\d+|true|false|"(?:[^"\\]|\\.)*")$)");
    const std::regex symbol_regex(R"(^(\(|\)|,|:|=|\{|\})$)");
    const std::regex attribute_regex("^key$|^autoincrement$|^unique$");

    bool expect_table_name = false;
    bool expect_attribute = false;
    bool expect_type_name = false;
    bool expect_default_value = false;


    for (const auto &raw_token: raw_tokens) {
        Token token;

        if (std::regex_match(raw_token, keyword_regex)) {
            token.type = Token::KEYWORD;
            if (to_lower(raw_token) == "from" || to_lower(raw_token) == "table" || to_lower(raw_token) == "to" ||
                    to_lower(raw_token) == "delete") {
                expect_table_name = true;
            }
        } else if (expect_table_name && std::regex_match(raw_token, table_name_regex)) {
            token.type = Token::TABLE_NAME;
            expect_table_name = false;
        } else if (expect_attribute && std::regex_match(raw_token, attribute_regex)) {
            token.type = Token::ATTRIBUTE;
        } else if (expect_type_name && std::regex_match(raw_token, type_name_regex)) {
            token.type = Token::TYPE_NAME;
            expect_type_name = false;
        } else if (expect_default_value && std::regex_match(raw_token, default_value_regex)) {
            token.type = Token::DEFAULT_VALUE;
            expect_default_value = false;
        } else if (std::regex_match(raw_token, value_regex)) {
            token.type = Token::VALUE;
        } else if (std::regex_match(raw_token, field_name_regex)) {
            token.type = Token::FIELD_NAME;
        } else if (std::regex_match(raw_token, operator_regex)) {
            token.type = Token::OPERATOR;
            if (raw_token == "=") {
                expect_default_value = true;
            }
        } else if (std::regex_match(raw_token, symbol_regex)) {
            token.type = Token::SYMBOL;
            if (raw_token == "{") {
                expect_attribute = true;
            }
            if (raw_token == "}") {
                expect_attribute = false;
            }
            if (raw_token == ":") {
                expect_type_name = true;
            }
        } else {
            token.type = Token::UNDEFINED;
        }

        token.value = raw_token;
        tokens.push_back(token);
    }

    return tokens;
}
