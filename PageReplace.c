// PageReplace.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_PAGES 100 // Length of the reference string
#define MIN_FRAMES 1
#define MAX_FRAMES 7  // Number of page frames varies from 1 to 7 [cite: 37]
#define PAGE_RANGE 10 // Page numbers range from 0 to 9 [cite: 36]

// Function to simulate FIFO algorithm
int fifo(int frames, int *ref_string, int len) {
    int page_faults = 0;
    int memory[MAX_FRAMES];
    int next_replace_index = 0;
    
    for (int i = 0; i < MAX_FRAMES; i++) memory[i] = -1; // -1 means empty

    for (int i = 0; i < len; i++) {
        int page = ref_string[i];
        int hit = 0;

        // Check for hit
        for (int j = 0; j < frames; j++) {
            if (memory[j] == page) {
                hit = 1;
                break;
            }
        }

        if (!hit) {
            // Page Fault
            page_faults++;
            // Replace the page at the next_replace_index (FIFO)
            memory[next_replace_index] = page;
            next_replace_index = (next_replace_index + 1) % frames;
        }
    }
    return page_faults;
}

// Function to simulate LRU algorithm
int lru(int frames, int *ref_string, int len) {
    int page_faults = 0;
    int memory[MAX_FRAMES];
    long last_used[MAX_FRAMES];
    long timer = 0; // Simulated time for tracking usage
    
    for (int i = 0; i < MAX_FRAMES; i++) {
        memory[i] = -1;
        last_used[i] = 0;
    }

    for (int i = 0; i < len; i++) {
        int page = ref_string[i];
        int hit_index = -1;

        // Check for hit and update usage
        for (int j = 0; j < frames; j++) {
            if (memory[j] == page) {
                hit_index = j;
                break;
            }
        }
        
        if (hit_index != -1) {
            // Page Hit: Update LRU timestamp
            last_used[hit_index] = ++timer;
        } else {
            // Page Fault
            page_faults++;
            int empty_index = -1;

            // 1. Check for empty frame
            for (int j = 0; j < frames; j++) {
                if (memory[j] == -1) {
                    empty_index = j;
                    break;
                }
            }
            
            if (empty_index != -1) {
                // Load into empty frame
                memory[empty_index] = page;
                last_used[empty_index] = ++timer;
            } else {
                // 2. No empty frame, find LRU victim
                int lru_index = 0;
                long min_lru_time = last_used[0];

                for (int j = 1; j < frames; j++) {
                    if (last_used[j] < min_lru_time) {
                        min_lru_time = last_used[j];
                        lru_index = j;
                    }
                }

                // Replace the LRU page
                memory[lru_index] = page;
                last_used[lru_index] = ++timer;
            }
        }
    }
    return page_faults;
}


int main() {
    srand(time(NULL));

    // Generate random page-reference string
    int ref_string[MAX_PAGES];
    printf("Generated Page Reference String (%d items): ", MAX_PAGES);
    for (int i = 0; i < MAX_PAGES; i++) {
        ref_string[i] = rand() % PAGE_RANGE;
        printf("%d ", ref_string[i]);
    }
    printf("\n\n");

    printf("--- Page Fault Comparison ---\n");
    printf("+------------+------------+------------+\n");
    printf("| Frame Size | FIFO Faults| LRU Faults |\n");
    printf("+------------+------------+------------+\n");
    
    // Simulate for frame sizes 1 to 7
    for (int frames = MIN_FRAMES; frames <= MAX_FRAMES; frames++) {
        int fifo_faults = fifo(frames, ref_string, MAX_PAGES);
        int lru_faults = lru(frames, ref_string, MAX_PAGES);

        printf("| %10d | %10d | %10d |\n", frames, fifo_faults, lru_faults);
    }
    
    printf("+------------+------------+------------+\n");
    printf("Note: LRU usually results in fewer page faults than FIFO.\n");
    
    return 0;
}