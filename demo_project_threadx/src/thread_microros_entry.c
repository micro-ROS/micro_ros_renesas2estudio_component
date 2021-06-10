#include "thread_microros.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>

#include <std_msgs/msg/int32.h>

#include <microros_allocators.h>
#include <microros_transports.h>

void timer_callback(rcl_timer_t * timer, int64_t last_call_time);

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
    (void) last_call_time;
    if (timer != NULL) {
        // Publish system clock ticks (ms)
        msg.data = tx_time_get();
        rcl_publish(&publisher, &msg, NULL);
    }
}

/* Thread micro-ROS entry function */
void thread_microros_entry(void)
{
    /* TODO: add your own code here */

    // Init transport callbacks and disable framing for UDP
    rmw_uros_set_custom_transport(
      false,    
      (void *) NULL,
      renesas_e2_transport_open,
      renesas_e2_transport_close,
      renesas_e2_transport_write,
      renesas_e2_transport_read);

    rcl_allocator_t custom_allocator = rcutils_get_zero_initialized_allocator();
    custom_allocator.allocate = microros_allocate;
    custom_allocator.deallocate = microros_deallocate;
    custom_allocator.reallocate = microros_reallocate;
    custom_allocator.zero_allocate =  microros_zero_allocate;

    if (!rcutils_set_default_allocator(&custom_allocator)) {
        printf("Error on default allocators (line %d)\n", __LINE__);
    }

    // micro-ROS app
    allocator = rcl_get_default_allocator();

    rcl_ret_t ret;

    // create init_options
    ret = rclc_support_init(&support, 0, NULL, &allocator);

    // create node
    ret = rclc_node_init_default(&node, "renesas_node", "", &support);

    // create publisher
    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "renesas_publisher");

    // create timer,
    rcl_timer_t timer;
    rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(50),
        timer_callback);

    // create executor
    rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_timer(&executor, &timer);

    while(1){
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1));
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
