#include "Handler.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <string>
#include <iostream>

std::string get_http_request(int socket) {
    char buffer[2048];
    int bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) return "";
    buffer[bytes_received] = '\0';
    return std::string(buffer);
}

std::string extract_path(const std::string& request) {
    std::istringstream req_stream(request);
    std::string method, path;
    req_stream >> method >> path;
    if (path == "/") return "/index.html";
    return path;
}

std::string build_response(const std::string& body, const std::string& content_type) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n" << body;
    return oss.str();
}

std::string build_404_response() {
    std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
    std::ostringstream oss;
    oss << "HTTP/1.1 404 Not Found\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n" << body;
    return oss.str();
}

std::string get_content_type(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    return "text/plain";
}

void handle_client(int client_socket) {
    std::string request = get_http_request(client_socket);
    if (request.empty()) {
        close(client_socket);
        return;
    }

    std::string path = extract_path(request);
    std::string filepath = "static" + path;
    std::ifstream file(filepath);

    if (file.good()) {
        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string body = buffer.str();
        std::string content_type = get_content_type(filepath);
        std::string response = build_response(body, content_type);
        send(client_socket, response.c_str(), response.size(), 0);
        log("200 OK: " + path);
    } else {
        std::string response = build_404_response();
        send(client_socket, response.c_str(), response.size(), 0);
        log("404 Not Found: " + path);
    }

    close(client_socket);
}
