#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "common.h"

int isVowel(char c) {
    c = tolower(c);
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
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return 1;
    }

    sem_t *sem = sem_open(SEM_NAME1, 0);
    if (sem == SEM_FAILED) {
        perror("Error opening semaphore");
        return 1;
    }

    int fd = open(MAPPED_FILE1, O_RDWR);
    if (fd == -1) {
        perror("Error opening mapped file");
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting file size");
        close(fd);
        return 1;
    }

    struct SharedData* shared = (struct SharedData*)mmap(NULL, sizeof(struct SharedData),
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return 1;
    }

    FILE* outFile = fopen(argv[1], "w");
    if (!outFile) {
        perror("Error opening output file");
        munmap(shared, sizeof(struct SharedData));
        close(fd);
        return 1;
    }

    char buffer[MAX_LINE];
    while (!shared->done || shared->size > 0) {

        if (shared->size > 0) {
            sem_wait(sem);
            strncpy(buffer, shared->data, shared->size);
            buffer[shared->size] = '\0';
            removeVowels(buffer);
            fprintf(outFile, "%s", buffer);
            shared->size = 0;
            msync(shared, sizeof(struct SharedData), MS_SYNC);
        } else
        {
            continue;
        }

        sem_post(sem);
    }

    fclose(outFile);
    munmap(shared, sizeof(struct SharedData));
    close(fd);
    sem_close(sem);
    return 0;
}
