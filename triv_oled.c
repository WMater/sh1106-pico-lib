#include "triv_oled.h"

void cmd(uint8_t cmd, DISPLAY* display){

    uint8_t data[2] = {SEND_COMMAND, cmd};
    i2c_write_blocking(display->i2c, display->addr, data, 2, false);
}

void write(uint8_t *buff, DISPLAY* display){

    uint8_t datacmd[DISPLAY_WIDTH + 1];
    datacmd[0] = SEND_DATA;
    
    for(int page = 0; page < 8; page++){
        
        memcpy(&datacmd[1], (buff + DISPLAY_WIDTH*page), DISPLAY_WIDTH);
        cmd(SET_PAGE + page, display);

        i2c_write_blocking(display->i2c, display->addr, datacmd, DISPLAY_WIDTH + 1, false);
    }
}

void write_sector(SECTOR sector, uint8_t* buff, DISPLAY* display){
    //updates selected sector only

    //setting helper array containing page data and data control byte
    uint8_t pagedata[sector.width + 1];
    pagedata[0] = SEND_DATA;
    
    int yk = sector.y0 - 1 + sector.height;
    if(yk > DISPLAY_HEIGHT - 1) yk = DISPLAY_HEIGHT - 1;
    
    int8_t page = sector.y0 / 8;
    int8_t last_page = yk / 8;

    //sending loop
    while(page <= last_page){
        
        //seting up sector starting column as writing start column
        cmd(SET_PAGE + page, display);
        cmd(SET_H_COL + (((sector.x0 + 2) >> 4) & 0b00001111), display);
        cmd(SET_L_COL + ((sector.x0 + 2) & 0b00001111), display);

        memcpy(&pagedata[1], (buff + DISPLAY_WIDTH*page + sector.x0), sector.width);

        i2c_write_blocking(display->i2c, display->addr, pagedata, sector.width+1, false);

        page++;
    }
        
    //back to initial starting line (line 2)
    cmd(SET_L_COL + 0b0010, display);
    cmd(SET_H_COL + 0b0000, display);
}

void clear(uint8_t *buff){

    uint8_t filler = 0x00;

    memset(buff, filler, DISPLAY_WIDTH * 8);
}

void init_display(uint8_t *buff, DISPLAY* display){

    //screen off
    cmd(SCREEN_OFF, display);

    //columns addresing policy
    cmd(SET_COL_W_REV, display);

    //starting column initialization
    cmd(SET_L_COL + 0b0010, display);//sh: 0b0010 ssd: 0b0000
    cmd(SET_H_COL + 0b0000, display);

    //clear screen
    clear(buff);
    write(buff, display);

    //screen on
    cmd(SCREEN_ON, display);
}


void draw_pixel(int x, int y, uint8_t *buff){
    
    int8_t page = y / 8;
    uint8_t pixel = 1 << (y % 8);

    if(y < DISPLAY_HEIGHT && y >= 0 && x >= 0 && x < DISPLAY_WIDTH){
        
        *(buff + DISPLAY_WIDTH*page + x) |= pixel;
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
    int8_t step_x = xk >= x0 ? 1 : -1; 
    int8_t step_y = yk >= y0 ? 1 : -1;

    int d;

    //helper pointers setup
    //in my approach to distinguising a main axis
    //data is stored in helers so i can mirror solution for x and y
    int *main_axis, *secondary_axis;

    int main_axis_d, secondary_axis_d;

    int8_t main_axis_step, secondary_axis_step;

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

void draw_letter(int x0, int y0, uint8_t* buff, char letter){

    int8_t shift = y0 % 8;

    int8_t top_page, bot_page;

    bot_page = y0 >> 3;
    top_page = bot_page + 1;

    if(bot_page >= 0 && top_page <= 7 && x0 >= 0 && x0 <= DISPLAY_WIDTH - 5){

        for(int i = 0; i < 5; i++){

            *(buff + DISPLAY_WIDTH*bot_page + x0 + i) |= ascii[letter - OFFSET][i] << shift;
            *(buff + DISPLAY_WIDTH*top_page + x0 + i) |= ascii[letter - OFFSET][i] >> 8-shift;
        }
    }
}

void draw_bitmap(SECTOR sector, const BITMAP bitmap, uint8_t* buff){
    //!bitmap is draw with up to page resolution
    //if not matched to sector it will overflow bottom boundries till next page
    //cursor is set to the bottom line tho to not break convention

    int yk = sector.y0 + sector.height - 1;
    if(yk > DISPLAY_HEIGHT - 1) yk = DISPLAY_HEIGHT - 1;

    int8_t last_page = yk / 8; //lower page
    int8_t first_page = sector.y0 / 8; // highter page

    int8_t shift = 7 - (yk % 8); //shift is different due to displaying from the top of the sector (bitshift is defferent then)

    int8_t page_offset = sector.height % 8 == 0 ? 0 : 1; //if bitmap was extended during procesing (e.g. 50px --> 56px) iterate one more page

    int bitmap_pages = ((bitmap.bitmap_height - 1) / 8) + 1; //how much pages bitmap needs to be iterated
    int sector_pages = last_page - first_page + page_offset; //how much pages iterate throught sector

    int incr_offset = sector_pages - bitmap_pages; //how much columns to skip if bitmap doesnt match sector, necessery for incrementor correction
    
    //bitmap iteratiom helpers
    int bitmap_index = 0;
    int bitmap_len = bitmap.bitmap_width * bitmap_pages;

    for(int i = 0; i < sector.width && bitmap_index < bitmap_len; i++){
        
        int page =  last_page; //draws from the top to avoid flipping binaries
        int sector_index = sector.x0 + DISPLAY_WIDTH*last_page; //buffor column select base

        while(page > first_page - page_offset && bitmap_index < bitmap_len){

            if(page < 8){

                *(buff + sector_index + i) |= (*(bitmap.bitmap + bitmap_index) >> shift) & 0b11111111;

                if(page > 0){

                    *(buff + sector_index - DISPLAY_WIDTH + i) |= (*(bitmap.bitmap + bitmap_index) << 8-shift) & 0b11111111;
                }

                //in case of bitmap being set to match pages one more iteration is needed due to bit shift being 0
                //TODO: consider breaking it into two possible scenarios and use memcpy if pages match
                if(shift == 0 && page == first_page + 1){

                    *(buff + sector_index - DISPLAY_WIDTH + i) |= (*(bitmap.bitmap + bitmap_index+1)) & 0b11111111;
                }
            }

            //index incrementation
            sector_index -= DISPLAY_WIDTH;
            bitmap_index++;
            page--;
        }
        
        //index corigation
        bitmap_index -= incr_offset;
    }
}

SECTOR draw_text(int x0, int y0, char* string, int len, uint8_t* buff){
    //returns sector to hold to if needed

    for(int i = 0; *(string+i) != '\0'; i++){

        draw_letter(x0 + 6*i, y0, buff, *(string+i));
        
    }
    
    SECTOR sector = {x0, y0, 8, len+(len*5)-1};
    return sector;
}

SECTOR draw_int(int x0, int y0, int integer, uint8_t* buff){

    int n = 0;
    int tmp = integer;

    //prepares string for displaying
    do{
        n++;
        tmp /= 10;
    }while (tmp > 0);

    char string[n+1];
    string[n] = '\0';

    for(int i = 0; i < n; i++){

        string[n - 1 - i] = integer % 10 + '0';
        integer /= 10;
    }

    return draw_text(x0, y0, string, n, buff);
}

void clear_sector(SECTOR sector, uint8_t* buff){

    int yk = sector.y0 - 1 + sector.height;
    if(yk > 63) yk = 63;
    
    int8_t page = sector.y0 / 8;
    int8_t last_page = yk / 8;

    int8_t top_shift = sector.y0 % 8;
    int8_t bottom_shift = 7- (yk % 8);

    //clears top and bottom line of the sector
    for(int i = 0; i < sector.width; i++){

    *(buff + DISPLAY_WIDTH*(last_page) + sector.x0 + i) &= ~((0b11111111 >> bottom_shift) & 0b11111111);
    *(buff + DISPLAY_WIDTH*page + sector.x0 + i) &= ~((0b11111111 << top_shift) & 0b11111111);
    }

    //clears whats left
    uint8_t filler = 0;

    for(int i = page+1; i < last_page; i++){

        memset((buff+DISPLAY_WIDTH*i + sector.x0), filler, sector.width);
    }
}