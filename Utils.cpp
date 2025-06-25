#include "Utils.hpp"
#include "Globals.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

// Logging with timestamp and active thread count
void log(const std::string& message) {
    std::cout << "[" << timestamp() << "] " << message
              << " | Active Threads: " << active_threads.load() << std::endl;
}

// Cross-platform timestamp generator
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm local_tm;

#if defined(_WIN32)
    localtime_s(&local_tm, &now);  // Windows safe
#else
    local_tm = *std::localtime(&now);  // POSIX fallback
#endif

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Manual endsWith for C++17 compatibility
static bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Get correct MIME type for given filename extension
std::string getMimeType(const std::string& filename) {
    if (endsWith(filename, ".html")) return "text/html";
    if (endsWith(filename, ".txt")) return "text/plain";
    if (endsWith(filename, ".mp3")) return "audio/mpeg";
    if (endsWith(filename, ".wav")) return "audio/wav";
    if (endsWith(filename, ".mp4")) return "video/mp4";
    if (endsWith(filename, ".avi")) return "video/x-msvideo";
    if (endsWith(filename, ".mkv")) return "video/x-matroska";
    return "application/octet-stream";
}
