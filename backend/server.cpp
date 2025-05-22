#include "Server.hpp"
#include "Handler.hpp"
#include "Utils.hpp"
#include <iostream>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

Server::Server(int port) : port(port), server_fd(0) {}

void Server::setup_socket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        log("Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        log("Bind failed.");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        log("Listen failed.");
        exit(EXIT_FAILURE);
    }

    log("Server listening on port " + std::to_string(port));
}

void Server::accept_clients() {
    sockaddr_in client_address;
    socklen_t addr_len = sizeof(client_address);

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr*)&client_address, &addr_len);
        if (client_socket >= 0) {
            std::thread(handle_client, client_socket).detach();
        } else {
            log("Accept failed.");
        }
    }
}

void Server::start() {
    setup_socket();
    accept_clients();
}
