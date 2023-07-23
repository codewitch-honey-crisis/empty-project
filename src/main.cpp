/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SPI_OP  SPI_OP_MODE_MASTER |SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <string.h>
#include <gfx.hpp>
using namespace gfx;
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

extern "C" int main(void);

#include "pca8685_def.h"
extern "C" {
	int pca9685_reset(const device* i2c_dev, uint8_t address);
	int pca9685_sleep(const device* i2c_dev, uint8_t address);
}
int pca9685_reset(const device* i2c_dev, uint8_t address) {
	int ret = i2c_reg_write_byte(i2c_dev, address,
                             PCA9685_MODE1, MODE1_RESTART);
    if (ret)
    {
        printk("Unable write to MODE1. (err %i)\n", ret);
        return ret;
    }
	k_sleep({10});
	return 0;
}
int pca9685_sleep(const device* i2c_dev, uint8_t address) {
	uint8_t val;
	int ret = i2c_reg_read_byte(i2c_dev,address,PCA9685_MODE1,&val);
	if (ret)
    {
        printk("Unable read from MODE1. (err %i)\n", ret);
        return ret;
    }
	uint8_t sleep = val | MODE1_SLEEP; // set sleep bit high
	ret = i2c_reg_write_byte(i2c_dev, address,
                             PCA9685_MODE1, sleep);
    if (ret)
    {
        printk("Unable write to MODE1. (err %i)\n", ret);
        return ret;
    }
	k_sleep({5});
	return 0;
}
/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

// A utility function to reverse a string
void reverse(char str[], int length)
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
char* citoa(int num, char* str, int base)
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
    if (isNegative)
        str[i++] = '-';
 
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
//static const struct device* i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
int main(void)
{
	int ret;
 	/*if (i2c_dev == NULL || !device_is_ready(i2c_dev))
    {
        printk("Could not get I2C device\n");
        return -1;
    }*/
	if (!gpio_is_ready_dt(&lcd_bl)) {
		printk("Could not get GPIO\n");
		return -1;
	}
	ret = gpio_pin_configure_dt(&lcd_bl, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}
	gpio_pin_set_dt(&lcd_bl,1);
	uint32_t count = 0U;
	char count_str[11] = {0};
	const struct device *display_dev;
	lv_obj_t *hello_world_label;
	lv_obj_t *count_label;
	display_dev =DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (display_dev == NULL) {
		LOG_ERR("device not found.  Aborting test.");
		gpio_pin_set_dt(&lcd_bl,0);
		return -1;
	}

	hello_world_label = lv_label_create(lv_scr_act());
	
	lv_label_set_text(hello_world_label, "Hello world!");
	
	lv_obj_align(hello_world_label,  LV_ALIGN_CENTER, 0, 0);
	count_label = lv_label_create(lv_scr_act());
	
	lv_obj_align(count_label, LV_ALIGN_CENTER, 0, 50);
	
	display_blanking_off(display_dev);
	char sz[16];
	while (1) {
		if ((count % 100) == 0U) {
			//sprintf(count_str, "%d", count/100U);
			citoa(count/100,sz,10);
			lv_label_set_text(count_label, sz);
		}
		
		lv_task_handler();
		k_timeout_t t;
		t.ticks = 10;
		k_sleep(t);
		++count;
		if(count>99900) {
			count = 0;
		}
	}

	return 0;
}
