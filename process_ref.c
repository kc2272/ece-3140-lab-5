/* process.c
 *
 * Nils Napp
 * Cornell University
 * All right reserved
 *
 * Jan 2024
 * Ithaca NY
 *
 * This file is part of the ECE3140/CS3420 offering for Fall 2026. If you
 * are not part of this class you should not have access to this file. Do not
 * post, share, or otherwise distribute this file. We will consider it an AI
 * violation if you do. If you somehow get this code and you are NOT enrolled
 * the Fall 2026 version of ECE3140 please contact the course staff
 * immediately and describe how you found it.
 */


/************************************ NOTE *************************************
 *
 *  This is a reference solution so you can proceed with the lab even if you
 *  are uncertain of your Lab 3 solution. You can use this, as a starting point
 *  for your submission. Ideally you would use your own process.c from Lab 3
 *  and build your own multi-processing system from scratch!
 *
 *******************************************************************************/


#include <stdlib.h>
#include "3140_concur.h"
#include "realtime.h"

volatile realtime_t current_time;

int process_deadline_met = 0;
int process_deadline_miss = 0;

static process_queue_t rt_queue_ready = {NULL};
static process_queue_t rt_queue_unready = {NULL};

static void process_free(process_t *proc) {
	process_stack_free(proc->orig_sp, proc->n);
	free(proc);
}

/* Starts up the concurrent execution */
void process_start (void) {
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
	PIT->MCR = 0;
	PIT->CHANNEL[0].LDVAL = 150000; // 15MHz / 100Hz
	NVIC_EnableIRQ(PIT_IRQn);

	// Don't enable the channel 0 timer yet. The scheduler will do so itself

	if(is_empty(&process_queue) && is_empty(&rt_queue_ready) && is_empty(&rt_queue_unready)) return;
	//bail out fast if no processes were ever created

	current_time.sec = 0;
	current_time.msec = 0;
	PIT->CHANNEL[1].LDVAL = 15000; // 15MHz / 1000Hz
	PIT->CHANNEL[1].TFLG  = PIT_TFLG_TIF_MASK;
	PIT->CHANNEL[1].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;

	process_begin();
}

/* Create a new non-RT process */
int process_create (void (*f)(void), int n) {
	unsigned int *sp = process_stack_init(f, n);
	if (!sp) return -2;
	
	process_t *proc = (process_t*) malloc(sizeof(process_t));
	if (!proc) {
		process_stack_free(sp, n);
		return -1;
	}
	
	proc->sp = proc->orig_sp = sp;
	proc->n = n;
	proc->is_rt = 0;
	proc->start_time = 0;
	proc->abs_deadline = 0;
	
	enqueue(proc,&process_queue);
	
	return 0;
}

/* Create a new RT process */
int process_rt_create(void (*f)(void), int n, realtime_t* start, realtime_t* deadline) {
	unsigned int *sp = process_stack_init(f, n);
	if (!sp) return -2;

	process_t *proc = (process_t*) malloc(sizeof(process_t));
	if (!proc) {
		process_stack_free(sp, n);
		return -1;
	}

	proc->sp = proc->orig_sp = sp;
	proc->n = n;
	proc->is_rt = 1;
	proc->start_time = start->sec * 1000 + start->msec;
	proc->abs_deadline = (deadline->sec * 1000 + deadline->msec) // relative deadline
			+ (start->sec * 1000 + start->msec); // absolute start time

	unsigned int primask = __get_PRIMASK();
	__disable_irq();
	enqueue(proc, &rt_queue_unready); // always start in unready queue before moving to ready queue
	__set_PRIMASK(primask);

	return 0;
}

void PIT1_Service(void) { // updates current_time
	PIT->CHANNEL[1].TFLG = PIT_TFLG_TIF_MASK;  // clear the channel 1 interrupt flag

	current_time.msec++;
	if (current_time.msec == 1000) {
		current_time.sec++;
		current_time.msec = 0;
	}
}

/* Called by the runtime system to select another process.
   "cursp" = the stack pointer for the currently running process
*/
unsigned int * process_select (unsigned int * cursp) {

	unsigned int now = current_time.sec * 1000 + current_time.msec;
	process_t **pp;
	process_t **best_pp;

	if (cursp) {
		//Suspending a process which has not yet finished
		//Save state and put it back in the queue it came from
		current_process_p->sp = cursp;
		if (current_process_p->is_rt) {
			enqueue(current_process_p,&rt_queue_ready);
		} else {
			enqueue(current_process_p,&process_queue);
		}
	} else if (current_process_p) {
		// update deadline trackers
		if (current_process_p->is_rt) {
			if (now <= current_process_p->abs_deadline) {
				process_deadline_met++;
			} else {
				process_deadline_miss++;
			}
		}
		process_free(current_process_p);
	}
	current_process_p = NULL;

	while (1) {
		// move unready processes that have become ready into RT ready queue
		pp = &rt_queue_unready.head;
		while (*pp != NULL) {
			if ((*pp)->start_time <= now) {
				process_t *p = *pp;
				*pp = p->next;
				enqueue(p, &rt_queue_ready);
			} else {
				pp = &(*pp)->next;
			}
		}

		// ready RT processes
		if (!is_empty(&rt_queue_ready)) {
			best_pp = &rt_queue_ready.head;
			for (pp = &(*best_pp)->next; *pp != NULL; pp = &(*pp)->next) {
				if ((*pp)->abs_deadline < (*best_pp)->abs_deadline) {
					best_pp = pp;
				}
			}
			current_process_p = *best_pp;
			*best_pp = current_process_p->next;
			current_process_p->next = NULL;
			break;
		}

		// normal non-RT processes
		if (!is_empty(&process_queue)) {
			current_process_p = dequeue(&process_queue);
			break;
		}

		// no processes at all: exit scheduler
		if (is_empty(&rt_queue_unready)) {
			return NULL;
		}

		// idle while waiting for unready processes to become ready
		while (!(PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK)) { }
		PIT1_Service(); // update current_time (need to do manually because interrupts are disabled
		now = current_time.sec * 1000 + current_time.msec;  // refresh and loop back to check again
	}

	return current_process_p->sp;
}
