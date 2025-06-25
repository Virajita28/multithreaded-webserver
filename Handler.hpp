#pragma once

#ifdef _WIN32
  #include <winsock2.h>
  typedef SOCKET SocketType;
#else
  #include <sys/socket.h>
  typedef int SocketType;
#endif

// Handles one client connection
void handleClient(SocketType client_socket);
