/* SPDX-License-Identifier: MIT */
#include <zephyr/kernel.h>
#include "air.h"
#include "match_usb.h"
#include "spi.h"
#include "link.h"

int main(void)
{
	struct link link;
	link_init(&link, air_nonce);
	match_init(CONSOLE_RX);
	int rc = match_usb_open(0);
	if (rc) return rc;
	rc = air_start(true);
	if (rc) return rc;
	rc = spi_init();
	if (rc) return rc;
	struct gp_frame reply = link_spi(&link, NULL, k_uptime_get_32());
	rc = spi_arm(&reply);
	while (!rc) {
		uint32_t ms = k_uptime_get_32();
		link_tick(&link, ms);
		struct gp_frame f;
		while (air_receive(&f)) {
			if (f.type == GP_STATUS || f.type == GP_OFFER) continue;
			reply = link_rf(&link, &f, ms);
			air_send(&reply);
		}
		int count = spi_take(&f);
		if (count) {
			reply = link_spi(&link, count > 0 ? &f : NULL, ms);
			rc = spi_arm(&reply);
		}
		match_usb_service();
		k_sleep(K_USEC(100));
	}
	return rc;
}
