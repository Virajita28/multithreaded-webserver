#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;

void handleClient(SOCKET clientSocket) {
    char buffer[2048];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0';

    std::istringstream iss(buffer);
    std::string method, path;
    iss >> method >> path;

    if (path == "/") path = "/index.html";

    std::string fullPath = "static" + path;
    std::ifstream file(fullPath, std::ios::binary);

    std::string response;
    if (!file.is_open()) {
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
    } else {
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string body = ss.str();

        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
                   + std::to_string(body.size()) + "\r\n\r\n" + body;
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Server is running on http://localhost:" << PORT << "\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        std::thread(handleClient, clientSocket).detach();
    }

    WSACleanup();
    return 0;
}


std::string response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Access-Control-Allow-Origin: http://localhost:3000\r\n"
    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type\r\n"
    "\r\n"
    "<html>...</html>";
