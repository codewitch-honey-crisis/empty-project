/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SPI_OP                                                                                     \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <gfx.hpp>
#include <uix.hpp>
#include "ui.hpp"
extern "C" int main(void);

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(app);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

using namespace gfx;
using namespace uix;

static const struct gpio_dt_spec lcd_bl = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, bl_gpios);

int main(void)
{
	main_screen_init();
	disconnected_screen_init();
	int ret;

	if (!gpio_is_ready_dt(&lcd_bl)) {
		printk("Could not get GPIO\n");
		return -1;
	}
	ret = gpio_pin_configure_dt(&lcd_bl, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}
	gpio_pin_set_dt(&lcd_bl, 1);
	uint32_t count = 0U;

	
	if (display_dev == NULL) {
		printk("device not found.  Aborting test.");
		gpio_pin_set_dt(&lcd_bl, 0);
		return -1;
	}


	display_blanking_off(display_dev);
	while (1) {
		
		active_screen.update();
		k_sleep(K_MSEC(1));
	}

	return 0;
}
