/* SPDX-License-Identifier: MIT */
#include "spi.h"
#include <zephyr/kernel.h>
#include <nrfx_spis.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>

enum { SCK = 20, MOSI = 17, MISO = 22, CS = 24, READY = 32 };
static const nrfx_spis_t device = NRFX_SPIS_INSTANCE(2);
static const nrfx_gpiote_t gpio_events = NRFX_GPIOTE_INSTANCE(0);
static uint8_t clear_on_cs, ready_task;
static struct {
	uint8_t tx[GP_WIRE_SIZE] __aligned(4);
	uint8_t rx[GP_WIRE_SIZE + 1] __aligned(4);
} dma[2];
static unsigned bank;
static atomic_t wait_cs_high;
static atomic_t received; /* 0 waiting, otherwise byte count + 1 */

static void event(const nrfx_spis_evt_t *e, void *context)
{
	ARG_UNUSED(context);
	if (e->evt_type == NRFX_SPIS_BUFFERS_SET_DONE) {
		if (nrf_gpio_pin_read(CS)) NRF_GPIOTE->TASKS_SET[ready_task] = 1;
		else atomic_set(&wait_cs_high, 1);
	} else if (e->evt_type == NRFX_SPIS_XFER_DONE) {
		atomic_clear(&wait_cs_high);
		atomic_set(&received, 1 + e->rx_amount);
	}
}

int spi_init(void)
{
	nrf_ppi_channel_t ppi;
	if (nrfx_gpiote_channel_alloc(&gpio_events, &clear_on_cs) ||
	    nrfx_gpiote_channel_alloc(&gpio_events, &ready_task) ||
	    nrfx_ppi_channel_alloc(&ppi)) return -ENOMEM;
	nrf_gpio_pin_clear(READY);
	nrf_gpio_cfg_output(READY);
	nrfx_spis_config_t cfg = NRFX_SPIS_DEFAULT_CONFIG(SCK, MOSI, MISO, CS);
	cfg.csn_pullup = NRF_GPIO_PIN_PULLUP;
	cfg.irq_priority = 3;
	IRQ_CONNECT(SPIM2_SPIS2_SPI2_IRQn, 3,
		    nrfx_spis_2_irq_handler, NULL, 0);
	if (nrfx_spis_init(&device, &cfg, event, NULL)) return -EIO;
	nrf_gpiote_event_configure(NRF_GPIOTE, clear_on_cs, CS, NRF_GPIOTE_POLARITY_HITOLO);
	nrf_gpiote_event_enable(NRF_GPIOTE, clear_on_cs);
	nrf_gpiote_task_configure(NRF_GPIOTE, ready_task, READY, NRF_GPIOTE_POLARITY_TOGGLE,
				  NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(NRF_GPIOTE, ready_task);
	if (nrfx_ppi_channel_assign(ppi, (uint32_t)&NRF_GPIOTE->EVENTS_IN[clear_on_cs],
				   (uint32_t)&NRF_GPIOTE->TASKS_CLR[ready_task]) ||
	    nrfx_ppi_channel_enable(ppi)) return -EIO;
	return 0;
}

int spi_arm(const struct gp_frame *reply)
{
	bank ^= 1;
	if (!gp_encode(dma[bank].tx, GP_WIRE_SIZE, reply)) return -EINVAL;
	return nrfx_spis_buffers_set(&device, dma[bank].tx, sizeof(dma[bank].tx),
				     dma[bank].rx, sizeof(dma[bank].rx)) ? -EIO : 0;
}

int spi_take(struct gp_frame *frame)
{
	/* A master may hold CS low at power-up. Advertise this buffer once
	 * after release; a subsequent falling edge still clears READY in hardware. */
	unsigned int key = irq_lock();
	if (atomic_get(&wait_cs_high) && nrf_gpio_pin_read(CS)) {
		atomic_clear(&wait_cs_high);
		NRF_GPIOTE->TASKS_SET[ready_task] = 1;
	}
	atomic_val_t count = atomic_set(&received, 0);
	irq_unlock(key);
	if (!count) return 0;
	return gp_decode(frame, dma[bank].rx, count - 1) ? 1 : -1;
}

void match_prepare_boot(void)
{
    irq_disable(SPIM2_SPIS2_SPI2_IRQn);
    NRF_GPIOTE->TASKS_CLR[ready_task]=1;
    nrfx_spis_uninit(&device);
}
