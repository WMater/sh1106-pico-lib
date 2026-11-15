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
    OLED_WAR_OUT_OF_BOUND,
    //errors
    OLED_ERR_INIT_FAILED,
    OLED_ERR_BITMAP_NFIT,
    OLED_ERR_OUT_OF_BOUND,
    OLED_ERR_GEN,
    OLED_ERR_NEGNUM
}OledStatus_t;

typedef struct{
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t* buff;
    OledStatus_t init;
}DISPLAY;

typedef struct{
    int x0;
    int y0;
    int x1;
    int y1;
    int height;
    int width;
    OledStatus_t init;
}SECTOR;

typedef struct{
    const unsigned int bitmap_height;
    const unsigned int bitmap_width;
    const uint8_t *bitmap;
}BITMAP;

OledStatus_t cmd(uint8_t cmd, DISPLAY* display);
void clear_buff(DISPLAY* display);

void init_display_struct(DISPLAY* display, i2c_inst_t* i2c, uint8_t addr, uint8_t* buff);

void init_sector_struct(SECTOR* sector, int x0, int y0, int height, int width);
void resize_sector(SECTOR* sector, int height, int width);
void move_sector(SECTOR* sector, int x0, int y0);

OledStatus_t write_screen(DISPLAY* display);
OledStatus_t write_sector(SECTOR* sector, DISPLAY* display);


OledStatus_t draw_pixel(int x, int y, DISPLAY* display);
OledStatus_t draw_circle(int x, int y, int radius, DISPLAY* display);
OledStatus_t draw_line(int x0, int y0, int x1, int y1, DISPLAY* display);

OledStatus_t draw_letter(int x0, int y0, char letter, DISPLAY* display);
OledStatus_t draw_text_with_init(int x0, int y0, char* string, size_t len, SECTOR* sector, DISPLAY* display);
OledStatus_t draw_text(char* string, int len, SECTOR* sector, DISPLAY* display);
OledStatus_t draw_int_with_init(int x0, int y0, int integer, SECTOR* sector, DISPLAY* display);
OledStatus_t draw_int(int integer, SECTOR* sector, DISPLAY* display);

OledStatus_t draw_bitmap(SECTOR* sector, const BITMAP bitmap, DISPLAY* display);

void clear_sector(SECTOR* sector, DISPLAY* display);

#endif