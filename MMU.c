// MMU.c
#include "common.h"

int main() {
    int shm_pt_id, shm_ffl_id, shm_lru_id, msg_id;
    PageTable *pt_shm;
    FreeFrameList *ffl_shm;
    LRUCounter *lru_counter_shm;
    
    // Attach to IPC resources
    shm_lru_id = shmget(SHM_LRU_COUNTER_KEY, sizeof(LRUCounter), 0666);
    lru_counter_shm = (LRUCounter *)shmat(shm_lru_id, NULL, 0);
    shm_pt_id = shmget(SHM_PAGE_TABLE_KEY, sizeof(PageTable), 0666);
    pt_shm = (PageTable *)shmat(shm_pt_id, NULL, 0);
    shm_ffl_id = shmget(SHM_FRAME_LIST_KEY, sizeof(FreeFrameList), 0666);
    ffl_shm = (FreeFrameList *)shmat(shm_ffl_id, NULL, 0);
    msg_id = msgget(MSG_QUEUE_KEY, 0666);
    
    if (pt_shm == (void *)-1 || ffl_shm == (void *)-1 || lru_counter_shm == (void *)-1 || msg_id == -1) { 
        perror("MMU shm/msg attach failed"); exit(1); 
    }

    [cite_start]printf("MMU started.\n"); [cite: 27]

    while (1) {
        Message request;
        // 1. Receive Page Request from Process
        if (msgrcv(msg_id, &request, sizeof(Message) - sizeof(long), MT_PROCESS_REQUEST, 0) == -1) {
             // Check if the queue was removed by master (end of simulation)
             break;
        }
        
        int pid = request.sender_pid;
        int page = request.page_number;
        
        // Handle Process Finished signal sent through the same channel
        if (request.status == 3) {
            Message response = {.mtype = MT_MMU_RESPONSE, .sender_pid = pid, .status = 3};
            msgsnd(msg_id, &response, sizeof(Message) - sizeof(long), 0);
            continue;
        }

        [cite_start]printf("MMU: Process %d requests page %d\n", pid, page); [cite: 28]

        [cite_start]// Check for illegal reference [cite: 9]
        if (page >= NUM_PAGES || page < 0) {
            printf("MMU: Illegal page reference by Process %d, Page %d. Terminating process.\n", pid, page);
            [cite_start]// In a real system, the process would be terminated [cite: 9]
            Message response = {.mtype = MT_MMU_RESPONSE, .sender_pid = pid, .status = 3};
            msgsnd(msg_id, &response, sizeof(Message) - sizeof(long), 0);
            continue;
        }

        // 2. Consult Page Table
        if (pt_shm->table[pid][page].present == 1) {
            [cite_start]// Page Hit [cite: 9]
            pt_shm->table[pid][page].last_access = ++(*lru_counter_shm);
            // Send 'Hit' status to Scheduler
            Message response = {.mtype = MT_MMU_RESPONSE, .sender_pid = pid, .status = 2};
            msgsnd(msg_id, &response, sizeof(Message) - sizeof(long), 0);
        } else {
            [cite_start]// Page Fault occurs [cite: 10]
            
            [cite_start]// 3. Page Fault Handler Routine [cite: 12]
            int frame_to_use = -1;
            
            if (ffl_shm->free_frame_count > 0) {
                [cite_start]// Case A: Free frame available [cite: 13, 14]
                frame_to_use = ffl_shm->free_frames[--ffl_shm->free_frame_count];
            } else {
                [cite_start]// Case B: No free frame, use LRU replacement [cite: 15]
                int victim_pid = -1, victim_page = -1;
                long min_lru_time = -1;
                
                // Find Victim Page (using LRU) 
                for (int i = 0; i < NUM_PROCESSES; i++) {
                    for (int j = 0; j < NUM_PAGES; j++) {
                        if (pt_shm->table[i][j].present == 1) {
                            if (min_lru_time == -1 || pt_shm->table[i][j].last_access < min_lru_time) {
                                min_lru_time = pt_shm->table[i][j].last_access;
                                victim_pid = i;
                                victim_page = j;
                            }
                        }
                    }
                }
                
                // Evict Victim
                if (victim_pid != -1) {
                    frame_to_use = pt_shm->table[victim_pid][victim_page].frame_number;
                    pt_shm->table[victim_pid][victim_page].present = 0;
                    printf("MMU: LRU replacement. Evicting P%d, Page %d from Frame %d\n", 
                           victim_pid, victim_page, frame_to_use);
                } else {
                    // Should not happen if physical memory is full and the process is running
                    fprintf(stderr, "MMU Error: No victim found despite full memory.\n");
                    continue; 
                }
            }
            
            [cite_start]// 4. Load Page (Simulated I/O) [cite: 10]
            pt_shm->table[pid][page].frame_number = frame_to_use;
            pt_shm->table[pid][page].present = 1;
            pt_shm->table[pid][page].last_access = ++(*lru_counter_shm);
            pt_shm->table[pid][page].process_id = pid;

            [cite_start]printf("Page Fault handled for Process %d, Page %d -> Frame %d\n", pid, page, frame_to_use); [cite: 29, 31]
            
            [cite_start]// 5. Send 'Page Fault' status to Scheduler (context switch) [cite: 11]
            Message response = {.mtype = MT_MMU_RESPONSE, .sender_pid = pid, .status = 1};
            msgsnd(msg_id, &response, sizeof(Message) - sizeof(long), 0);
        }
    }
    
    [cite_start]printf("MMU terminating.\n"); [cite: 33]
    // Detach shared memory
    shmdt(pt_shm); shmdt(ffl_shm); shmdt(lru_counter_shm);
    return 0;
}