/* SPDX-License-Identifier: MIT */
#ifndef BRIDGE_SPI_H
#define BRIDGE_SPI_H
#include "protocol.h"
int spi_init(void);
int spi_arm(const struct gp_frame *reply);
/* One transaction: 1 valid, -1 malformed, 0 still armed. */
int spi_take(struct gp_frame *frame);
#endif
