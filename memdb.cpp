#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <regex>
#include <variant>
#include <iomanip>
#include "memdb.h"
#include "exceptions.h"


void memdb::Table::add_row(const row &row) {
    rows.emplace_back(row);
}

void memdb::Table::ValuePrinter::operator()(const std::monostate &) const {
    std::cout << "NULL";
}

void memdb::Table::ValuePrinter::operator()(const int &value) const {
    std::cout << value;
}

void memdb::Table::ValuePrinter::operator()(const std::string &value) const {
    std::cout << value;
}

void memdb::Table::ValuePrinter::operator()(const bool &value) const {
    std::cout << (value ? "true" : "false");
}

void memdb::Table::ValuePrinter::operator()(const std::vector<uint8_t> &value) const {
    for (unsigned char i: value) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(i);
    }
}


void memdb::Table::print() const {
    std::cout << "------ Table name: " << name << " ------" << std::endl;
    for (const auto &row: info_row) {
        std::cout << row.name << "(" << row.type << ")" << "{" << row.key << ' ' << row.autoincrement << ' ' << row.unique << "}";
        std::visit(ValuePrinter{}, row.default_value);
        std::cout << "\t";
    }
    std::cout << std::endl;
    for (const auto &row: rows) {
        for (const auto &value: row.values) {
            std::visit(ValuePrinter{}, value);
            std::cout << "\t";
        }
        std::cout << std::endl;
    }
}

memdb::Table::column_value memdb::parse_value(const std::string &raw_value) {
    if (std::regex_match(raw_value, std::regex(R"(^\d+$)"))) {
        return std::stoi(raw_value);
    } else if (std::regex_match(raw_value, std::regex(R"(^true$|^false$)", std::regex_constants::icase))) {
        return (memdb::to_lower(raw_value) == "true");
    } else if (raw_value[0] == '"') {
        return raw_value;
    } else if (std::regex_match(raw_value, std::regex(R"(^0x[0-9a-fA-F]+$)"))) {
        std::vector<uint8_t> bytes;
        for (size_t i = 2; i < raw_value.size(); i += 2) {
            bytes.push_back(std::stoi(raw_value.substr(i, 2), nullptr, 16));
        }
        return bytes;
    } else {
        throw memdb::BadQuery("Bad query: Unsupported value: " + raw_value);
    }
}

memdb::Table memdb::create_table(const std::vector<Token> &tokens) {

    Table result_table;

    if (tokens[2].type == Token::TABLE_NAME) {
        result_table.name = tokens[2].value;
    } else {
        throw BadQuery("Bad query: create table query without table name");
    }

    bool key = false;
    bool autoincrement = false;
    bool unique = false;
    std::string name;
    std::string type;
    Table::column_value default_value = std::monostate{};
    int index = 4;

    while (tokens[index].value != ")") {
        if (tokens[index].type == Token::ATTRIBUTE) {
            if (tokens[index].value == "key") {
                key = true;
            }
            if (tokens[index].value == "autoincrement") {
                autoincrement = true;
            }
            if (tokens[index].value == "unique") {
                unique = true;
            }
        }
        if (tokens[index].type == Token::FIELD_NAME) {
            name = tokens[index].value;
        }
        if (tokens[index].type == Token::TYPE_NAME) {
            type = tokens[index].value;
            if (tokens[index + 1].value == "=") {
                std::string raw_value = tokens[index + 2].value;
                default_value = parse_value(raw_value);
            }
            Table::column_info info(key, unique, autoincrement, name, type, default_value);
            result_table.info_row.emplace_back(info);
            key = false;
            autoincrement = false;
            unique = false;
            name = "";
            type = "";
            default_value = std::monostate{};
        }

        index++;
    }

    return result_table;
}

memdb::Table::row memdb::insert_row(const std::vector<Token>& tokens, Table& table) {
    Table::row new_row;
    std::vector<Table::column_value> row_values(table.info_row.size(), std::monostate{});

    size_t index = 1;
    if (tokens[index].value != "(") {
        throw BadQuery("Bad query: expected '(' after insert");
    }
    ++index;

    size_t col_idx = 0;
    bool named_mode = false;

    while (tokens[index].value != ")") {
        if (tokens[index].type == Token::FIELD_NAME) {
            named_mode = true;
            std::string column_name = tokens[index].value;
            ++index;

            if (tokens[index].value != "=") {
                throw BadQuery("Bad query: expected '=' after column name");
            }
            ++index;

            auto target_idx = static_cast<size_t>(-1);
            for (size_t i = 0; i < table.info_row.size(); ++i) {
                if (table.info_row[i].name == column_name) {
                    target_idx = i;
                    break;
                }
            }

            if (target_idx == static_cast<size_t>(-1)) {
                throw BadQuery("Bad query: column '" + column_name + "' not found in table");
            }

            row_values[target_idx] = parse_value(tokens[index].value);
            ++index;
        } else if (!named_mode) {
            if (tokens[index].value == ",") {
                ++col_idx;
            } else {
                if (col_idx >= table.info_row.size()) {
                    throw BadQuery("Bad query: too many values provided");
                }
                row_values[col_idx] = parse_value(tokens[index].value);
                ++col_idx;
            }
            ++index;
        } else {
            throw BadQuery("Bad query: mixed positional and named parameters in insert");
        }

        if (tokens[index].value == ",") {
            ++index;
        }
    }

    for (size_t i = 0; i < table.info_row.size(); ++i) {
        if (std::holds_alternative<std::monostate>(row_values[i])) {
            if (table.info_row[i].autoincrement) {
                row_values[i] = table.info_row[i].auto_increment_counter++;
            } else if (!std::holds_alternative<std::monostate>(table.info_row[i].default_value)) {
                row_values[i] = table.info_row[i].default_value;
            } else {
                throw BadQuery("Bad query: missing value for column '" + table.info_row[i].name + "'");
            }
        }

        const std::regex type_regex(R"(^(string|bytes)\[(\d+)\]$)");
        std::smatch match;

        if (std::regex_match(table.info_row[i].type, match, type_regex)) {
            size_t max_length = std::stoul(match[2]); // Извлекаем X из типа
            if (match[1] == "string") {
                if (auto str_val = std::get_if<std::string>(&row_values[i])) {
                    if (str_val->size() > max_length) {
                        throw BadQuery("Bad query: string value too long for column '" + table.info_row[i].name + "'");
                    }
                }
            } else if (match[1] == "bytes") {
                if (auto bytes_val = std::get_if<std::vector<uint8_t>>(&row_values[i])) {
                    if (bytes_val->size() > max_length) {
                        throw BadQuery("Bad query: byte sequence too long for column '" + table.info_row[i].name + "'");
                    }
                }
            }
        }


        if (table.info_row[i].key || table.info_row[i].unique) {
            for (const auto& existing_row : table.rows) {
                if (existing_row.values[i] == row_values[i]) {
                    throw BadQuery("Bad query: duplicate value for unique or key column '" + table.info_row[i].name + "'");
                }
            }
        }
    }

    new_row.values = std::move(row_values);
    return new_row;
}

std::vector<memdb::Token> memdb::prepare_condition(const std::vector<Token>& tokens) {
    size_t where_index = 0;
    bool found_where = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (to_lower(tokens[i].value) == "where") {
            where_index = i;
            found_where = true;
            break;
        }
    }

    if (!found_where) {
        throw BadQuery("Bad query: 'where' clause not found in select query");
    }

    std::vector<Token> condition_tokens(tokens.begin() + where_index + 1, tokens.end());

    return condition_tokens;
}

bool memdb::variant_to_bool(Table::column_value variant) {
    if (auto* value = std::get_if<int>(&variant)) {
        return *value != 0;
    }
    if (auto* value = std::get_if<std::string>(&variant)) {
        return value->empty();
    }
    if (auto* value = std::get_if<std::vector<uint8_t>>(&variant)) {
        return value->empty();
    }
    if (auto* value = std::get_if<bool>(&variant)) {
        return *value;
    }
    throw BadQuery("Bad query: something wrong with condition part");
}

bool memdb::evaluate_condition(const std::vector<memdb::Token>& condition,
                               const Table::row& row, const std::vector<Table::column_info>& info_row) {

    if (condition.empty()) {
        throw BadQuery("Bad query: empty condition");
    }

    if (condition.size() == 1) {
        if (condition[0].type == Token::FIELD_NAME) {
            for (size_t i = 0; i < info_row.size(); ++i) {
                if (info_row[i].name == condition[0].value) {
                    return variant_to_bool(row.values[i]);
                }
            }
            throw BadQuery("Bad query: field name not found in condition");
        } else {
            throw BadQuery("Bad query: invalid single-token condition");
        }
    }

    if (condition.size() == 3) {
        const Token& left = condition[0];
        const Token& op = condition[1];
        const Token& right = condition[2];

        if (left.type != Token::FIELD_NAME || right.type != Token::VALUE) {
            throw BadQuery("Bad query: invalid condition format");
        }

        size_t column_index = -1;
        for (size_t i = 0; i < info_row.size(); ++i) {
            if (info_row[i].name == left.value) {
                column_index = i;
                break;
            }
        }

        if (column_index == static_cast<size_t>(-1)) {
            throw BadQuery("Bad query: column not found in condition");
        }

        const Table::column_value& column_value = row.values[column_index];
        Table::column_value parsed_value = parse_value(right.value);

        if (op.value == "==") {
            return column_value == parsed_value;
        } else if (op.value == "!=") {
            return column_value != parsed_value;
        } else if (op.value == "<") {
            return column_value < parsed_value;
        } else if (op.value == ">") {
            return column_value > parsed_value;
        } else if (op.value == "<=") {
            return column_value <= parsed_value;
        } else if (op.value == ">=") {
            return column_value >= parsed_value;
        } else {
            throw BadQuery("Bad query: unsupported operator in condition");
        }
    }

    // Сложные условия с логическими операторами
    for (size_t i = 0; i < condition.size(); ++i) {
        if (condition[i].type == Token::OPERATOR) {
            if (condition[i].value == "&&") {
                auto left_condition = std::vector<Token>(condition.begin(), condition.begin() + i);
                auto right_condition = std::vector<Token>(condition.begin() + i + 1, condition.end());
                return evaluate_condition(left_condition, row, info_row) &&
                       evaluate_condition(right_condition, row, info_row);
            } else if (condition[i].value == "||") {
                auto left_condition = std::vector<Token>(condition.begin(), condition.begin() + i);
                auto right_condition = std::vector<Token>(condition.begin() + i + 1, condition.end());
                return evaluate_condition(left_condition, row, info_row) ||
                       evaluate_condition(right_condition, row, info_row);
            }
        }
    }

    throw BadQuery("Bad query: unsupported condition");
}


std::vector<bool> memdb::check_condition(const std::vector<memdb::Token>& condition, Table& table) {
    std::vector<bool> results;
    for (size_t i = 0; i < table.rows.size(); ++i) {
        bool matches = evaluate_condition(condition, table.rows[i], table.info_row);
        results.push_back(matches);
    }

    return results;
}

memdb::Table::column_info memdb::find_column_info(const Table& table, const std::string& field_name) {
    for (const auto& col : table.info_row) {
        if (col.name == field_name) {
            return col;
        }
    }
    throw BadQuery("Bad query: Field '" + field_name + "' not found in table '" + table.name + "'");
}

size_t memdb::find_column_index(const Table& table, const std::string& field_name) {
    for (size_t i = 0; i < table.info_row.size(); ++i) {
        if (table.info_row[i].name == field_name) {
            return i;
        }
    }
    throw BadQuery("Bad query: Field '" + field_name + "' not found in table '" + table.name + "'");
}



memdb::Table& memdb::Database::find_table(const std::string& table_name) {
    for (auto& table : tables) {
        if (table.name == table_name) {
            return table;
        }
    }
    throw BadQuery("Bad query: Table '" + table_name + "' not found.");
}

void memdb::Database::execute(const std::string &str) {

    std::vector<Token> tokens = tokenize(str);
    check_syntax(tokens);

    if (tokens.size() < 2) {
        throw BadQuery("Bad query: too short query");
    }

    if (tokens[0].value == "create") {
        tables.emplace_back(create_table(tokens));
    }
    else if (tokens[0].value == "insert") {

        if ((tokens.end() - 1)->type == Token::TABLE_NAME) {
            bool flag = false;
            for (auto &table: tables) {
                if (table.name == (tokens.end() - 1)->value) {
                    Table::row row = insert_row(tokens, table);
                    table.rows.emplace_back(row);
                    flag = true;
                    break;
                }
            }
            if (!flag) {
                throw BadQuery("Bad query: " + (tokens.end() - 1)->value + " name doesn't except");
            }
        } else {
            for (const auto &token: tokens) {
                std::cout << "Value: " << token.value << ", Type: " << tokenTypeToString(token.type) << std::endl;
            }
            throw BadQuery("Bad query: insert query without table name");
        }
    }
    else if (tokens[0].value == "select") {
        std::string table_name;
        bool flag = false;

        for (const auto& token: tokens) {
            if (token.type == Token::TABLE_NAME) {
                table_name = token.value;
                flag = true;
                break;
            }
        }
        if (!flag) {
            throw BadQuery("Bad query: select query without table name");
        }

        Table source_table = find_table(table_name);

        std::vector<std::string> select_fields;
        for (const auto& token: tokens) {
            if (token.type == Token::FIELD_NAME) {
                select_fields.emplace_back(token.value);
            }
            if (token.value == "from") {
                break;
            }
        }

        std::vector<bool> check_results = check_condition(prepare_condition(tokens), source_table);

        Table new_table;
        new_table.name = "select_table";

        for (const auto & field_name : select_fields) {
            Table::column_info col_info = find_column_info(source_table, field_name);
            new_table.info_row.push_back(col_info);
        }

        for (size_t i = 0; i < source_table.rows.size(); ++i) {
            if (check_results[i]) {
                Table::row new_row;
                for (const auto & select_field : select_fields) {
                    size_t col_idx = find_column_index(source_table, select_field);
                    new_row.values.push_back(source_table.rows[i].values[col_idx]);
                }
                new_table.rows.push_back(new_row);
            }
        }

        tables.push_back(new_table);
    }
    else if (tokens[0].value == "delete") {
        std::string table_name;
        if (tokens[1].type == Token::TABLE_NAME) {
            table_name = tokens[1].value;
        } else {
            throw BadQuery("Bad query: query without table name");
        }

        Table& target_table = find_table(table_name);

        auto condition = prepare_condition(tokens);

        auto check_results = check_condition(condition, target_table);

        auto it = target_table.rows.begin();
        size_t index = 0;
        while (it != target_table.rows.end()) {
            if (check_results[index]) {
                it = target_table.rows.erase(it);
            } else {
                ++it;
            }
            ++index;
        }
    } else {
        throw BadQuery("Bad query: unknown query");
    }
}
