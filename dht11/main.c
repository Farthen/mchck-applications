// Lots of stuff stolen from https://github.com/grober-unfug/mchck/blob/master/examples/weather-station/weather-station.c

#include <mchck.h>

#include <usb/usb.h>
#include <usb/cdc-acm.h>

#define SysTick_MAX_TICKS    (0xFFFFFFUL)

#define DHT11

static struct cdc_ctx cdc;
static struct timeout_ctx t;

static int tick;
static int count;

static uint8_t data[5];

/* SysTick */

static void
systick_init(uint32_t ticks)
{
    /* Set reload register. -1 is used to have ticks ticks from 0 to 0. */
    SYST_RVR = (ticks - 1) & SysTick_MAX_TICKS;
    /* Reset counter value */
    SYST_CVR = 0;
    /* Enable SysTick counter with processor clock. */
    SYST_CSR = (1UL << 2) | (1UL << 0);
}

static void
systick_restart(void)
{
    tick = SYST_CVR;
}

static uint32_t
systick_diff(void)
{
    /* this code only works, if the time from 0 to 0 is larger than the measured time. */
    int32_t tick_diff = tick - SYST_CVR;
    if (SYST_CSR & (1UL << 16)) /* counted to 0 since last time this was read */
        return tick_diff + SYST_RVR + 1;
    else
        return tick_diff;
}

static void
humitemp_done(void)
{
    /* The first 16 bit of the transmission hold the humidity,
     * the second 16 bit the temperature in sign and magnitude
     * representation, and the last 8 bit are a checksum */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4])
    {
        printf("Checksum fail!\r\n");
    }

#ifdef DHT11
    accum humidity = data[0];
#else
    accum humidity = ((uint16_t)data[0]) / 10;
#endif
    printf("Humidity: %.1k\%\r\n", humidity);

#ifdef DHT11
    accum temp = (data[2] & 0x7f) + (accum)data[3] / 64;
#else
    accum temp = ((uint16_t)data[2]) / 10
#endif
    if (data[2] & 0x80) /* MSB set */
    {
        /* convert from sign and magnitude representation
         * to standard signed integer with two's complement */
        temp = -temp;
    }

    printf("Temperature: %.1k°C\r\n", temp);
}

static void
humitemp_read(uint32_t tick_diff)
{
    /* count starts at -2, the first two falling flanks signal start of transmission */
    if (count >= 0)
    {
        /* duration between two falling flanks,
           around 76µs should be a zero and 120µs should be a one */
        data[count/8] = (data[count/8] << 1) | (tick_diff/48 > 100);
    }

    count++;

    /* 40 bits are transmitted */
    if ( count >= 40)
    {
        humitemp_done();
    }
}

void
PORTB_Handler(void)
{
    if (pin_physport_from_pin(GPIO_PTB0)->pcr[pin_physpin_from_pin(GPIO_PTB0)].isf)
    {
        pin_physport_from_pin(GPIO_PTB0)->pcr[pin_physpin_from_pin(GPIO_PTB0)].raw |= 0; /* clear isf */
        uint32_t tick_diff = systick_diff();
        systick_restart();
        humitemp_read(tick_diff);
    }
}

static void
humitemp_startreading(void *data)
{
    /* setup pin for reading */
    gpio_dir(GPIO_PTB0, GPIO_INPUT);
    systick_restart();

    /* set digital debounce/filter */
    pin_physport_from_pin(GPIO_PTB0)->dfcr.cs = PORT_CS_LPO;
    pin_physport_from_pin(GPIO_PTB0)->dfwr.filt = 31;

    /* falling flank interrupt */
    pin_physport_from_pin(GPIO_PTB0)->pcr[pin_physpin_from_pin(GPIO_PTB0)].irqc = PCR_IRQC_INT_FALLING;

    /* start reading */
    int_enable(IRQ_PORTB);
}

static void
humitemp_start(void)
{
    /* reset */
    int_disable(IRQ_PORTB);
    count = -2;
    memset(data, 0, sizeof(data));

    /* 1ms LOW to initiate transmission */
    gpio_dir(GPIO_PTB0, GPIO_OUTPUT);
    gpio_write(GPIO_PTB0, GPIO_LOW);

    systick_init(SysTick_MAX_TICKS); /* SysTick runs for about 349ms between 0 and 0 */
    timeout_add(&t, 1, humitemp_startreading, NULL);
}



/* Communication over USB */
static void
new_data(uint8_t *data, size_t len)
{
    for (int i = 0; i < len; i++)
    {
        if (data[i] == '\r')
        {
            humitemp_start();
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
    sys_yield_for_frogs();
}
