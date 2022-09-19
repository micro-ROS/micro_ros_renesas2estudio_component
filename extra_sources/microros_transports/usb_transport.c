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

// Renesas FSP USB-PCDC handling

#define NUM_STRING_DESCRIPTOR   (7U)             /* String descriptor */
#define READ_BUF_SIZE           (8U)
#define LINE_CODING_LENGTH      (0x07U)

usb_class_t g_usb_class_type = 0x00;
usb_setup_t usb_setup;
usb_pcdc_linecoding_t g_line_coding;
// uint8_t g_buf[READ_BUF_SIZE] = {0};

extern uint8_t g_apl_device[];
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_hs_configuration[];
extern uint8_t g_apl_qualifier_descriptor[];
extern uint8_t * gp_apl_string_table[];

const usb_descriptor_t g_usb_descriptor =
{
    g_apl_device,                   /* Pointer to the device descriptor */
    g_apl_configuration,            /* Pointer to the configuration descriptor for Full-speed */
    g_apl_hs_configuration,         /* Pointer to the configuration descriptor for Hi-speed */
    g_apl_qualifier_descriptor,     /* Pointer to the qualifier descriptor */
    gp_apl_string_table,             /* Pointer to the string descriptor table */
    NUM_STRING_DESCRIPTOR
};

#define USB_NO_TIMEOUT -1

typedef enum handle_usb_operation_t {
    USB_NOOP,
    USB_WRITE,
    USB_READ,
} handle_usb_operation_t;

size_t handle_usb(handle_usb_operation_t op, uint8_t * buf, size_t len, int timeout);

uint8_t reading_buffer[1000];
size_t reading_buffer_size = 0;
size_t reading_buffer_ptr;

size_t handle_usb(handle_usb_operation_t op, uint8_t * buf, size_t len, int timeout)
{
    usb_status_t event = {0};
    fsp_err_t err = FSP_SUCCESS;

    if(op == USB_WRITE){
        err = R_USB_Write(&g_basic0_ctrl, buf, len, (uint8_t)g_usb_class_type);
    } else if (op == USB_READ) {
        err = R_USB_Read(&g_basic0_ctrl, reading_buffer, sizeof(reading_buffer), (uint8_t)g_usb_class_type);
    }

    int64_t start = uxr_millis();
    while (timeout == USB_NO_TIMEOUT || (uxr_millis() -  start < timeout))
    {
        err = R_USB_EventGet(&g_basic0_ctrl, &event);
        if (FSP_SUCCESS != err){ return 0;}

        switch (event)
        {
            case USB_STATUS_CONFIGURED:
            case USB_STATUS_WRITE_COMPLETE:
                if (op == USB_WRITE) {
                    return len;
                }
                break;
            case USB_STATUS_READ_COMPLETE:
                if (op == USB_READ) {
                    return g_basic0_ctrl.data_size;
                }
                break;
            case USB_STATUS_REQUEST:   /* Receive Class Request */
                R_USB_SetupGet(&g_basic0_ctrl, &usb_setup);

                /* Check for the specific CDC class request IDs */
                if (USB_PCDC_SET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataGet(&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_GET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataSet(&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_SET_CONTROL_LINE_STATE == (usb_setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlStatusSet(&g_basic0_ctrl, USB_SETUP_STATUS_ACK);
                    if (FSP_SUCCESS != err){ return 0;}
                }
                break;
            case USB_STATUS_SUSPEND:
            case USB_STATUS_DETACH:
            default:
                break;
        }
    }
    return 0;
}

// micro-ROS USB-PCDC transports

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;

    R_USB_Open(&g_basic0_ctrl, &g_basic0_cfg);
    return true;
}

bool renesas_e2_transport_close(struct uxrCustomTransport * transport){
    (void) transport;

    R_USB_Close(&g_basic0_ctrl);
    return true;
}

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * error){
    (void) transport;
    (void) error;

    return handle_usb(USB_WRITE, (uint8_t *) buf, len, WRITE_TIMEOUT);
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* error){
    (void) transport;
    (void) error;

    size_t readed = 0;

    if (reading_buffer_size == 0) {
        reading_buffer_size = handle_usb(USB_READ, NULL, 0, timeout);
        reading_buffer_ptr = 0;
    }

    while (reading_buffer_ptr < reading_buffer_size && readed < len) {
        buf[readed] = reading_buffer[reading_buffer_ptr];
        reading_buffer_ptr++;
        readed++;
    }

    if (reading_buffer_ptr == reading_buffer_size)
    {
        reading_buffer_size = 0;
    }

    return readed;
}

#endif //RMW_UXRCE_TRANSPORT_CUSTOM
