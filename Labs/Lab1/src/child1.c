#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 1000

int is_vowel(char c) {
    c = tolower(c);
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

void remove_vowels(char *str) {
    int i, j;
    for (i = 0, j = 0; str[i]; i++) {
        if (!is_vowel(str[i])) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen(argv[1], "w");
    if (file == NULL) {
        perror("Ошибка открытия файла");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, stdin) != NULL) {
        remove_vowels(line);
        fprintf(file, "%s", line);
    }

    fclose(file);
    return 0;
}
