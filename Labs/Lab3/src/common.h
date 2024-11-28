// common.h
#ifndef COMMON_H
#define COMMON_H

#define MAX_LINE 1000
#define SHARED_MEM_SIZE (MAX_LINE * 100)
#define PROB_FILE1 0.8
#define MAPPED_FILE1 "/tmp/mapped_file1"
#define MAPPED_FILE2 "/tmp/mapped_file2"
#define SEM_NAME1 "/mysem1"
#define SEM_NAME2 "/mysem2"

struct SharedData {
    char data[SHARED_MEM_SIZE];
    size_t size;
    bool done;
};

#endif
