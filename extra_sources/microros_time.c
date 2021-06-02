#include <time.h>
#include "hal_data.h"

int clock_gettime( int clock_id, struct timespec * tp );

#define micro_rollover_useconds_32bits 4294967295UL

static bool gtp_init = false;

int clock_gettime( int clock_id, struct timespec * tp )
{
    (void)clock_id;

    if (!gtp_init){
        R_BSP_MODULE_START(FSP_IP_POEG, 0U);
        (void) R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
        (void) R_GPT_Start(&g_timer0_ctrl);
        gtp_init = true;
    }

    timer_status_t status;
    (void) R_GPT_StatusGet(&g_timer0_ctrl, &status);

    static uint32_t rollover = 0;
    static uint64_t last_measure = 0;

    uint64_t m = (uint64_t) status.counter * 10;
    tp->tv_sec = (long int)(m / 1000000UL);
    tp->tv_nsec = (long int)((m % 1000000UL) * 1000UL);

    // Rollover handling
    rollover += (m < last_measure) ? 1 : 0;
    uint64_t rollover_extra_us = rollover * micro_rollover_useconds_32bits;
    tp->tv_sec += (long int)(rollover_extra_us / 1000000UL);
    tp->tv_nsec += (long int)((rollover_extra_us % 1000000UL) * 1000UL);
    last_measure = m;

    return 0;
}
