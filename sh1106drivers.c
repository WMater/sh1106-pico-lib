#include <stdio.h>
#include <stdlib.h>
#include "font.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define I2C_SDA 6
#define I2C_SCL 7

#define SET_PAGE 0xB0
#define SET_COL_W_REV 0xA1
#define SET_L_COL 0x00
#define SET_H_COL 0x10

#define SCREEN_ON 0xAF
#define SCREEN_OFF 0xAE

typedef struct{
    int x0;
    int y0;
    int height;
    int width;
}SECTOR;


uint8_t screen_buff[8][132];
static int addr = 0x3C;


void cmd(uint8_t cmd){

    uint8_t data[2] = {0x00, cmd};
    i2c_write_blocking(i2c1, addr, data, 2, false);
}

void write(uint8_t *buff){

    for(int page = 0; page<8; page++){

        cmd(SET_PAGE + page);

        for(int i = 0; i<=131; i++){

            uint8_t ctr[2] = {0x40, *(buff + 132*page + i)};

            i2c_write_blocking(i2c1, addr, ctr, 2, true);
            
        }
    }
}

void clear(bool color, uint8_t *buff){
    uint8_t filler = color == true ? 0xff : 0x00;

    for(int page = 0; page <8; page++){
        for(int column = 0; column <132; column++){
            //buff[page][column] = filler;
            *(buff + 132* page + column) = filler;
        }
    }
}

void init_display(uint8_t *buff){

    //screen off
    cmd(SCREEN_OFF);

    //columns addresing policy
    cmd(SET_COL_W_REV);

    //starting column initialization
    cmd(SET_L_COL + 0b0010);//sh: 0b0010 ssd: 0b0000
    cmd(SET_H_COL + 0b0000);

    //clear screen
    clear(false, buff);
    write(buff);

    //screen on
    cmd(SCREEN_ON);
}


bool draw_pixel(int x, int y, uint8_t *buff){
    
    int page = y>>3; //:8
    uint8_t height = 1 << (y % 8 );

    if(y < 64 && y >= 0 && x >= 0 && x < 132){
        
        *(buff + 132*page + x) |= height;
        return true;
    }else{

        return false; //failed to draw pixel
    }
}


void draw_circle(int x, int y, int radius, uint8_t *buff){ //bresenhama for circle
    
    int xi = 0;
    int yi = radius;

    int d = 2*xi - yi + 1; //d0 = 1.25 - radius
        
    while(xi <= yi){
            
        draw_pixel(x + xi, y + yi, buff);
        draw_pixel(x + xi, y - yi, buff);

        draw_pixel(x - xi, y + yi, buff);
        draw_pixel(x - xi, y - yi, buff);

        draw_pixel(x + yi, y + xi, buff);
        draw_pixel(x - yi, y + xi, buff);

        draw_pixel(x + yi, y - xi, buff);
        draw_pixel(x - yi, y - xi, buff);

        if(d >= 0){

            d += 2*xi -2*yi + 5;
            yi = yi - 1;
        }else{
                
            d +=2*xi + 3;
        }

        xi++;
    }
}


void draw_line(int x0, int y0, int xk, int yk, uint8_t *buff){//bersenham for line

    int xi = 0, yi = 0; //step counter setup
    int dx = xk - x0, dy = y0 - yk; //delta setup

    //steps setup
    int step_x = xk >= x0 ? 1 : -1; 
    int step_y = yk >= y0 ? 1 : -1;

    int d;

    //helper pointers setup
    //i used unorthodox approach to distinguising a main axis
    //data is stored in helers so i can mirror solution for x considerung y
    int *main_axis, *secondary_axis;

    int main_axis_d, secondary_axis_d;

    int main_axis_step, secondary_axis_step;

    //case setup
    if(abs(dy) <= abs(dx)){

        d = abs(dx);

        main_axis = &xi;
        secondary_axis = &yi;

        main_axis_d = abs(dx);
        
        secondary_axis_d = -abs(dy);

        main_axis_step = step_x;
        secondary_axis_step = step_y;
    }else{

        d = abs(dy);

        main_axis = &yi;
        secondary_axis = &xi;

        main_axis_d = abs(dy);
        secondary_axis_d = -abs(dx);

        main_axis_step = step_y;
        secondary_axis_step = step_x;
    }

    int lim_x = abs(dx), lim_y = abs(dy);

    //loop
    while(abs(xi) <= lim_x && abs(yi) <= lim_y){

        draw_pixel(x0+xi, y0+yi, buff);

        if(d >= 0){
            
            d += 2*secondary_axis_d;
        }else{
            
            d += 2*(secondary_axis_d + main_axis_d);
            (*secondary_axis) += secondary_axis_step;
        }

        (*main_axis) += main_axis_step;
    }
}


int draw_letter(int x, int y, uint8_t* buff, char letter){

    int shift = 8 - y % 8;

    int h_page, l_page;

    l_page = y >> 3;
    h_page = l_page + 1;

    if(l_page < 0 || h_page > 7 || x < 0 || x > 122) return 2;

    for(int i = 0; i < 5; i++){

        *(buff + 132*l_page + x + i) |= ascii[letter - OFFSET][i] << 8-shift;
    }

    for(int i = 0; i < 5; i++){

        *(buff + 132*h_page + x + i) |= ascii[letter - OFFSET][i] >> shift;
    }

    return 0;
}

SECTOR draw_text(int x0, int y0, char* string, int len, uint8_t* buff){

    for(int i = 0; *(string+i) != '\0'; i++){

        draw_letter(x0 + 6*i, y0, buff, *(string+i));
        
    }
    

    SECTOR sector = {x0, y0, 8, len+(len*5)-1};
    return sector;
}


int main()
{
    stdio_init_all();

    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    

    init_display((uint8_t*)screen_buff);
    sleep_ms(100);

    //draw_line(3, 0, 3, 63, (uint8_t*)screen_buff);
    //draw_line(0, 1, 127, 1, (uint8_t*)screen_buff);
    clear(false, (uint8_t*)screen_buff);
    //SECTOR string1 = draw_text(0, 22, "Anastazja", 8, (uint8_t*)screen_buff);
    //SECTOR string3 = draw_text(7*5, 13, "jest cudowna.", 8, (uint8_t*)screen_buff);
    //SECTOR string2 = draw_text(7, 4, "i zda mature na 100%.", 12, (uint8_t*)screen_buff);

    draw_line(0, 63, 0, 0, (uint8_t*)screen_buff);
    draw_line(127, 0, 127, 63, (uint8_t*)screen_buff);
    draw_line(0, 0, 127, 0, (uint8_t*)screen_buff);
    draw_line(127, 63, 0, 63, (uint8_t*)screen_buff);

    draw_line(0, 52, 128, 52, (uint8_t*)screen_buff);
    draw_text(2, 54, "Player:999", 7, (uint8_t*)screen_buff);

    draw_line(52, 0, 52, 52, (uint8_t*)screen_buff);
    //draw_line(52, 26, 128, 26, (uint8_t*)screen_buff);
    //draw_line(38+52, 0, 38+52, 52, (uint8_t*)screen_buff);
    //draw_line(37+52, 0, 37+52, 52, (uint8_t*)screen_buff);

    draw_text(55, 40, "Unit:999", 5, (uint8_t*)screen_buff);
    draw_text(55, 28, "Number:999", 7, (uint8_t*)screen_buff);
    draw_text(55, 16, "Points:999", 7, (uint8_t*)screen_buff);
    draw_text(55, 4, "Pts.Sum:999", 8, (uint8_t*)screen_buff);
    

    write((uint8_t*)screen_buff);
    while (true) {
        //draw_letter(63, 50,(uint8_t*)screen_buff, ';');

        //printf("height: %d, width: %d\n", string2.height, string2.width);
        //write((uint8_t*)screen_buff);

    }
}

