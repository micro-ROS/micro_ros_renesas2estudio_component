#include "hal_data.h"

#include <uxr/client/transport.h>
#include <uxr/client/util/time.h>

#include <rmw_microxrcedds_c/config.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef RMW_UXRCE_TRANSPORT_CUSTOM

bool renesas_e2_transport_open(struct uxrCustomTransport * transport);
bool renesas_e2_transport_close(struct uxrCustomTransport * transport);
size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, uint8_t * buf, size_t len, uint8_t * err);
size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err);

#define NUM_STRING_DESCRIPTOR   (7U)             /* String descriptor */
#define READ_BUF_SIZE           (8U)
#define LINE_CODING_LENGTH      (0x07U)

usb_class_t g_usb_class_type = 0x00;
usb_status_t usb_event;
usb_setup_t usb_setup;
usb_pcdc_linecoding_t g_line_coding;
uint8_t g_buf[READ_BUF_SIZE] = {0};

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

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;
    fsp_err_t err = FSP_SUCCESS;
    err = R_USB_Open (&g_basic0_ctrl, &g_basic0_cfg);
    char aux[100];

    while (true)
    {
        err = R_USB_EventGet (&g_basic0_ctrl, &usb_event);
        if (FSP_SUCCESS != err){ return 0;}

        sprintf(aux, "Event: %d\n", usb_event);
        err = R_USB_Write (&g_basic0_ctrl, (uint8_t*)aux, strlen(aux), (uint8_t)g_usb_class_type);

        switch (usb_event)
        {
            case USB_STATUS_CONFIGURED:
            {
                err = R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, (uint8_t)g_usb_class_type);
                /* Handle error */
                if (FSP_SUCCESS != err){ return 0;}
                break;
            }

            case USB_STATUS_REQUEST : /* Receive Class Request */
            {
                R_USB_SetupGet(&g_basic0_ctrl, &usb_setup);

                /* Check for the specific CDC class request IDs */
                if (USB_PCDC_SET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataGet (&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_GET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataSet (&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_SET_CONTROL_LINE_STATE == (usb_setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlStatusSet (&g_basic0_ctrl, USB_SETUP_STATUS_ACK);
                    if (FSP_SUCCESS != err){ return 0;}
                }
                break;
            }

            case USB_STATUS_DETACH:
            case USB_STATUS_SUSPEND:
            case USB_STATUS_RESUME:
                break;
            case USB_STATUS_WRITE_COMPLETE:
                return 1;
            default:
                break;
        }
    }
}

bool renesas_e2_transport_close(struct uxrCustomTransport * transport){
    return true;
}

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, uint8_t * buf, size_t len, uint8_t * error){
    fsp_err_t err = FSP_SUCCESS;

    while (true)
    {
        err = R_USB_Write (&g_basic0_ctrl, buf, len, (uint8_t)g_usb_class_type);

        err = R_USB_EventGet (&g_basic0_ctrl, &usb_event);
        if (FSP_SUCCESS != err){ return 0;}

        switch (usb_event)
        {
            case USB_STATUS_CONFIGURED:
            {
                err = R_USB_Read (&g_basic0_ctrl, g_buf, READ_BUF_SIZE, (uint8_t)g_usb_class_type);
                /* Handle error */
                if (FSP_SUCCESS != err){ return 0;}
                break;
            }

            case USB_STATUS_REQUEST : /* Receive Class Request */
            {
                R_USB_SetupGet(&g_basic0_ctrl, &usb_setup);

                /* Check for the specific CDC class request IDs */
                if (USB_PCDC_SET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataGet (&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_GET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataSet (&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_SET_CONTROL_LINE_STATE == (usb_setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlStatusSet (&g_basic0_ctrl, USB_SETUP_STATUS_ACK);
                    if (FSP_SUCCESS != err){ return 0;}
                }
                break;
            }

            case USB_STATUS_DETACH:
            case USB_STATUS_SUSPEND:
            case USB_STATUS_RESUME:
                break;
            case USB_STATUS_WRITE_COMPLETE:
                return len;
            default:
                break;
        }
    }
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* error){
    fsp_err_t err = FSP_SUCCESS;

    uint64_t start = uxr_millis();
    while (true && (uxr_millis() -  start < timeout))
    {
        err = R_USB_Read (&g_basic0_ctrl, buf, len, (uint8_t)g_usb_class_type);

        err = R_USB_EventGet (&g_basic0_ctrl, &usb_event);
        if (FSP_SUCCESS != err){ return 0;}

        switch (usb_event)
        {
            case USB_STATUS_REQUEST : /* Receive Class Request */
            {
                R_USB_SetupGet(&g_basic0_ctrl, &usb_setup);

                /* Check for the specific CDC class request IDs */
                if (USB_PCDC_SET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataGet (&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_GET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataSet (&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_SET_CONTROL_LINE_STATE == (usb_setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlStatusSet (&g_basic0_ctrl, USB_SETUP_STATUS_ACK);
                    if (FSP_SUCCESS != err){ return 0;}
                }
                break;
            }

            case USB_STATUS_DETACH:
            case USB_STATUS_SUSPEND:
            case USB_STATUS_RESUME:
                break;
            case USB_STATUS_READ_COMPLETE:
                return g_basic0_ctrl.data_size;
            default:
                break;
        }
    }
    return 0;
}

#endif //RMW_UXRCE_TRANSPORT_CUSTOM
