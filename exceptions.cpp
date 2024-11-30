#include <iostream>
#include <vector>
#include "memdb.h"
#include "exceptions.h"


void memdb::check_syntax(std::vector<memdb::Token> tokens) {

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


