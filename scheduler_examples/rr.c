#include "rr.h"
#include "queue.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>


#define RR_QUANTUM_MS 500


void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        // If slice not started yet, set slice start
        if ((*cpu_task)->slice_start_ms == 0) {
            (*cpu_task)->slice_start_ms = current_time_ms;
        }
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        // Check if finished
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = { .pid = (*cpu_task)->pid, .request = PROCESS_REQUEST_DONE, .time_ms = current_time_ms };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) perror("write");
            free((*cpu_task));
            *cpu_task = NULL;
        } else {
            // Check quantum expiration
            uint32_t used = current_time_ms - (*cpu_task)->slice_start_ms;
            if (used >= RR_QUANTUM_MS) {
                // time slice over, put at end of ready queue
                (*cpu_task)->slice_start_ms = 0;
                enqueue_pcb(rq, *cpu_task);
                *cpu_task = NULL;
            }
        }
    }


    if (*cpu_task == NULL) {
        *cpu_task = dequeue_pcb(rq);
        if (*cpu_task) {
            (*cpu_task)->slice_start_ms = current_time_ms;
        }
    }
}