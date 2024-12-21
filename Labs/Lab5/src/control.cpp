#include "./lib.h"

int main()
{
    std::unordered_set<int> all_id;
    all_id.insert(-1);
    std::list<Message> saved_mes;
    std::list<Node> children;

    std::string command;
    while (true)
    {
        // проверяем сообщения от детей
        for (auto &child : children)
        {
            Message message = get_mes(child);
            switch (message.command)
            {
            case Create:
                all_id.insert(message.id);
                std::cout << "Ok: " << message.num << std::endl;
                for (auto messageIter = saved_mes.begin(); messageIter != saved_mes.end(); ++messageIter)
                {
                    if (messageIter->command == Create and messageIter->num == message.id)
                    {
                        saved_mes.erase(messageIter);
                        break;
                    }
                }
                break;
            case Ping:
                std::cout << "Ok: " << message.id << " is available" << std::endl;
                for (auto messageIter = saved_mes.begin(); messageIter != saved_mes.end(); ++messageIter)
                {
                    if (messageIter->command == Ping and messageIter->id == message.id)
                    {
                        saved_mes.erase(messageIter);
                        break;
                    }
                }
                break;
            case ExecErr:
                std::cout << "Ok: " << message.id << " '" << message.st << "'not fount" << std::endl;
                for (auto messageIter = saved_mes.begin(); messageIter != saved_mes.end(); ++messageIter)
                {
                    if (messageIter->command == ExecFnd and messageIter->id == message.id)
                    {
                        saved_mes.erase(messageIter);
                        break;
                    }
                }
                break;
            case ExecAdd:
                std::cout << "Ok: " << message.id << std::endl;
                for (auto messageIter = saved_mes.begin(); messageIter != saved_mes.end(); ++messageIter)
                {
                    if (messageIter->command == ExecAdd and messageIter->id == message.id)
                    {
                        saved_mes.erase(messageIter);
                        break;
                    }
                }
                break;
            case ExecFnd:
                std::cout << "Ok: " << message.id << " '" << message.st << "' " << message.num << std::endl;
                for (auto messageIter = saved_mes.begin(); messageIter != saved_mes.end(); ++messageIter)
                {
                    if (messageIter->command == ExecFnd and messageIter->id == message.id)
                    {
                        saved_mes.erase(messageIter);
                        break;
                    }
                }
                break;
            default:
                continue;
            }
        }
        // проверяем недошедшие сообщения
        for (auto messageIter = saved_mes.begin(); messageIter != saved_mes.end(); ++messageIter)
        {
            if (std::difftime(t_now(), messageIter->sent_time) > 5)
            {
                switch (messageIter->command)
                {
                case Ping:
                    std::cout << "Error: Ok " << messageIter->id << " is unavailable" << std::endl;
                    break;
                case Create:
                    std::cout << "Error: Parent  " << messageIter->id << " is unavailable" << std::endl;
                    break;
                case ExecAdd:
                case ExecFnd:
                    std::cout << "Error: Node  " << messageIter->id << " is unavailable" << std::endl;
                    break;
                default:
                    break;
                }
                saved_mes.erase(messageIter); // Удаляем элемент
                break;
            }
        }

        // обрабатываем команды ввода
        if (!inputAvailable())
        {
            continue;
        }
        std::cin >> command;
        if (command == "create")
        {
            int parent_id, child_id;
            std::cin >> child_id >> parent_id;
            if (all_id.count(child_id))
            {
                std::cout << "Error: Node with id " << child_id << " already exists" << std::endl;
            }
            else if (!all_id.count(parent_id))
            {
                std::cout << "Error: Parent with id " << parent_id << " not found" << std::endl;
            }
            else if (parent_id == -1) // добавляем себе нового ребенка
            {
                Node child = createProcess(child_id);
                children.push_back(child);
                all_id.insert(child_id);
                std::cout << "Ok: " << child.pid << std::endl;
            }
            else // отправляем команду на добавление ребенка детям
            {
                Message message(Create, parent_id, child_id);
                saved_mes.push_back(message);
                for (auto &child : children)
                    send_mes(child, message);
            }
        }
        else if (command == "exec")
        {
            char input[100];
            fgets(input, sizeof(input), stdin);

            int id, val;
            char key[30];

            // Используем sscanf для разбора строки
            if (sscanf(input, "%d %30s %d", &id, key, &val) == 3)
            {
                if (!all_id.count(id))
                {
                    std::cout << "Error: Node with id " << id << " doesn't exist" << std::endl;
                    continue;
                }
                Message message = {ExecAdd, id, val, key};
                saved_mes.push_back(message);
                for (auto &child : children)
                    send_mes(child, message);
            }
            else if (sscanf(input, "%d %30s", &id, key) == 2)
            {
                if (!all_id.count(id))
                {
                    std::cout << "Error: Node with id " << id << " doesn't exist" << std::endl;
                    continue;
                }
                Message message = {ExecFnd, id, -1, key};
                saved_mes.push_back(message);
                for (auto &child : children)
                    send_mes(child, message);
            }
        }
        else if (command == "ping")
        {
            int id;
            std::cin >> id;
            if (!all_id.count(id))
            {
                std::cout << "Error: Node with id " << id << " doesn't exist" << std::endl;
            }
            else
            {
                Message message(Ping, id, 0);
                saved_mes.push_back(message);
                for (auto &child : children)
                    send_mes(child, message);
            }
        }
        else
            std::cout << "Error: Command doesn't exist!" << std::endl;
        //usleep(100000);
    }
    return 0;
}
