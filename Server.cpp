#include "Server.hpp"
#include "Handler.hpp"
#include "Utils.hpp"

#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")  // Link with Winsock library

// Static members
std::atomic<int> Server::s_active{0};
std::chrono::steady_clock::time_point Server::s_startTime;

// Constructor
Server::Server(int port)
    : m_port(port), m_listenSock(INVALID_SOCKET) {}

// Return number of active clients
int Server::activeClients() {
    return s_active.load(std::memory_order_relaxed);
}

// Return server uptime in seconds
long Server::uptimeSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(steady_clock::now() - s_startTime).count();
}

// Start the server
void Server::run() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[!] WSAStartup failed.\n";
        return;
    }

    m_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSock == INVALID_SOCKET) {
        std::cerr << "[!] Socket creation failed.\n";
        WSACleanup();
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);

    if (bind(m_listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[!] Bind failed.\n";
        closesocket(m_listenSock);
        WSACleanup();
        return;
    }

    if (listen(m_listenSock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[!] Listen failed.\n";
        closesocket(m_listenSock);
        WSACleanup();
        return;
    }

    s_startTime = std::chrono::steady_clock::now();
    std::cout << "[+] " << timestamp() << " | Listening on port " << m_port << "\n";

    acceptLoop();
}

// Accept clients and handle each in a separate thread
void Server::acceptLoop() {
    while (true) {
        SOCKET clientSock = accept(m_listenSock, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) {
            std::cerr << "[!] Accept failed.\n";
            continue;
        }

        s_active.fetch_add(1, std::memory_order_relaxed);

        std::thread([clientSock]() {
            handleClient(clientSock);
            closesocket(clientSock);
            Server::s_active.fetch_sub(1, std::memory_order_relaxed);
        }).detach();
    }
}
