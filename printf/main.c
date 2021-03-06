#include <mchck.h>

#include <usb/usb.h>
#include <usb/cdc-acm.h>

#include "desc.c"

static struct cdc_ctx cdc;

static void
new_data(uint8_t *data, size_t len)
{
    for (int i = 0; i < len; ++i)
    {
    	if (data[i] == '\n' || data[i] == '\r')
    	{
    		onboard_led(-1);
    		printf("H%xll%i W%ir%id!\n\r", 0xe, 0, 0, 1);
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

const static struct usbd_config cdc_config = {
    .init = init_vcdc,
    .desc = &cdc_desc_config.config,
    .function = { &cdc_function, NULL },
};

const struct usbd_device cdc_device = {
    .dev_desc = &cdc_dev_desc,
    .string_descs = cdc_string_descs,
    .configs = { &cdc_config, NULL }
};


void
main(void)
{
    usb_init(&cdc_device);
    sys_yield_for_frogs();
}
