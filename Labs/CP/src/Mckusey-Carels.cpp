#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <iostream>

static const size_t PAGE_SIZE = 4096;
static const size_t MIN_BLOCK_SIZE = 32;
static const size_t MAX_BLOCK_SIZE = 4096;
static const size_t BUCKET_COUNT = 8; // 32, 64, 128, 256, 512, 1024, 2048, 4096

class McKusickAllocator {
private:
    struct FreeBlock {
        FreeBlock* next;
    };

    // Separate metadata structure
    struct PageInfo {
        void* page_addr;       // Pointer to the actual page
        size_t size;          // Size of blocks on the page
        PageInfo* next;       // Next page info in the list
        bool is_start_of_buffer; // For multi-page allocations
    };

    FreeBlock* freelistarr[BUCKET_COUNT];  // Array of free block lists
    PageInfo* free_page_info;              // List of free page information
    void* initial_memory;                  // Initial memory pointer
    void* metadata_memory;                 // Memory for metadata
    size_t total_pages;                    // Total number of pages

    static size_t get_bucket_index(size_t size) {
        if (size > MAX_BLOCK_SIZE) return BUCKET_COUNT;

        size_t normalized = std::max(MIN_BLOCK_SIZE,
                                   size_t(1) << (64 - __builtin_clzl(size - 1)));

        return __builtin_ctz(normalized) - 5;  // since 32 is 2^5
    }

    static size_t get_required_pages(size_t size) {
        return (size + PAGE_SIZE - 1) / PAGE_SIZE;
    }

void insert_sorted(PageInfo* page) {
        // Keep free list sorted by address
        if (!free_page_info || page->page_addr < free_page_info->page_addr) {
            page->next = free_page_info;
            free_page_info = page;
            return;
        }

        PageInfo* current = free_page_info;
        while (current->next && current->next->page_addr < page->page_addr) {
            current = current->next;
        }
        page->next = current->next;
        current->next = page;
    }

    PageInfo* find_consecutive_pages(size_t num_pages) {
        if (num_pages == 0) return nullptr;

        PageInfo* current = free_page_info;
        PageInfo* prev = nullptr;

        while (current) {
            PageInfo* potential_start = current;
            size_t found_consecutive = 1;
            PageInfo* page_walker = current;

            // Check for consecutive pages
            while (found_consecutive < num_pages && page_walker->next) {
                uintptr_t expected_addr = reinterpret_cast<uintptr_t>(potential_start->page_addr)
                                        + (found_consecutive * PAGE_SIZE);
                if (reinterpret_cast<uintptr_t>(page_walker->next->page_addr) != expected_addr) {
                    break;
                }

                page_walker = page_walker->next;
                found_consecutive++;
            }

            if (found_consecutive == num_pages) {
                // Found enough consecutive pages
                PageInfo* next_after_range = page_walker->next;

                // Remove the range from free list
                if (prev)
                    prev->next = next_after_range;
                else
                    free_page_info = next_after_range;

                // Link the allocated pages
                page_walker = potential_start;
                for (size_t i = 0; i < num_pages - 1; i++) {
                    page_walker->is_start_of_buffer = (i == 0);
                    page_walker->next = page_walker + 1;
                    page_walker = page_walker->next;
                }
                page_walker->next = nullptr;
                page_walker->is_start_of_buffer = false;

                return potential_start;
            }

            prev = current;
            current = current->next;
        }

        return nullptr;
    }

    void coalesce_free_pages() {
        if (!free_page_info) return;

        PageInfo* current = free_page_info;
        while (current && current->next) {
            uintptr_t current_end = reinterpret_cast<uintptr_t>(current->page_addr) + PAGE_SIZE;

            if (current_end == reinterpret_cast<uintptr_t>(current->next->page_addr)) {
                // Coalesce adjacent pages
                current->next = current->next->next;
            } else {
                current = current->next;
            }
        }
    }

    void split_page(PageInfo* page_info, size_t block_size) {
        size_t blocks_per_page = PAGE_SIZE / block_size;
        page_info->size = block_size;
        page_info->is_start_of_buffer = false;

        uint8_t* block_ptr = static_cast<uint8_t*>(page_info->page_addr);
        size_t bucket = get_bucket_index(block_size);

        for (size_t i = 0; i < blocks_per_page - 1; ++i) {
            FreeBlock* block = reinterpret_cast<FreeBlock*>(block_ptr);
            block->next = reinterpret_cast<FreeBlock*>(block_ptr + block_size);
            block_ptr += block_size;
        }

        FreeBlock* last_block = reinterpret_cast<FreeBlock*>(block_ptr);
        last_block->next = freelistarr[bucket];
        freelistarr[bucket] = reinterpret_cast<FreeBlock*>(page_info->page_addr);
    }

    PageInfo* find_page_info(void* ptr) {
        if (!ptr) return nullptr;

        // Calculate the page start address by masking off the low bits
        uintptr_t page_addr = reinterpret_cast<uintptr_t>(ptr) & ~(PAGE_SIZE - 1);

        // Calculate the index in our metadata array
        uintptr_t base_addr = reinterpret_cast<uintptr_t>(initial_memory);
        size_t page_index = (page_addr - base_addr) / PAGE_SIZE;

        // Check if the index is valid
        if (page_index >= (total_pages - 1)) return nullptr;

        return static_cast<PageInfo*>(metadata_memory) + page_index;
    }


public:
    McKusickAllocator(size_t memory_size = 16 * PAGE_SIZE) :
    free_page_info(nullptr) {
        total_pages = (memory_size + PAGE_SIZE - 1) / PAGE_SIZE;

        std::memset(freelistarr, 0, sizeof(freelistarr));


        // Один вызов mmap вместо двух
        void* memory = mmap(nullptr, total_pages * PAGE_SIZE,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS,
                           -1, 0);

        if (memory == MAP_FAILED) {
            throw std::runtime_error("Failed to allocate memory");
        }

        // Устанавливаем указатели
        metadata_memory = memory;
        initial_memory = static_cast<char*>(memory) + PAGE_SIZE;

        // Дальше ваш код без изменений
        PageInfo* info_ptr = static_cast<PageInfo*>(metadata_memory);
        uint8_t* mem_ptr = static_cast<uint8_t*>(initial_memory);

        // Initialize in ascending order
        for (size_t i = 0; i < total_pages - 1; ++i) {
            info_ptr[i].page_addr = mem_ptr + (i * PAGE_SIZE);
            info_ptr[i].size = 0;
            info_ptr[i].is_start_of_buffer = false;
        }

        // Link pages in ascending order
        for (size_t i = 0; i < total_pages - 2; ++i) {
            info_ptr[i].next = &info_ptr[i + 1];
        }
        info_ptr[total_pages - 2].next = nullptr;

        free_page_info = &info_ptr[0];  // Start with first page
    }


    void* alloc(size_t size) {
        if (size == 0) return nullptr;

        if (size <= MAX_BLOCK_SIZE) {
            size_t bucket = get_bucket_index(size);

            if (freelistarr[bucket] == nullptr) {
                if (free_page_info == nullptr) {
                    return nullptr;
                }

                PageInfo* page_info = free_page_info;
                free_page_info = free_page_info->next;
                split_page(page_info, MIN_BLOCK_SIZE << bucket);
            }

            FreeBlock* block = freelistarr[bucket];
            freelistarr[bucket] = block->next;
            return block;
        } else {
            size_t pages_needed = get_required_pages(size);

            // Debug: Print available pages
            PageInfo* current = free_page_info;
            size_t available_pages = 0;
            while (current) {
                available_pages++;
                current = current->next;
            }

            PageInfo* start_page_info = find_consecutive_pages(pages_needed);
            if (!start_page_info) {
                return nullptr;
            }

            start_page_info->size = size;
            start_page_info->is_start_of_buffer = true;
            return start_page_info->page_addr;
        }
    }


    void free(void* ptr) {
        if (!ptr) return;

        PageInfo* page_info = find_page_info(ptr);
        if (!page_info) return;

        if (page_info->size <= MAX_BLOCK_SIZE) {
            size_t bucket = get_bucket_index(page_info->size);
            FreeBlock* block = static_cast<FreeBlock*>(ptr);
            block->next = freelistarr[bucket];
            freelistarr[bucket] = block;
        } else if (page_info->is_start_of_buffer) {
            size_t pages_count = get_required_pages(page_info->size);

            // Free all pages in the allocation
            for (size_t i = 0; i < pages_count; ++i) {
                PageInfo* current = page_info + i;
                current->size = 0;
                current->is_start_of_buffer = false;
                insert_sorted(current);
            }

            coalesce_free_pages();
        }
    }

    ~McKusickAllocator() {
        if (metadata_memory) {
            munmap(metadata_memory, total_pages * PAGE_SIZE);
        }
    }
};
