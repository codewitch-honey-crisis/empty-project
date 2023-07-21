/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <string.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

extern "C" int main(void);

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

int main(void)
{
	printk("got here 1\r\n");
	int ret;

	if (!gpio_is_ready_dt(&lcd_bl)) {
		return -1;
	}
	printk("got here 2\r\n");
	ret = gpio_pin_configure_dt(&lcd_bl, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}
	gpio_pin_set_dt(&lcd_bl,1);
	printk("got here 3\r\n");
	uint32_t count = 0U;
	char count_str[11] = {0};
	const struct device *display_dev;
	lv_obj_t *hello_world_label;
	lv_obj_t *count_label;
	display_dev =DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	printk("got here 4\r\n");
	if (display_dev == NULL) {
		LOG_ERR("device not found.  Aborting test.");
		gpio_pin_set_dt(&lcd_bl,0);
		return -1;
	}

	hello_world_label = lv_label_create(lv_scr_act());
	printk("got here 6\r\n");
	
	lv_label_set_text(hello_world_label, "Hello world!");
	printk("got here 7\r\n");
	
	lv_obj_align(hello_world_label,  LV_ALIGN_CENTER, 0, 0);
	printk("got here 8\r\n");
	
	count_label = lv_label_create(lv_scr_act());
	printk("got here 9\r\n");
	
	lv_obj_align(count_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
	printk("got here 10\r\n");
	
	display_blanking_off(display_dev);
	printk("got here 11\r\n");
	char sz[16];
	while (1) {
		if ((count % 100) == 0U) {
			//sprintf(count_str, "%d", count/100U);
			citoa(count/100,sz,10);
			lv_label_set_text(count_label, sz);
		}
		printk("try task handler\r\n");
/*
try task handler

[00:00:00.132,000] [1B][1;31m<err> os: ***** MPU FAULT *****[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os:   Stacking error (context area might be not valid)[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os:   Data Access Violation[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os:   MMFAR Address: 0x24011b64[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os: r0/a1:  0x00000000  r1/a2:  0x0000013f  r2/a3:  0x000000ef[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os: r3/a4:  0x00000000 r12/ip:  0x29df952a r14/lr:  0x1dfd7b90[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os:  xpsr:  0x22b84000[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os: Faulting instruction address (r15/pc): 0x0459c661[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os: >>> ZEPHYR FATAL ERROR 2: Stack overflow on CPU 0[1B][0m
[00:00:00.132,000] [1B][1;31m<err> os: Current thread: 0x24000f30 (unknown)[1B][0m
[00:00:00.188,000] [1B][1;31m<err> os: Halting system[1B][0m
*/
		lv_task_handler();
		printk("got here 12\r\n");
		k_timeout_t t;
		t.ticks = 10;
		k_sleep(t);
		++count;
		printk(".\r\n");
		if(count>99900) {
			count = 0;
		}
	}

	return 0;
}
