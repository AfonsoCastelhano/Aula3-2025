#include "mlfq.h"
#include "queue.h"
#include <stdint.h>
#include <stdio.h>
#include "msg.h"
#include <unistd.h>

#define MLFQ_QUANTUM_MS 500

static void run_step(uint32_t current_time_ms, pcb_t **cpu_task, queue_t *q, uint32_t quantum_ms, queue_t *q_level_2, queue_t *q_level_3, int demote_to_level) {
    if (*cpu_task == NULL) {
        *cpu_task = dequeue_pcb(q);
        if (*cpu_task) {
            (*cpu_task)->slice_start_ms = current_time_ms;
        }
    }
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = { .pid = (*cpu_task)->pid, .request = PROCESS_REQUEST_DONE, .time_ms = current_time_ms };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) perror("write");
            free((*cpu_task));
            *cpu_task = NULL;
        } else {
            uint32_t used = current_time_ms - (*cpu_task)->slice_start_ms;
            if (used >= quantum_ms) {
                (*cpu_task)->slice_start_ms = 0;
                if (demote_to_level == 2) enqueue_pcb(q_level_2, *cpu_task);
                else enqueue_pcb(q_level_3, *cpu_task);
                *cpu_task = NULL;
            }
        }
    }
}

void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task, queue_t *q_level_1, queue_t *q_level_2, queue_t *q_level_3) {
    while (rq->head) {
        pcb_t* p = dequeue_pcb(rq);
        enqueue_pcb(q_level_1, p);
    }

    if (q_level_1->head || (*cpu_task && (*cpu_task)->slice_start_ms!=0)) {
        run_step(current_time_ms, cpu_task, q_level_1, MLFQ_QUANTUM_MS, q_level_2, q_level_3, 2);
        return;
    }
    if (q_level_2->head) {
        run_step(current_time_ms, cpu_task, q_level_2, 2 * MLFQ_QUANTUM_MS, q_level_2, q_level_3, 3);
        return;
    }
    if (q_level_3->head) {
        if (*cpu_task == NULL) {
            *cpu_task = dequeue_pcb(q_level_3);
        }
        if (*cpu_task) {
            (*cpu_task)->ellapsed_time_ms += TICKS_MS;
            if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
                msg_t msg = { .pid = (*cpu_task)->pid, .request = PROCESS_REQUEST_DONE, .time_ms = current_time_ms };
                if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) perror("write");
                free((*cpu_task));
                *cpu_task = NULL;
            }
        }
    }
}
