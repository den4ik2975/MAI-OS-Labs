#pragma once

#include "zmq.h"
#include "message.h"
#include <iostream>
#include <chrono>
#include <list>
#include <map>

// Base node
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

// Funcs for node creation
Node create_node(int id, bool is_child);
Node create_process(int id);

// Child node
class ComputingNode {
private:
    Node node_;
    std::map<std::string, int> dictionary_;
    std::list<Node> children_;
    std::chrono::milliseconds heartbeat = std::chrono::milliseconds::zero();
    std::chrono::system_clock::time_point last_beat = now();

    void handle_create(const Message& message);
    void handle_ping(const Message& message);
    void handle_heartbeat(const Message& message);
    void handle_exec_add(const Message& message);
    void handle_exec_find(const Message& message);
    void broadcast_to_children(const Message& message);

public:
    explicit ComputingNode(int id)
        : node_(create_node(id, true)) {}

    void run();
};

