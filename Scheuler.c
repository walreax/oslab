// Scheduler.c
#include "common.h"

int main() {
    int msg_id = msgget(MSG_QUEUE_KEY, 0666);
    if (msg_id == -1) { perror("msgget Scheduler"); exit(1); }

    // Simple FCFS Ready Queue implementation
    int ready_queue[NUM_PROCESSES];
    int head = 0, tail = 0;
    int processes_finished = 0;

    // Initially, all processes are in the ready queue
    for (int i = 0; i < NUM_PROCESSES; i++) {
        ready_queue[tail++] = i;
    }

    printf("Scheduler started. (FCFS)\n");

    while (processes_finished < NUM_PROCESSES) {
        if (head != tail) {
            int current_pid = ready_queue[head];
            
            // 1. Send 'Ready to Run' signal to current_pid
            Message cmd;
            cmd.mtype = MT_SCHEDULER_CMD;
            cmd.sender_pid = current_pid;
            cmd.status = 4; // Ready_To_Run

            if (msgsnd(msg_id, &cmd, sizeof(Message) - sizeof(long), 0) == -1) {
                perror("msgsnd Scheduler CMD");
            }
            
            // 2. Wait for Status Update from MMU (will carry the PID of the process that generated the event)
            Message response;
            if (msgrcv(msg_id, &response, sizeof(Message) - sizeof(long), MT_MMU_RESPONSE, 0) == -1) {
                 // Check if the queue was removed by master (end of simulation)
                 if (processes_finished < NUM_PROCESSES) {
                    perror("msgrcv Scheduler response");
                 }
                 break;
            }

            // The scheduler handles events based on the process that just ran (response.sender_pid)
            int event_pid = response.sender_pid;

            if (response.status == 1) { // Page Fault occurred
                [cite_start]// Context switch: Put the process at the back of the queue (FCFS) [cite: 11]
                ready_queue[tail % NUM_PROCESSES] = event_pid;
                tail = (tail + 1) % NUM_PROCESSES;
                head = (head + 1) % NUM_PROCESSES; // Move to the next process
                printf("Scheduler: Process %d Page Fault. Context switch (-> %d).\n", 
                       event_pid, ready_queue[head % NUM_PROCESSES]);
            } else if (response.status == 2) { // Page Hit
                // Continue execution. Process will send another request, or finish.
                // Re-add to the front temporarily to allow more executions, or just keep FCFS strict:
                ready_queue[tail % NUM_PROCESSES] = event_pid;
                tail = (tail + 1) % NUM_PROCESSES;
                head = (head + 1) % NUM_PROCESSES; // Move to the next process
            } else if (response.status == 3) { // Process Finished
                processes_finished++;
                head = (head + 1) % NUM_PROCESSES; // Remove from ready queue
                printf("Scheduler: Process %d finished. %d remaining.\n", event_pid, NUM_PROCESSES - processes_finished);
            }
        }
    }

    printf("Scheduler terminating.\n");
    return 0;
}