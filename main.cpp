#include "memdb.h"

int main() {
    memdb::Database db;

    db.execute("create table users (id: int32, login: string[16])");
    for (int i = 0; i < 20; i++) {
        db.execute("insert (id = " + std::to_string(i) + ", login = \"number" + std::to_string(i) + "\") to users");
    }
    db.execute("select id from users where login == \"number5\"");
    db.execute("delete users where id < 10");
    for (const auto& table: db.tables) {
        table.print();
    }

}