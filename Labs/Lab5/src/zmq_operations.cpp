#include "zmq_operations.h"

namespace zmq_lib {
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
}
