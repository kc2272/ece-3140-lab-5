/******************************************************************************
 *
 * Lab 5 test r1: EDF scheduling
 *
 * pNRT:  ^__________g g g g v
 * pRT1:  ^g g v
 * pRT2:  ^_____r r r v
 *
 *   You should see the sequence of processes depicted above:
 *     - All three processes start at time zero.
 *     - pRT1 has the earliest deadline and blinks green 2 times.
 *     - pRT2 has the later deadline and blinks red 3 times.
 *     - After both RT processes finish, pNRT blinks green 4 times.
 *
 *   Both RT processes should meet their deadlines.
 *
 *   Expected:
 *     process_deadline_met = 2
 *     process_deadline_miss = 0
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
#define NRT_STACK 20
#define RT_STACK  20

/*--------------------------------------*/
/* Time structs for real-time processes */
/*--------------------------------------*/

/* Process start times */
realtime_t t_pRT1 = {0, 0};
realtime_t t_pRT2 = {0, 0};

/* Relative deadlines */
realtime_t d_pRT1 = {3, 0};
realtime_t d_pRT2 = {7, 0};

/* Variables for checking execution order */
volatile int early_done = 0;
volatile int late_done = 0;
volatile int normal_done = 0;
volatile int test_error = 0;

/*----------------------------------------------------
 * Non real-time process
 *
 * Blinks green LED 4 times.
 * Should execute after both real-time processes.
 *----------------------------------------------------*/

void pNRT(void) {
    int i;
    if (!early_done || !late_done) {
        test_error = 1;
    }
    for (i = 0; i < 4; i++) {
        green_on_frdm();
        delay(200);
        green_toggle_frdm();
        delay(200);
    }
    normal_done = 1;
}

/*----------------------------------------------------
 * Real-time process 1
 *
 * Blinks green LED 2 times.
 * Has the earliest deadline and should run first.
 *----------------------------------------------------*/

void pRT1(void) {
    int i;
    if (early_done || late_done || normal_done) {
        test_error = 1;
    }
    for (i = 0; i < 2; i++) {
        green_on_frdm();
        delay(250);
        green_toggle_frdm();
        delay(250);
    }
    early_done = 1;
}

/*----------------------------------------------------
 * Real-time process 2
 *
 * Blinks red LED 3 times.
 * Should execute after pRT1 and before pNRT.
 *----------------------------------------------------*/

void pRT2(void) {
    int i;
    if (!early_done || normal_done) {
        test_error = 1;
    }
    for (i = 0; i < 3; i++) {
        red_on_frdm();
        delay(200);
        red_toggle_frdm();
        delay(200);
    }
    late_done = 1;
}

/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/

int main(void) {
    led_init();
    /* Create processes */
    if (process_create(pNRT, NRT_STACK) < 0) {
        return -1;
    }
    if (process_rt_create(pRT2, RT_STACK, &t_pRT2, &d_pRT2) < 0) {
        return -1;
    }

    if (process_rt_create(pRT1, RT_STACK, &t_pRT1, &d_pRT1) < 0) {
        return -1;
    }
    /* Launch concurrent execution */
    process_start();
    green_off_frdm();
    red_off_frdm();

    /* Check scheduling order and deadline counters */
    if (test_error == 0 && early_done && late_done && normal_done && process_deadline_met == 2 && process_deadline_miss == 0) {
        green_on_frdm();
    }
    else {
        red_on_frdm();
    }

    /* Hang out in infinite loop */
    while (1);

    return 0;
}

