#include "Handler.hpp"
#include "Utils.hpp"
#include "Globals.hpp"


#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <string>

// C++17 compatible ends_with
bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// --- UPLOAD HANDLER ---
void handleFileUpload(SocketType client_socket, const std::string& initialRequest, const std::string& uploadType) {
    char buffer[8192];
    std::string request = initialRequest;

    // Extract Content-Length
    size_t contentLength = 0;
    std::regex lengthRegex(R"(Content-Length:\s*(\d+))", std::regex_constants::icase);
    std::smatch lenMatch;
    if (std::regex_search(request, lenMatch, lengthRegex)) {
        contentLength = std::stoul(lenMatch[1].str());
    }

    // Read rest of the request
    while (request.size() < contentLength) {
        int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        request.append(buffer, bytes);
    }

    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        send(client_socket, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n", 44, 0);
        closesocket(client_socket);
        active_threads--;
        return;
    }

    std::string headers = request.substr(0, headerEnd);
    std::string body = request.substr(headerEnd + 4);

    // Extract boundary
    std::regex boundaryRegex(R"(boundary=(.*))");
    std::smatch boundaryMatch;
    std::string boundary;
    if (std::regex_search(headers, boundaryMatch, boundaryRegex)) {
        boundary = "--" + boundaryMatch[1].str();
    } else {
        send(client_socket, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n", 44, 0);
        closesocket(client_socket);
        active_threads--;
        return;
    }

    // Find first multipart part
    size_t partStart = body.find(boundary);
    if (partStart == std::string::npos) {
        send(client_socket, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n", 44, 0);
        closesocket(client_socket);
        active_threads--;
        return;
    }

    size_t partEnd = body.find(boundary, partStart + boundary.length());
    if (partEnd == std::string::npos) partEnd = body.size();

    std::string part = body.substr(partStart, partEnd - partStart);

    // ✅ Extract filename from the part's headers (not main headers!)
    std::string filename = "uploaded_file";
   std::regex filenameRegex("filename=\\\"([^\\\"]+)\\\"");
    std::smatch match;
    if (std::regex_search(part, match, filenameRegex)) {
        filename = match[1].str();
    }

    // Extract file content
    size_t fileStart = part.find("\r\n\r\n");
    if (fileStart == std::string::npos) {
        send(client_socket, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n", 44, 0);
        closesocket(client_socket);
        active_threads--;
        return;
    }
    fileStart += 4;
    std::string fileContent = part.substr(fileStart);

    // Trim trailing CRLF if present
    if (fileContent.size() >= 2 && fileContent.substr(fileContent.size() - 2) == "\r\n") {
        fileContent = fileContent.substr(0, fileContent.size() - 2);
    }

    // Save to disk
    std::filesystem::create_directories("www/files");
    std::ofstream out("www/files/" + filename, std::ios::binary);
    out.write(fileContent.c_str(), fileContent.size());
    out.close();

    // Respond to client
    std::string msg = "Upload received (" + uploadType + "): " + filename;
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(msg.size()) + "\r\n"
        "Connection: close\r\n\r\n" + msg;

    send(client_socket, response.c_str(), (int)response.size(), 0);
    closesocket(client_socket);
    active_threads--;
}



// --- CLIENT HANDLER ---
void handleClient(SocketType client_socket) {
    active_threads++;
    std::cout << "[+] Active Threads: " << active_threads.load() << std::endl;

    std::string request;
    char buffer[8192];

    int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(client_socket);
        active_threads--;
        return;
    }

    request.append(buffer, bytesReceived);
    log("Received request:\n" + request);

    // Route: /status
    if (request.find("GET /status") == 0) {
        std::string body = std::to_string(active_threads.load());
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n" + body;

        send(client_socket, response.c_str(), (int)response.size(), 0);
        closesocket(client_socket);
        active_threads--;
        return;
    }

    // Route: /
    if (request.find("GET / ") == 0 || request.find("GET / HTTP") == 0) {
        std::ifstream file("index.html", std::ios::binary);
        if (file.is_open()) {
            std::string html((std::istreambuf_iterator<char>(file)), {});
            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + std::to_string(html.size()) + "\r\n"
                "Connection: close\r\n\r\n" + html;

            send(client_socket, response.c_str(), (int)response.size(), 0);
        } else {
            std::string error = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(client_socket, error.c_str(), (int)error.size(), 0);
        }
        closesocket(client_socket);
        active_threads--;
        return;
    }

    // Route: /files/list
    if (request.find("GET /files/list") == 0) {
        std::ostringstream html;
        html << "<html><body><h2>📂 Uploaded Files</h2><ul>";

        for (const auto& entry : std::filesystem::directory_iterator("www/files")) {
            std::string fname = entry.path().filename().string();
            html << "<li><a href=\"/files/" << fname << "\">" << fname << "</a></li>";
        }

        html << "</ul></body></html>";
        std::string body = html.str();

        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n" + body;

        send(client_socket, response.c_str(), (int)response.size(), 0);
        closesocket(client_socket);
        active_threads--;
        return;
    }

    // Route: /files/<filename>
    if (request.find("GET /files/") == 0 && request.find("/files/list") == std::string::npos) {
        size_t start = request.find("GET /files/") + 10;
        size_t end = request.find(" ", start);
        std::string filename = request.substr(start, end - start);
        std::string filepath = "www/files/" + filename;

        std::ifstream file(filepath, std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            std::string content = ss.str();
            std::string contentType = getMimeType(filename);

            std::string responseHeader =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: " + contentType + "\r\n"
                "Content-Length: " + std::to_string(content.size()) + "\r\n"
                "Connection: close\r\n\r\n";

            send(client_socket, responseHeader.c_str(), (int)responseHeader.size(), 0);
            send(client_socket, content.c_str(), (int)content.size(), 0);
        } else {
            std::string notFound =
                "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            send(client_socket, notFound.c_str(), (int)notFound.size(), 0);
        }
        closesocket(client_socket);
        active_threads--;
        return;
    }

    // Routes: POST /upload/audio | /video | /file
    if (request.find("POST /upload/audio") == 0) {
        handleFileUpload(client_socket, request, "audio");
        return;
    }
    if (request.find("POST /upload/video") == 0) {
        handleFileUpload(client_socket, request, "video");
        return;
    }
    if (request.find("POST /upload/file") == 0) {
        handleFileUpload(client_socket, request, "file");
        return;
    }

    // Default: 404 Not Found
    std::string notFound =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n";
    send(client_socket, notFound.c_str(), (int)notFound.size(), 0);

    closesocket(client_socket);
    active_threads--;
}
