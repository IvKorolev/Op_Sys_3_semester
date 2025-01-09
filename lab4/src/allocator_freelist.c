#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include "allocator.h"

typedef struct FreeBlock {
    size_t size;
    struct FreeBlock *next;
} FreeBlock;

struct Allocator {
    void *memory_start;
    size_t memory_size;
    FreeBlock *free_list;
};

EXPORT Allocator* allocator_create(void *const memory, const size_t size) {
    if (!memory || size < sizeof(FreeBlock)) return NULL;

    Allocator *allocator = malloc(sizeof(Allocator));
    if (!allocator) return NULL;

    allocator->memory_start = memory;
    allocator->memory_size = size;
    allocator->free_list = (FreeBlock *)memory;
    allocator->free_list->size = size;
    allocator->free_list->next = NULL;

    return allocator;
}

FreeBlock* find_best_fit(FreeBlock **head, size_t size, FreeBlock ***prev) {
    FreeBlock *best_fit = NULL;
    FreeBlock **best_fit_prev = NULL;
    FreeBlock **current = head;

    while (*current) {
        if ((*current)->size >= size) {
            if (!best_fit || (*current)->size < best_fit->size) {
                best_fit = *current;
                best_fit_prev = current;
            }
        }
        current = &((*current)->next);
    }

    if (prev) *prev = best_fit_prev;
    return best_fit;
}

EXPORT void* allocator_alloc(Allocator *const allocator, const size_t size) {
    if (!allocator || size == 0) return NULL;

    FreeBlock **prev = NULL;
    FreeBlock *best_fit = find_best_fit(&allocator->free_list, size + sizeof(FreeBlock), &prev);

    if (!best_fit) {
        return NULL;
    }

    size_t remaining_size = best_fit->size - size - sizeof(FreeBlock);

    if (remaining_size >= sizeof(FreeBlock)) {
        FreeBlock *new_block = (FreeBlock *)((char *)best_fit + sizeof(FreeBlock) + size);
        new_block->size = remaining_size;
        new_block->next = best_fit->next;

        if (prev) {
            *prev = new_block;
        }
    } else {
        if (prev) {
            *prev = best_fit->next;
        }
    }

    best_fit->size = size;
    return (void *)((char *)best_fit + sizeof(FreeBlock));
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
    if (!allocator || !memory) return;

    FreeBlock *block_to_free = (FreeBlock *)((char *)memory - sizeof(FreeBlock));
    FreeBlock **current = &allocator->free_list;

    while (*current && *current < block_to_free) {
        current = &((*current)->next);
    }

    block_to_free->next = *current;
    *current = block_to_free;

    if ((char *)block_to_free + sizeof(FreeBlock) + block_to_free->size == (char *)block_to_free->next) {
        block_to_free->size += sizeof(FreeBlock) + block_to_free->next->size;
        block_to_free->next = block_to_free->next->next;
    }

    if (current != &allocator->free_list && (char *)(*current) + sizeof(FreeBlock) + (*current)->size == (char *)block_to_free) {
        (*current)->size += sizeof(FreeBlock) + block_to_free->size;
        (*current)->next = block_to_free->next;
    }
}

EXPORT void allocator_destroy(Allocator *const allocator) {
    if (allocator) {
        allocator->memory_start = NULL;
        allocator->free_list = NULL;
        allocator->memory_size = 0;
        free(allocator);
    }
}
