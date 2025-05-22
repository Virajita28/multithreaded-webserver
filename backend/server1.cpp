#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <sstream>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>

#define PORT 8080

std::mutex log_mutex;

std::string get_content_type(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".mp4")) return "video/mp4";
    if (path.ends_with(".mp3")) return "audio/mpeg";
    return "text/plain";
}

void handle_client(int client_socket) {
    char buffer[2048] = {0};
    int bytes_read = read(client_socket, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }

    std::string request(buffer);
    std::istringstream iss(request);
    std::string method, path;
    iss >> method >> path;

    if (path == "/") path = "/index.html";

    std::string filepath = "static" + path;
    std::ifstream file(filepath, std::ios::binary);

    if (!file.is_open()) {
        std::string notFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
        send(client_socket, notFound.c_str(), notFound.size(), 0);
        close(client_socket);
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string body = ss.str();
    std::string header = "HTTP/1.1 200 OK\r\nContent-Type: " + get_content_type(path) +
                         "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n";

    send(client_socket, header.c_str(), header.size(), 0);
    send(client_socket, body.c_str(), body.size(), 0);
    close(client_socket);

    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[+] Served: " << path << "\n";
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    std::cout << "🚀 Server running on http://localhost:" << PORT << std::endl;

    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        std::thread(handle_client, client_socket).detach();
    }

    return 0;
}
