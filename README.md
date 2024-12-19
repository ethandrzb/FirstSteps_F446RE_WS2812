# WS2812 addressable RGB LED effect and control library for STM32 MCUs

## Getting Started

### Using the library in a new project

1. Copy `Core/Inc/WS2812.h` to `Core/Inc/` in your project
2. Copy `Core/Src/WS2812.c` to `Core/Src/` in your project
3. Use STM32CubeMX to configure an SPI interface on your MCU to control the LEDS.
   1. Connect LED strip to your MCU and an appropriate power supply
      1. Data: **PB0** (SPI3 MOSI) ==> DIN (data in) on your LED strip
      2. Power
         1. **MCU and LED strip MUST share a common ground**
         2. WS2812 ==> 5V
         3. WS2811 ==> 12V
   2. Parameter settings
      1. **Mode: Half-duplex master**
      2. Data size: 8 bits
      3. First bit: MSB first
      4. **Baud rate: 2.5 MBits/sec**
           1. **NOTE:** WS2812 LEDs require precise bit timings to work with the SPI interface. **The baud rate of your SPI interface MUST match the specified baud rate for the library to work.**
      5. Clock polarity (CPOL): Low
      6. Clock phase (CPHA): 1 Edge
   3. NVIC settings
      1. **SPI global interrupt: Enabled**
4. Change the value of the `NUM_PHYSICAL_LEDS` macro in `Core/Inc/WS2812.h` to the number of LEDs in your LED strip or daisy chain
5. Change the handle of the SPI interface referenced at the top of `Core/Src/WS2812.c` to the one you configured in STM32CubeMX in Step 3. By default, the library is configured to use **SPI3**.

```
extern SPI_HandleTypeDef hspi3;
#define LED_SPI hspi3
```

6. Happy coding!

### Using the included example project for the STM32F446RE

1. Connect LED strip to your MCU and an appropriate power supply
   1. Data: **PB0** (SPI3 MOSI) ==> DIN (data in) on your LED strip
   1. Power
      1. **MCU and LED strip MUST share a common ground**
      2. WS2812 ==> 5V
      3. WS2811 ==> 12V
1. Clone the repo
2. Import the repo folder in STM32CubeIDE. It should recognize the project inside.
3. Change the value of the `NUM_PHYSICAL_LEDS` macro in `Core/Inc/WS2812.h` to the number of LEDs in your LED strip or daisy chain
4. Uncomment `#define EXAMPLE_1` in `Core/Src/main.c`
5. Compile and upload the code to your STM32F446RE
6. Happy coding!

## Compatibility

Project was built using an STM32F446RE Nucleo board using included hardware abstraction library (HAL). Should be compatible with most STM32 MCUs, but this is the only one that has been tested so far.

## Credits

- Adapted from ControllersTech's [WS2812 SPI video](https://youtu.be/71SRVEcbEwc)
- Comet effect based on [this tutorial](https://www.youtube.com/watch?v=yM5dY7K2KHM) from Dave's Garage