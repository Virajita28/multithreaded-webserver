#pragma once

#include <string>

// Logs a message with timestamp to console
void log(const std::string& message);

// Returns current time as [HH:MM:SS] string
std::string timestamp();

// Returns appropriate MIME type for a given filename extension
std::string getMimeType(const std::string& filename);
