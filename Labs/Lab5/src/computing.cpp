// computing.cpp
#include "lib.h"

class ComputingNode {
private:
    Node node_;
    std::map<std::string, int> dictionary_;
    std::list<Node> children_;

    void handle_create(const Message& message) {
        if (message.id == node_.id) {
            Node child = create_process(message.num);
            children_.push_back(std::move(child));
            send_message(node_, Message(CommandType::Create, child.id, child.pid));
        } else {
            broadcast_to_children(message);
        }
    }

    void handle_ping(const Message& message) {
        if (message.id == node_.id) {
            send_message(node_, message);
        } else {
            broadcast_to_children(message);
        }
    }

    void handle_exec_add(const Message& message) {
        if (message.id == node_.id) {
            dictionary_[std::string(message.st)] = message.num;
            send_message(node_, message);
        } else {
            broadcast_to_children(message);
        }
    }

    void handle_exec_find(const Message& message) {
        if (message.id == node_.id) {
            auto it = dictionary_.find(std::string(message.st));
            if (it != dictionary_.end()) {
                send_message(node_, Message(CommandType::ExecFnd,
                    node_.id, it->second, message.st));
            } else {
                send_message(node_, Message(CommandType::ExecErr,
                    node_.id, -1, message.st));
            }
        } else {
            broadcast_to_children(message);
        }
    }

    void broadcast_to_children(const Message& message) {
        for (auto& child : children_) {
            send_message(child, message);
        }
    }

public:
    explicit ComputingNode(int id)
        : node_(create_node(id, true)) {}

    void run() {
        while (true) {
            // Handle children messages
            for (auto& child : children_) {
                Message message = receive_message(child);
                if (message.command != CommandType::None) {
                    send_message(node_, message);
                }
            }

            // Handle parent messages
            Message message = receive_message(node_);

            switch (message.command) {
            case CommandType::Create:
                    handle_create(message);
                    break;
            case CommandType::Ping:
                    handle_ping(message);
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
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <node_id>" << std::endl;
        return 1;
    }

    try {
        ComputingNode node(std::atoi(argv[1]));
        node.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
