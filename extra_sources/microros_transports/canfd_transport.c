#include <microros_transports.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <hal_data.h>

#include <uxr/client/transport.h>
#include <uxr/client/util/time.h>
#include <rmw_microxrcedds_c/config.h>

#define CAN_BUFFER_SIZE 8

// Set device Extended ID (0x0000-0x1FFF)
const uint32_t CAN_ID = 0x0001;
bool enable_BRS = false;

#ifdef RMW_UXRCE_TRANSPORT_CUSTOM

uint8_t len_to_dlc(size_t len);

can_frame_t g_canfd_rx_frame[CAN_BUFFER_SIZE];
static size_t it_head = 0, it_tail = 0;

bool g_write_complete = false;
bool g_error = false;

const canfd_afl_entry_t p_canfd0_afl[CANFD_CFG_AFL_CH0_RULE_NUM] =
{
 { // Accept all messages with Extended ID 0x1000-0x1FFF
   .id =
   {
    // Specify the ID, ID type and frame type to accept.
    .id         = CAN_ID << 12,
    .frame_type = CAN_FRAME_TYPE_DATA,
    .id_mode    = CAN_ID_MODE_EXTENDED
   },

   .mask =
   {
    // These values mask which ID/mode bits to compare when filtering messages.
    .mask_id         = 0x1FFFF000U,
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

// Temporal fix for r_canfd_bytes_to_dlc
uint8_t len_to_dlc(size_t len)
{
    uint8_t result = 0;

    if (len<= 8)
    {
        result = (uint8_t) len;
    }
    else if (len <= 12)
    {
        result = 12;
    }
    else if (len <= 16)
    {
        result = 16;
    }
    else if (len <= 20)
    {
        result = 20;
    }
    else if (len <= 24)
    {
        result = 24;
    }
    else if (len <= 32)
    {
        result = 32;
    }
    else if (len <= 48)
    {
        result = 48;
    }
    else
    {
        result = 64;
    }

    return result;
}

void canfd0_callback(can_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case CAN_EVENT_TX_COMPLETE:
        case CAN_EVENT_TX_FIFO_EMPTY:
        {
            g_write_complete = true;
            break;
        }
        case CAN_EVENT_RX_COMPLETE:
        {
            memcpy(&g_canfd_rx_frame[it_tail], &p_args->frame, sizeof(p_args->frame));
            it_tail++;

            if (CAN_BUFFER_SIZE == it_tail)
            {
                it_tail = 0;
            }

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
            g_error = true;
            break;
        }
    }
}

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;

    if (UXR_CONFIG_CUSTOM_TRANSPORT_MTU != 64) {
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

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * error) {
    (void) transport;
    (void) error;

    can_frame_t g_canfd_tx_frame;
    g_canfd_tx_frame.id = CAN_ID << 12;
    g_canfd_tx_frame.id_mode = CAN_ID_MODE_EXTENDED,
    g_canfd_tx_frame.type = CAN_FRAME_TYPE_DATA;
    g_canfd_tx_frame.data_length_code = len_to_dlc(len);
    g_canfd_tx_frame.options = CANFD_FRAME_OPTION_FD;

    if (enable_BRS){
        g_canfd_tx_frame.options |= CANFD_FRAME_OPTION_BRS;   // Optional: Bit Rate Switching mode
    }

    g_write_complete = false;
    g_error = false;

    if (len > UXR_CONFIG_CUSTOM_TRANSPORT_MTU) {
        // This should never execute
        return 0;
    }

    memcpy(&g_canfd_tx_frame.data[0], buf, len);
    memset(&g_canfd_tx_frame.data[len], 0, g_canfd_tx_frame.data_length_code - len);
    fsp_err_t err =  R_CANFD_Write(&g_canfd0_ctrl, 0, &g_canfd_tx_frame);

    if (err != FSP_SUCCESS) {
        return 0;
    }

    while (!g_write_complete && !g_error) {
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
    }

    return g_write_complete ? len : 0;
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* error) {
    (void) transport;
    (void) error;

    int64_t start = uxr_millis();
    size_t wrote = 0;

    while ((uxr_millis() -  start) < timeout)
    {
        if (it_head != it_tail)
        {
            if (g_canfd_rx_frame[it_head].data_length_code > len) {
                // This should never execute
                return 0;
            }

            memcpy(buf, &g_canfd_rx_frame[it_head].data[0], g_canfd_rx_frame[it_head].data_length_code);
            wrote = g_canfd_rx_frame[it_head].data_length_code;
            it_head++;

            if (CAN_BUFFER_SIZE == it_head)
            {
                it_head = 0;
            }

            break;
        }

        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MICROSECONDS);
    }

    return wrote;
}

#endif //RMW_UXRCE_TRANSPORT_CUSTOM
