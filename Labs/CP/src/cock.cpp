#include <iostream>
#include <vector>
#include <random>
#include <cassert> // подключаем ваш файл
#include "list_of_free_blocks.cpp"
#include "Mckusey-Carels.cpp"

struct TestStruct {
    int data[8];  // 32 bytes
    bool operator==(const TestStruct& other) const {
        for (int i = 0; i < 8; i++) {
            if (data[i] != other.data[i]) return false;
        }
        return true;
    }
};

void test_small_allocations() {
    std::cout << "Testing small allocations...\n";

    McKusickAllocator allocator;

    // Test 32-byte allocation
    TestStruct* ptr1 = static_cast<TestStruct*>(allocator.alloc(sizeof(TestStruct)));
    assert(ptr1 != nullptr);

    // Initialize data
    for (int i = 0; i < 8; i++) {
        ptr1->data[i] = i;
    }

    // Test 64-byte allocation
    int* ptr2 = static_cast<int*>(allocator.alloc(64));
    assert(ptr2 != nullptr);

    // Verify first allocation still valid
    for (int i = 0; i < 8; i++) {
        assert(ptr1->data[i] == i);
    }

    // Free and reallocate
    allocator.free(ptr1);
    TestStruct* ptr3 = static_cast<TestStruct*>(allocator.alloc(sizeof(TestStruct)));
    assert(ptr3 != nullptr);

    allocator.free(ptr2);
    allocator.free(ptr3);

    std::cout << "Small allocations test passed!\n";
}

void test_large_allocations() {
    std::cout << "Testing large allocations...\n";

    // Create allocator with more pages
    McKusickAllocator allocator(32 * 4096);  // 32 pages instead of 16

    // Test 1KB allocation
    void* ptr1 = allocator.alloc(1024);
    assert(ptr1 != nullptr);

    // Test 8KB allocation
    void* ptr2 = allocator.alloc(8192);
    assert(ptr2 != nullptr);

    // Fill memory with pattern
    std::memset(ptr1, 0xAA, 1024);
    std::memset(ptr2, 0xBB, 8192);

    // Verify pattern
    uint8_t* check1 = static_cast<uint8_t*>(ptr1);
    uint8_t* check2 = static_cast<uint8_t*>(ptr2);

    for (size_t i = 0; i < 1024; i++) {
        assert(check1[i] == 0xAA);
    }

    for (size_t i = 0; i < 8192; i++) {
        assert(check2[i] == 0xBB);
    }

    allocator.free(ptr1);
    allocator.free(ptr2);

    std::cout << "Large allocations test passed!\n";
}


void stress_test() {
    std::cout << "Running stress test...\n";

    McKusickAllocator allocator;
    std::vector<std::pair<void*, size_t>> allocations;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dis(32, 1024);
    std::uniform_int_distribution<> op_dis(0, 1);

    for (int i = 0; i < 1000; i++) {
        if (allocations.empty() || op_dis(gen) == 1) {
            // Allocate
            size_t size = size_dis(gen);
            void* ptr = allocator.alloc(size);
            if (ptr) {
                allocations.push_back({ptr, size});
                std::memset(ptr, 0xCD, size); // Fill with pattern
            }
        } else {
            // Free
            size_t index = gen() % allocations.size();
            allocator.free(allocations[index].first);
            allocations.erase(allocations.begin() + index);
        }
    }

    // Cleanup remaining allocations
    for (auto& alloc : allocations) {
        allocator.free(alloc.first);
    }

    std::cout << "Stress test passed!\n";
}

int main() {
    try {
        test_small_allocations();
        test_large_allocations();
        stress_test();
        std::cout << "All tests passed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
