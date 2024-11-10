#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "common.h"

void createMappedFile(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Error creating mapped file");
        exit(1);
    }

    // Устанавливаем размер файла
    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("Error setting file size");
        close(fd);
        exit(1);
    }
    close(fd);
}

int main() {
    std::srand(std::time(nullptr));

    // Создаем mapped files
    createMappedFile(MAPPED_FILE1);
    createMappedFile(MAPPED_FILE2);

    // Открываем mapped files
    int fd1 = open(MAPPED_FILE1, O_RDWR);
    int fd2 = open(MAPPED_FILE2, O_RDWR);
    if (fd1 == -1 || fd2 == -1) {
        perror("Error opening mapped files");
        exit(1);
    }

    // Отображаем файлы в память
    SharedData* shared1 = (SharedData*)mmap(NULL, sizeof(SharedData),
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED, fd1, 0);
    SharedData* shared2 = (SharedData*)mmap(NULL, sizeof(SharedData),
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED, fd2, 0);
    if (shared1 == MAP_FAILED || shared2 == MAP_FAILED) {
        perror("Error mapping files");
        exit(1);
    }

    // Инициализируем shared memory
    shared1->size = 0;
    shared1->done = false;
    shared2->size = 0;
    shared2->done = false;

    // Получаем имена выходных файлов
    char filename1[256], filename2[256];
    std::cout << "Enter filename for child1: ";
    std::cin >> filename1;
    std::cout << "Enter filename for child2: ";
    std::cin >> filename2;
    std::cin.ignore();

    // Создаем дочерние процессы
    pid_t child1 = fork();
    if (child1 == -1) {
        perror("Error creating first child");
        exit(1);
    }
    if (child1 == 0) {
        execl("./child1", "child1", filename1, NULL);
        perror("Error executing child1");
        exit(1);
    }

    pid_t child2 = fork();
    if (child2 == -1) {
        perror("Error creating second child");
        exit(1);
    }
    if (child2 == 0) {
        execl("./child2", "child2", filename2, NULL);
        perror("Error executing child2");
        exit(1);
    }

    // Читаем ввод и распределяем между процессами
    std::cout << "Enter lines (Ctrl+D to finish):\n";
    char line[MAX_LINE];
    while (std::cin.getline(line, MAX_LINE)) {
        SharedData* target;
        if (static_cast<double>(rand()) / RAND_MAX < PROB_FILE1) {
            target = shared1;
        } else {
            target = shared2;
        }

        // Ждем, пока предыдущие данные будут обработаны
        while (target->size > 0) {
            usleep(1000);
        }

        size_t len = strlen(line);
        strncpy(target->data, line, len);
        target->data[len] = '\n';
        target->size = len + 1;
        msync(target, sizeof(SharedData), MS_SYNC);
    }

    // Сигнализируем о завершении
    shared1->done = true;
    shared2->done = true;
    msync(shared1, sizeof(SharedData), MS_SYNC);
    msync(shared2, sizeof(SharedData), MS_SYNC);

    // Ждем завершения дочерних процессов
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    // Очищаем ресурсы
    munmap(shared1, sizeof(SharedData));
    munmap(shared2, sizeof(SharedData));
    close(fd1);
    close(fd2);
    unlink(MAPPED_FILE1);
    unlink(MAPPED_FILE2);

    std::cout << "All processes completed.\n";
    return 0;
}
