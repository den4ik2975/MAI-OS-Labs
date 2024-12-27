#include <cstddef>
#include <sys/mman.h>
#include <stdexcept>
#include <unistd.h>
#include <cstring>

class BestFitAllocator {
private:
    struct BlockHeader {
        size_t size;           // Размер блока включая заголовок
        bool is_free;          // Флаг, свободен ли блок
        BlockHeader* next;     // Указатель на следующий блок
        BlockHeader* prev;     // Указатель на предыдущий блок
    };

    BlockHeader* head;         // Начало списка блоков
    size_t total_size;        // Общий размер выделенной памяти
    static const size_t MINIMUM_BLOCK_SIZE = sizeof(BlockHeader) + 8;

public:
    BestFitAllocator(size_t memory_size = 16 * 4096) {
        // Округляем размер до страницы
        total_size = (memory_size + 7) & ~7;

        // Выделяем память через mmap
        void* memory = mmap(nullptr, total_size,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS,
                          -1, 0);

        if (memory == MAP_FAILED) {
            throw std::runtime_error("Failed to allocate memory");
        }

        // Инициализируем первый блок
        head = static_cast<BlockHeader*>(memory);
        head->size = total_size;
        head->is_free = true;
        head->next = nullptr;
        head->prev = nullptr;
    }

    ~BestFitAllocator() {
        munmap(head, total_size);
    }

    void* alloc(size_t size) {
        if (size == 0) return nullptr;

        // Выравниваем размер и добавляем место под заголовок
        size_t aligned_size = (size + sizeof(BlockHeader) + 7) & ~7;

        // Ищем наиболее подходящий блок
        BlockHeader* best_block = nullptr;
        BlockHeader* current = head;
        size_t min_suitable_size = total_size + 1;

        while (current != nullptr) {
            if (current->is_free && current->size >= aligned_size) {
                if (current->size < min_suitable_size) {
                    min_suitable_size = current->size;
                    best_block = current;
                }
            }
            current = current->next;
        }

        if (!best_block) return nullptr;

        // Если блок достаточно большой, разделяем его
        if (best_block->size >= aligned_size + MINIMUM_BLOCK_SIZE) {
            BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                reinterpret_cast<char*>(best_block) + aligned_size
            );

            new_block->size = best_block->size - aligned_size;
            new_block->is_free = true;
            new_block->next = best_block->next;
            new_block->prev = best_block;

            if (best_block->next) {
                best_block->next->prev = new_block;
            }

            best_block->size = aligned_size;
            best_block->next = new_block;
        }

        best_block->is_free = false;
        return reinterpret_cast<char*>(best_block) + sizeof(BlockHeader);
    }

    void free(void* ptr) {
        if (!ptr) return;

        BlockHeader* block = reinterpret_cast<BlockHeader*>(
            static_cast<char*>(ptr) - sizeof(BlockHeader)
        );

        block->is_free = true;

        // Слияние с последующим свободным блоком
        if (block->next && block->next->is_free) {
            block->size += block->next->size;
            block->next = block->next->next;
            if (block->next) {
                block->next->prev = block;
            }
        }

        // Слияние с предыдущим свободным блоком
        if (block->prev && block->prev->is_free) {
            block->prev->size += block->size;
            block->prev->next = block->next;
            if (block->next) {
                block->next->prev = block->prev;
            }
        }
    }

};
