#include <mchck.h>
#include <inttypes.h>
#include "nrf24l01.h"

static uint8_t nrf_spi_send_buffer[257];
static uint8_t nrf_spi_recv_buffer[257];
static struct spi_ctx nrf_command_ctx;

typedef void (nrf_command_cb_t)(void *);

void
nrf_command(uint8_t command, size_t data_len, spi_cb *cb, void *cbdata)
{
    nrf_spi_send_buffer[0] = command;
    spi_queue_xfer(&nrf_command_ctx, SPI_PCS2,
        nrf_spi_send_buffer, 1,
        nrf_spi_recv_buffer, data_len + 1,
        cb, cbdata);
}

void
nrf_init()
{
    // Configure the SPI
    spi_init();

    // Configure the SPI pins: CS, CLK, MOSI, MISO
    pin_mode(GPIO_PTC2, PIN_MODE_MUX_ALT2);
    pin_mode(GPIO_PTC5, PIN_MODE_MUX_ALT2);
    pin_mode(GPIO_PTC6, PIN_MODE_MUX_ALT2);
    pin_mode(GPIO_PTC7, PIN_MODE_MUX_ALT2);
}

void
nrf_print_config_callback()
{
    printf("Successfully read register %" SCNu8 ": %" SCNu8 "\r\n", nrf_spi_send_buffer[0] & 0x1F, nrf_spi_recv_buffer[1]);
}

void
nrf_print_config()
{
    printf("Reading config register...\r\n");
    nrf_command(NRF_COMMAND_R_REGISTER(NRF_REGISTER_CONFIG), 1, &nrf_print_config_callback, NULL);
}
