#pragma once

#include <atomic>
#include <chrono>
#include <winsock2.h>   // For SOCKET
#include <windows.h>    // Ensure full WinAPI types like `INVALID_SOCKET` are available

class Server {
public:
    explicit Server(int port = 8080);
    void run();

    static int  activeClients();
    static long uptimeSeconds();

private:
    int m_port;
    SOCKET m_listenSock;

    void acceptLoop();

    static std::atomic<int> s_active;
    static std::chrono::steady_clock::time_point s_startTime;
};
