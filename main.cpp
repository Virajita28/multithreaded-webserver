#include "Server.hpp"
#include <iostream>
#include <filesystem>

int main() {
    try {
        // ✅ Ensure upload directory exists
        std::filesystem::create_directories("www/files");

        std::cout << "[*] Starting Blitz Server on port 8080...\n";
        Server server(8080);  // Change port if needed
        server.run();
    }
    catch (const std::exception& ex) {
        std::cerr << "[!] Exception occurred: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
