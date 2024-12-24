#pragma once

#include <chrono>
#include "unordered_set"
#include <unistd.h>
#include <iostream>

// Command types for inter-process communication
enum class CommandType : uint8_t {
    None = 0,
    Create = 1,
    Ping = 2,
    ExecAdd = 3,
    ExecFnd = 4,
    ExecErr = 5,
    HeartBeat = 6,
};


// Helper functions
bool input_available();
std::chrono::system_clock::time_point now();
