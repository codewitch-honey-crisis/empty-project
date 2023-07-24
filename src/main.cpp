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
#include <stdio.h>
#include <gfx.hpp>
#include <uix.hpp>
#include "interface.hpp"
#include "ui.hpp"
extern "C" int main(void);

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(app);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

using namespace gfx;
using namespace uix;

static const struct gpio_dt_spec lcd_bl = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, bl_gpios);


static uint32_t get_ms() {
	return k_uptime_get();
}

static int serial_getch() {
	return -1;
    //return getc(stdin);
}
static size_t serial_read_bytes(uint8_t* buf, size_t len) {
	return fread(buf,1,len,stdin);
}
int main(void)
{
	printf("booted\n");
	static uint32_t timeout_ts=0;
	bool connected=false;
	int old_cpu_temp=-1;
	int old_gpu_temp=0;
	char cpu_sz[16];
	char gpu_sz[16];
	main_screen_init();
	disconnected_screen_init();
	int ret;
	
	if (!gpio_is_ready_dt(&lcd_bl)) {
		printf("Could not get backlight GPIO. Aborting\n");
		return -1;
	}
	ret = gpio_pin_configure_dt(&lcd_bl, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}
	gpio_pin_set_dt(&lcd_bl, 1);
	
	if (display_dev == NULL) {
		printf("device not found.  Aborting.\n");
		gpio_pin_set_dt(&lcd_bl, 0);
		return -1;
	}
	display_blanking_off(display_dev);
	while(serial_getch()!=-1);
	printf("cleared serial buffer\n");
	while (1) {
		// timeout for disconnection detection (1 second)
		if(timeout_ts!=0 && get_ms()>timeout_ts+1000) {
			timeout_ts = 0;
			active_screen=&disconnected_screen;
			active_screen->invalidate();
			connected = false;
		}
		
		// listen for incoming serial
		bool done = false;
		while(!done) {
			int i = serial_getch();
			uint8_t tmp;    
			if(i>-1) { // if data received...
				// reset the disconnect timeout
				timeout_ts = get_ms(); 
				if(!connected) {
					active_screen = &main_screen;
					active_screen->invalidate();
					connected = true;
				}
				printf("found serial data\n");
				switch(i) {
					case read_status_t::command: {
						read_status_t data;
						if(sizeof(data)==serial_read_bytes((uint8_t*)&data,sizeof(data))) {
							// update the CPU graph buffer (usage)
							if (cpu_buffers[0].full()) {
								cpu_buffers[0].get(&tmp);
							}
							cpu_buffers[0].put((data.cpu_usage/100.0f)*255);
							// update the bar and label values (usage)
							cpu_values[0]=data.cpu_usage/100.0f;
							// update the CPU graph buffer (temperature)
							if (cpu_buffers[1].full()) {
								cpu_buffers[1].get(&tmp);
							}
							cpu_buffers[1].put((data.cpu_temp/(float)data.cpu_temp_max)*255);
							if(data.cpu_temp>cpu_max_temp) {
								cpu_max_temp = data.cpu_temp;
							}
							// update the bar and label values (temperature)
							cpu_values[1]=data.cpu_temp/(float)data.cpu_temp_max;
							// force a redraw of the CPU bar and graph
							cpu_graph.invalidate();
							cpu_bar.invalidate();
							// update CPU the label (temperature)
							if(old_cpu_temp!=data.cpu_temp) {
								old_cpu_temp = data.cpu_temp;
								sprintf(cpu_sz,"%dC",data.cpu_temp);
								cpu_temp_label.text(cpu_sz);
							}
							// update the GPU graph buffer (usage)
							if (gpu_buffers[0].full()) {
								gpu_buffers[0].get(&tmp);
							}
							gpu_buffers[0].put((data.gpu_usage/100.0f)*255);
							// update the bar and label values (usage)
							gpu_values[0] = data.gpu_usage/100.0f;
							// update the GPU graph buffer (temperature)
							if (gpu_buffers[1].full()) {
								gpu_buffers[1].get(&tmp);
							}
							gpu_buffers[1].put((data.gpu_temp/(float)data.gpu_temp_max)*255);
							if(data.gpu_temp>gpu_max_temp) {
								gpu_max_temp = data.gpu_temp;
							}
							// update the bar and label values (temperature)
							gpu_values[1] = data.gpu_temp/(float)data.gpu_temp_max;
							// force a redraw of the GPU bar and graph
							gpu_graph.invalidate();
							gpu_bar.invalidate();
							// update GPU the label (temperature)
							if(old_gpu_temp!=data.gpu_temp) {
								old_gpu_temp = data.gpu_temp;
								sprintf(gpu_sz,"%dC",data.gpu_temp);
								gpu_temp_label.text(gpu_sz);   
							}
						} else {
							// eat bad data
							while(-1!=serial_getch());
						}
					}
					break;
					default:
						// eat unrecognized data
						while(-1!=serial_getch());
						break;
				};
			} else {
				done = true;
			}
		}
		if(active_screen!=nullptr) {
			active_screen->update();
		}		
	}
	
	return 0;
}
