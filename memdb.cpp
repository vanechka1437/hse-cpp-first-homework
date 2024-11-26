#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <regex>
#include <sstream>
#include <cstdint>
#include <variant>
#include <iomanip>


class BadQuery : public std::exception {
private:
    std::string message;
public:

    explicit BadQuery(std::string msg) : message(std::move(msg)) {}

    [[nodiscard]] const char *what() const noexcept override {
            return message.c_str();
    }
};

// Токен
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

// Функция для перевода enum в строку
std::string tokenTypeToString(Token::token_type type) {
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

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");
    return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

std::vector<std::string> splitIntoTokens(const std::string &str) {
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

std::string to_lower(const std::string &str) {
    std::string res;
    for (auto ch: str) {
        res += std::tolower(static_cast<unsigned char>(ch));
    }
    return res;
}

// Основной разбор строки
std::vector<Token> tokenize(const std::string &str) {
    std::vector<Token> tokens;
    std::vector<std::string> raw_tokens = splitIntoTokens(str);

    // Регулярные выражения для различных элементов
    const std::regex keyword_regex("^(create|table|insert|select|from|where|to)$", std::regex_constants::icase);
    const std::regex field_name_regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    const std::regex type_name_regex(R"(^int32$|^bool$|^string\[\d+\]$|^bytes\[\d+\]$)");
    const std::regex table_name_regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    const std::regex operator_regex(R"(^(%|\||<|>|<=|>=|=|!=|\\|\\||&&)$)");
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
            if (to_lower(raw_token) == "from" || to_lower(raw_token) == "table" || to_lower(raw_token) == "to") {
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


void check_syntax(std::vector<Token> tokens) {

    if (tokens[0].type != Token::KEYWORD) {
        throw BadQuery("Bad query: query have to start with keyword");
    }

    if (to_lower(tokens[0].value) == "create" && to_lower(tokens[1].value) != "table") {
        throw BadQuery("Bad query: maybe without " + tokens[1].value + " you wanted use \"table\"");
    }

    if (to_lower(tokens[0].value) == "create" && to_lower(tokens[1].value) == "table" &&
        tokens[2].type != Token::TABLE_NAME) {
        throw BadQuery("Bad query: expected table name after create table");
    }

    int keywords = 0;
    int brackets1 = 0;
    int brackets2 = 0;

    for (auto &token: tokens) {
        if (token.type == Token::UNDEFINED) {
            throw BadQuery("Bad query: undefined value " + token.value);
        }
        if (token.type == Token::KEYWORD) {
            keywords += 1;
        }
        if (token.value == "(" || token.value == ")") {
            brackets1 += 1;
        }
        if (token.value == "{" || token.value == "}") {
            brackets2 += 1;
        }
    }

    if (brackets1 % 2 == 1) {
        throw BadQuery("Bad query: problems with ( and )");
    }

    if (brackets2 % 2 == 1) {
        throw BadQuery("Bad query: problems with { and }");
    }

    if (tokens[0].value == "insert" || tokens[0].value == "create" || tokens[0].value == "delete") {
        if (keywords < 2) {
            throw BadQuery("Bad query: too few keywords");
        }
        if (keywords > 2) {
            throw BadQuery("Bad query: too many keywords");
        }
    }

    if (tokens[0].value == "select" || tokens[0].value == "update") {
        if (keywords < 3) {
            throw BadQuery("Bad query: too few keywords");
        }
        if (keywords > 3) {
            throw BadQuery("Bad query: too many keywords");
        }
    }

    for (auto it = tokens.begin(); it != (tokens.end() - 1); it++) {
        if ((it->type == Token::FIELD_NAME && (it + 1)->type == Token::FIELD_NAME) ||
            (it->type == Token::ATTRIBUTE && (it + 1)->type == Token::ATTRIBUTE) ||
            (it->type == Token::VALUE && (it + 1)->type == Token::VALUE)) {
            throw BadQuery("Bad query: expected , or : between " + it->value + " and " + (it + 1)->value);
        }

        if (it->value == "{" && (it + 1)->type != Token::ATTRIBUTE) {
            throw BadQuery("Bad query: after { have to be attribute value, not " + (it + 1)->value);
        }
    }
}

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

    void add_row(const row &row) {
        rows.emplace_back(row);
    }

    struct ValuePrinter {
        void operator()(const std::monostate &) const {
            std::cout << "NULL";
        }

        void operator()(const int &value) const {
            std::cout << value;
        }

        void operator()(const std::string &value) const {
            std::cout << value;
        }

        void operator()(const bool &value) const {
            std::cout << (value ? "true" : "false");
        }

        void operator()(const std::vector<uint8_t> &value) const {
            for (unsigned char i: value) {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(i);
            }
        }
    };

    void print() {
        std::cout << "------ Table name: " << name << " ------" << std::endl;
        for (const auto &row: info_row) {
            std::cout << row.name << "(" << row.type << ")" << "{" << row.key << ' ' << row.autoincrement << ' '
                      << row.unique << "}";
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

};

Table::column_value parse_value(const std::string &raw_value) {
    if (std::regex_match(raw_value, std::regex(R"(^\d+$)"))) {
        return std::stoi(raw_value);
    } else if (std::regex_match(raw_value, std::regex(R"(^true$|^false$)", std::regex_constants::icase))) {
        return (to_lower(raw_value) == "true");
    } else if (raw_value[0] == '"') {
        return raw_value;
    } else if (std::regex_match(raw_value, std::regex(R"(^0x[0-9a-fA-F]+$)"))) {
        std::vector<uint8_t> bytes;
        for (size_t i = 2; i < raw_value.size(); i += 2) {
            bytes.push_back(std::stoi(raw_value.substr(i, 2), nullptr, 16));
        }
        return bytes;
    } else {
        throw BadQuery("Bad query: Unsupported value: " + raw_value);
    }
}

Table create_table(const std::vector<Token> &tokens) {

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

Table::row insert_row(const std::vector<Token>& tokens, Table& table) {
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


struct Database {
    std::vector<Table> tables;

    void execute(const std::string &str) {

        std::vector<Token> tokens = tokenize(str);
        check_syntax(tokens);

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
    }
};


int main() {

    std::vector<std::string> good_inputs = {
            "create table users ({key, autoincrement} id : int32, {unique} login: string[32], password_hash: bytes[8], is_admin: bool = false)",
            "insert (,\"vasya\", 0xdeadbeefdeadbeef) to users",
            "insert (login = \"vasya\", password_hash = 0xdeadbeefdeadbeef) to users",
            "insert (,\"admin\", 0x0000000000000000, true) to users",
            "select id, login from users where is_admin || id < 10",
            "update users set is_admin = true where login = \"vasya\"",
            "update users set login = login + \"_deleted\", is_admin = false where password_hash < 0x00000000ffffffff",
            "delete users where |login| % 2 = 0",
            "delete users\nwhere\nlogin > 2",
            "insert (, \"\t\n\")",
            "CreaTe Table users (id: int32, login: string[8])"};

    std::vector<std::string> bad_inputs = {"create table ({unique} id : int32 = 0)"};

    std::vector<std::string> create_inputs = {
            "create table users ({key, autoincrement} id : int32, {unique} login: string[32], password_hash: bytes[8], is_admin: bool = false)",
            R"(create table os (mac: string[16], windows: string[16] = "windows10", linux: string[16] = "ubuntu"))",
            "create table hashes (hash1: bytes[8] = 0xabababab)"};

    std::vector<std::string> check_inputs = {"create table users ({key, autoincrement} id: int32, {unique} login: string[32])",
                                             "insert (,\"vasya\") to users",
                                             "insert (login = \"igor\") to users",
                                             "insert (2, \"vanya\") to users",
                                             "insert (, \"fedya\") to users"};


    for (auto input: good_inputs) {

        std::cout << "-----------------------------------------------------------------" << std::endl;
        std::vector<Token> tokens = tokenize(input);

        for (const auto &token: tokens) {
            std::cout << "Value: " << token.value << ", Type: " << tokenTypeToString(token.type) << std::endl;
        }
    }

    return 0;
}
