#pragma once
#include <iostream>
#include <vector>
#include <variant>
#include <cstdint>

namespace memdb {

    struct Token {
        enum token_type {
            KEYWORD,
            FIELD_NAME,
            TABLE_NAME,
            TYPE_NAME,
            DEFAULT_VALUE,
            OPERATOR,
            VALUE,
            SYMBOL,
            ATTRIBUTE,
            UNDEFINED
        };

        token_type type = UNDEFINED;
        std::string value;
    };

    std::string tokenTypeToString(Token::token_type type);

    std::string trim(const std::string &str);

    std::vector<std::string> splitIntoTokens(const std::string &str);

    std::string to_lower(const std::string &str);

    std::vector<Token> tokenize(const std::string &str);

    struct Table {

        using column_value = std::variant<std::monostate, int, std::string, bool, std::vector<uint8_t>>;

        struct column_info {
            bool autoincrement = false;
            bool unique = false;
            bool key = false;
            std::string name;
            std::string type;
            column_value default_value = std::monostate{};
            int auto_increment_counter = 0;

            column_info(bool key, bool unique, bool autoincrement, std::string name, std::string type,
                        column_value default_value) :
                    autoincrement(autoincrement), unique(unique), key(key),
                    name(std::move(name)), type(std::move(type)), default_value(std::move(default_value)) {}
        };

        struct row {
            std::vector<column_value> values;
        };

        std::string name;
        std::vector<column_info> info_row;
        std::vector<row> rows;

        void add_row(const row &row);

        struct ValuePrinter {
            void operator()(const std::monostate &) const;

            void operator()(const int &value) const;

            void operator()(const std::string &value) const;

            void operator()(const bool &value) const;

            void operator()(const std::vector<uint8_t> &value) const;
        };

        void print() const;

    };

    Table::column_value parse_value(const std::string &raw_value);

    Table create_table(const std::vector<Token> &tokens);

    Table::row insert_row(const std::vector<Token>& tokens, Table& table);

    std::vector<Token> prepare_condition(const std::vector<Token>& tokens);

    bool variant_to_bool(Table::column_value variant);

    std::vector<bool> check_condition(const std::vector<Token>& condition, Table& table);

    Table::column_info find_column_info(const Table& table, const std::string& field_name);

    size_t find_column_index(const Table& table, const std::string& field_name);

    bool evaluate_condition(const std::vector<Token>& condition, const Table::row& row, const std::vector<Table::column_info>& info_row);

    struct Database {
        std::vector<Table> tables;

        Table& find_table(const std::string& table_name);

        void execute(const std::string &str);

    };
}


