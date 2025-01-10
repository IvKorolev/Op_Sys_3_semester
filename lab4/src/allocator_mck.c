#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include "allocator.h"

#define ALIGN_SIZE(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))
#define FREE_LIST_ALIGNMENT 8

typedef struct Block {
    size_t size;
    struct Block *next_block;
} Block;

struct Allocator {
    char *base_addr;
    size_t total_size;
    Block *free_list_head;
};

EXPORT Allocator *allocator_create(void *memory_region, const size_t region_size) {
    if (memory_region == NULL || region_size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *) memory_region;
    allocator->base_addr = (char *) memory_region + sizeof(Allocator);
    allocator->total_size = region_size - sizeof(Allocator);
    allocator->free_list_head = (Block *) allocator->base_addr;

    allocator->free_list_head->size = allocator->total_size - sizeof(Block);
    allocator->free_list_head->next_block = NULL;

    return allocator;
}

EXPORT void *allocator_alloc(Allocator *allocator, size_t alloc_size) {
    if (allocator == NULL || alloc_size == 0) {
        return NULL;
    }

    size_t aligned_size = ALIGN_SIZE(alloc_size, FREE_LIST_ALIGNMENT);
    Block *previous_block = NULL;
    Block *current_block = allocator->free_list_head;

    while (current_block != NULL) {
        if (current_block->size >= aligned_size) { // достаточно ли места в блоке
            size_t remaining_size = current_block->size - aligned_size;

            if (remaining_size >= sizeof(Block) + FREE_LIST_ALIGNMENT) {
                // Разделяем блок
                Block *new_block = (Block *)((char *)current_block + aligned_size + sizeof(Block));
                new_block->size = remaining_size;
                new_block->next_block = current_block->next_block;
                if (previous_block != NULL) {
                    previous_block->next_block = new_block;
                } else {
                    allocator->free_list_head = new_block;
                }
            } else {
                // Используем весь блок
                if (previous_block != NULL) {
                    previous_block->next_block = current_block->next_block;
                } else {
                    allocator->free_list_head = current_block->next_block;
                }
            }

            return (void *)((char *)current_block + sizeof(Block));
        }

        previous_block = current_block;
        current_block = current_block->next_block;
    }

    return NULL; // не смогли выделить память
}

EXPORT void allocator_free(Allocator *allocator, void *memory_block) {
    if (allocator == NULL || memory_block == NULL) {
        return;
    }

    Block *block_to_free = (Block *)((char *)memory_block - sizeof(Block));
    Block *current = allocator->free_list_head;
    Block *prev = NULL;

    while (current != NULL && current < block_to_free) {
        prev = current;
        current = current->next_block;
    }

    if (prev != NULL && (char *)prev + prev->size + sizeof(Block) == (char *)block_to_free) {
        prev->size += block_to_free->size + sizeof(Block);
        block_to_free = prev;
    } else {
        block_to_free->next_block = current;
        if (prev != NULL) {
            prev->next_block = block_to_free;
        } else {
            allocator->free_list_head = block_to_free;
        }
    }

    if (current != NULL && (char *)block_to_free + block_to_free->size + sizeof(Block) == (char *)current) {
        block_to_free->size += current->size + sizeof(Block);
        block_to_free->next_block = current->next_block;
    }
}

EXPORT void allocator_destroy(Allocator *allocator) {
    if (allocator != NULL) {
        allocator->base_addr = NULL;
        allocator->total_size = 0;
        allocator->free_list_head = NULL;
    }
}
