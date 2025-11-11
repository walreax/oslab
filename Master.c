// Master.c
#include "common.h"

int main() {
    int shm_pt_id, shm_ffl_id, shm_lru_id, msg_id;
    PageTable *pt_shm;
    FreeFrameList *ffl_shm;
    LRUCounter *lru_counter_shm;

    printf("Master: Starting Simulation...\n");

    // 1. Initialize Shared Memory for LRU Counter
    shm_lru_id = shmget(SHM_LRU_COUNTER_KEY, sizeof(LRUCounter), IPC_CREAT | 0666);
    if (shm_lru_id == -1) { perror("shmget lru"); exit(1); }
    lru_counter_shm = (LRUCounter *)shmat(shm_lru_id, NULL, 0);
    if (lru_counter_shm == (void *)-1) { perror("shmat lru"); exit(1); }
    *lru_counter_shm = 0; // Initialize global time counter

    // 2. Initialize Shared Memory for Page Table
    shm_pt_id = shmget(SHM_PAGE_TABLE_KEY, sizeof(PageTable), IPC_CREAT | 0666);
    if (shm_pt_id == -1) { perror("shmget pt"); exit(1); }
    pt_shm = (PageTable *)shmat(shm_pt_id, NULL, 0);
    if (pt_shm == (void *)-1) { perror("shmat pt"); exit(1); }
    
    // Initialize Page Table: present = 0
    memset(pt_shm, 0, sizeof(PageTable));

    // 3. Initialize Shared Memory for Free Frame List
    shm_ffl_id = shmget(SHM_FRAME_LIST_KEY, sizeof(FreeFrameList), IPC_CREAT | 0666);
    if (shm_ffl_id == -1) { perror("shmget ffl"); exit(1); }
    ffl_shm = (FreeFrameList *)shmat(shm_ffl_id, NULL, 0);
    if (ffl_shm == (void *)-1) { perror("shmat ffl"); exit(1); }

    // Initialize Free Frame List: all frames available
    ffl_shm->free_frame_count = NUM_FRAMES;
    for (int i = 0; i < NUM_FRAMES; i++) {
        ffl_shm->free_frames[i] = NUM_FRAMES - 1 - i; // Fill the stack
    }

    // 4. Initialize Message Queue
    msg_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (msg_id == -1) { perror("msgget"); exit(1); }

    printf("Master: IPC resources created. Starting modules...\n");
    char pid_str[10];

    // 5. Create Child Processes (MMU, Scheduler, Processes)
    // MMU
    if (fork() == 0) {
        execlp("./MMU", "MMU", NULL);
        perror("execlp MMU"); exit(1);
    }

    // Scheduler
    if (fork() == 0) {
        execlp("./Scheduler", "Scheduler", NULL);
        perror("execlp Scheduler"); exit(1);
    }

    // Processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (fork() == 0) {
            sprintf(pid_str, "%d", i);
            execlp("./Process", "Process", pid_str, NULL);
            perror("execlp Process"); exit(1);
        }
    }

    // 6. Wait for all children to finish
    int active_children = NUM_PROCESSES + 2;
    while (active_children > 0) {
        wait(NULL);
        active_children--;
    }

    printf("Master: All modules terminated. Starting cleanup...\n");

    // 7. Cleanup
    shmdt(pt_shm); shmctl(shm_pt_id, IPC_RMID, NULL);
    shmdt(ffl_shm); shmctl(shm_ffl_id, IPC_RMID, NULL);
    shmdt(lru_counter_shm); shmctl(shm_lru_id, IPC_RMID, NULL);
    msgctl(msg_id, IPC_RMID, NULL);

    printf("Master: IPC resources released. Simulation finished.\n");
    return 0;
}