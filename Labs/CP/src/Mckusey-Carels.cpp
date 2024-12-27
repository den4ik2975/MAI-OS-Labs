#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <iostream>

static const size_t PAGE_SIZE = 4096;      // Размер страницы памяти в байтах
static const size_t MIN_BLOCK_SIZE = 32;   // Минимальный размер блока памяти
static const size_t MAX_BLOCK_SIZE = 4096; // Максимальный размер блока памяти
static const size_t BUCKET_COUNT = 8;      // Количество корзин для разных размеров блоков
                                          // Размеры: 32, 64, 128, 256, 512, 1024, 2048, 4096

class McKusickAllocator {
private:
    // Структура для хранения информации о свободном блоке памяти
    struct FreeBlock {
        FreeBlock* next; // Указатель на следующий свободный блок
    };

    // Структура для хранения метаданных страницы
    struct PageInfo {
        void* page_addr;         // Указатель на начало страницы
        size_t size;            // Размер блоков на странице
        PageInfo* next;         // Следующая страница в списке
        bool is_start_of_buffer; // Флаг начала буфера (для многостраничных аллокаций)
    };

    FreeBlock* freelistarr[BUCKET_COUNT];  // Массив списков свободных блоков
    PageInfo* free_page_info;              // Список информации о свободных страницах
    void* initial_memory;                  // Указатель на начало выделенной памяти
    void* metadata_memory;                 // Память для метаданных
    size_t total_pages;                    // Общее количество страниц
    size_t total_meta_pages;               // Количество страниц под метаданные

    // Определяет индекс корзины для заданного размера блока
    static size_t get_bucket_index(size_t size) {
        if (size > MAX_BLOCK_SIZE) return BUCKET_COUNT;

        // Нормализация размера до ближайшей степени двойки
        size_t normalized = std::max(MIN_BLOCK_SIZE,
                                   size_t(1) << (64 - __builtin_clzl(size - 1)));

        return __builtin_ctz(normalized) - 5;  // 5 = log2(32)
    }

    // Вычисляет необходимое количество страниц для заданного размера
    static size_t get_required_pages(size_t size) {
        return (size + PAGE_SIZE - 1) / PAGE_SIZE;
    }

    // Вставляет страницу в отсортированный список свободных страниц
    void insert_sorted(PageInfo* page) {
        // Поддержание сортировки списка по адресам
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

    // Поиск последовательных свободных страниц
    PageInfo* find_consecutive_pages(size_t num_pages) {
        if (num_pages == 0) return nullptr;

        PageInfo* current = free_page_info;
        PageInfo* prev = nullptr;

        while (current) {
            PageInfo* potential_start = current;
            size_t found_consecutive = 1;
            PageInfo* page_walker = current;

            // Проверка последовательности страниц
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
                // Найдено достаточное количество последовательных страниц
                PageInfo* next_after_range = page_walker->next;

                // Удаление диапазона из списка свободных
                if (prev)
                    prev->next = next_after_range;
                else
                    free_page_info = next_after_range;

                // Связывание выделенных страниц
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


    // Разделение страницы на блоки заданного размера
    void split_page(PageInfo* page_info, size_t block_size) {
        size_t blocks_per_page = PAGE_SIZE / block_size;
        page_info->size = block_size;
        page_info->is_start_of_buffer = false;

        uint8_t* block_ptr = static_cast<uint8_t*>(page_info->page_addr);
        size_t bucket = get_bucket_index(block_size);

        // Создание связанного списка блоков
        for (size_t i = 0; i < blocks_per_page - 1; ++i) {
            FreeBlock* block = reinterpret_cast<FreeBlock*>(block_ptr);
            block->next = reinterpret_cast<FreeBlock*>(block_ptr + block_size);
            block_ptr += block_size;
        }

        // Подключение к существующему списку свободных блоков
        FreeBlock* last_block = reinterpret_cast<FreeBlock*>(block_ptr);
        last_block->next = freelistarr[bucket];
        freelistarr[bucket] = reinterpret_cast<FreeBlock*>(page_info->page_addr);
    }

    // Поиск информации о странице по указателю
    PageInfo* find_page_info(void* ptr) {
        if (!ptr) return nullptr;

        // Вычисление адреса начала страницы
        uintptr_t page_addr = reinterpret_cast<uintptr_t>(ptr) & ~(PAGE_SIZE - 1);

        // Вычисление индекса в массиве метаданных
        uintptr_t base_addr = reinterpret_cast<uintptr_t>(initial_memory);
        size_t page_index = (page_addr - base_addr) / PAGE_SIZE;

        // Проверка валидности индекса
        if (page_index >= (total_pages - total_meta_pages)) return nullptr;

        return static_cast<PageInfo*>(metadata_memory) + page_index;
    }

public:
    // Конструктор аллокатора
    McKusickAllocator(size_t memory_size = 16 * PAGE_SIZE) :
    free_page_info(nullptr) {
        total_pages = (memory_size + PAGE_SIZE - 1) / PAGE_SIZE;

        std::memset(freelistarr, 0, sizeof(freelistarr));

        // Выделение памяти одним вызовом mmap
        void* memory = mmap(nullptr, total_pages * PAGE_SIZE,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS,
                           -1, 0);

        if (memory == MAP_FAILED) {
            throw std::runtime_error("Не удалось выделить память");
        }

        // Расчет размера метаданных
        total_meta_pages = (total_pages * sizeof(PageInfo)) / PAGE_SIZE;
        if ((total_meta_pages * PAGE_SIZE) < total_pages * sizeof(PageInfo)) {
            total_meta_pages++;
        }

        // Установка указателей на области памяти
        metadata_memory = memory;
        initial_memory = static_cast<char*>(memory) + PAGE_SIZE * total_meta_pages;

        // Инициализация метаданных страниц
        PageInfo* info_ptr = static_cast<PageInfo*>(metadata_memory);
        uint8_t* mem_ptr = static_cast<uint8_t*>(initial_memory);

        // Инициализация в порядке возрастания адресов
        for (size_t i = 0; i < total_pages - total_meta_pages; ++i) {
            info_ptr[i].page_addr = mem_ptr + (i * PAGE_SIZE);
            info_ptr[i].size = 0;
            info_ptr[i].is_start_of_buffer = false;
        }

        // Связывание страниц в список
        for (size_t i = 0; i < total_pages - total_meta_pages - 1; ++i) {
            info_ptr[i].next = &info_ptr[i + 1];
        }
        info_ptr[total_pages - total_meta_pages - 1].next = nullptr;

        free_page_info = &info_ptr[0];  // Начало с первой страницы
    }

    // Выделение памяти заданного размера
    void* alloc(size_t size) {
        if (size == 0) return nullptr;

        if (size <= MAX_BLOCK_SIZE) {
            // Выделение маленьких блоков
            size_t bucket = get_bucket_index(size);

            if (freelistarr[bucket] == nullptr) {
                if (free_page_info == nullptr) {
                    return nullptr;
                }

                // Разделение новой страницы на блоки
                PageInfo* page_info = free_page_info;
                free_page_info = free_page_info->next;
                split_page(page_info, MIN_BLOCK_SIZE << bucket);
            }

            // Получение блока из списка свободных
            FreeBlock* block = freelistarr[bucket];
            freelistarr[bucket] = block->next;
            return block;
        } else {
            // Выделение больших блоков
            size_t pages_needed = get_required_pages(size);

            PageInfo* start_page_info = find_consecutive_pages(pages_needed);
            if (!start_page_info) {
                return nullptr;
            }

            start_page_info->size = size;
            start_page_info->is_start_of_buffer = true;
            return start_page_info->page_addr;
        }
    }

    // Освобождение выделенной памяти
    void free(void* ptr) {
        if (!ptr) return;

        PageInfo* page_info = find_page_info(ptr);
        if (!page_info) return;

        if (page_info->size <= MAX_BLOCK_SIZE) {
            // Освобождение маленького блока
            size_t bucket = get_bucket_index(page_info->size);
            FreeBlock* block = static_cast<FreeBlock*>(ptr);
            block->next = freelistarr[bucket];
            freelistarr[bucket] = block;
        } else if (page_info->is_start_of_buffer) {
            // Освобождение большого блока
            size_t pages_count = get_required_pages(page_info->size);

            // Освобождение всех страниц в аллокации
            for (size_t i = 0; i < pages_count; ++i) {
                PageInfo* current = page_info + i;
                current->size = 0;
                current->is_start_of_buffer = false;
                insert_sorted(current);
            }

        }
    }

    // Деструктор
    ~McKusickAllocator() {
        if (metadata_memory) {
            munmap(metadata_memory, total_pages * PAGE_SIZE);
        }
    }
};
