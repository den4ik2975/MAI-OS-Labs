// lib.h
#pragma once

#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include "zmq.h"

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

class Message {
public:
    Message() = default;

    Message(CommandType cmd, int id, int add_data)
        : command(cmd)
        , id(id)
        , add_data(add_data)
        , sent_time(std::chrono::system_clock::now())
    {}

    Message(CommandType cmd, int id, int add_data, const char* s)
        : command(cmd)
        , id(id)
        , add_data(add_data)
        , sent_time(std::chrono::system_clock::now())
    {
        std::strncpy(val, s, sizeof(val) - 1);
        val[sizeof(val) - 1] = '\0';
    }

    bool operator==(const Message& other) const {
        return command == other.command &&
               id == other.id &&
               add_data == other.add_data &&
               sent_time == other.sent_time;
    }

    CommandType command{CommandType::None};
    int id{-1};
    int add_data{-1};
    std::chrono::system_clock::time_point sent_time;
    char val[30]{};
};

class Node {
public:
    Node() = default;
    ~Node() {
        if (socket) zmq_close(socket);
        if (context) zmq_ctx_destroy(context);
    }

    // Prevent copying
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    // Allow moving
    Node(Node&& other) noexcept;
    Node& operator=(Node&& other) noexcept;

    bool operator==(const Node& other) const {
        return id == other.id && pid == other.pid;
    }

    int id{-1};
    pid_t pid{-1};
    void* context{nullptr};
    void* socket{nullptr};
    std::string address;
};

// Helper functions
bool input_available();
std::chrono::system_clock::time_point now();
Node create_node(int id, bool is_child);
Node create_process(int id);
void send_message(Node& node, const Message& msg);
Message receive_message(Node& node);
