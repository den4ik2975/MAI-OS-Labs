#include "lib.h"

int main(int argc, char *argv[])
{
    Node currentNode = createNode(atoi(argv[1]), true);
    std::map<std::string, int> dict;

    std::list<Node> children;
    while (true)
    {
        for (auto &child : children)
        {
            Message message = get_mes(child);
            if (message.command != None)
                send_mes(currentNode, message);
        }

        // проверяем сообщение от родителя
        Message message = get_mes(currentNode);
        switch (message.command)
        {
        case Create:
            if (message.id == currentNode.id) // добавляем себе нового ребенка
            {
                Node child = createProcess(message.num);
                children.push_back(child);
                send_mes(currentNode, {Create, child.id, child.pid});
            }
            else // отправляем команду на добавление ребенка детям
                for (auto &child : children)
                    send_mes(child, message);
            break;
        case Ping:
            if (message.id == currentNode.id)
                send_mes(currentNode, message); // спросили меня, отправляем что мы живы
            else                // отправляем детям запрос
                for (auto &child : children)
                    send_mes(child, message);
            break;
        case ExecAdd:
            if (message.id == currentNode.id) // спросили меня, отправляем ответ
            {
                dict[std::string(message.st)] = message.num;
                send_mes(currentNode, message);
            }
            else // отправляем детям запрос
                for (auto &child : children)
                    send_mes(child, message);
            break;
        case ExecFnd:
            if (message.id == currentNode.id) // спросили меня, отправляем ответ
                if (dict.find(std::string(message.st)) != dict.end())
                    send_mes(currentNode, {ExecFnd, currentNode.id, dict[std::string(message.st)], message.st});
                else
                    send_mes(currentNode, {ExecErr, currentNode.id, -1, message.st});
            else // отправляем детям запрос
                for (auto &child : children)
                    send_mes(child, message);
            break;
        default:
            break;
        }
        usleep(100000);
    }
    return 0;
}