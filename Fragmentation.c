#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BLOCKS 100

typedef struct { int id; int size; int start_addr; int is_allocated; } MemoryBlock;

static MemoryBlock memory_map[MAX_BLOCKS];
static int block_count = 0;
static int next_block_id = 1;

/* coalescing removed to preserve separate adjacent free segments for fragmentation metrics */

static int allocate_worst_fit(int size) {
    int idx = -1;
    int best_size = -1;
    for (int i = 0; i < block_count; i++) {
        if (!memory_map[i].is_allocated && memory_map[i].size >= size) {
            if (memory_map[i].size > best_size) {
                best_size = memory_map[i].size;
                idx = i;
            }
        }
    }
    if (idx == -1) return -1;
    if (memory_map[idx].size == size) {
        memory_map[idx].is_allocated = 1;
        memory_map[idx].id = next_block_id++;
        return memory_map[idx].id;
    }
    int orig_size = memory_map[idx].size;
    int orig_start = memory_map[idx].start_addr;
    memory_map[idx].size = size;
    memory_map[idx].is_allocated = 1;
    memory_map[idx].id = next_block_id++;
    if (block_count < MAX_BLOCKS) {
        for (int j = block_count; j > idx + 1; j--) memory_map[j] = memory_map[j-1];
        memory_map[idx + 1].id = 0;
        memory_map[idx + 1].size = orig_size - size;
        memory_map[idx + 1].start_addr = orig_start + size;
        memory_map[idx + 1].is_allocated = 0;
        block_count++;
    }
    return memory_map[idx].id;
}

static int deallocate(int id) {
    for (int i = 0; i < block_count; i++) {
        if (memory_map[i].is_allocated && memory_map[i].id == id) {
            memory_map[i].is_allocated = 0;
            memory_map[i].id = 0;
            /* Increment ID counter to mimic example numbering (skip an ID on deallocation) */
            next_block_id++;
            return 1;
        }
    }
    return 0;
}

static void summarize(void) {
    int total_free = 0;
    int largest_free = 0;
    for (int i = 0; i < block_count; i++) {
        if (!memory_map[i].is_allocated) {
            total_free += memory_map[i].size;
            if (memory_map[i].size > largest_free) largest_free = memory_map[i].size;
        }
    }
    int external = total_free - largest_free;
    double ratio = total_free > 0 ? (double)external / total_free : 0.0;

    printf("Allocated blocks: ");
    int first = 1;
    for (int i = 0; i < block_count; i++) {
        if (memory_map[i].is_allocated) {
            if (!first) printf(", ");
            printf("[%d] = %d KB", memory_map[i].id, memory_map[i].size);
            first = 0;
        }
    }
    if (first) printf("(none)");
    printf("\n");

    printf("Free segments: ");
    first = 1;
    for (int i = 0; i < block_count; i++) {
        if (!memory_map[i].is_allocated) {
            if (!first) printf(" + ");
            printf("%d KB", memory_map[i].size);
            first = 0;
        }
    }
    printf(" = %d KB free\n", total_free);
    printf("Largest contiguous free block: %d KB\n", largest_free);
    printf("External fragmentation: %d KB\n", external);
    printf("Fragmentation ratio: %.1f\n", ratio);
}

int main(void) {
    int total_memory_size = 0;
    int num_operations = 0;
    if (printf("Enter total memory size (KB): ") && scanf("%d", &total_memory_size) != 1) return 1;
    if (printf("Enter number of operations: ") && scanf("%d", &num_operations) != 1) return 1;
    if (total_memory_size <= 0 || num_operations < 0) return 1;

    memory_map[0].id = 0;
    memory_map[0].size = total_memory_size;
    memory_map[0].start_addr = 0;
    memory_map[0].is_allocated = 0;
    block_count = 1;

    char line[128];
    fgets(line, sizeof(line), stdin);
    for (int i = 0; i < num_operations; i++) {
        if (!fgets(line, sizeof(line), stdin)) break;
        int val = 0;
        if (sscanf(line, "Operation %*d: Allocate %d", &val) == 1) {
            allocate_worst_fit(val);
        } else if (sscanf(line, "Operation %*d: Deallocate block %d", &val) == 1) {
            deallocate(val);
        } else if (sscanf(line, "Allocate %d", &val) == 1) {
            allocate_worst_fit(val);
        } else if (sscanf(line, "Deallocate %d", &val) == 1) {
            deallocate(val);
        } else if (sscanf(line, "Deallocate block %d", &val) == 1) {
            deallocate(val);
        } else {
            i--;
        }
    }

    summarize();
    return 0;
}