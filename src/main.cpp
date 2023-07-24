/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SPI_OP                                                                                     \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <string.h>
#include <gfx.hpp>
#include <uix.hpp>
#include <zephyr/logging/log.h>
#define OPENSANS_REGULAR_IMPLEMENTATION
#include "OpenSans_Regular.hpp"
extern "C" int main(void);

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(app);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

using namespace gfx;
using namespace uix;

using screen_t = screen<320, 240, rgb_pixel<16>>;
using label_t = label<screen_t::control_surface_type>;
using color_t = color<typename screen_t::pixel_type>;
using color32_t = color<rgba_pixel<32>>;

// A utility function to reverse a string
static void reverse(char str[], int length)
{
	int start = 0;
	int end = length - 1;
	while (start < end) {
		char temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		end--;
		start++;
	}
}
// Implementation of citoa()
static char *citoa(int num, char *str, int base)
{
	int i = 0;
	bool isNegative = false;

	/* Handle 0 explicitly, otherwise empty string is
	 * printed for 0 */
	if (num == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	// In standard itoa(), negative numbers are handled
	// only with base 10. Otherwise numbers are
	// considered unsigned.
	if (num < 0 && base == 10) {
		isNegative = true;
		num = -num;
	}

	// Process individual digits
	while (num != 0) {
		int rem = num % base;
		str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		num = num / base;
	}

	// If number is negative, append '-'
	if (isNegative) {
		str[i++] = '-';
	}

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	reverse(str, i);

	return str;
}

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec lcd_bl = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, bl_gpios);
static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
constexpr static const size_t lcd_buffer_size = 64 * 1024;
static uint8_t lcd_buffer[lcd_buffer_size];
static screen_t main_screen(lcd_buffer_size, lcd_buffer);
constexpr static const rgba_pixel<32> transparent(0, 0, 0, 0);
static label_t hello_world_label(main_screen);
static label_t count_label(main_screen);

static void uix_flush(const rect16 &bounds, const void *bmp, void *state)
{
	uint16_t w = bounds.x2 - bounds.x1 + 1;
	uint16_t h = bounds.y2 - bounds.y1 + 1;
	struct display_buffer_descriptor desc;
	desc.buf_size = screen_t::bitmap_type::sizeof_buffer({w, h});
	desc.width = w;
	desc.pitch = w;
	desc.height = h;
	display_write(display_dev, bounds.x1, bounds.y1, &desc, (void *)bmp);

	main_screen.set_flush_complete();
}

int main(void)
{
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

	main_screen.on_flush_callback(uix_flush);

	hello_world_label.text_open_font(&OpenSans_Regular);
	hello_world_label.text("Hello world!");
	hello_world_label.background_color(transparent);
	hello_world_label.border_color(transparent);
	hello_world_label.text_color(color32_t::black);
	hello_world_label.text_line_height(30);
	hello_world_label.padding({0, 0});
	hello_world_label.text_justify(uix_justify::center);
	hello_world_label.bounds(
		srect16(0, 0, main_screen.bounds().x2, hello_world_label.text_line_height() - 1)
			.center_vertical(main_screen.bounds()));
	main_screen.register_control(hello_world_label);

	count_label.text_open_font(&OpenSans_Regular);
	count_label.text("0");
	count_label.background_color(transparent);
	count_label.border_color(transparent);
	count_label.text_color(color32_t::black);
	count_label.text_line_height(30);
	count_label.padding({0, 0});
	count_label.text_justify(uix_justify::center);
	count_label.bounds(
		srect16(0, hello_world_label.bounds().y2 + 1, main_screen.bounds().x2,
			count_label.text_line_height() - 1 + hello_world_label.bounds().y2 + 1));
	main_screen.register_control(count_label);
	main_screen.background_color(color_t::old_lace);

	display_blanking_off(display_dev);
	char sz[16];
	while (1) {
		if ((count % 100) == 0U) {
			citoa(count / 100, sz, 10);
			count_label.text(sz);
		}

		main_screen.update();
		k_timeout_t t;
		t.ticks = 10;
		k_sleep(t);
		++count;
		if (count > 99900) {
			count = 0;
		}
	}

	return 0;
}
