#pragma once

#include "nodes.h"

namespace zmq_lib {
    void send_message(Node& node, const Message& msg);
    Message receive_message(Node& node);
}
