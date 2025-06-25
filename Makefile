# === Makefile for Blitz Web Server ===

# Compiler and flags
CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -pthread
LDFLAGS   = -lws2_32

# Source and output
SRCS      = main.cpp Server.cpp Handler.cpp Utils.cpp
OBJS      = $(SRCS:.cpp=.o)
TARGET    = webserver.exe

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /Q *.o *.exe 2>nul || true
