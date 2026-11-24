/* SPDX-License-Identifier: MIT */
#pragma once
#include "match.h"
#include "protocol.h"
int match_usb_open(unsigned slots);
void match_usb_service(void);
unsigned match_usb_epoch(void);
bool match_usb_ready(void);
bool match_usb_submit(unsigned slot,const struct gp_input *pad);
unsigned match_usb_completions(unsigned slot);
uint32_t match_usb_lamp(unsigned slot);
