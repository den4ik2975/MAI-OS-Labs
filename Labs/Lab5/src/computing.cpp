// computing.cpp
#include "lib.h"

class ComputingNode {
private:
    Node node_;
    std::map<std::string, int> dictionary_;
    std::list<Node> children_;
    std::chrono::milliseconds heartbeat = std::chrono::milliseconds::zero();
    std::chrono::system_clock::time_point last_beat = now();

    void handle_create(const Message& message) {
        if (message.id == node_.id) {
            Node child = create_process(message.add_data);
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

    void handle_heartbeat(const Message& message) {
        heartbeat = std::chrono::milliseconds(message.add_data);
        broadcast_to_children(message);
    }

    void handle_exec_add(const Message& message) {
        if (message.id == node_.id) {
            dictionary_[std::string(message.val)] = message.add_data;
            send_message(node_, message);
        } else {
            broadcast_to_children(message);
        }
    }

    void handle_exec_find(const Message& message) {
        if (message.id == node_.id) {
            auto it = dictionary_.find(std::string(message.val));
            if (it != dictionary_.end()) {
                send_message(node_, Message(CommandType::ExecFnd,
                    node_.id, it->second, message.val));
            } else {
                send_message(node_, Message(CommandType::ExecErr,
                    node_.id, -1, message.val));
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

            if (heartbeat > std::chrono::milliseconds::zero() &&
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now() - last_beat).count() > heartbeat.count())
            {
                send_message(node_, Message(CommandType::HeartBeat, node_.id, -1));
                last_beat = now();
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
