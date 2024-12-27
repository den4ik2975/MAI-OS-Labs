#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <stdexcept>

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
        return (normalized / MIN_BLOCK_SIZE) - 1;
    }

    static size_t get_required_pages(size_t size) {
        return (size + PAGE_SIZE - 1) / PAGE_SIZE;
    }

    PageInfo* find_consecutive_pages(size_t num_pages) {
        PageInfo* current = free_page_info;
        PageInfo* start = nullptr;
        size_t consecutive = 0;

        while (current && consecutive < num_pages) {
            uintptr_t expected_next = reinterpret_cast<uintptr_t>(current->page_addr) + PAGE_SIZE;

            if (!start) {
                start = current;
                consecutive = 1;
            } else if (reinterpret_cast<uintptr_t>(current->page_addr) == expected_next) {
                consecutive++;
            } else {
                start = current;
                consecutive = 1;
            }

            if (consecutive == num_pages) {
                // Remove these pages from free list
                PageInfo* new_free_head = current->next;
                PageInfo* prev = nullptr;
                current = free_page_info;

                while (current != start) {
                    prev = current;
                    current = current->next;
                }

                if (prev) prev->next = new_free_head;
                else free_page_info = new_free_head;

                return start;
            }

            current = current->next;
        }

        return nullptr;
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

public:
    McKusickAllocator(size_t pages_count = 16) :
        free_page_info(nullptr),
        total_pages(pages_count) {

        std::memset(freelistarr, 0, sizeof(freelistarr));

        // Allocate memory for pages
        size_t total_size = pages_count * PAGE_SIZE;
        initial_memory = mmap(nullptr, total_size,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);

        if (initial_memory == MAP_FAILED) {
            throw std::runtime_error("Failed to allocate memory for pages");
        }

        // Allocate memory for metadata
        size_t metadata_size = pages_count * sizeof(PageInfo);
        metadata_memory = mmap(nullptr, metadata_size,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS,
                             -1, 0);

        if (metadata_memory == MAP_FAILED) {
            munmap(initial_memory, total_size);
            throw std::runtime_error("Failed to allocate memory for metadata");
        }

        // Initialize page info structures
        PageInfo* info_ptr = static_cast<PageInfo*>(metadata_memory);
        uint8_t* mem_ptr = static_cast<uint8_t*>(initial_memory);

        for (size_t i = 0; i < pages_count; ++i) {
            info_ptr[i].page_addr = mem_ptr + (i * PAGE_SIZE);
            info_ptr[i].size = 0;
            info_ptr[i].is_start_of_buffer = false;
            info_ptr[i].next = free_page_info;
            free_page_info = &info_ptr[i];
        }
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
            PageInfo* start_page_info = find_consecutive_pages(pages_needed);

            if (!start_page_info) return nullptr;

            start_page_info->size = size;
            start_page_info->is_start_of_buffer = true;
            return start_page_info->page_addr;
        }
    }

    void free(void* ptr) {
        if (!ptr) return;

        // Find corresponding page info
        PageInfo* info_ptr = static_cast<PageInfo*>(metadata_memory);
        PageInfo* page_info = nullptr;

        for (size_t i = 0; i < total_pages; ++i) {
            if (info_ptr[i].page_addr == ptr ||
                (ptr >= info_ptr[i].page_addr &&
                 ptr < static_cast<uint8_t*>(info_ptr[i].page_addr) + PAGE_SIZE)) {
                page_info = &info_ptr[i];
                break;
            }
        }

        if (!page_info) return;

        if (page_info->size <= MAX_BLOCK_SIZE) {
            size_t bucket = get_bucket_index(page_info->size);
            FreeBlock* block = static_cast<FreeBlock*>(ptr);
            block->next = freelistarr[bucket];
            freelistarr[bucket] = block;
        } else if (page_info->is_start_of_buffer) {
            size_t pages_count = get_required_pages(page_info->size);

            for (size_t i = 0; i < pages_count; ++i) {
                PageInfo* current = page_info + i;
                current->next = free_page_info;
                current->is_start_of_buffer = false;
                current->size = 0;
                free_page_info = current;
            }
        }
    }

    ~McKusickAllocator() {
        if (initial_memory) {
            munmap(initial_memory, total_pages * PAGE_SIZE);
        }
        if (metadata_memory) {
            munmap(metadata_memory, total_pages * sizeof(PageInfo));
        }
    }
};
