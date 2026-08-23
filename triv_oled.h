#ifndef TRIV_OLED_H
#define TRIV_OLED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "font.h"

#define DISPLAY_WIDTH 132
#define DISPLAY_HEIGHT 64
#define ST_LINE 2

#define SET_PAGE 0xB0
#define SET_COL_W_REV 0xA1
#define SET_L_COL 0x00
#define SET_H_COL 0x10

#define SEND_COMMAND 0x00
#define SEND_DATA 0x40

#define SCREEN_ON 0xAF
#define SCREEN_OFF 0xAE

typedef enum{
    OLED_OK,
    //warnings
    OLED_WAR_PIXEL_OUT_OF_BOUND,
    OLED_WAR_LINE_OUT_OF_BOUND,
    OLED_WAR_CIRCLE_OUT_OF_BOUND,
    //errors
    OLED_ERR_SECTOR_INIT_FAILED,
    OLED_ERR_BITMAP_NFIT,
    OLED_ERR_TEXT_OUT_OF_BOUND
}OledStatus_t;

typedef struct{
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t* buff;
}DISPLAY;

typedef struct{
    unsigned int x0;
    unsigned int y0;
    unsigned int x1;
    unsigned int y1;
    unsigned int height;
    unsigned int width;
}SECTOR;

typedef struct{
    const unsigned int bitmap_height;
    const unsigned int bitmap_width;
    const uint8_t *bitmap;
}BITMAP;

void init_display_struct(DISPLAY* display, i2c_inst_t* i2c, uint8_t addr, uint8_t* buff);

void init_sector_struct(SECTOR* sector, int x0, int y0, int height, int width);

void cmd(uint8_t cmd, DISPLAY* display);

void write_screen(DISPLAY* display);

void write_sector(SECTOR sector, DISPLAY* display);

void clear_buff(DISPLAY* display);

void init_display(DISPLAY* display);

void draw_pixel(int x, int y, DISPLAY* display);

void draw_circle(int x, int y, int radius, DISPLAY* display);

void draw_line(int x0, int y0, int x1, int y1, DISPLAY* display);

void draw_letter(int x0, int y0, char letter, DISPLAY* display);

void draw_bitmap(SECTOR sector, const BITMAP bitmap, DISPLAY* display);

SECTOR draw_text(int x0, int y0, char* string, int len, DISPLAY* display);

SECTOR draw_int(int x0, int y0, int integer, DISPLAY* display);

void clear_sector(SECTOR sector, DISPLAY* display);

#endif