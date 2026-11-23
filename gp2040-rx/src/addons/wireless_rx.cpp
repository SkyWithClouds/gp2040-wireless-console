/* SPDX-License-Identifier: MIT */
#include "addons/wireless_rx.h"
#include "console_pad.h"
#include "wireless_delivery.h"
#include "storagemanager.h"
#include "hardware/spi.h"
#include "pico/rand.h"
#include "pico/stdlib.h"

namespace {
constexpr uint SCK=18, MOSI=19, MISO=16, CS=17, READY=20;
box_pad pad{};
bool capture, outstanding;
uint8_t endpoint;
uint16_t bytes, sequence;
uint64_t tag;
uint32_t last_spi;
uint64_t new_id() {
	uint64_t value;
	do { value = get_rand_64(); } while (!value);
	return value;
}
void poll_spi() {
	const auto now = time_us_64();
	const uint32_t ms = now / 1000;
	box_tick(&pad, ms, new_id);
	if (!gpio_get(READY) || uint32_t(now - last_spi) < 300) return;
	last_spi = now;
	uint8_t tx[GP_WIRE_SIZE], rx[GP_WIRE_SIZE];
	auto f = box_reply(&pad);
	if (!gp_encode(tx, sizeof(tx), &f)) return;
	gpio_put(CS, false);
	spi_write_read_blocking(spi0, tx, rx, sizeof(rx));
	while (spi_is_busy(spi0)) tight_loop_contents();
	gpio_put(CS, true);
	if (gp_decode(&f, rx, sizeof(rx))) box_receive(&pad, &f, ms);
}
}

void WirelessRxAddon::setup() {
	gpio_init(CS);
	gpio_put(CS, true);
	gpio_set_dir(CS, GPIO_OUT);
	gpio_init(READY);
	gpio_pull_down(READY);
	spi_init(spi0, 1000000);
	spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
	for (auto pin : {SCK, MOSI, MISO}) gpio_set_function(pin, GPIO_FUNC_SPI);
	box_start(&pad, new_id());
}

void WirelessRxAddon::process() {
	poll_spi();
	auto *gamepad = Storage::getInstance().GetGamepad();
	const gp_input &s = pad.state;
	GamepadState &out = gamepad->state;
	out = GamepadState{};
	out.dpad = out.dpadOriginal = s.buttons & 15;
	constexpr uint32_t buttons[] = {GAMEPAD_MASK_S2, GAMEPAD_MASK_S1, GAMEPAD_MASK_L3,
		GAMEPAD_MASK_R3, GAMEPAD_MASK_L1, GAMEPAD_MASK_R1, GAMEPAD_MASK_A1, GAMEPAD_MASK_A2,
		GAMEPAD_MASK_B1, GAMEPAD_MASK_B2, GAMEPAD_MASK_B3, GAMEPAD_MASK_B4};
	for (unsigned i=0; i<12; ++i)
		if (s.buttons & (1u << (i+4))) out.buttons |= buttons[i];
	if (s.lt >= 128) out.buttons |= GAMEPAD_MASK_L2;
	if (s.rt >= 128) out.buttons |= GAMEPAD_MASK_R2;
	out.lt = s.lt;
	out.rt = s.rt;
	out.lx = 32768 + s.lx;
	out.rx = 32768 + s.rx;
	out.ly = 32767 - s.ly;
	out.ry = 32767 - s.ry;
	gamepad->hasAnalogTriggers = gamepad->hasLeftAnalogStick = gamepad->hasRightAnalogStick = true;
}

bool wireless_pending() { return !pad.delivered && !outstanding; }
void wireless_capture(bool enable) { capture = enable; }
void wireless_submitted(uint8_t ep, uint16_t size) {
	if (!capture || !(ep & 0x80) || ep == 0x80 || !wireless_pending()) return;
	tag = pad.tag;
	sequence = pad.state.seq;
	endpoint = ep;
	bytes = size;
	outstanding = true;
}
void wireless_completed(uint8_t ep, bool success, uint32_t size) {
	if (!outstanding || endpoint != ep) return;
	outstanding = false;
	box_complete(&pad, tag, sequence, success && size == bytes);
}
void wireless_bus_reset() {
	outstanding = capture = false;
	pad.led_valid = false;
	box_start(&pad, new_id());
}
void wireless_xinput_out(uint8_t const *data, uint32_t size) {
	if (size >= 3 && data[0] == 1 && data[1] == 3) {
		pad.led_valid = true;
		pad.led_gen++;
		pad.led_mode = data[2];
	}
}
