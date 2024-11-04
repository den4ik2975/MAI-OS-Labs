#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LINE 1000

int main() {
    int pipe1[2], pipe2[2];
    pid_t child1, child2;

    // Создаем два канала
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Ошибка создания канала");
        exit(1);
    }

    // Создаем первый дочерний процесс
    if ((child1 = fork()) == -1) {
        perror("Ошибка создания первого дочернего процесса");
        exit(1);
    }

    if (child1 == 0) {
        // Код для первого дочернего процесса (child1)
        close(pipe1[1]);  // Закрываем конец записи pipe1
        close(pipe2[0]);  // Закрываем конец чтения pipe2
        dup2(pipe1[0], STDIN_FILENO);  // Перенаправляем чтение с pipe1 на stdin
        dup2(pipe2[1], STDOUT_FILENO);  // Перенаправляем вывод на pipe2
        close(pipe1[0]);
        close(pipe2[1]);

        execl("./child1", "child1", NULL);
        perror("Ошибка execl для child1");
        exit(1);
    }

    // Создаем второй дочерний процесс
    if ((child2 = fork()) == -1) {
        perror("Ошибка создания второго дочернего процесса");
        exit(1);
    }

    if (child2 == 0) {
        // Код для второго дочернего процесса (child2)
        close(pipe1[0]);
        close(pipe1[1]);  // Не используем pipe1
        close(pipe2[1]);  // Закрываем конец записи pipe2
        dup2(pipe2[0], STDIN_FILENO);  // Перенаправляем pipe2 на stdin
        close(pipe2[0]);

        execl("./child2", "child2", NULL);
        perror("Ошибка execl для child2");
        exit(1);
    }

    // Родительский процесс
    close(pipe1[0]);  // Закрываем конец чтения pipe1
    close(pipe2[0]);  // Закрываем конец чтения pipe2
    close(pipe2[1]);  // Закрываем конец записи pipe2 для родителя

    char line[MAX_LINE];
    printf("Введите строки (Ctrl+D для завершения):\n");

    // Чтение строк от пользователя и пересылка их в child1
    while (fgets(line, MAX_LINE, stdin) != NULL) {
        write(pipe1[1], line, strlen(line));
    }

    close(pipe1[1]);  // Закрываем конец записи pipe1 после ввода

    // Ожидание завершения дочерних процессов
    wait(NULL);  // Ждем завершения child1
    wait(NULL);  // Ждем завершения child2

    printf("Все процессы завершены.\n");

    return 0;
}
