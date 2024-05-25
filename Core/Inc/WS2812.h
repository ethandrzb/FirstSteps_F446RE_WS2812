/*
 * WS2182.h
 *
 *  Created on: Mar 17, 2024
 *      Author: ethan
 */

#ifndef INC_WS2812_H_
#define INC_WS2812_H_

#include <stdint.h>
#include <stdbool.h>

#define NUM_LEDS 100
#define NUM_LED_PARAMS 4
#define NUM_MAX_COMETS 10

#ifndef MAX
#define MAX(x,y) (x > y) ? x : y
#endif

typedef struct color
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} color;

typedef struct comet
{
	uint16_t position;
	color color;
	uint8_t size;
	bool active;
} comet;

void WS2812_SetLED(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_SetLEDAdditive(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_SetAllLEDsAdditive(uint8_t red, uint8_t green, uint8_t blue);
void WS2812_FadeAll(uint8_t denominator);
void WS2812_ShiftLEDs(int8_t shiftAmount);
void WS2812_SendSingleLED(uint32_t red, uint32_t green, uint32_t blue);
void WS2812_SendAll(void);
void WS2812_InitMultiCometEffect();
void WS2812_AddComet(color color, uint8_t size);
void WS2812_MultiCometEffect();
void WS2812_CometEffect();

#endif /* INC_WS2812_H_ */
