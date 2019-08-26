#include <stdint.h>
#include <stm8s.h>
#include <delay.h>
#include <i2c.h>

#include <oled_ssd1306.h>

#define LED_PIN     5


void
main() 
{
    PB_DDR |= (1 << LED_PIN);
    PB_CR1 |= (1 << LED_PIN);

    I2C_FREQR = (1 << I2C_FREQR_FREQ1);
    I2C_CCRL = 0x0A; // 100kHz
    I2C_OARH = (1 << I2C_OARH_ADDMODE); // 7-bit addressing
    //I2C_CR1 = (1 << I2C_CR1_PE);

    //i2c_init();
    //i2c_start();
    //i2c_write_addr(0x3C + I2C_WRITE);

    PB_ODR = 0;

    while (1)
        ;
}
