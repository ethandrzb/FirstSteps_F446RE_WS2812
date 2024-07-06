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

#define NUM_LEDS 97
#define NUM_LED_PARAMS 3
#define NUM_MAX_COMETS 10

#define USE_NEW_SEND_FUNCTIONS

//#define WS2811_MODE

#ifndef MAX
#define MAX(x,y) (x > y) ? x : y
#endif

typedef struct colorRGB
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} colorRGB;

typedef struct colorHSV
{
	uint16_t hue;
	float saturation;
	float value;
} colorHSV;

typedef struct comet
{
	uint16_t position;
	colorRGB color;
	uint8_t size;
	bool active;
} comet;

// Base LED functions
void WS2812_SetLED(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_SetLEDAdditive(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_SetAllLEDs(uint32_t red, uint32_t green, uint32_t blue);
void WS2812_ClearLEDs(void);
void WS2812_FadeAll(uint8_t denominator);
void WS2812_ShiftLEDs(int8_t shiftAmount);

// Communication with LED strip
#ifndef USE_NEW_SEND_FUNCTIONS
void WS2812_SendSingleLED(uint32_t red, uint32_t green, uint32_t blue);
#else
uint8_t *WS2812_GetSingleLEDData(uint32_t red, uint32_t green, uint32_t blue);
#endif
void WS2812_SendAll(void);

// Effect functions
void WS2812_InitMultiCometEffect();
void WS2812_AddComet(colorRGB color, uint8_t size);
void WS2812_MultiCometEffect(void);
void WS2812_CometEffect(void);
void WS2812_SimpleMeterEffect(colorRGB color, uint8_t level, bool flip);
void WS2812_MirroredMeterEffect(colorRGB color, uint8_t level, bool centered);
void WS2812_FillRainbow(colorHSV startingColor, int8_t deltaHue);

// Utility functions
void WS2812_SetBackgroundColor(uint8_t red, uint8_t green, uint8_t blue);
colorRGB WS2812_HSVToRGB(uint16_t hue, float saturation, float value);

#endif /* INC_WS2812_H_ */
