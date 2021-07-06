#include <stdint.h>
#include <string.h>
#include <stm8s.h>
#include <delay.h>
#include <i2c.h>
#include <uart.h>
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


const char*
skip_to_next_comma(const char *ptr, uint8_t count, uint8_t display)
{
    while (*ptr) {
        if (*ptr == ',') { 
            if (--count == 0)
                break;
        }
        if (display)
            LCD_Chr(*ptr);
        ++ptr;
    }
    return ptr;
}


const char*
display_chars(const char *str, uint8_t count)
{
    while (*str && count) {
        LCD_Chr(*str);
        ++str;
        --count;
    }
    return str;
}

const char*
display_time(const char *str)
{
    LCD_FStr("Time: ");
    str = display_chars(str, 2);
    LCD_Chr(':');
    str = display_chars(str, 2);
    LCD_Chr(':');
    str = display_chars(str, 5);
    LCD_Chr(' ');
    return str;
}

const char*
skip_chars(const char *str, int count)
{
    while (*str && count) {
        ++str;
        --count;
    }
    return str;
}

uint32_t
str_to_num(const char *str, int len)
{
    uint32_t num = 0;

    while (*str && len) {
	num *= 10;
        num +=	*str - '0';
	++str;
	--len;
    }
    return num;
}


void
display_num(uint32_t num,uint8_t len)
{
    char str[16];
    uint8_t i;

    i = 0;
    do {
	str[i] = (num % 10) + '0';
	num /= 10;
    } while (++i < 16 && num > 0);

    while ((i-- != 0) && (len-- != 0)) {
	LCD_Chr(str[i]);
    }
}

const char*
display_latlng(const char *str, int ll)
{
    uint32_t num;

    LCD_FStr(ll == 0 ? "Lat:  " : "Lon: ");
    if (ll == 0)
	str = display_chars(str, 2);
    else {
	if (str[0] == '0') {
	    LCD_Chr(' ');
	    str = display_chars(str+1, 2);
	} else
	    str = display_chars(str, 3);
    }
    LCD_Chr(' ');
    str = display_chars(str, 2);
    LCD_Chr(' ');
    str = skip_chars(str, 1);

    num = str_to_num(str, 5);	
    num *= 60;
    num /= 100000;
    str += 5;
    display_num(num, 2);
    str = skip_chars(str, 1);		/* skip of comma */
    LCD_Chr(' ');
    str = display_chars(str, 1);	/* N/S/E/W */
    return str;
}


const char*
display_altitude(const char *str)
{
    LCD_FStr("Alt:  ");
    str = skip_to_next_comma(str, 1, 1);
    str = skip_chars(str, 1);
    LCD_Chr(' ');
    str = display_chars(str, 1);
    return str;
}


void
display_nema_gga(const char *str)
{
    str = skip_to_next_comma(str, 1, 0);
    if (!(*str))
        goto fmt_err;
    ++str;
    str = display_time(str);
    LCD_Newline();

    if (!(*str) || *str != ',')
        goto fmt_err;
    ++str;

    str = display_latlng(str, 0);
    LCD_Newline();

    if (!(*str) || *str != ',')
        goto fmt_err;
    ++str;

    str = display_latlng(str, 1);
    LCD_Newline();

    if (!(*str) || *str != ',')
        goto fmt_err;
    ++str;

    str = skip_to_next_comma(str, 3, 0);
    if (!(*str) || *str != ',')
        goto fmt_err;
    ++str;

    str = display_altitude(str);

fmt_err:
    return;
}

#define BUFSZ           128
#define BUF_MAX_IDX     126

static uint8_t buf[BUFSZ];

void
read_nema_line()
{
    static uint8_t pos = 0;

    while (1) {
        buf[pos] = uart_read();
        if (buf[pos] == '\n') {
            buf[pos + 1] = '0';
            pos = 0;
            break;
        }
        if (pos < BUF_MAX_IDX)
            ++pos;
    }
}

void
delay_sec(uint8_t s)
{
    while (s-- > 0) {
        delay_ms(1000);
    }
}


void
main() 
{
    uint8_t fmt;
    // static const char *str = "$GPGGA,081828.00,1257.20740,N,07738.93172,E,1,07,6.39,866.5,M,-86.4,M,,*73";

    conf_led();

    //conf_clk();
    //conf_i2c();

    i2c_init();
    uart_init();

    
    LCD_Init();
    delay_ms(10);

    LCD_On();
    delay_ms(10);

    fmt = 0;
    while (1) {
        read_nema_line();
	//strcpy(buf, str);

        if (buf[0] == '$' &&
            buf[1] == 'G' &&
            buf[2] == 'P' &&
            buf[3] == 'G' &&
            buf[4] == 'G' &&
            buf[5] == 'A')
        {
            LCD_Clear();
            if (fmt == 0)
                display_nema_gga(buf);
            else
                LCD_FStr(buf);

            LCD_Update();
            delay_sec(20);
            //fmt ^= 1;
        }
    }

    //I2C_CR1 &= (uint8_t)~1;
    //blink();
}
