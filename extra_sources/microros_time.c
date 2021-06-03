#include <time.h>
#include "hal_data.h"
#include "./microros_app.h"

int clock_gettime( int clock_id, struct timespec * tp );

static bool gtp_init = false;
static uint64_t rollover_count = 0;

void micro_ros_timer_cb(timer_callback_args_t * p_args){
    (void) p_args;
    rollover_count++;
}

int clock_gettime( int clock_id, struct timespec * tp )
{
    (void)clock_id;

    if (!gtp_init){
        R_AGT_Open(&MICRO_ROS_TIMER_CTRL, &MICRO_ROS_TIMER_CFG);
        R_AGT_Start(&MICRO_ROS_TIMER_CTRL);
        gtp_init = true;
    }

    timer_status_t status;
    R_AGT_StatusGet(&MICRO_ROS_TIMER_CTRL, &status);

    uint64_t ns = (uint64_t) (MICRO_ROS_TIMER_CFG.period_counts - status.counter) * micro_ros_timer_ns_per_tick;
    tp->tv_sec = (long int)((ns / NS_TO_S));
    tp->tv_nsec = (long int)(ns % NS_TO_S);

    uint64_t rollover_ns = (uint64_t) (rollover_count * MICRO_ROS_TIMER_CFG.period_counts * micro_ros_timer_ns_per_tick);
    tp->tv_sec += (long int)((rollover_ns / NS_TO_S));
    tp->tv_nsec += (long int)(rollover_ns % NS_TO_S);


    return 0;
}
