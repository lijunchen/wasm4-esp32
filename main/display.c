#include <stdio.h>

#include "runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7789.h"
#include "lcd.h"
#include "esp_task_wdt.h"
#include "control.h"
#include "wasm_export.h"
#include "esp_log.h"

// static uint32_t pixels[160*160];

static int viewportX = 0;
static int viewportY = 0;
static int viewportSize = 3*160;


extern wasm_module_t wasm_module;
extern wasm_module_inst_t wasm_module_inst;
extern wasm_function_inst_t start;
extern wasm_function_inst_t update;
extern wasm_exec_env_t exec_env;

void w4_windowBoot () {
    int counter = 0;
    do {
        if (!wasm_module_inst || !start || !update || !exec_env) {
            continue;
        }
        // Player 1
        uint8_t gamepad = get_player_state(0);
        // printf("player 1 state: %d\n", gamepad);
        w4_runtimeSetGamepad(0, gamepad);

        // Player 2
        gamepad = get_player_state(1);
        // printf("player 2 state: %d\n", gamepad);
        w4_runtimeSetGamepad(1, gamepad);

        clear_all_player_state();


        // Mouse handling
        uint8_t mouseButtons = 0;
        int mouseX = 0;
        int mouseY = 0;
        w4_runtimeSetMouse(160*(mouseX-viewportX)/viewportSize, 160*(mouseY-viewportY)/viewportSize, mouseButtons);
        w4_runtimeUpdate();
    } while (1);
}

int counter = 0;

uint16_t convert(uint32_t c) {
    uint16_t color = rgb565(
        (c & 0xff0000) >> 16,
        (c & 0xff00) >> 8,
         c & 0xff
    );
    return color;
}


void w4_windowComposite (const uint32_t* palette, const uint8_t* framebuffer) {
    // Convert indexed 2bpp framebuffer to XRGB output
    // uint32_t* out = pixels;
    for (int n = 0; n < 160*160/4; ++n) {
        uint8_t quartet = framebuffer[n];
        int color1 = (quartet & 0b00000011) >> 0;
        int color2 = (quartet & 0b00001100) >> 2;
        int color3 = (quartet & 0b00110000) >> 4;
        int color4 = (quartet & 0b11000000) >> 6;

        uint16_t c1 = convert(palette[color1]);
        uint16_t c2 = convert(palette[color2]);
        uint16_t c3 = convert(palette[color3]);
        uint16_t c4 = convert(palette[color4]);
        uint16_t cs[4] = {c1, c2, c3, c4};

        for (int i = n * 4; i < n * 4 + 4; i++) {
            int x = i % 160;
            int y = i / 160;
            uint16_t c = cs[i - n * 4];
            if (c != last[i]) {
                lcdDrawPixel(&dev, x + 40, y + 40, c);
                last[i] = c;
            }
        }
    }
}

TickType_t FillTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    {
        for (int i = 0; i < 2; i++) {
            TickType_t startTick, endTick, diffTick;
            startTick = xTaskGetTickCount();
            lcdFillScreen(dev, WHITE);
            lcdFillScreen(dev, GREEN);
            lcdFillScreen(dev, BLACK);
            endTick = xTaskGetTickCount();
            diffTick = endTick - startTick;
            ESP_LOGI(__FUNCTION__, "write a line elapsed time[ms]:%"PRIu32,diffTick*portTICK_PERIOD_MS);
        }
    }


    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%"PRIu32,diffTick*portTICK_PERIOD_MS);
    return diffTick;
}

TFT_t dev;

#define CUSTOM_MOSI_GPIO 23
#define CUSTOM_SCLK_GPIO 20
#define CUSTOM_CS_GPIO -1
#define CUSTOM_DC_GPIO 21
#define CUSTOM_RESET_GPIO 22
#define CUSTOM_BL_GPIO -1

void ST7789(void *pvParameters)
{
    // set font file
    FontxFile fx16G[2];
    FontxFile fx24G[2];
    FontxFile fx32G[2];
    FontxFile fx32L[2];
    InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
    InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
    InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
    InitFontx(fx32L,"/spiffs/LATIN32B.FNT",""); // 16x32Dot Latin

    FontxFile fx16M[2];
    FontxFile fx24M[2];
    FontxFile fx32M[2];
    InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
    InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
    InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo
    
    // Change SPI Clock Frequency
    //spi_clock_speed(40000000); // 40MHz
    //spi_clock_speed(60000000); // 60MHz

    spi_master_init(&dev, CUSTOM_MOSI_GPIO, CUSTOM_SCLK_GPIO, CUSTOM_CS_GPIO, CUSTOM_DC_GPIO, CUSTOM_RESET_GPIO, CUSTOM_BL_GPIO);
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

    lcdFillScreen(&dev, BLACK);
}