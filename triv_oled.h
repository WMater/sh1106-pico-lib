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

#define SET_PAGE 0xB0
#define SET_COL_W_REV 0xA1
#define SET_L_COL 0x00
#define SET_H_COL 0x10

#define SEND_COMMAND 0x00
#define SEND_DATA 0x40

#define SCREEN_ON 0xAF
#define SCREEN_OFF 0xAE


typedef struct{
    i2c_inst_t *i2c;
    const uint8_t addr;
}DISPLAY;

typedef struct{
    unsigned int x0;
    unsigned int y0;
    unsigned int height;
    unsigned int width;
}SECTOR;

typedef struct{
    const unsigned int bitmap_height;
    const unsigned int bitmap_width;
    const uint8_t *bitmap;
}BITMAP;


void cmd(uint8_t cmd, DISPLAY* display);

void write(uint8_t *buff, DISPLAY* display);

void write_sector(SECTOR sector, uint8_t* buff, DISPLAY* display);

void clear(uint8_t *buff);

void init_display(uint8_t *buff, DISPLAY* display);

void draw_pixel(int x, int y, uint8_t *buff);

void draw_circle(int x, int y, int radius, uint8_t *buff);

void draw_line(int x0, int y0, int xk, int yk, uint8_t *buff);

void draw_letter(int x0, int y0, uint8_t* buff, char letter);

void draw_bitmap(SECTOR sector, const BITMAP bitmap, uint8_t* buff);

SECTOR draw_text(int x0, int y0, char* string, int len, uint8_t* buff);

SECTOR draw_int(int x0, int y0, int integer, uint8_t* buff);

void clear_sector(SECTOR sector, uint8_t* buff);


#endif