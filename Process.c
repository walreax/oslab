// Process.c
#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Process requires a PID argument.\n");
        return 1;
    }
    int my_pid = atoi(argv[1]);
    
    int msg_id = msgget(MSG_QUEUE_KEY, 0666);
    if (msg_id == -1) { perror("msgget Process"); exit(1); }
    
    // Seed random generation uniquely for each process
    srand(time(NULL) * my_pid + getpid()); 

    // Generate a fixed page-reference string for this process
    int reference_string[REFERENCE_STRING_LEN];
    for (int i = 0; i < REFERENCE_STRING_LEN; i++) {
        [cite_start]// Generate page numbers from 0 to 9 [cite: 36]
        reference_string[i] = rand() % NUM_PAGES; 
    }
    
    printf("Process %d started. Reference string generated.\n", my_pid);

    // Execution loop
    for (int i = 0; i < REFERENCE_STRING_LEN; i++) {
        // 1. Wait for 'Ready to Run' signal from Scheduler
        Message cmd;
        if (msgrcv(msg_id, &cmd, sizeof(Message) - sizeof(long), MT_SCHEDULER_CMD, 0) == -1) {
            break; // Queue removed, simulation finished
        }
        if (cmd.sender_pid != my_pid || cmd.status != 4) {
            // Not my command, re-queue and continue
            msgsnd(msg_id, &cmd, sizeof(Message) - sizeof(long), 0);
            continue;
        }

        // 2. Send Page Request to MMU
        Message request;
        request.mtype = MT_PROCESS_REQUEST;
        request.sender_pid = my_pid;
        request.page_number = reference_string[i];
        request.status = 0; // Request
        
        if (msgsnd(msg_id, &request, sizeof(Message) - sizeof(long), 0) == -1) {
            perror("msgsnd Process request");
            break;
        }
        
        // 3. Process is now waiting for MMU/Scheduler to resolve the request (Handled by Scheduler)
        
        // Simulate instruction execution time
        usleep(100); 
    }
    
    [cite_start]printf("Process %d finished.\n", my_pid); [cite: 32]
    
    // 4. Notify MMU/Scheduler of completion
    Message request;
    request.mtype = MT_PROCESS_REQUEST;
    request.sender_pid = my_pid;
    request.page_number = -1; // Sentinel
    request.status = 3; // Finished
    msgsnd(msg_id, &request, sizeof(Message) - sizeof(long), 0);

    return 0;
}