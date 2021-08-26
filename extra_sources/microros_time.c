#include <time.h>
#include "r_agt.h"
#include "r_timer_api.h"

#define NS_IN_S 1000000000UL

extern const timer_instance_t g_timer0;
extern agt_instance_ctrl_t g_timer0_ctrl;
extern const timer_cfg_t g_timer0_cfg;

#define MICRO_ROS_TIMER g_timer0
#define MICRO_ROS_TIMER_CLK_SOURCE_HZ BSP_STARTUP_PCLKB_HZ

#define MICRO_ROS_ADD_SUFFIX_I(X,Y) X##_##Y
#define MICRO_ROS_ADD_SUFFIX(X,Y) MICRO_ROS_ADD_SUFFIX_I(X,Y)

#define MICRO_ROS_TIMER_CFG MICRO_ROS_ADD_SUFFIX(MICRO_ROS_TIMER,cfg)
#define MICRO_ROS_TIMER_CTRL MICRO_ROS_ADD_SUFFIX(MICRO_ROS_TIMER,ctrl)

void micro_ros_timer_cb(timer_callback_args_t * p_args);
int clock_gettime( int clock_id, struct timespec * tp );

static bool timer_init = false;
static uint64_t rollover_count = 0;

void micro_ros_timer_cb(timer_callback_args_t * p_args){
    (void) p_args;
    rollover_count++;
}

int clock_gettime( int clock_id, struct timespec * tp )
{
    (void)clock_id;

    static uint64_t ns_per_tick;
    static uint64_t ns_per_period;

    if (!timer_init){
        R_AGT_Open(&MICRO_ROS_TIMER_CTRL, &MICRO_ROS_TIMER_CFG);
        R_AGT_Start(&MICRO_ROS_TIMER_CTRL);

        ns_per_tick = (uint64_t)(NS_IN_S * (((double)(1 << MICRO_ROS_TIMER_CFG.source_div)/(double)MICRO_ROS_TIMER_CLK_SOURCE_HZ)));
        ns_per_period = MICRO_ROS_TIMER_CFG.period_counts * ns_per_tick;

        timer_init = true;
    }

    timer_status_t status;
    R_AGT_StatusGet(&MICRO_ROS_TIMER_CTRL, &status);

    uint64_t ns = (uint64_t) (MICRO_ROS_TIMER_CFG.period_counts - status.counter) * ns_per_tick + (uint64_t) (rollover_count * ns_per_period);
    tp->tv_sec = (long int)((ns / NS_IN_S));
    tp->tv_nsec = (long int)(ns % NS_IN_S);

    return 0;
}
