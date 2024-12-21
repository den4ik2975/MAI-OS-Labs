#include "lib.h"

bool inputAvailable()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(STDIN_FILENO, &fds));
}

std::time_t t_now()
{
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

Node createNode(int id, bool is_child)
{
    Node node;
    node.id = id;
    node.pid = getpid();

    node.context = zmq_ctx_new();
    node.socket = zmq_socket(node.context, ZMQ_DEALER);

    node.address = "tcp://127.0.0.1:" + std::to_string(5555 + id);

    if (is_child)
        zmq_connect(node.socket, (node.address).c_str());
    else
        zmq_bind(node.socket, (node.address).c_str());

    return node;
}

Node createProcess(int id)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // Дочерний процесс
        execl("./computing", "computing", std::to_string(id).c_str(), NULL);
        std::cerr << "execl failed" << std::endl;
        exit(1);
    }
    else if (pid == -1)
    {
        // Ошибка при создании процесса
        std::cerr << "Fork failed" << std::endl;
        exit(1);
    }
    // Родительский процесс
    Node node = createNode(id, false);
    node.pid = pid;
    return node;
}

void send_mes(Node &node, Message m)
{
    zmq_msg_t request_message;
    zmq_msg_init_size(&request_message, sizeof(m));
    std::memcpy(zmq_msg_data(&request_message), &m, sizeof(m));
    zmq_msg_send(&request_message, node.socket, ZMQ_DONTWAIT);
}

Message get_mes(Node &node)
{
    zmq_msg_t request;
    zmq_msg_init(&request);
    auto result = zmq_msg_recv(&request, node.socket, ZMQ_DONTWAIT);
    if (result == -1)
        return Message(None, -1, -1);
    Message message;
    std::memcpy(&message, zmq_msg_data(&request), sizeof(Message));
    return message;
}