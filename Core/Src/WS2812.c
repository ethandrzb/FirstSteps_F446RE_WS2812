/*
 * WS2182.c
 *
 *  Created on: Mar 17, 2024
 *      Author: ethan
 *      Adapted from ControllersTech's WS2812 video: https://youtu.be/71SRVEcbEwc
 */


#include "main.h"
#include <limits.h>
#include "WS2812.h"

uint8_t LEDData[NUM_LEDS][NUM_LED_PARAMS];

color background = {.red = 0, .blue = 0, .green = 0};

comet comets[NUM_MAX_COMETS];

// Change to SPI handle connected to LEDs
extern SPI_HandleTypeDef hspi3;
#define LED_SPI hspi3

void WS2812_SetLED(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)
{
	if(index >= NUM_LEDS)
	{
		return;
	}

	// These LEDs are supposed to use GRB color ordering, but RGB seems to be the real answer
	LEDData[index][0] = red;
	LEDData[index][1] = green;
	LEDData[index][2] = blue;

//	LEDData[index][0] = green;
//	LEDData[index][1] = red;
//	LEDData[index][2] = blue;
}

void WS2812_SetLEDAdditive(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)
{
	LEDData[index][0] = (LEDData[index][0]  + red) > UINT8_MAX ? 255 : (LEDData[index][0] + red);
	LEDData[index][1] = (LEDData[index][1] + green) > UINT8_MAX ? 255 : (LEDData[index][1] + green);
	LEDData[index][2] = (LEDData[index][2] + blue) > UINT8_MAX ? 255 : (LEDData[index][2] + blue);
}

void WS2812_ClearLEDs(void)
{
	for(int i = 0; i < NUM_LEDS; i++)
	{
		for(int j = 0; j <= 2; j++)
		{
			LEDData[i][j] = 0;
		}
	}
}

void WS2812_FadeAll(uint8_t denominator)
{
	for(int i = 0; i < NUM_LEDS; i++)
	{
		for(int j = 0; j <= 2; j++)
		{
			LEDData[i][j] /= denominator;
		}
	}
}

void WS2812_ShiftLEDs(int8_t shiftAmount)
{
	uint8_t tmp[NUM_LEDS][NUM_LED_PARAMS];
	for(int i = 0; i < NUM_LEDS; i++)
	{
		tmp[i][0] = LEDData[(i - shiftAmount) % NUM_LEDS][0];
		tmp[i][1] = LEDData[(i - shiftAmount) % NUM_LEDS][1];
		tmp[i][2] = LEDData[(i - shiftAmount) % NUM_LEDS][2];
	}

	for(int i = 0; i < NUM_LEDS; i++)
	{
		LEDData[i][0] = tmp[i][0];
		LEDData[i][1] = tmp[i][1];
		LEDData[i][2] = tmp[i][2];
	}
}

void WS2812_SendSingleLED(uint32_t red, uint32_t green, uint32_t blue)
{
	uint32_t color = (green << 16) | (red << 8) | (blue);
	uint8_t data[24];
	int index = 0;

	// Convert color data to format expected by WS2182
	// The SPI baud rate was set to 3x that of the LEDs so we can generate the bit timings expected by the LEDs
	// using simple bit sequences (0b100 for 0 and 0b110 for 1)
	for(int i = 23; i >= 0; i--)
	{

		if((color >> i) & 0x01)
		{
			// 2 high bits and 1 low bit
			data[index] = 0b110;
		}
		else
		{
			// 1 high bit and 2 low bits
			data[index] = 0b100;
		}
		index++;
	}

	HAL_SPI_Transmit(&LED_SPI, data, 24, 1000);
}

void WS2812_SendAll(void)
{
	for(int i = 0; i < NUM_LEDS; i++)
	{
		// Apply background color
		WS2812_SetLEDAdditive(i, background.red, background.green, background.blue);

		WS2812_SendSingleLED(LEDData[i][0], LEDData[i][1], LEDData[i][2]);
	}
}

void WS2812_InitMultiCometEffect(void)
{
	for(int i = 0; i < NUM_MAX_COMETS; i++)
	{
		comets[i].position = 0;
		comets[i].color.red = 0;
		comets[i].color.green = 0;
		comets[i].color.blue = 0;
		comets[i].size = 0;
		comets[i].active = false;
	}
}

void WS2812_AddComet(color color, uint8_t size)
{
	uint16_t index = 0;

	while(comets[index].active)
	{
		index++;
	}

	// All comets active
	if(index >= NUM_MAX_COMETS)
	{
		return;
	}

	comets[index].position = 0;
	comets[index].color = color;
	comets[index].size = size;
	comets[index].active = true;
}

void WS2812_MultiCometEffect(void)
{
	// Fade LEDs one step
	WS2812_FadeAll(2);

	// Increment position of active comets
	for(int i = 0; i < NUM_MAX_COMETS; i++)
	{
		if(comets[i].active)
		{
			// Deactivate comets when head reaches the end of the LED strip
			// -1 accounts for the fact that comets are drawn from [position, position + size], NOT [position + 1, position + 1 + size]
//			if(comets[i].position >= (NUM_LEDS - (comets[i].size - 1)))
			// Deactivate comet when it reaches the end of the strip
			// This is safe because WS2812_SetLED only sets the LED if it is in bounds
			if(comets[i].position >= NUM_LEDS)
			{
				comets[i].active = false;
			}
			else
			{
				// Draw comet
				for(int j = 0; j < comets[i].size; j++)
				{
					WS2812_SetLED(j + comets[i].position, comets[i].color.red, comets[i].color.green, comets[i].color.blue);
				}

				comets[i].position++;
			}
		}
	}
}

void WS2812_CometEffect(void)
{
	const uint8_t fadeAmount = 2;
	const uint32_t cometSize = 5;

	static uint8_t direction = 1;
	static uint8_t position = 0;

	position += direction;

	// Flip direction when we hit the ends of the strip
	if(position == (NUM_LEDS - cometSize) || (position == 0))
	{
		direction *= -1;
	}

	// Draw comet
	for(int i = 0; i < cometSize; i++)
	{
		WS2812_SetLED(i + position, 64, 64, 64);
	}

	// Fade LEDs one step
	WS2812_FadeAll(fadeAmount);
}

void WS2812_SetBackgroundColor(color newColor)
{
	background = newColor;
}
