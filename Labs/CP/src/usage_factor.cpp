#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <iomanip>
#include "Mckusey-Carels.cpp"
#include "list_of_free_blocks.cpp"

// Тестовая структура для хранения выделенной памяти
struct Allocation {
    void* ptr;
    size_t size;
};

template<typename Allocator>
double test_random_with_deallocations(Allocator& allocator, int num_operations, double free_probability) {
    std::vector<Allocation> active_allocations;
    size_t current_used = 0;
    size_t max_used = 0;  // Отслеживаем максимальное использование

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dis(8, 1024);
    std::uniform_real_distribution<> prob_dis(0.0, 1.0);

    // Выполняем операции
    for (int i = 0; i < num_operations; ++i) {
        bool should_free = !active_allocations.empty() &&
                          prob_dis(gen) < free_probability;

        if (should_free) {
            std::uniform_int_distribution<> index_dis(0, active_allocations.size() - 1);
            size_t index = index_dis(gen);

            current_used -= active_allocations[index].size;
            allocator.free(active_allocations[index].ptr);

            active_allocations[index] = active_allocations.back();
            active_allocations.pop_back();
        } else {
            size_t size = size_dis(gen);
            void* ptr = allocator.alloc(size);
            if (ptr) {
                active_allocations.push_back({ptr, size});
                current_used += size;
                max_used = std::max(max_used, current_used);  // Обновляем максимум
            }
        }
    }

    // Очищаем оставшиеся аллокации
    for (const auto& alloc : active_allocations) {
        allocator.free(alloc.ptr);
    }

    // Используем максимальное использование вместо общей суммы
    size_t total_allocated = 16 * 4096;
    return (double)max_used / total_allocated * 100.0;
}



// Функция для тестирования с рандомными размерами
template<typename Allocator>
double test_random_allocations(Allocator& allocator, int num_allocations) {
    std::vector<Allocation> allocations;
    size_t total_requested = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(8, 1024);

    // Выполняем аллокации
    for (int i = 0; i < num_allocations; ++i) {
        size_t size = dis(gen);
        void* ptr = allocator.alloc(size);
        if (ptr) {
            allocations.push_back({ptr, size});
            total_requested += size;
        }
    }

    // Освобождаем память
    for (const auto& alloc : allocations) {
        allocator.free(alloc.ptr);
    }

    // Расчет фактора использования
    size_t total_allocated;
    total_allocated = 16 * 4096;

    return (double)total_requested / total_allocated * 100.0;
}

// Функция для тестирования с размерами степени двойки
template<typename Allocator>
double test_power_of_two_allocations(Allocator& allocator, int num_allocations) {
    std::vector<Allocation> allocations;
    size_t total_requested = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(3, 10); // 2^3 = 8 до 2^10 = 1024

    // Выполняем аллокации
    for (int i = 0; i < num_allocations; ++i) {
        size_t power = dis(gen);
        size_t size = 1 << power;
        void* ptr = allocator.alloc(size);
        if (ptr) {
            allocations.push_back({ptr, size});
            total_requested += size;
        }
    }

    // Освобождаем память
    for (const auto& alloc : allocations) {
        allocator.free(alloc.ptr);
    }

    // Расчет фактора использования
    size_t total_allocated;
    total_allocated = 16 * 4096;
    return (double)total_requested / total_allocated * 100.0;
}

int main() {
    const int NUM_TESTS = 10;
    const int ALLOCATIONS_PER_TEST = 1000;
    const int OPERATIONS_PER_TEST = 2000;
    const double FREE_PROBABILITY = 0.3; // 30% вероятность освобождения

    // Тестируем McKusick алокатор
    double mckusick_random_total = 0;
    double mckusick_power_total = 0;
    double mckusick_random_dealloc_total = 0;

    for (int i = 0; i < NUM_TESTS; ++i) {
        McKusickAllocator mckusick;
        mckusick_random_total += test_random_allocations(mckusick, ALLOCATIONS_PER_TEST);
        mckusick_power_total += test_power_of_two_allocations(mckusick, ALLOCATIONS_PER_TEST);

        McKusickAllocator mckusick_dealloc;
        mckusick_random_dealloc_total += test_random_with_deallocations(
            mckusick_dealloc, OPERATIONS_PER_TEST, FREE_PROBABILITY);
    }

    // Тестируем BestFit алокатор
    double bestfit_random_total = 0;
    double bestfit_power_total = 0;
    double bestfit_random_dealloc_total = 0;

    for (int i = 0; i < NUM_TESTS; ++i) {
        BestFitAllocator bestfit;
        bestfit_random_total += test_random_allocations(bestfit, ALLOCATIONS_PER_TEST);
        bestfit_power_total += test_power_of_two_allocations(bestfit, ALLOCATIONS_PER_TEST);

        BestFitAllocator bestfit_dealloc;
        bestfit_random_dealloc_total += test_random_with_deallocations(
            bestfit_dealloc, OPERATIONS_PER_TEST, FREE_PROBABILITY);
    }

    // Выводим результаты
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Средний фактор использования:\n\n";
    std::cout << "McKusick-Karels:\n";
    std::cout << "  Случайные размеры: " << mckusick_random_total/NUM_TESTS << "%\n";
    std::cout << "  Степени двойки: " << mckusick_power_total/NUM_TESTS << "%\n";
    std::cout << "  Случайные с освобождением: " << mckusick_random_dealloc_total/NUM_TESTS << "%\n\n";
    std::cout << "Best Fit:\n";
    std::cout << "  Случайные размеры: " << bestfit_random_total/NUM_TESTS << "%\n";
    std::cout << "  Степени двойки: " << bestfit_power_total/NUM_TESTS << "%\n";
    std::cout << "  Случайные с освобождением: " << bestfit_random_dealloc_total/NUM_TESTS << "%\n";


    return 0;
}
