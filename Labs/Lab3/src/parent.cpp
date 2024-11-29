// parent.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "common.h"

void prepareFileForMapping(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Error creating mapped file");
        exit(1);
    }

    if (ftruncate(fd, sizeof(struct SharedData)) == -1) {
        perror("Error setting file size");
        close(fd);
        exit(1);
    }
    close(fd);
}

int main() {
    std::srand(std::time(nullptr));

    // Создаем семафоры
    sem_t *sem1 = sem_open(SEM_NAME1, O_CREAT | O_EXCL, 0666, 1);
    sem_t *sem2 = sem_open(SEM_NAME2, O_CREAT | O_EXCL, 0666, 1);

    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED) {
        perror("Error creating semaphores");
        exit(1);
    }

    prepareFileForMapping(MAPPED_FILE1);
    prepareFileForMapping(MAPPED_FILE2);

    int fd1 = open(MAPPED_FILE1, O_RDWR);
    int fd2 = open(MAPPED_FILE2, O_RDWR);
    if (fd1 == -1 || fd2 == -1) {
        perror("Error opening mapped files");
        exit(1);
    }

    struct SharedData* shared1 = (struct SharedData*)mmap(NULL, sizeof(struct SharedData),
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED, fd1, 0);
    struct SharedData* shared2 = (struct SharedData*)mmap(NULL, sizeof(struct SharedData),
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED, fd2, 0);
    if (shared1 == MAP_FAILED || shared2 == MAP_FAILED) {
        perror("Error mapping files");
        exit(1);
    }

    shared1->size = 0;
    shared1->done = 0;
    shared2->size = 0;
    shared2->done = 0;

    char filename1[256], filename2[256];
    printf("Enter filename for child1: ");
    scanf("%255s", filename1);
    printf("Enter filename for child2: ");
    scanf("%255s", filename2);
    getchar();

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

    printf("Enter lines (Ctrl+D to finish):\n");
    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, stdin) != NULL) {
        struct SharedData* target;
        sem_t* current_sem;
        double random = static_cast<double>(std::rand()) / RAND_MAX;

        if (random < PROB_FILE1) {
            target = shared1;
            current_sem = sem1;
        } else {
            target = shared2;
            current_sem = sem2;
        }

        sem_wait(current_sem);
        size_t len = strlen(line);
        strncpy(target->data, line, len);
        target->size = len;
        msync(target, sizeof(struct SharedData), MS_SYNC);

        sem_post(current_sem);
    }

    shared1->done = 1;
    shared2->done = 1;
    msync(shared1, sizeof(struct SharedData), MS_SYNC);
    msync(shared2, sizeof(struct SharedData), MS_SYNC);

    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    munmap(shared1, sizeof(struct SharedData));
    munmap(shared2, sizeof(struct SharedData));
    close(fd1);
    close(fd2);

    sem_close(sem1);
    sem_close(sem2);
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);

    unlink(MAPPED_FILE1);
    unlink(MAPPED_FILE2);

    printf("All processes completed.\n");
    return 0;
}
