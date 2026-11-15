#include "triv_oled.h"
//config
OledStatus_t cmd(uint8_t cmd, DISPLAY* display){

    if(display->init != OLED_OK) return OLED_ERR_GEN;
    static uint8_t data[2];
    data[0] = SEND_COMMAND;
    data[1] = cmd;
    if(i2c_write_blocking(display->i2c, display->addr, data, 2, false) == PICO_ERROR_GENERIC) return OLED_ERR_GEN;
    return OLED_OK;
}

void clear_buff(DISPLAY* display){

    uint8_t filler = 0x00;

    memset(display->buff, filler, DISPLAY_WIDTH * 8);
}

OledStatus_t init_display(DISPLAY* display){

    OledStatus_t status = OLED_OK;
    //screen off
    status |= cmd(SCREEN_OFF, display);

    //columns addresing policy
    status |= cmd(SET_COL_W_REV, display);

    //starting column initialization
    status |= cmd(SET_L_COL + (ST_LINE & 0b00001111), display);//sh: 0b0010 ssd: 0b0000
    status |= cmd(SET_H_COL + (ST_LINE & 0b11110000), display);

    //clear screen
    clear_buff(display);
    status |= write_screen(display);

    //screen on
    status |= cmd(SCREEN_ON, display);

    return status;
}

//structs
void init_display_struct(DISPLAY* display, i2c_inst_t* i2c, uint8_t addr, uint8_t* buff){

    display->i2c = i2c;
    display->addr = addr;
    display->buff = buff;

    display->init = init_display(display);
}

void init_sector_struct(SECTOR* sector, int x0, int y0, int height, int width){
    if((unsigned int)x0 >= DISPLAY_WIDTH - (2*ST_LINE) || (unsigned int)y0 >= DISPLAY_HEIGHT){

        sector->init = OLED_ERR_INIT_FAILED;
        return;
    }
    sector->x0 = x0;
    sector->y0 = y0;
    sector->height = height;
    sector->width = width;
    
    int x1 = x0 + width - 1;
    int y1 = y0 - height + 1;
    if((unsigned int)x1 >= DISPLAY_WIDTH - 4 || (unsigned int)y1 >= DISPLAY_HEIGHT){

        sector->init = OLED_ERR_INIT_FAILED;
        return;
    }
    sector->x1 = x1;
    sector->y1 = y1;
    
    sector->init = OLED_OK;
}

void resize_sector(SECTOR* sector, int height, int width){

    int x1 = sector->x0 + width - 1;
    int y1 = sector->y0 - height + 1;
    if((unsigned int)x1 >= (DISPLAY_WIDTH - (2*ST_LINE)) || (unsigned int)y1 >= DISPLAY_HEIGHT){

        sector->init = OLED_ERR_INIT_FAILED;
    }

    sector->height = height;
    sector->width = width;
    sector->x1 = x1;
    sector->y1 = y1;
}

void move_sector(SECTOR* sector, int x0, int y0){
    //moves it only logicly, should be cleared before moving
    //for moving filled sectors it is recomended to write a simple helper function f.eg.
    //clear_sector()
    //move_sector()
    //draw to sector
    //write_sector
    int x1 = x0 + sector->width - 1;
    int y1 = y0 - sector->height + 1;

    if((unsigned int)x0 >= (DISPLAY_WIDTH - (2*ST_LINE)) || (unsigned int)y0 >= DISPLAY_HEIGHT || (unsigned int)x1 >= (DISPLAY_WIDTH - (2*ST_LINE)) || (unsigned int)y1 >= DISPLAY_HEIGHT){

        sector->init = OLED_ERR_INIT_FAILED;
    }

    sector->x0 = x0;
    sector->y0 = y0;
    
    sector->x1 = x1;
    sector->y1 = y1;
}

//sending buffor
OledStatus_t write_screen(DISPLAY* display){

    if(display->init != OLED_OK) return OLED_ERR_INIT_FAILED;

    OledStatus_t status = OLED_OK;
    static uint8_t datacmd[DISPLAY_WIDTH + 1];
    datacmd[0] = SEND_DATA;
    
    for(int page = 0; page < 8; page++){
        
        memcpy(&datacmd[1], (display->buff + DISPLAY_WIDTH*page), DISPLAY_WIDTH);
        status |= cmd(SET_PAGE + page, display);

        if(i2c_write_blocking(display->i2c, display->addr, datacmd, DISPLAY_WIDTH + 1, false) == PICO_ERROR_GENERIC) status |= OLED_ERR_GEN;
    }

    return status;
}

OledStatus_t write_sector(SECTOR* sector, DISPLAY* display){
    //updates selected sector only

    if(sector->init != OLED_OK || display->init != OLED_OK) return OLED_ERR_INIT_FAILED;
    OledStatus_t status = OLED_OK;

    //setting helper array containing page data and data control byte
    static uint8_t pagedata[DISPLAY_WIDTH];
    pagedata[0] = SEND_DATA;
    
    int8_t page = sector->y0 / 8;
    int8_t last_page = sector->y1  / 8;

    //sending loop
    while(page >= last_page){
        
        //seting up sector starting column as writing start column
        status |= cmd(SET_PAGE + page, display);
        status |= cmd(SET_H_COL + (((sector->x0 + ST_LINE) & 0b11110000)>>4), display);
        status |= cmd(SET_L_COL + ((sector->x0 + ST_LINE) & 0b00001111), display);

        memcpy(&pagedata[1], (display->buff + DISPLAY_WIDTH*page + sector->x0), sector->width);

        if(i2c_write_blocking(display->i2c, display->addr, pagedata, sector->width+1, false) == PICO_ERROR_GENERIC) status |= OLED_ERR_GEN;

        page--;
    }
        
    //back to initial starting line (line 2)
    status |= cmd(SET_L_COL + (ST_LINE & 0b00001111), display);
    status |= cmd(SET_H_COL + ((ST_LINE & 0b11110000)>>4), display);

    return status;
}

//filling buff
OledStatus_t draw_pixel(int x, int y, DISPLAY* display){
    
    int8_t page = y / 8;
    uint8_t pixel = 1 << (y % 8);

    if(display->init != OLED_OK) return OLED_ERR_INIT_FAILED;

    if((unsigned int)y >= DISPLAY_HEIGHT || (unsigned int)x >= (DISPLAY_WIDTH - (2*ST_LINE))){
        
        return OLED_WAR_OUT_OF_BOUND;
    }

    *(display->buff + DISPLAY_WIDTH*page + x) |= pixel;
    return OLED_OK;
}


OledStatus_t draw_circle(int x, int y, int radius, DISPLAY* display){ //midpoint for circle, takes central point
    
    OledStatus_t status = OLED_OK;

    int xi = 0;
    int yi = radius;

    int d = 2*xi - yi + 1; //d0 = 1.25 - radius
        
    while(xi <= yi){
            
        status |= draw_pixel(x + xi, y + yi, display);
        status |= draw_pixel(x + xi, y - yi, display);

        status |= draw_pixel(x - xi, y + yi, display);
        status |= draw_pixel(x - xi, y - yi, display);

        status |= draw_pixel(x + yi, y + xi, display);
        status |= draw_pixel(x - yi, y + xi, display);

        status |= draw_pixel(x + yi, y - xi, display);
        status |= draw_pixel(x - yi, y - xi, display);

        if(d >= 0){

            d += 2*xi -2*yi + 5;
            yi = yi - 1;
        }else{
                
            d +=2*xi + 3;
        }

        xi++;
    }

    return status;
}

OledStatus_t draw_line(int x0, int y0, int x1, int y1, DISPLAY* display){
    
    OledStatus_t status = OLED_OK;

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);

    int xstep = x0 < x1 ? 1 : -1;
    int ystep = y0 < y1 ? 1 : -1;

    int d = dx + dy;

    int x = x0;
    int y = y0;
    while(!(x == x1 && y == y1)){
        
        status |= draw_pixel(x, y, display);

        int d2 = 2*d;
        if(d2 >= dy){
            d += dy;
            x += xstep;
        }
        if(d2 <= dx){
            d += dx;
            y += ystep;
        }
    }
    status |= draw_pixel(x, y, display);

    return status;
}


OledStatus_t draw_letter(int x0, int y0, char letter, DISPLAY* display){

    if(letter > '~') return OLED_ERR_GEN;
    int8_t shift = y0 % 8 + 1;
    int8_t top_page, bot_page;

    top_page = y0 / 8;
    bot_page = top_page - 1;

    if(y0 < DISPLAY_HEIGHT && y0 >= 7 && (unsigned int)x0 <= DISPLAY_WIDTH - 5){

        for(int i = 0; i < 5; i++){

            *(display->buff + (DISPLAY_WIDTH*bot_page) + x0 + i) |= ascii[letter - OFFSET][i] << shift;
            *(display->buff + (DISPLAY_WIDTH*top_page) + x0 + i) |= ascii[letter - OFFSET][i] >> (8-shift);
        }
        return OLED_OK;
    }else{
        return OLED_ERR_OUT_OF_BOUND;
    }
}


OledStatus_t draw_bitmap(SECTOR* sector, const BITMAP bitmap, DISPLAY* display){

    if(sector->init != OLED_OK || display->init != OLED_OK) return OLED_ERR_INIT_FAILED;

    if(sector->height != bitmap.bitmap_height || sector->width != bitmap.bitmap_width) return OLED_ERR_BITMAP_NFIT;

    int8_t top_page = sector->y0 / 8;
    int8_t bot_page = sector->y1 / 8;
    if(top_page >= 8 || bot_page < 0) return OLED_ERR_OUT_OF_BOUND;

    int shift = 8 - ((sector->y0 + 1) % 8);
    if(shift == 8) shift = 0;
    
    int bitmap_index = 0;
    for(int x = sector->x0; x <= sector->x1; x++){

        int page = top_page;
        for(int i = 0; i < ((bitmap.bitmap_height + 7)/8); i++){

            *(display->buff + (page*DISPLAY_WIDTH) + x) |= (*(bitmap.bitmap + bitmap_index) >> shift);
            if(page > bot_page){
                *(display->buff + ((page-1)*DISPLAY_WIDTH) + x) |= (*(bitmap.bitmap + bitmap_index) << (8-shift));
            }
            page--;
            bitmap_index++;
        }
    }
    return OLED_OK;
}

OledStatus_t draw_text_with_init(int x0, int y0, char* string, size_t len, SECTOR* sector, DISPLAY* display){

    OledStatus_t status = OLED_OK;
    for(int i = 0; *(string+i) != '\0'; i++){

        status |= draw_letter(x0 + 6*i, y0, *(string+i), display);
        
    }
    
    init_sector_struct(sector, x0, y0, 8, len+(len*5)-1);
    status |= sector->init;
    return status;
}

OledStatus_t draw_text(char* string, int len, SECTOR* sector, DISPLAY* display){
    if(len+(len*5)-1 > sector->width) return OLED_ERR_OUT_OF_BOUND;

    OledStatus_t status = OLED_OK;
    for(int i = 0; *(string+i) != '\0'; i++){

        status |= draw_letter(sector->x0 + 6*i, sector->y0, *(string+i), display);
    }

    return status;
}

OledStatus_t draw_int_with_init(int x0, int y0, int integer, SECTOR* sector, DISPLAY* display){

    if(integer < 0) return OLED_ERR_NEGNUM;

    int n = 0;
    int tmp = integer;

    //prepares string for displaying
    do{
        n++;
        tmp /= 10;
    }while (tmp > 0);

    static char string[12];
    string[n] = '\0';

    for(int i = 0; i < n; i++){

        string[n - 1 - i] = integer % 10 + '0';
        integer /= 10;
    }

    return draw_text_with_init(x0, y0, string, n, sector, display);
}

OledStatus_t draw_int(int integer, SECTOR* sector, DISPLAY* display){

    if(integer < 0) return OLED_ERR_NEGNUM;
    int n = 0;
    int tmp = integer;

    //prepares string for displaying
    do{
        n++;
        tmp /= 10;
    }while (tmp > 0);

    static char string[12];
    string[n] = '\0';

    for(int i = 0; i < n; i++){

        string[n - 1 - i] = integer % 10 + '0';
        integer /= 10;
    }

    return draw_text(string, n, sector, display);
}

void clear_sector(SECTOR* sector, DISPLAY* display){
    
    int8_t page = sector->y0 / 8;
    int8_t last_page = sector->y1 / 8;

    int8_t top_shift = 8 - ((sector->y0 + 1) % 8); //sector.y0 % 8;
    if(top_shift == 8) top_shift = 0;
    int8_t bottom_shift = sector->y1 % 8;

    //clears top and bottom line of the sector
    for(int i = 0; i < sector->width; i++){

        *(display->buff + DISPLAY_WIDTH*(last_page) + sector->x0 + i) &= ~(0b11111111 << bottom_shift);
        *(display->buff + DISPLAY_WIDTH*page + sector->x0 + i) &= ~(0b11111111 >> top_shift);
    }

    //clears whats left
    uint8_t filler = 0;

    for(int i = page-1; i > last_page; i--){

        memset((display->buff+DISPLAY_WIDTH*i + sector->x0), filler, sector->width);
    }
}