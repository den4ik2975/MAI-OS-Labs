#include "zmq_operations.h"

Node::Node(Node&& other) noexcept
    : id(other.id)
    , pid(other.pid)
    , context(other.context)
    , socket(other.socket)
    , address(std::move(other.address))
{
    other.context = nullptr;
    other.socket = nullptr;
}

Node& Node::operator=(Node&& other) noexcept {
    if (this != &other) {
        if (socket) zmq_close(socket);
        if (context) zmq_ctx_destroy(context);

        id = other.id;
        pid = other.pid;
        context = other.context;
        socket = other.socket;
        address = std::move(other.address);

        other.context = nullptr;
        other.socket = nullptr;
    }
    return *this;
}


Node create_node(int id, bool is_child) {
    Node node;
    node.id = id;
    node.pid = getpid();

    node.context = zmq_ctx_new();
    if (!node.context) {
        throw std::runtime_error("Failed to create ZMQ context");
    }

    node.socket = zmq_socket(node.context, ZMQ_DEALER);
    if (!node.socket) {
        zmq_ctx_destroy(node.context);
        throw std::runtime_error("Failed to create ZMQ socket");
    }

    node.address = "tcp://127.0.0.1:" + std::to_string(5555 + id);

    int rc = is_child
        ? zmq_connect(node.socket, node.address.c_str())
        : zmq_bind(node.socket, node.address.c_str());

    if (rc != 0) {
        throw std::runtime_error("Failed to " +
            std::string(is_child ? "connect" : "bind") +
            " ZMQ socket");
    }

    return node;
}

Node create_process(int id) {
    pid_t pid = fork();
    if (pid == 0) {
        execl("./computing", "computing", std::to_string(id).c_str(), nullptr);
        std::cerr << "execl failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    if (pid == -1) {
        throw std::runtime_error("Fork failed: " +
            std::string(strerror(errno)));
    }

    Node node = create_node(id, false);
    node.pid = pid;
    return node;
}

void ComputingNode::handle_create(const Message& message) {
    if (message.id == node_.id) {
        Node child = create_process(message.add_data);
        children_.push_back(std::move(child));
        zmq_lib::send_message(node_, Message(CommandType::Create, child.id, child.pid));
    } else {
        broadcast_to_children(message);
    }
}

void ComputingNode::handle_ping(const Message& message) {
    if (message.id == node_.id) {
        zmq_lib::send_message(node_, message);
    } else {
        broadcast_to_children(message);
    }
}

void ComputingNode::handle_heartbeat(const Message& message) {
    heartbeat = std::chrono::milliseconds(message.add_data);
    broadcast_to_children(message);
}

void ComputingNode::handle_exec_add(const Message& message) {
    if (message.id == node_.id) {
        dictionary_[std::string(message.val)] = message.add_data;
        zmq_lib::send_message(node_, message);
    } else {
        broadcast_to_children(message);
    }
}

void ComputingNode::handle_exec_find(const Message& message) {
    if (message.id == node_.id) {
        auto it = dictionary_.find(std::string(message.val));
        if (it != dictionary_.end()) {
            zmq_lib::send_message(node_, Message(CommandType::ExecFnd,
                node_.id, it->second, message.val));
        } else {
            zmq_lib::send_message(node_, Message(CommandType::ExecErr,
                node_.id, -1, message.val));
        }
    } else {
        broadcast_to_children(message);
    }
}

void ComputingNode::broadcast_to_children(const Message& message) {
    for (auto& child : children_) {
        zmq_lib::send_message(child, message);
    }
}


void ComputingNode::run() {
    while (true) {
        // Handle children messages
        for (auto& child : children_) {
            Message message = zmq_lib::receive_message(child);
            if (message.command != CommandType::None) {
                zmq_lib::send_message(node_, message);
            }
        }

        if (heartbeat > std::chrono::milliseconds::zero() &&
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now() - last_beat).count() > heartbeat.count())
        {
            zmq_lib::send_message(node_, Message(CommandType::HeartBeat, node_.id, -1));
            last_beat = now();
        }

        // Handle parent messages
        Message message = zmq_lib::receive_message(node_);

        switch (message.command) {
        case CommandType::Create:
                handle_create(message);
                break;
        case CommandType::Ping:
                handle_ping(message);
                break;
        case CommandType::HeartBeat:
            handle_heartbeat(message);
            break;
        case CommandType::ExecAdd:
            handle_exec_add(message);
            break;
        case CommandType::ExecFnd:
            handle_exec_find(message);
            break;
        default:
            break;
        }
        usleep(100000);
    }
}
