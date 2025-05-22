#ifndef SERVER_HPP
#define SERVER_HPP

class Server {
public:
    Server(int port);
    void start();

private:
    int port;
    int server_fd;

    void setup_socket();
    void accept_clients();
};

#endif
