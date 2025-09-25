#include "sjf.h"
#include "queue.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "msg.h"
#include <unistd.h>


/**
* Simple preemptive SJF (shortest remaining time first) implementation.
* - If a new task with shorter remaining time arrives it will preempt the CPU.
* - Uses ready queue as a FIFO list; we search for the shortest job each tick.
*/


static queue_elem_t* find_shortest_elem(queue_t* q) {
queue_elem_t* it = q->head;
queue_elem_t* best = NULL;
uint32_t best_rem = UINT32_MAX;
while (it) {
pcb_t* p = it->pcb;
uint32_t rem = (p->time_ms > p->ellapsed_time_ms) ? (p->time_ms - p->ellapsed_time_ms) : 0;
if (rem < best_rem) {
best_rem = rem;
best = it;
}
it = it->next;
}
return best;
}


void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
// Update running task
if (*cpu_task) {
(*cpu_task)->ellapsed_time_ms += TICKS_MS;
if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
// task finished
msg_t msg = { .pid = (*cpu_task)->pid, .request = PROCESS_REQUEST_DONE, .time_ms = current_time_ms };
if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) perror("write");
free((*cpu_task));
*cpu_task = NULL;
}
}


// If CPU idle, pick the shortest job from ready queue
if (*cpu_task == NULL && rq->head) {
queue_elem_t* best_elem = find_shortest_elem(rq);
if (best_elem) {
// remove that element from queue
queue_elem_t* removed = remove_queue_elem(rq, best_elem);
if (removed) {
*cpu_task = removed->pcb;
free(removed);
}
}
} else if (*cpu_task && rq->head) {
// Preemption check: if there is a ready task with remaining time shorter than current
queue_elem_t* best_elem = find_shortest_elem(rq);
if (best_elem) {
pcb_t* candidate = best_elem->pcb;
uint32_t cand_rem = (candidate->time_ms > candidate->ellapsed_time_ms) ? (candidate->time_ms - candidate->ellapsed_time_ms) : 0;
uint32_t curr_rem = ((*cpu_task)->time_ms > (*cpu_task)->ellapsed_time_ms) ? ((*cpu_task)->time_ms - (*cpu_task)->ellapsed_time_ms) : 0;
if (cand_rem < curr_rem) {
// preempt: put current back to ready queue and take candidate
enqueue_pcb(rq, *cpu_task);
queue_elem_t* removed = remove_queue_elem(rq, best_elem);
if (removed) {
*cpu_task = removed->pcb;
free(removed);
                }
            }
        }
    }
}