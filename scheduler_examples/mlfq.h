#ifndef MLFQ_H
#define MLFQ_H


#include "queue.h"


void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task, queue_t *q_level_1, queue_t *q_level_2, queue_t *q_level_3);


#endif // MLFQ_H