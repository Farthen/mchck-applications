// Lots of stuff stolen from https://github.com/grober-unfug/mchck/blob/master/examples/weather-station/weather-station.c

#include <mchck.h>

#include <usb/usb.h>
#include <usb/cdc-acm.h>

#include "nrf24l01.h"

static struct cdc_ctx cdc;
static struct timeout_ctx t;

/* Communication over USB */
static void
new_data(uint8_t *data, size_t len)
{
    for (int i = 0; i < len; i++)
    {
        if (data[i] == '\r')
        {
            nrf_print_config();
        }
    }

    cdc_read_more(&cdc);
}

static void
init_vcdc(int config)
{
    cdc_init(new_data, NULL, &cdc);
    cdc_set_stdout(&cdc);
}

static const struct usbd_device cdc_device =
    USB_INIT_DEVICE(0x2323,  /* vid */
        3,                   /* pid */
        u"mchck.org",        /* vendor */
        u"MCHCK DHT11",      /* product" */
        (init_vcdc,          /* init */
         CDC)                /* functions */
    );


void
main(void)
{
    timeout_init();
    usb_init(&cdc_device);
	nrf_init();
    sys_yield_for_frogs();
}
