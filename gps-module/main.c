#include <stdint.h>
#include <stm8s.h>
#include <delay.h>
#include <i2c.h>
#include "SSD1306.h"

#define LED_PIN     5

void
led_off()
{
    PB_ODR |= (1 << LED_PIN);
}


void
led_on()
{
    PB_ODR &= (uint8_t) (~(1 << LED_PIN));
}


void
conf_led()
{
    PB_DDR |= (1 << LED_PIN);
    PB_CR1 |= (1 << LED_PIN);
    led_off();    
}


void
conf_clk()
{
    CLK_CKDIVR = 0;
    CLK_SWCR = (2 | 4);
    CLK_SWR = 0xe1;
    while((CLK_SWCR & CLK_SWCR_SWBSY) != 0)
        ;
}


void
conf_i2c()
{
    I2C_FREQR &= (uint8_t) ~0x3f;
    I2C_FREQR |= (uint8_t) 16;
    I2C_CR1 &= (uint8_t) ~1;
    I2C_CCRH &= (uint8_t) ~(0x80 | 0x40 | 0x0f);
    I2C_CCRL = 0;

    I2C_TRISER = 5;
    I2C_CCRL = 7;
    I2C_CCRH = 0x80;
    I2C_CR1 |= 1;

    I2C_CR2 |= 0x04;

    I2C_OARL = 0x70;
    I2C_OARH = 0x40;
}

void
blink()
{
    while (1) {
        led_on();
        delay_ms(1000);
        led_off();
        delay_ms(1000);
    }
}

void
main() 
{
    conf_led();
    conf_clk();
    conf_i2c();

    delay_ms(1);
    
    //i2c_start();
    //ii2c_write_addr(0x78);
    //i2c_stop();

    LCD_Init();
    LCD_On();
    LCD_Clear();
    LCD_Update();

    I2C_CR1 &= (uint8_t)~1;
    blink();
}
