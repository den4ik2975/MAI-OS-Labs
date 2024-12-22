// lib.cpp
#include "lib.h"

bool input_available() {
    timeval tv{0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

std::chrono::system_clock::time_point now() {
    return std::chrono::system_clock::now();
}

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

void send_message(Node& node, const Message& msg) {
    zmq_msg_t message;
    zmq_msg_init_size(&message, sizeof(msg));
    std::memcpy(zmq_msg_data(&message), &msg, sizeof(msg));
    zmq_msg_send(&message, node.socket, ZMQ_DONTWAIT);
    zmq_msg_close(&message);
}

Message receive_message(Node& node) {
    zmq_msg_t message;
    zmq_msg_init(&message);

    if (zmq_msg_recv(&message, node.socket, ZMQ_DONTWAIT) == -1) {
        zmq_msg_close(&message);
        return Message(CommandType::None, -1, -1);
    }

    Message msg;
    std::memcpy(&msg, zmq_msg_data(&message), sizeof(Message));
    zmq_msg_close(&message);
    return msg;
}
