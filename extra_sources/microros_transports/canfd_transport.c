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

// Set device can identifier
const uint32_t CAN_ID = 0x00001500;

#define CAN_MAILBOX_NUMBER_0  0U
#define CANFD_MTU 64

uint8_t len_to_dlc(size_t len);

// --- micro-ROS Transports ---
bool g_write_complete = false;
bool g_error = false;

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
    .mask_id         = 0x1FFFFFFFU,
    .mask_frame_type = CAN_FRAME_TYPE_DATA,
    .mask_id_mode    = CAN_ID_MODE_EXTENDED
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

uint8_t len_to_dlc(size_t len)
{
    uint8_t result = 0;

    if (len<= 8)
    {
        result = (uint8_t) len;
    }
    else if (len<= 24)
    {
        len -= 8U;
        result =  (uint8_t) (8U + (len / 4U));

        if(len % 4U){
            result++;
        }
    }
    else if (len<= 64)
    {
        result = (uint8_t) (0xDU + ((len / 16U) - 2U));

        if(len % 16U){
            result++;
        }
    }

    return result;
}

void canfd0_callback(can_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case CAN_EVENT_TX_COMPLETE:
        {
            g_write_complete = true;
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
            // volatile canfd_error_t error = p_args->error;
            g_error = true;
            break;
        }

        case CAN_EVENT_RX_COMPLETE: // Currently driver don't support this. This is unreachable code for now.
        case CAN_EVENT_TX_FIFO_EMPTY:
        default:
            break;
    }
}

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;

    if (UXR_CONFIG_CUSTOM_TRANSPORT_MTU != CANFD_MTU) {
        return false;
    }

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
    g_canfd_tx_frame.data_length_code = len_to_dlc(len);
    g_canfd_tx_frame.options = CANFD_FRAME_OPTION_FD;

    g_write_complete = false;
    g_error = false;

    memcpy(&g_canfd_tx_frame.data[0], buf, len);
    fsp_err_t err =  R_CANFD_Write(&g_canfd0_ctrl, CAN_MAILBOX_NUMBER_0, &g_canfd_tx_frame);

    if (err != FSP_SUCCESS) {
        return 0;
    }

    while(!g_write_complete) {
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);

        if (g_error) {
            return 0;
        }
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
    g_error = false;

    while ((uxr_millis() -  start) < timeout && !g_error)
    {
        err = R_CANFD_InfoGet(&g_canfd0_ctrl, &can_rx_info);

        if (FSP_SUCCESS != err)
        {
            // TODO: set error
            return 0;
        }

        if(can_rx_info.rx_mb_status)
        {
            // Read the input frame received
            err = R_CANFD_Read(&g_canfd0_ctrl, CANFD_RX_MB_0, &g_canfd_rx_frame);

            if (FSP_SUCCESS != err)
            {
                // TODO: set error
                return 0;
            }

            if (g_canfd_rx_frame.data_length_code > len) {
                // This should never execute
                return 0;
            }

            memcpy(buf, &g_canfd_rx_frame.data[0], g_canfd_rx_frame.data_length_code);
            wrote = g_canfd_rx_frame.data_length_code;
            break;
        }

        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MICROSECONDS);
    }

    return wrote;
}

#endif //RMW_UXRCE_TRANSPORT_CUSTOM
