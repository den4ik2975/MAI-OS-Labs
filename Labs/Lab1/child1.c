#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE 1000

int main() {
    char line[MAX_LINE];

    // Чтение строк из stdin (перенаправленный pipe1)
    while (fgets(line, MAX_LINE, stdin) != NULL) {
        // Преобразование строки в верхний регистр
        for (int i = 0; line[i]; i++) {
            line[i] = toupper(line[i]);
        }
        // Отправка строки в pipe2 (stdout был перенаправлен)
        printf("%s", line);
    }

    return 0;
}
