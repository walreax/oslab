#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

// --- Configuration ---
[cite_start]#define NUM_PAGES 10             // Pages per process (0-9) [cite: 36]
[cite_start]#define NUM_FRAMES 5             // Physical memory frames (1 to 7 possible, using 5) [cite: 37]
#define NUM_PROCESSES 2          // Number of processes
#define REFERENCE_STRING_LEN 15  // Length of the generated reference string

// --- IPC Keys (Use ftok for real systems, using fixed keys for simulation simplicity) ---
#define SHM_PAGE_TABLE_KEY 1000
#define SHM_FRAME_LIST_KEY 2000
#define MSG_QUEUE_KEY 3000

// --- Shared Data Structures ---

// Global LRU counter (shared variable)
#define SHM_LRU_COUNTER_KEY 4000
typedef long LRUCounter;

// Page Table Entry (PTE)
typedef struct {
    int frame_number;
    int present;      // 1 if page is in memory
    long last_access; [cite_start]// For LRU: timestamp of last access [cite: 107]
    int process_id;   // Owner Process ID
} PTE;

// Shared Memory Structure for the Page Table
typedef struct {
    PTE table[NUM_PROCESSES][NUM_PAGES];
} PageTable;

// Shared Memory Structure for the Free Frame List
typedef struct {
    int free_frame_count;
    int free_frames[NUM_FRAMES]; // Acts as a stack for available frames
} FreeFrameList;

// --- Message Queue Structure for IPC (Request/Response) ---
typedef struct {
    long mtype;       // Used for routing messages (PID + 1 or other unique ID)
    int sender_pid;   // Which Process sent the request (0 to NUM_PROCESSES-1)
    int page_number;  // The requested page
    int status;       // 0: Request, 1: Page Fault, 2: Hit, 3: Finished, 4: Ready_To_Run
} Message;

// Define message types for clarity
#define MT_PROCESS_REQUEST (long)1
#define MT_MMU_RESPONSE    (long)2
#define MT_SCHEDULER_CMD   (long)3

#endif // COMMON_H