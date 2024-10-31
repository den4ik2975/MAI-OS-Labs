#include <iostream>
#include <vector>
#include <pthread.h>
#include <cstdlib>
#include <chrono>

struct ThreadData {
    std::vector<int>* arr;
    int low;
    int cnt;
    int dir;
};

pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int max_threads;
int active_threads = 0;

// Пул потоков для переиспользования
pthread_t* thread_pool = nullptr;
int thread_pool_size = 0;

void compareAndSwap(std::vector<int>& arr, int i, int j, int dir) {
    if (dir == (arr[i] > arr[j])) {
        std::swap(arr[i], arr[j]);
    }
}

void* bitonicMerge(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    std::vector<int>& arr = *(data->arr);
    int low = data->low;
    int cnt = data->cnt;
    int dir = data->dir;

    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++) {
            compareAndSwap(arr, i, i + k, dir);
        }

        // Используем рекурсивный вызов для небольших размеров
        if (k < 1024) {
            ThreadData left_data = {data->arr, low, k, dir};
            ThreadData right_data = {data->arr, low + k, k, dir};
            bitonicMerge(&left_data);
            bitonicMerge(&right_data);
            return nullptr;
        }

        ThreadData left_data = {data->arr, low, k, dir};
        ThreadData right_data = {data->arr, low + k, k, dir};

        pthread_mutex_lock(&thread_count_mutex);
        if (active_threads + 2 <= max_threads) {
            active_threads += 2;
            pthread_mutex_unlock(&thread_count_mutex);

            pthread_t thread;
            pthread_create(&thread, nullptr, bitonicMerge, &left_data);
            bitonicMerge(&right_data);
            pthread_join(thread, nullptr);

            pthread_mutex_lock(&thread_count_mutex);
            active_threads -= 2;
            pthread_mutex_unlock(&thread_count_mutex);
        } else {
            pthread_mutex_unlock(&thread_count_mutex);
            bitonicMerge(&left_data);
            bitonicMerge(&right_data);
        }
    }
    return nullptr;
}

void* bitonicSort(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    std::vector<int>& arr = *(data->arr);
    int low = data->low;
    int cnt = data->cnt;
    int dir = data->dir;

    if (cnt > 1) {
        int k = cnt / 2;

        ThreadData left_data = {data->arr, low, k, 1};
        ThreadData right_data = {data->arr, low + k, k, 0};

        pthread_t threads[2];

        pthread_mutex_lock(&thread_count_mutex);
        if (active_threads + 2 <= max_threads) {
            active_threads += 2;
            pthread_mutex_unlock(&thread_count_mutex);

            pthread_create(&threads[0], nullptr, bitonicSort, &left_data);
            pthread_create(&threads[1], nullptr, bitonicSort, &right_data);

            pthread_join(threads[0], nullptr);
            pthread_join(threads[1], nullptr);

            pthread_mutex_lock(&thread_count_mutex);
            active_threads -= 2;
            pthread_mutex_unlock(&thread_count_mutex);
        } else {
            pthread_mutex_unlock(&thread_count_mutex);
            bitonicSort(&left_data);
            bitonicSort(&right_data);
        }

        ThreadData merge_data = {data->arr, low, cnt, dir};
        bitonicMerge(&merge_data);
    }
    return nullptr;
}


std::chrono::duration<double> make_calculations(int n, int max_threads) {
    thread_pool_size = max_threads;
    thread_pool = new pthread_t[thread_pool_size];

    std::vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 1000;
    }

//    std::cout << "Starting sorting array with length: " << n << "\n";
//    std::cout << "Max threads: " << max_threads << "\n";

    auto start = std::chrono::high_resolution_clock::now();

    ThreadData initial_data = {&arr, 0, n, 1};
    bitonicSort(&initial_data);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

//    std::cout << "Time taken: " << duration << " seconds\n";

    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[i-1]) {
            std::cout << "Sorting failed!\n";
            return std::chrono::nanoseconds::zero();
        }
    }
//    std::cout << "Sorting successful!\n";

    delete[] thread_pool;
    return duration;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {  // Теперь нам нужен только размер массива
        std::cerr << "Usage: " << argv[0] << " <array_size>\n";
        return 1;
    }

    int n = std::atoi(argv[1]);

    // Размер массива должен быть степенью 2
    if ((n & (n - 1)) != 0) {
        std::cerr << "Array size must be a power of 2\n";
        return 1;
    }

    std::cout << '[';

    for (int thread_count = 1; thread_count <= 64; thread_count++) {
        max_threads = thread_count;  // Обновляем глобальную переменную
        std::chrono::duration<double> res = std::chrono::nanoseconds::zero();

        for (int i = 0; i < 5; i++) {
            res += make_calculations(n, max_threads);
        }
        std::cout << (res / 5).count() << ", ";
    }
    std::cout << ']';
    return 0;
}

