/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool wireless_pending(void);
void wireless_capture(bool enable);
void wireless_submitted(uint8_t ep, uint16_t bytes);
void wireless_completed(uint8_t ep, bool success, uint32_t bytes);
void wireless_bus_reset(void);
void wireless_xinput_out(uint8_t const *data, uint32_t size);
#ifdef __cplusplus
}
// Only surround a driver's actual gameplay-IN submission, never its auth/keepalive traffic.
template<class Submit> inline bool wireless_report(Submit submit) {
	wireless_capture(true);
	bool ok = submit();
	wireless_capture(false);
	return ok;
}
#endif
