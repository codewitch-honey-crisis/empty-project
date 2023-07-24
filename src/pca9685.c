#include "pca9685_def.h"
#include "pca9685.h"
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
int pca9685_reset(const struct device *i2c_dev, uint8_t address)
{
	int ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1, MODE1_RESTART);
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	k_timeout_t t;
	t.ticks = 10;
	k_sleep(t);
	return 0;
}
int pca9685_sleep(const struct device *i2c_dev, uint8_t address)
{
	uint8_t val;
	int ret = i2c_reg_read_byte(i2c_dev, address, PCA9685_MODE1, &val);
	if (ret) {
		printk("Unable to read from MODE1. (err %i)\n", ret);
		return ret;
	}
	uint8_t sleep = val | MODE1_SLEEP; // set sleep bit high
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1, sleep);
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}

	k_timeout_t t;
	t.ticks = 10;
	k_sleep(t);
	return 0;
}
int pca9685_wake(const struct device *i2c_dev, uint8_t address)
{
	uint8_t sleep;
	int ret = i2c_reg_read_byte(i2c_dev, address, PCA9685_MODE1, &sleep);
	if (ret) {
		printk("Unable to read from MODE1. (err %i)\n", ret);
		return ret;
	}
	uint8_t wakeup = sleep & ~MODE1_SLEEP; // set sleep bit low
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1, wakeup);
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	return 0;
}
int pca9685_set_ext_clock(const struct device *i2c_dev, uint8_t address, uint8_t prescale)
{
	uint8_t oldmode;
	int ret = i2c_reg_read_byte(i2c_dev, address, PCA9685_MODE1, &oldmode);
	if (ret) {
		printk("Unable to read from MODE1. (err %i)\n", ret);
		return ret;
	}
	uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1,
				 newmode); // go to sleep, turn off internal oscillator
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	// This sets both the SLEEP and EXTCLK bits of the MODE1 register to switch to
	// use the external clock.
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1, (newmode |= MODE1_EXTCLK));
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_PRESCALE, prescale); // set the prescaler
	if (ret) {
		printk("Unable to write to PRESCALE. (err %i)\n", ret);
		return ret;
	}
	k_timeout_t t;
	t.ticks = 5;
	k_sleep(t);
	// clear the SLEEP bit to start
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1,
				 (newmode & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI);
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	return 0;
}
int pca9685_get_ext_clock(const struct device *i2c_dev, uint8_t address, uint8_t *out_prescale)
{
	int ret = i2c_reg_read_byte(i2c_dev, address, PCA9685_PRESCALE,
				    out_prescale); // set the prescaler
	if (ret) {
		printk("Unable to read from PRESCALE. (err %i)\n", ret);
		return ret;
	}
	return 0;
}
int pca9685_set_pwm_freq(const struct device *i2c_dev, uint8_t address, float oscillator_freq,
			 float freq)
{
	// Range output modulation frequency is dependant on oscillator
	if (freq < 1) {
		freq = 1;
	}
	if (freq > 3500) {
		freq = 3500; // Datasheet limit is 3052=50MHz/(4*4096)
	}

	float prescaleval = ((oscillator_freq / (freq * 4096.0)) + 0.5) - 1;
	if (prescaleval < PCA9685_PRESCALE_MIN) {
		prescaleval = PCA9685_PRESCALE_MIN;
	}
	if (prescaleval > PCA9685_PRESCALE_MAX) {
		prescaleval = PCA9685_PRESCALE_MAX;
	}
	uint8_t prescale = (uint8_t)prescaleval;

	uint8_t oldmode;
	int ret = i2c_reg_read_byte(i2c_dev, address, PCA9685_MODE1, &oldmode);
	if (ret) {
		printk("Unable to read from MODE1. (err %i)\n", ret);
		return ret;
	}
	uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP;         // sleep
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1, newmode); // go to sleep
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_PRESCALE, prescale); // set the prescaler
	if (ret) {
		printk("Unable to write to PRESCALE. (err %i)\n", ret);
		return ret;
	}
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1, oldmode);
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	k_timeout_t t;
	t.ticks = 5;
	k_sleep(t);
	// This sets the MODE1 register to turn on auto increment.
	ret = i2c_reg_write_byte(i2c_dev, address, PCA9685_MODE1,
				 oldmode | MODE1_RESTART | MODE1_AI);
	if (ret) {
		printk("Unable to write to MODE1. (err %i)\n", ret);
		return ret;
	}
	return 0;
}
int pca9685_set_pwm(const struct device *i2c_dev, uint8_t address, uint8_t num, uint16_t on,
		    uint16_t off)
{
	uint8_t buffer[5];
	buffer[0] = PCA9685_LED0_ON_L + 4 * num;
	buffer[1] = on;
	buffer[2] = on >> 8;
	buffer[3] = off;
	buffer[4] = off >> 8;
	int ret = i2c_write(i2c_dev, buffer, sizeof(buffer), address);
	if (ret) {
		printk("Unable to set PWM. (err %i)\n", ret);
		return ret;
	}

	return 0;
}
int pca9685_get_pwm(const struct device *i2c_dev, uint8_t address, uint8_t num, uint16_t *out_on,
		    uint16_t *out_off)
{
	uint8_t out_buf[2];
    uint8_t in_buf[2];
	in_buf[0] = PCA9685_LED0_ON_L + 4 * num;
	in_buf[1] = 0;
	int ret;
    if (out_on != NULL) {
		ret = i2c_write_read(i2c_dev,address,in_buf,sizeof(in_buf),out_buf,sizeof(out_buf));
        if(ret) {
            printk("Unable to get PWM on value. (err %i)\n", ret);
            return ret;
        }
        *out_on =  ((uint16_t)out_buf[0]) | (((uint16_t)out_buf[1]) << 8);
	}
    if(out_off!=NULL) {
        in_buf[0]+=2;
        ret = i2c_write_read(i2c_dev,address,in_buf,sizeof(in_buf),out_buf,sizeof(out_buf));
        if(ret) {
            printk("Unable to get PWM off value. (err %i)\n", ret);
            return ret;
        }
        *out_off =  ((uint16_t)out_buf[0]) | (((uint16_t)out_buf[1]) << 8);
    }
    return 0;
}
