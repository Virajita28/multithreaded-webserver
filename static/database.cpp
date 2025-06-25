#include "database.hpp"
#include <sqlite3.h>
#include <iostream>

sqlite3* db = nullptr;

bool initDatabase() {
    int rc = sqlite3_open("files.db", &db);
    if (rc) {
        std::cerr << "DB open error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS files ("
                      "name TEXT PRIMARY KEY, "
                      "data BLOB NOT NULL);";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "DB init error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool storeFileToDB(const std::string& filename, const std::vector<char>& data) {
    const char* sql = "REPLACE INTO files (name, data) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, data.data(), static_cast<int>(data.size()), SQLITE_STATIC);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool getFileFromDB(const std::string& filename, std::vector<char>& data) {
    const char* sql = "SELECT data FROM files WHERE name = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);
        data.assign((const char*)blob, (const char*)blob + size);
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}
