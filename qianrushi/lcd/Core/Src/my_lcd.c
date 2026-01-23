#include "my_lcd.h"
#include "gpio.h"
#include "lcd.h"
void my_Lcd_Display(unsigned char  Line, unsigned char *ptr){
	__IO unsigned int PCout=GPIOC->ODR;
	LCD_DisplayStringLine(Line,ptr);
	GPIOC->ODR=PCout;
}
