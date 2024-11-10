#include <iostream>
#include <cstring>
#include <cctype>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "common.h"

bool isVowel(char c) {
    c = std::tolower(c);
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

void removeVowels(char* str) {
    int i, j;
    for (i = 0, j = 0; str[i]; i++) {
        if (!isVowel(str[i])) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <output_file>\n";
        return 1;
    }

    // Открываем mapped file для чтения
    int fd = open(MAPPED_FILE1, O_RDWR);
    if (fd == -1) {
        perror("Error opening mapped file");
        return 1;
    }

    // Получаем размер файла
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting file size");
        close(fd);
        return 1;
    }

    // Отображаем файл в память
    SharedData* shared = (SharedData*)mmap(NULL, sizeof(SharedData),
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return 1;
    }

    // Открываем выходной файл
    int outFd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outFd == -1) {
        perror("Error opening/creating output file");
        munmap(shared, sizeof(SharedData));
        close(fd);
        return 1;
    }
    FILE* outFile = fdopen(outFd, "w");
    if (!outFile) {
        perror("Error converting file descriptor");
        close(outFd);
        munmap(shared, sizeof(SharedData));
        close(fd);
        return 1;
    }

    // Обрабатываем данные
    char buffer[MAX_LINE];
    while (!shared->done || shared->size > 0) {
        if (shared->size > 0) {
            strncpy(buffer, shared->data, shared->size);
            buffer[shared->size] = '\0';
            removeVowels(buffer);
            fprintf(outFile, "%s", buffer);
            shared->size = 0;
            msync(shared, sizeof(SharedData), MS_SYNC);
        }
        usleep(1000); // Небольшая задержка
    }

    fclose(outFile);
    munmap(shared, sizeof(SharedData));
    close(fd);
    return 0;
}
