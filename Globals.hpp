#pragma once
#include <atomic>

// Shared counter for active threads across all files
inline std::atomic<int> active_threads{0};
