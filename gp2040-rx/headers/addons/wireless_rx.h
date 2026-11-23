/* SPDX-License-Identifier: MIT */
#pragma once
#include "gpaddon.h"
class WirelessRxAddon final : public GPAddon {
public:
	bool available() override { return true; }
	void setup() override;
	void preprocess() override {}
	void process() override;
	void postprocess(bool) override {}
	void reinit() override {}
	std::string name() override { return "WirelessRxAddon"; }
};
