#include <iostream>
#include <cassert>
#include "memdb.h"
#include "exceptions.h"

void Test1() {
    /*
     * Общая проверка работоспособности
     * create table
     * */
    std::cout << "================ TEST 1 ================" << std::endl;
    std::string query = "create table users ({key, autoincrement} id : int32, {unique} login: string[32], password_hash: bytes[8], is_admin: bool = false)";
    memdb::Database db;
    db.execute(query);

    assert(db.tables.size() == 1);
    assert(db.tables[0].name == "users");
    assert(db.tables[0].info_row.size() == 4);

    assert(db.tables[0].info_row[0].name == "id");
    assert(db.tables[0].info_row[1].name == "login");
    assert(db.tables[0].info_row[2].name == "password_hash");
    assert(db.tables[0].info_row[3].name == "is_admin");

    assert(db.tables[0].info_row[0].type == "int32");
    assert(db.tables[0].info_row[1].type == "string[32]");
    assert(db.tables[0].info_row[2].type == "bytes[8]");
    assert(db.tables[0].info_row[3].type == "bool");

    assert(db.tables[0].info_row[0].key == true);
    assert(db.tables[0].info_row[0].autoincrement == true);
    assert(db.tables[0].info_row[1].unique == true);

    auto& default_value = db.tables[0].info_row[3].default_value;

    if (auto* value = std::get_if<bool>(&default_value)) {
        assert(*value == false);
    } else {
        std::cerr << "Default value for is_admin is not a bool or is missing." << std::endl;
        assert(false);
    }

    std::cout << "Test1 passed!" << std::endl;
}

void Test2() {
    /*
     * Общая проверка работоспособности insert-1
     */
    std::cout << "================ TEST 2 ================" << std::endl;

    std::string query = "create table users ({key, autoincrement} id : int32, {unique} login: string[32], password_hash: bytes[8], is_admin: bool = false)";
    memdb::Database db;
    db.execute(query);

    std::string insert_query = "insert (,\"vasya\", 0xdeadbeefdeadbeef) to users";
    db.execute(insert_query);

    if (auto* value = std::get_if<int>(&db.tables[0].rows[0].values[0])) {
        assert(*value == 0);
    } else {
        assert(false);
    }
    if (auto* value = std::get_if<std::string>(&db.tables[0].rows[0].values[1])) {
        assert(*value == "\"vasya\"");
    } else {
        assert(false);
    }
    if (auto* value = std::get_if<std::vector<uint8_t>>(&db.tables[0].rows[0].values[2])) {
        std::vector<uint8_t> test = {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef};
        assert(test == *value);
    } else {
        assert(false);
    }
    if (auto* value = std::get_if<bool>(&db.tables[0].rows[0].values[3])) {
        assert(*value == false);
    } else {
        assert(false);
    }

    std::cout << "Test2 passed!" << std::endl;
}

void Test3() {
    /*
     * Общая проверка работоспособности insert-2
     */
    std::cout << "================ TEST 3 ================" << std::endl;

    std::string query = "create table users ({key, autoincrement} id : int32, {unique} login: string[32], password_hash: bytes[8], is_admin: bool = false)";
    memdb::Database db;
    db.execute(query);

    std::string insert_query = "insert (login = \"vasya\", password_hash = 0xdeadbeefdeadbeef) to users";
    db.execute(insert_query);

    if (auto* value = std::get_if<int>(&db.tables[0].rows[0].values[0])) {
        assert(*value == 0);
    } else {
        assert(false);
    }
    if (auto* value = std::get_if<std::string>(&db.tables[0].rows[0].values[1])) {
        assert(*value == "\"vasya\"");
    } else {
        assert(false);
    }
    if (auto* value = std::get_if<std::vector<uint8_t>>(&db.tables[0].rows[0].values[2])) {
        std::vector<uint8_t> test = {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef};
        assert(test == *value);
    } else {
        assert(false);
    }
    if (auto* value = std::get_if<bool>(&db.tables[0].rows[0].values[3])) {
        assert(*value == false);
    } else {
        assert(false);
    }

    std::cout << "Test3 passed!" << std::endl;
}

void Test4() {
    /*
     * Общая проверка работоспособности select
     */
    std::cout << "================ TEST 4 ================" << std::endl;

    memdb::Database db;
    db.execute("create table users ({autoincrement} id: int32, login: string[16])");
    for (int i = 0; i < 10; i++) {
        db.execute("insert (,\"number" + std::to_string(i) + "\") to users");
    }
    db.execute("select id from users where id < 3");

    assert(db.tables[1].name == "select_table");
    assert(db.tables[1].info_row[0].autoincrement == true);
    assert(db.tables[1].info_row[0].unique == false);
    assert(db.tables[1].info_row[0].key == false);
    assert(std::get_if<std::monostate>(&db.tables[1].info_row[0].default_value));
    assert(db.tables[1].info_row[0].type == "int32");
    if (auto* value = std::get_if<int>(&db.tables[0].rows[0].values[0])) {
        assert(*value == 0);
    }
    if (auto* value = std::get_if<int>(&db.tables[0].rows[1].values[0])) {
        assert(*value == 1);
    }
    if (auto* value = std::get_if<int>(&db.tables[0].rows[2].values[0])) {
        assert(*value == 2);
    }

    std::cout << "Test4 passed!" << std::endl;
}

void Test5() {
    /*
     * Общая проверка работоспособности delete
     */
    std::cout << "================ TEST 5 ================" << std::endl;

    memdb::Database db;

    db.execute("create table users ({autoincrement} id: int32, login: string[16])");
    for (int i = 0; i < 10; i++) {
        db.execute("insert (,\"number" + std::to_string(i) + "\") to users");
    }
    db.execute("delete users where id > 0");

    assert(db.tables[0].rows.size() == 1);
    if (auto* value = std::get_if<std::string>(&db.tables[0].rows[0].values[1])) {
        assert(*value == "\"number0\"");
    }

    std::cout << "Test5 passed!" << std::endl;
}

void Test6() {
    /*
     * Проверка перехвата ошибок синтаксиса и плохих запросов
     */

    std::cout << "================ TEST 6 ================" << std::endl;

    memdb::Database db;

    try{
        db.execute("make table users ({autoincrement} id: int32 = 0)");
    }
    catch (memdb::BadQuery&) {}

    try{
        db.execute("create users ({autoincrement} id: int32 = 0)");
    }
    catch (memdb::BadQuery&) {}

    try{
        db.execute("create table ({autoincrement} id: int32 = 0)");
    }
    catch (memdb::BadQuery&) {}

    try{
        db.execute("create table users ({autoincrement} id: int32 = 0)");
    }
    catch (memdb::BadQuery&) {}

    try{
        db.execute("create table users ({autoincrement} id: int32 = 0");
    }
    catch (memdb::BadQuery&) {}

    try{
        db.execute("create table users ({autoincrement, key, unique id: int32 = 0)");
    }
    catch (memdb::BadQuery&) {}

    try{
        db.execute("create table users ({autoincrement, key, unique} id: int32 = 0)");
        db.execute("insert (id = 1)");
    }
    catch (memdb::BadQuery&) {}
    try{
        db.execute("create table users ({autoincrement, key, unique} id: int32 = 0)");
        db.execute("insert (id = 1) to user");
    }
    catch (memdb::BadQuery&) {}
    try{
        db.execute("create table users ({autoincrement, key, unique} id: int32 = 0)");
        for (int i = 0; i < 10; i++) {
            db.execute("insert (,) to users");
        }
        db.execute("select id from users where id >> 10");
    }
    catch (memdb::BadQuery&) {}
    try{
        db.execute("create table users ({autoincrement, key, unique} id: int32 = 0)");
        for (int i = 0; i < 10; i++) {
            db.execute("insert (,) to users");
        }
        db.execute("select id from users");
    }
    catch (memdb::BadQuery&) {}
    try{
        db.execute("create table users ({autoincrement, key, unique} id: int32 = 0, login: string[16])");
        db.execute("insert (10, \"vasya\") to users");

    }
    catch (memdb::BadQuery&) {}
    try{
        db.execute("create table users ({autoincrement, key, unique, good} id: int32 = 0, login: string[16])");

    }
    catch (memdb::BadQuery&) {}
    try{
        db.execute("create table users ({key, autoincrement} id: int32, login: string[16])");
        db.execute(R"(insert ("notAnInt", "user1") to users)");
    }
    catch (memdb::BadQuery&) {}

    std::cout << "Test6 passed!" << std::endl;
}

void Test7() {
    /*
     * Проверка работоспособности при необычном регистре
     */

    std::cout << "================ TEST 7 ================" << std::endl;

    memdb::Database db;

    db.execute("CreATe TABLE users (id: int32, login: string[16])");
    db.execute("InSErT (1, \"vasya\") TO users");
    db.execute("SElect id from users where id == 1");
    db.execute("dElETE users WHERE id == 1");

    std::cout << "Test7 passed!" << std::endl;
}

void Test8() {

    /*
     * Проверка работоспособности автоматического инкремента и подстановки дефолтных значений
     */

    std::cout << "================ TEST 8 ================" << std::endl;

    memdb::Database db;
    db.execute("create table users ({autoincrement} id: int32, login: string[16], is_admin: bool = false)");

    db.execute("insert (,\"user1\",) to users");
    db.execute("insert (,\"user2\",) to users");
    db.execute("insert (,\"user3\",) to users");

    if (auto* value = std::get_if<int>(&db.tables[0].rows[0].values[0])) {
        assert(*value == 0);
    }
    if (auto* value = std::get_if<int>(&db.tables[0].rows[1].values[0])) {
        assert(*value == 1);
    }
    if (auto* value = std::get_if<int>(&db.tables[0].rows[2].values[0])) {
        assert(*value == 2);
    }
    if (auto* value = std::get_if<bool>(&db.tables[0].rows[0].values[2])) {
        assert(*value == false);
    }
    if (auto* value = std::get_if<bool>(&db.tables[0].rows[1].values[2])) {
        assert(*value == false);
    }
    if (auto* value = std::get_if<bool>(&db.tables[0].rows[2].values[2])) {
        assert(*value == false);
    }

    std::cout << "Test8 passed!" << std::endl;
}

void Test9() {
    /*
     * Работоспособность с длинным логическим выражением
     */
    std::cout << "================ TEST 9 ================" << std::endl;

    memdb::Database db;
    db.execute("create table users ({key, autoincrement} id: int32, login: string[16], age: int32, is_admin: bool)");

    db.execute("insert (,\"admin1\", 30, true) to users");
    db.execute("insert (,\"user1\", 25, false) to users");
    db.execute("insert (,\"admin2\", 40, true) to users");
    db.execute("insert (,\"user2\", 35, false) to users");

    db.execute("select id, login from users where is_admin && age <= 35 && id > 0");

    assert(db.tables[1].rows.empty());

    std::cout << "Test9 passed!" << std::endl;
}

int main() {
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
    Test8();
    Test9();

    return 0;
}
