#include <time.h>
#include "hal_data.h"

#define NS_TO_S 1000000000UL
#define NS_PER_TICK 3906250UL
#define COUNTER_16BITS 65535UL
int clock_gettime( int clock_id, struct timespec * tp );
static bool gtp_init = false;


// int clock_gettime_gpt( int clock_id, struct timespec * tp );

// #define rollover_32bits_timers 4294967295UL


// int clock_gettime_gpt( int clock_id, struct timespec * tp )
// {
//     (void)clock_id;

//     if (!gtp_init){
//         R_BSP_MODULE_START(FSP_IP_POEG, 0U);
//         (void) R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
//         (void) R_GPT_Start(&g_timer0_ctrl);
//         gtp_init = true;
//     }

//     timer_status_t status;
//     (void) R_GPT_StatusGet(&g_timer0_ctrl, &status);

//     static uint32_t rollover = 0;
//     static uint64_t last_measure = 0;

//     uint64_t m = (uint64_t) status.counter * 10;
//     tp->tv_sec = (long int)(m / 1000000UL);
//     tp->tv_nsec = (long int)((m % 1000000UL) * 1000UL);

//     // Rollover handling
//     rollover += (m < last_measure) ? 1 : 0;
//     uint64_t rollover_extra_us = rollover * rollover_32bits_timers;
//     tp->tv_sec += (long int)(rollover_extra_us / 1000000UL);
//     tp->tv_nsec += (long int)((rollover_extra_us % 1000000UL) * 1000UL);
//     last_measure = m;

//     return 0;
// }


static uint64_t rollover_extra_s = 0;

void timer1_irq(timer_callback_args_t * p_args){
    (void) p_args;
    rollover_extra_s += 256UL;
}

int clock_gettime( int clock_id, struct timespec * tp )
{
    (void)clock_id;

    if (!gtp_init){
        R_AGT_Open(&g_timer1_ctrl, &g_timer1_cfg);
        R_AGT_Start(&g_timer1_ctrl);
        gtp_init = true;
    }

    timer_status_t status;
    R_AGT_StatusGet(&g_timer1_ctrl, &status);

    uint64_t ns = (uint64_t) (COUNTER_16BITS - status.counter) * NS_PER_TICK;
    tp->tv_sec = (long int)(rollover_extra_s + (ns / NS_TO_S));
    tp->tv_nsec = (long int)(ns % NS_TO_S);

    return 0;
}
