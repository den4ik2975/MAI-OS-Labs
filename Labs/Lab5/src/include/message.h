#pragma once

#include <cstring>
#include "utils.h"

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

