/******************************************************************************
 *
 * Lab 5 test r2: delayed start and deadline miss
 *
 * pRT1: ^g v
 * pRT2: ^__________r v
 *
 *   You should see the sequence of processes depicted above:
 *     - pRT1 starts at time zero and blinks green once.
 *     - pRT1 should finish before its 300 ms deadline.
 *     - pRT2 cannot start until 500 ms have elapsed.
 *     - pRT2 blinks red once (slow).
 *     - pRT2 should miss its 100 ms relative deadline.
 *
 *   The scheduler must wait for pRT2 to become ready,
 *   even though there are no other processes to run.
 *
 *   Expected:
 *     process_deadline_met = 1
 *     process_deadline_miss = 1
 *
 *   Green LED at end = pass
 *   Red LED at end = fail
 *
 ******************************************************************************/

#include "led.h"
#include "3140_concur.h"
#include "realtime.h"

/*--------------------------*/
/* Parameters for test case */
/*--------------------------*/

/* Stack space for processes */
#define RT_STACK 20

/*--------------------------------------*/
/* Time structs for real-time processes */
/*--------------------------------------*/

/* Relative deadlines */
realtime_t d_pRT1 = {0, 300};
realtime_t d_pRT2 = {0, 100};

/* Process start times */
realtime_t t_pRT1 = {0, 0};
realtime_t t_pRT2 = {0, 500};

/* Variables for checking execution */
volatile int on_time_done = 0;
volatile int delayed_done = 0;
volatile int test_error = 0;

/*-------------------
 * Real-time process 1
 *
 * Blinks green once.
 * Should meet deadline.
 *-------------------*/

void pRT1(void) {
    green_on_frdm();
    delay(100);
    green_toggle_frdm();
    on_time_done = 1;
}

/*----------------------------------------------------
 * Real-time process 2
 *
 * Starts at 500 ms.
 * Blinks red once.
 * Should miss its deadline.
 *----------------------------------------------------*/

void pRT2(void) {
    unsigned long elapsed_ms;
    /* Check current time */
    elapsed_ms = 1000UL * (unsigned long)current_time.sec + (unsigned long)current_time.msec;
    /* Check that pRT2 did not start too early */
    if (elapsed_ms < 500UL || !on_time_done) {
        test_error = 1;
    }
    red_on_frdm();
    delay(660);
    red_toggle_frdm()
    delayed_done = 1;
}

/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/

int main(void) {
    led_init();
    /* Create processes */
    if (process_rt_create(pRT1, RT_STACK, &t_pRT1, &d_pRT1) < 0) {
        return -1;
    }
    if (process_rt_create(pRT2, RT_STACK, &t_pRT2, &d_pRT2) < 0) {
        return -1;
    }

    /* Launch concurrent execution */
    process_start();
    green_off_frdm();
    red_off_frdm();

    /* Check process completion and deadline counters */
    if (test_error == 0 && on_time_done && delayed_done && process_deadline_met == 1 && process_deadline_miss == 1) {
        green_on_frdm();
    } 
    else {
        red_on_frdm();
    }

    /* Hang out in infinite loop */
    while (1);

    return 0;
}
