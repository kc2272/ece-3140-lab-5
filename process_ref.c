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
	// Don't enable the timer yet. The scheduler will do so itself
	
	if(is_empty(&process_queue)) return;
	//bail out fast if no processes were ever created

	process_begin();
}

/* Create a new process */
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
	
	enqueue(proc,&process_queue);
	
	return 0;
}

/* Called by the runtime system to select another process.
   "cursp" = the stack pointer for the currently running process
*/
unsigned int * process_select (unsigned int * cursp) {

	if (cursp) {
		//Suspending a process which has not yet finished
		//Save state and enqueue it on the process queueu
		current_process_p->sp = cursp;
		enqueue(current_process_p,&process_queue);
	} else {
		//Check if a process was running
		//Free its resources if it finished
		if (current_process_p) {
			process_free(current_process_p);
		}
	}
	
	// Select a new process from the queue and make it current
	current_process_p = dequeue(&process_queue);
	
	if (current_process_p) {
		// Launch the process which was just popped off the queue
		return current_process_p->sp;
	} else {
		// No process was selected, exit the scheduler
		return NULL;
	}
}
