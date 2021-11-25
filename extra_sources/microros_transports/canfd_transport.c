#include <microros_transports.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <hal_data.h>

#include <uxr/client/transport.h>
#include <uxr/client/util/time.h>
#include <rmw_microxrcedds_c/config.h>

#ifdef RMW_UXRCE_TRANSPORT_CUSTOM

// --- micro-ROS Transports ---
bool g_write_complete = false;
bool g_read_complete = false;

// TODO: handle user CAN_ID
const uint32_t CAN_ID = 0x1500;

// TODO: Acceptance filter array parameters 
const canfd_afl_entry_t p_canfd0_afl[CANFD_CFG_AFL_CH0_RULE_NUM] =
{
 { // Accept all messages with Extended ID 0x1000-0x1FFF
   .id =
   {
    // Specify the ID, ID type and frame type to accept.
    .id         = CAN_ID,
    .frame_type = CAN_FRAME_TYPE_DATA,
    .id_mode    = CAN_ID_MODE_EXTENDED
   },

   .mask =
   {
    // These values mask which ID/mode bits to compare when filtering messages.
    .mask_id         = 0x0000FF00,
    .mask_frame_type = 0,
    .mask_id_mode    = 1
   },

   .destination =
   {
    // If DLC checking is enabled any messages shorter than the below setting will be rejected.
    .minimum_dlc = (canfd_minimum_dlc_t)0,

    // Optionally specify a Receive Message Buffer (RX MB) to store accepted frames. RX MBs do not have an
    // interrupt or overwrite protection and must be checked with R_CANFD_InfoGet and R_CANFD_Read.
    .rx_buffer   = CANFD_RX_MB_0,

    // Specify which FIFO(s) to send filtered messages to. Multiple FIFOs can be OR'd together.
    .fifo_select_flags = CANFD_RX_FIFO_0,
   }
 },
};

void canfd0_callback(can_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case CAN_EVENT_TX_COMPLETE:
        // case CAN_EVENT_TX_FIFO_EMPTY:
        {
            g_write_complete = true;
            break;
        }
        // TODO: Check this comment
        case CAN_EVENT_RX_COMPLETE: // Currently driver don't support this. This is unreachable code for now.
        {
            g_read_complete = true;
            break;
        }
        case CAN_EVENT_ERR_WARNING:             //error warning event
        case CAN_EVENT_ERR_PASSIVE:             //error passive event
        case CAN_EVENT_ERR_BUS_OFF:             //error Bus Off event
        case CAN_EVENT_BUS_RECOVERY:            //Bus recovery error event
        case CAN_EVENT_MAILBOX_MESSAGE_LOST:    //overwrite/overrun error event
        case CAN_EVENT_ERR_BUS_LOCK:            // Bus lock detected (32 consecutive dominant bits).
        case CAN_EVENT_ERR_CHANNEL:             // Channel error has occurred.
        case CAN_EVENT_TX_ABORTED:              // Transmit abort event.
        case CAN_EVENT_ERR_GLOBAL:              // Global error has occurred.
        {
            volatile canfd_error_t error = p_args->error;
            break;
        }
    }
}

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;

    // Use mailbox or FIFO?
    fsp_err_t err = R_CANFD_Open(&g_canfd0_ctrl, &g_canfd0_cfg);
    return err == FSP_SUCCESS;
}

bool renesas_e2_transport_close(struct uxrCustomTransport * transport){
    (void) transport;

    fsp_err_t err = R_CANFD_Close(&g_canfd0_ctrl);

    return err == FSP_SUCCESS;
}

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * error){
    (void) transport;
    (void) error;

    can_frame_t g_canfd_tx_frame;
    g_canfd_tx_frame.id = CAN_ID;
    g_canfd_tx_frame.id_mode = CAN_ID_MODE_EXTENDED,
    g_canfd_tx_frame.type = CAN_FRAME_TYPE_DATA;
    g_canfd_tx_frame.data_length_code = (uint8_t) len;
    g_canfd_tx_frame.options = CANFD_FRAME_OPTION_FD;

    g_write_complete = false;
    memcpy(&g_canfd_tx_frame.data[0], buf, len);
    fsp_err_t err =  R_CANFD_Write(&g_canfd0_ctrl, 0, &g_canfd_tx_frame);

    if (err != FSP_SUCCESS)
    {
        return 0;
    }

    while(!g_write_complete)
    {
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
    }

    return len;
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* error){
    (void) transport;
    (void) error;

    can_frame_t g_canfd_rx_frame;
    can_info_t can_rx_info;
    fsp_err_t err;

    int64_t start = uxr_millis();
    size_t wrote = 0;

    while ((uxr_millis() -  start) < timeout)
    {
        err = R_CANFD_InfoGet(&g_canfd0_ctrl, &can_rx_info);

        if (FSP_SUCCESS != err)
        {
            // TODO: set error
            return 0;
        }

        if(can_rx_info.rx_mb_status)
        {
            /* Read the input frame received */
            err = R_CANFD_Read(&g_canfd0_ctrl, 0, &g_canfd_rx_frame);

            if (FSP_SUCCESS != err)
            {
                // TODO: set error
                return 0;
            }

            // TODO: check len
            (void) len;
            memcpy(buf, &g_canfd_rx_frame.data[0], g_canfd_rx_frame.data_length_code);
            wrote = g_canfd_rx_frame.data_length_code;
            break;
        }

        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MICROSECONDS);
    }

    return wrote;
}

#endif //RMW_UXRCE_TRANSPORT_CUSTOM
