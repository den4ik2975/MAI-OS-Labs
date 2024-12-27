#include <chrono>
#include <vector>
#include <random>
#include <iostream>
#include <iomanip>
#include "Mckusey-Carels.cpp"
#include "list_of_free_blocks.cpp"

class Timer {
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::nanoseconds;

    TimePoint start;
public:
    Timer() : start(Clock::now()) {}

    double elapsed() {
        auto end = Clock::now();
        auto duration = std::chrono::duration_cast<Duration>(end - start);
        return duration.count() * 1e-9;
    }
};

struct TestResult {
    double alloc_time;
    double free_time;
    size_t operations;
    size_t failed_allocs;

    double avg_alloc() const { return alloc_time / operations; }
    double avg_free() const { return free_time / operations; }
};

template<typename Allocator>
TestResult run_test(Allocator& alloc, const std::vector<size_t>& sizes) {
    std::vector<void*> ptrs;
    ptrs.reserve(sizes.size());
    size_t failed_allocs = 0;

    Timer alloc_timer;
    for (size_t size : sizes) {
        void* ptr = alloc.alloc(size);
        if (ptr) {
            ptrs.push_back(ptr);
        } else {
            failed_allocs++;
        }
    }
    double alloc_time = alloc_timer.elapsed();

    Timer free_timer;
    for (void* ptr : ptrs) {
        alloc.free(ptr);
    }
    double free_time = free_timer.elapsed();

    return {alloc_time, free_time, sizes.size(), failed_allocs};
}

void print_results(const char* test_name, const TestResult& mc_result,
                  const TestResult& bf_result) {
    std::cout << "\n=== " << test_name << " ===\n";
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "McKusick-Karels:\n"
              << "  Alloc total: " << mc_result.alloc_time << "s\n"
              << "  Alloc avg:   " << mc_result.avg_alloc() << "s\n"
              << "  Free total:  " << mc_result.free_time << "s\n"
              << "  Free avg:    " << mc_result.avg_free() << "s\n";

    std::cout << "Best Fit:\n"
              << "  Alloc total: " << bf_result.alloc_time << "s\n"
              << "  Alloc avg:   " << bf_result.avg_alloc() << "s\n"
              << "  Free total:  " << bf_result.free_time << "s\n"
              << "  Free avg:    " << bf_result.avg_free() << "s\n";
}

std::vector<size_t> generate_test_sizes(size_t count, size_t min_size, size_t max_size) {
    std::vector<size_t> sizes;
    sizes.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(min_size, max_size);

    for (size_t i = 0; i < count; ++i) {
        sizes.push_back(dist(gen));
    }
    return sizes;
}

int main() {
    const size_t MEMORY_SIZE = 32 * 1024 * 1024; // 32MB
    const size_t TEST_COUNT = 10000;

    // Small blocks test
    {
        std::cout << "\nRunning small blocks test...\n";
        McKusickAllocator mc_alloc(MEMORY_SIZE);
        BestFitAllocator bf_alloc(MEMORY_SIZE);
        auto sizes = generate_test_sizes(TEST_COUNT, 32, 160);
        auto mc_result = run_test(mc_alloc, sizes);
        auto bf_result = run_test(bf_alloc, sizes);
        print_results("Small Blocks Test", mc_result, bf_result);
    }

    // Large blocks test
    {
        std::cout << "\nRunning large blocks test...\n";
        McKusickAllocator mc_alloc(MEMORY_SIZE);
        BestFitAllocator bf_alloc(MEMORY_SIZE);
        auto sizes = generate_test_sizes(TEST_COUNT / 10, 4096, 16384);
        auto mc_result = run_test(mc_alloc, sizes);
        auto bf_result = run_test(bf_alloc, sizes);
        print_results("Large Blocks Test", mc_result, bf_result);
    }

    // Mixed sizes test
    {
        std::cout << "\nRunning mixed sizes test...\n";
        McKusickAllocator mc_alloc(MEMORY_SIZE);
        BestFitAllocator bf_alloc(MEMORY_SIZE);
        auto sizes = generate_test_sizes(TEST_COUNT, 32, 8192);
        auto mc_result = run_test(mc_alloc, sizes);
        auto bf_result = run_test(bf_alloc, sizes);
        print_results("Mixed Sizes Test", mc_result, bf_result);
    }

    return 0;
}
