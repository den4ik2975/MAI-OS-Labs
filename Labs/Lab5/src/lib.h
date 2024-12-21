#include <iostream>
#include <list>
#include <unordered_set>
#include <chrono>
#include <ctime>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include "zmq.h"
#include <sys/select.h>
#include <map>

bool inputAvailable();

std::time_t t_now();

enum Command : char
{
    None = 0,
    Create = 1,
    Ping = 2,
    ExecAdd = 3,
    ExecFnd = 4,
    ExecErr = 5
};

class Message
{
public:
    Message() {}
    Message(Command command, int id, int num) // констуктор без строки
        : command(command), id(id), num(num), sent_time(t_now())
    {
    }
    Message(Command command, int id, int num, char s[]) // констуктор со строкой
        : command(command), id(id), num(num), sent_time(t_now())
    {
        for (int i = 0; i < 30; ++i)
            st[i] = s[i];
    }

    bool operator==(const Message &other) const
    {
        return command == other.command && id == other.id && num == other.num && sent_time == other.sent_time;
    }

    Command command;           // команда
    int id;                // айди кому шлем
    int num;               // данные, либо число либо строка
    std::time_t sent_time; // время отправки
    char st[30];           // строка
};

class Node
{
public:
    int id;
    pid_t pid;
    void *context;
    void *socket;
    std::string address;

    bool operator==(const Node &other) const
    {
        return id == other.id && pid == other.pid;
    }
};

Node createNode(int id, bool is_child);

Node createProcess(int id);

void send_mes(Node &node, Message m);

Message get_mes(Node &node);