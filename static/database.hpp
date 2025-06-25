#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector>

bool initDatabase();
bool storeFileToDB(const std::string& filename, const std::vector<char>& data);
bool getFileFromDB(const std::string& filename, std::vector<char>& data);

#endif
