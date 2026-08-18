#include "triv_oled.h"

//struct inits
void init_display_struct(DISPLAY* display, i2c_inst_t* i2c, uint8_t addr, uint8_t* buff){

    display->i2c = i2c;
    display->addr = addr;
    display->buff = buff;

    init_display(display);
}

void init_sector_struct(SECTOR* sector, int x0, int y0, int height, int width){
/*
    if(x0 < 0 || x0 >= DISPLAY_WIDTH - 4 || y0 < 0 || y0 >= DISPLAY_HEIGHT) return;
    sector->x0 = x0;
    sector->y0 = y0;
    sector->height = height;
    sector->width = width;
    
    int x1 = x0 + width - 1;
    int y1 = y0 - height + 1;
    if(x1 < 0 || x1 >= DISPLAY_WIDTH - 4 || y1 < 0 || y1 >= DISPLAY_HEIGHT) return;
    sector->x1 = x1;
    sector->y1 = y1;

    printf("x0: %d, y0: %d, x1: %d, y1: %d", sector->x0, sector->y0, sector->x1, sector->y1);
*/}

//operations
void cmd(uint8_t cmd, DISPLAY* display){

    uint8_t data[2] = {SEND_COMMAND, cmd};
    i2c_write_blocking(display->i2c, display->addr, data, 2, false);
}

void write_screen(DISPLAY* display){

    uint8_t datacmd[DISPLAY_WIDTH + 1];
    datacmd[0] = SEND_DATA;
    
    for(int page = 0; page < 8; page++){
        
        memcpy(&datacmd[1], (display->buff + DISPLAY_WIDTH*page), DISPLAY_WIDTH);
        cmd(SET_PAGE + page, display);

        i2c_write_blocking(display->i2c, display->addr, datacmd, DISPLAY_WIDTH + 1, false);
    }
}

void write_sector(SECTOR sector, DISPLAY* display){
    //updates selected sector only

    //setting helper array containing page data and data control byte
    uint8_t pagedata[DISPLAY_WIDTH];
    pagedata[0] = SEND_DATA;
    
    int8_t page = sector.y0 / 8;
    int8_t last_page = sector.y0 / 8;

    //sending loop
    while(page <= last_page){
        
        //seting up sector starting column as writing start column
        cmd(SET_PAGE + page, display);
        cmd(SET_H_COL + ((sector.x0 + 2) & 0b11110000), display);
        cmd(SET_L_COL + ((sector.x0 + 2) & 0b00001111), display);

        memcpy(&pagedata[1], (display->buff + DISPLAY_WIDTH*page + sector.x0), sector.width);

        i2c_write_blocking(display->i2c, display->addr, pagedata, sector.width+1, false);

        page++;
    }
        
    //back to initial starting line (line 2)
    cmd(SET_L_COL + 0b0010, display);
    cmd(SET_H_COL + 0b0000, display);
}

void clear_buff(DISPLAY* display){

    uint8_t filler = 0x00;

    memset(display->buff, filler, DISPLAY_WIDTH * 8);
}

void init_display(DISPLAY* display){

    //screen off
    cmd(SCREEN_OFF, display);

    //columns addresing policy
    cmd(SET_COL_W_REV, display);

    //starting column initialization
    cmd(SET_L_COL + 0b0010, display);//sh: 0b0010 ssd: 0b0000
    cmd(SET_H_COL + 0b0000, display);

    //clear screen
    clear_buff(display);
    write_screen(display);

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


void draw_circle(int x, int y, int radius, uint8_t *buff){ //midpoint for circle
    
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

void draw_line(int x0, int y0, int x1, int y1, uint8_t *buff){
    if((unsigned int)x0 >= DISPLAY_WIDTH || (unsigned int)y0 >= DISPLAY_HEIGHT 
    || (unsigned int)x1 >= DISPLAY_WIDTH || (unsigned int)y1 >= DISPLAY_HEIGHT) return;

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);

    int xstep = x0 < x1 ? 1 : -1;
    int ystep = y0 < y1 ? 1 : -1;

    int d = dx + dy;

    int x = x0;
    int y = y0;
    while(!(x == x1 && y == y1)){
        
        draw_pixel(x, y, buff);

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
    draw_pixel(x, y, buff);
}


void draw_letter(int x0, int y0, uint8_t* buff, char letter){

    int8_t shift = y0 % 8;

    int8_t top_page, bot_page;

    top_page = y0 / 8;
    bot_page = top_page - 1;

    if(bot_page >= 0 && top_page <= 7 && x0 >= 0 && x0 <= DISPLAY_WIDTH - 5){

        for(int i = 0; i < 5; i++){

            *(buff + (DISPLAY_WIDTH*bot_page) + x0 + i) |= ascii[letter - OFFSET][i] << shift;
            *(buff + (DISPLAY_WIDTH*top_page) + x0 + i) |= ascii[letter - OFFSET][i] >> (8-shift);
        }
    }
}

/*
void draw_bitmap(SECTOR sector, const BITMAP bitmap, uint8_t *buff){

    if(sector.height != bitmap.bitmap_height || sector.width != bitmap.bitmap_width) return;

    uint8_t top_page = sector.y0 / 8;
    uint8_t bot_page = (sector.y0 - sector.height + 1) / 8;
    if(top_page >= 8 || bot_page < 0) return;

    int shift = 8 - ((sector.y0 + 1) % 8);
    if(shift == 8) shift = 0;
    printf("shift: %d\n", shift);

    int total_height;
    int diff = bitmap.bitmap_height % 8;
    if(diff == 0){
        total_height = bitmap.bitmap_height;
    }else{
        total_height = bitmap.bitmap_height + (8-diff);
    }
    
    int bitmap_length = (total_height * bitmap.bitmap_width) / 8; 
    printf("tot: %d\n", bitmap_length);
    
    int page = top_page;
    int x = sector.x0;
    for(int i = 0; i < bitmap_length; i++){

        *(buff + (page*DISPLAY_WIDTH) + x) |= (*(bitmap.bitmap + i) >> shift);
        *(buff + ((page-1)*DISPLAY_WIDTH) + x) |= (*(bitmap.bitmap + i) << (8-shift));
        printf("page: %d\n", page);
        printf("x: %d\n", x);
        if(page == bot_page){
            x++;
            page = top_page;
        }else{
            
            page--;
        }
    }
}*/

void draw_bitmap(SECTOR sector, const BITMAP bitmap, uint8_t *buff){

    if(sector.height != bitmap.bitmap_height || sector.width != bitmap.bitmap_width) return;

    uint8_t top_page = sector.y0 / 8;
    uint8_t bot_page = (sector.y0 - sector.height + 1) / 8;
    if(top_page >= 8 || bot_page < 0) return;

    int shift = 8 - ((sector.y0 + 1) % 8);
    if(shift == 8) shift = 0;
    printf("shift: %d\n", shift);

    int total_height;
    int diff = bitmap.bitmap_height % 8;
    if(diff == 0){
        total_height = bitmap.bitmap_height;
    }else{
        total_height = bitmap.bitmap_height + (8-diff);
    }
    
    int bitmap_length = (total_height * bitmap.bitmap_width) / 8; 
    printf("tot: %d\n", bitmap_length);
    
    int bitmap_index = 0;
    for(int x = sector.x0; x <= (sector.x0 -1 + sector.width); x++){

        int page = top_page;
        for(int i = 0; i < (total_height/8); i++){

            *(buff + (page*DISPLAY_WIDTH) + x) |= (*(bitmap.bitmap + bitmap_index) >> shift);
            if(page > bot_page){
                *(buff + ((page-1)*DISPLAY_WIDTH) + x) |= (*(bitmap.bitmap + bitmap_index) << (8-shift));
            }
            page--;
            bitmap_index++;
        }
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