#ifndef PCA9685_H
#define PCA9685_H
#include <zephyr/device.h>
#define PCA9685_I2C_ADDRESS 0x40      /**< Default PCA9685 I2C Slave Address */
#define PCA9685_FREQUENCY_OSCILLATOR 25000000 /**< Int. osc. frequency in datasheet */
#ifdef __cplusplus
extern "C" {
#endif
	int pca9685_reset(const struct device* i2c_dev, uint8_t address);
	int pca9685_sleep(const struct device* i2c_dev, uint8_t address);
    int pca9685_wake(const struct device* i2c_dev, uint8_t address);
	int pca9685_set_ext_clock(const struct device* i2c_dev, uint8_t address,uint8_t prescale);
	int pca9685_get_ext_clock(const struct device* i2c_dev, uint8_t address,uint8_t* out_prescale);
	int pca9685_set_pwm_freq(const struct device* i2c_dev, uint8_t address,float oscillator_freq, float freq);
	int pca9685_set_pwm(const struct device*i2c_dev, uint8_t address, uint8_t num, uint16_t on, uint16_t off);
	int pca9685_get_pwm(const struct device *i2c_dev, uint8_t address, uint8_t num, uint16_t* out_on, uint16_t* out_off);
#ifdef __cplusplus
}
#endif
#endif