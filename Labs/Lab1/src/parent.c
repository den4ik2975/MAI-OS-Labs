#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 1000
#define PROB_PIPE1 0.8

int main() {
    int pipe1[2], pipe2[2];
    pid_t child1, child2;
    char filename1[256], filename2[256];

    // Создаем каналы
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Ошибка создания канала");
        exit(1);
    }

    // Инициализируем генератор случайных чисел
    srand(time(NULL));

    // Получаем имена файлов от пользователя
    printf("Введите имя файла для child1: ");
    scanf("%255s", filename1);
    printf("Введите имя файла для child2: ");
    scanf("%255s", filename2);

    // Создаем первый дочерний процесс
    child1 = fork();
    if (child1 == -1) {
        perror("Ошибка создания первого дочернего процесса");
        exit(1);
    } else if (child1 == 0) {
        // Код для child1
        close(pipe1[1]);
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);
        execl("./child1", "child1", filename1, NULL);
        perror("Ошибка execl для child1");
        exit(1);
    }

    // Создаем второй дочерний процесс
    child2 = fork();
    if (child2 == -1) {
        perror("Ошибка создания второго дочернего процесса");
        exit(1);
    } else if (child2 == 0) {
        // Код для child2
        close(pipe2[1]);
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        close(pipe1[0]);
        close(pipe1[1]);
        execl("./child2", "child2", filename2, NULL);
        perror("Ошибка execl для child2");
        exit(1);
    }

    // Код родительского процесса
    close(pipe1[0]);
    close(pipe2[0]);

    char line[MAX_LINE];
    printf("Введите строки (Ctrl+D для завершения):\n");
    while (fgets(line, MAX_LINE, stdin) != NULL) {
        if (((double)rand() / RAND_MAX) < PROB_PIPE1) {
            write(pipe1[1], line, strlen(line));
        } else {
            write(pipe2[1], line, strlen(line));
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    // Ожидаем завершения дочерних процессов
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    printf("Все процессы завершены.\n");
    return 0;
}
