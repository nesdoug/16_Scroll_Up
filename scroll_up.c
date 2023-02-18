/*	example code for cc65, for NES
 *  Scroll Up with metatile system
 *	using neslib
 *	Doug Fraker 2018
 */	
 

#include "LIB/neslib.h"
#include "LIB/nesdoug.h"
#include "Sprites.h" // holds our metasprite data
#include "scroll_up.h"



	
	
void main (void) {
	
	ppu_off(); // screen off
	
	// load the palettes
	pal_bg(palette_bg);
	pal_spr(palette_sp);
	
	// use the second set of tiles for sprites
	// both bg and sprites are set to 0 by default
	bank_spr(1);
	
	set_vram_buffer(); // do at least once, sets a pointer to a buffer
	
	load_room();
	
	scroll_y = MAX_SCROLL; // start at MAX, downward to zero
	set_scroll_y(scroll_y);
	
	ppu_on_all(); // turn on screen
	

	while (1){
		// infinite loop
		ppu_wait_nmi(); // wait till beginning of the frame
		
		pad1 = pad_poll(0); // read the first controller
		
		movement();
		// set scroll
		set_scroll_x(scroll_x);
		set_scroll_y(scroll_y);
		draw_screen_U();
		draw_sprites();
	}
}




void load_room(void){
	set_data_pointer(Rooms[MAX_ROOMS]);
	set_mt_pointer(metatiles1);
	temp1 = (MAX_SCROLL >> 8) + 1;
	temp1 = (temp1 & 1) << 1;
	for(y=0; ;y+=0x20){
		for(x=0; ;x+=0x20){
			address = get_ppu_addr(temp1, x, y);
			index = (y & 0xf0) + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			flush_vram_update2();
			if (x == 0xe0) break;
		}
		if (y == 0xe0) break;
	}
	
	
	temp1 = temp1 ^ 2; // flip that 0000 0010 bit
	// a little bit in the other room
	set_data_pointer(Rooms[MAX_ROOMS-1]);
	for(x=0; ;x+=0x20){
		y = 0xe0;
		address = get_ppu_addr(temp1, x, y);
		index = (y & 0xf0) + (x >> 4);
		buffer_4_mt(address, index); // ppu_address, index to the data
		flush_vram_update2();
		if (x == 0xe0) break;
	}
	
	//copy the room to the collision map
	memcpy (c_map, Rooms[MAX_ROOMS], 240);
}




void draw_sprites(void){
	// clear all sprites from sprite buffer
	oam_clear();
	
	temp_x = BoxGuy1.x >> 8;
	temp_y = BoxGuy1.y >> 8;
	if(temp_x == 0) temp_x = 1;
	if(temp_y == 0) temp_y = 1;
	
	// draw 1 metasprite
	if(direction == LEFT) {
		oam_meta_spr(temp_x, temp_y, RoundSprL);
	}
	else{
		oam_meta_spr(temp_x, temp_y, RoundSprR);
	}
}
	

	
	
void movement(void){
	// handle x
	old_x = BoxGuy1.x;
	
	if(pad1 & PAD_LEFT){
		direction = LEFT;
		BoxGuy1.vel_x = -SPEED;
	}
	else if (pad1 & PAD_RIGHT){
		direction = RIGHT;
		BoxGuy1.vel_x = SPEED;
	}
	else { // nothing pressed
		BoxGuy1.vel_x = 0;
	}
	
	BoxGuy1.x += BoxGuy1.vel_x;
	
	if(BoxGuy1.x > 0xf100) { // too far, don't wrap around
        
        if(old_x >= 0x8000){
            BoxGuy1.x = 0xf100; // max right
        }
        else{
            BoxGuy1.x = 0x0000; // max left
        }
        
	} 
	
	
	Generic.x = BoxGuy1.x >> 8; // the collision routine needs an 8 bit value
	Generic.y = BoxGuy1.y >> 8;
	Generic.width = HERO_WIDTH;
	Generic.height = HERO_HEIGHT;
	
	if(BoxGuy1.vel_x < 0){ // going left
		if(bg_coll_L() ){ // check collision left
            high_byte(BoxGuy1.x) = high_byte(BoxGuy1.x) - eject_L;
            
        }
	}
	else if(BoxGuy1.vel_x > 0){ // going right
		if(bg_coll_R() ){ // check collision right
            high_byte(BoxGuy1.x) = high_byte(BoxGuy1.x) - eject_R;
            
        }
	}
	// else 0, skip it
	
	
	
	// handle y
	old_y = BoxGuy1.y;

	if(pad1 & PAD_UP){
		BoxGuy1.vel_y = -SPEED;
	}
	else if (pad1 & PAD_DOWN){
		BoxGuy1.vel_y = SPEED;
	}
	else { // nothing pressed
		BoxGuy1.vel_y = 0;
	}

	BoxGuy1.y += BoxGuy1.vel_y;
	
	if(BoxGuy1.y > 0xe100) { // too far, don't wrap around
        
        if(old_y >= 0x8000){
            BoxGuy1.y = 0xe100; // max down
        }
        else{
            BoxGuy1.y = 0x0000; // max up
        }
        
	} 
	
	Generic.x = BoxGuy1.x >> 8; // the collision routine needs an 8 bit value
	Generic.y = BoxGuy1.y >> 8;
//	Generic.width = HERO_WIDTH; // already is this
//	Generic.height = HERO_HEIGHT;
	
	if(BoxGuy1.vel_y < 0){ // going up
		if(bg_coll_U() ){ // check collision left
            high_byte(BoxGuy1.y) = high_byte(BoxGuy1.y) - eject_U;
            
        }
	}
	else if(BoxGuy1.vel_y > 0){ // going down
		if(bg_coll_D() ){ // check collision right
            high_byte(BoxGuy1.y) = high_byte(BoxGuy1.y) - eject_D;
            
        }
	}
	// else 0, skip it
	
	
	
	// scroll
	temp5 = BoxGuy1.y;
	if (BoxGuy1.y < MAX_UP){
		temp1 = (MAX_UP - BoxGuy1.y + 0x80) >> 8; // the numbers work better with +80 (like 0.5)
		scroll_y = sub_scroll_y(temp1, scroll_y);
		BoxGuy1.y = BoxGuy1.y + (temp1 << 8);
	}
	
	
	if((high_byte(scroll_y) >= 0x80) || (scroll_y <= MIN_SCROLL)) { // 0x80 = negative
		scroll_y = MIN_SCROLL; // stop scrolling up, end of level
		BoxGuy1.y = temp5;
		if(high_byte(BoxGuy1.y) < 2) BoxGuy1.y = 0x0200;
		if(high_byte(BoxGuy1.y) > 0xf0) BoxGuy1.y = 0x0200; // > 0xf0 wrapped to bottom
	}
	

	// do we need to load a new collision map? (scrolled into a new room)
	if((scroll_y & 0xff) >= 0xec){
		new_cmap(); //
	}
}	




char bg_coll_L(void){
    // check 2 points on the left side
    temp_x = Generic.x;
    
    eject_L = temp_x | 0xf0;
	temp1 = Generic.y + 2;
	temp5 = add_scroll_y(temp1, scroll_y);
	temp_y = (char)temp5; // low byte
	temp_room = temp5 >> 8; // high byte
    if(bg_collision_sub() ) return 1;
    
    temp1 = Generic.y + Generic.height;
    temp1 -= 2;
	temp5 = add_scroll_y(temp1, scroll_y);
	temp_y = (char)temp5; // low byte
	temp_room = temp5 >> 8; // high byte
    if(bg_collision_sub() ) return 1;
    
    return 0;
}

char bg_coll_R(void){
    // check 2 points on the right side
	temp_x = Generic.x + Generic.width;
    
    eject_R = (temp_x + 1) & 0x0f;
	temp1 = Generic.y + 2;
	temp5 = add_scroll_y(temp1, scroll_y);
	temp_y = (char)temp5; // low byte
	temp_room = temp5 >> 8; // high byte
    if(bg_collision_sub() ) return 1;
    
    temp1 = Generic.y + Generic.height;
    temp1 -= 2;
	temp5 = add_scroll_y(temp1, scroll_y);
	temp_y = (char)temp5; // low byte
	temp_room = temp5 >> 8; // high byte
    if(bg_collision_sub() ) return 1;
    
    return 0;
}

char bg_coll_U(void){
    // check 2 points on the top side
    temp_x = Generic.x;
    
	temp1 = Generic.y;
	temp5 = add_scroll_y(temp1, scroll_y); 
    temp_y = (char)temp5; // low byte
	temp_room = temp5 >> 8; // high byte
    eject_U = temp_y | 0xf0;
    if(bg_collision_sub() ) return 1;
    
    temp_x = Generic.x + Generic.width;
	temp_x -= 2;
    
    if(bg_collision_sub() ) return 1;
    
    return 0;
}

char bg_coll_D(void){
    // check 2 points on the bottom side
	temp_x = Generic.x;
    
	temp1 = Generic.y + Generic.height;
	temp5 = add_scroll_y(temp1, scroll_y);
    temp_y = (char)temp5; // low byte
	temp_room = temp5 >> 8; // high byte
    eject_D = (temp_y + 1) & 0x0f;
    if(bg_collision_sub() ) return 1;
    
    temp_x = Generic.x + Generic.width;
	temp_x -= 2;
    
    if(bg_collision_sub() ) return 1;
    
    return 0;
}



char bg_collision_sub(void){
    if(temp_y >= 0xf0) return 0;
    
	coordinates = (temp_x >> 4) + (temp_y & 0xf0);
    // we just need 4 bits each from x and y
	
	map = temp_room&1; // high byte
	if(!map){
		collision = c_map[coordinates];
	}
	else{
		collision = c_map2[coordinates];
	}
	
    return collision;
}


void draw_screen_U(void){
	pseudo_scroll_y = sub_scroll_y(0x20,scroll_y);
	
	temp1 = pseudo_scroll_y >> 8;
	
	set_data_pointer(Rooms[temp1]);
	nt = (temp1 & 1) << 1; // 0 or 2
	y = pseudo_scroll_y & 0xff;
	
	switch(scroll_count){
		case 0:
			address = get_ppu_addr(nt, 0, y);
			index = (y & 0xf0) + 0;
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, 0x20, y);
			index = (y & 0xf0) + 2;
			buffer_4_mt(address, index); // ppu_address, index to the data
			break;
			
		case 1:
			address = get_ppu_addr(nt, 0x40, y);
			index = (y & 0xf0) + 4;
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, 0x60, y);
			index = (y & 0xf0) + 6;
			buffer_4_mt(address, index); // ppu_address, index to the data
			break;
			
		case 2:
			address = get_ppu_addr(nt, 0x80, y);
			index = (y & 0xf0) + 8;
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, 0xa0, y);
			index = (y & 0xf0) + 10;
			buffer_4_mt(address, index); // ppu_address, index to the data
			break;
			
		default:
			address = get_ppu_addr(nt, 0xc0, y);
			index = (y & 0xf0) + 12;
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, 0xe0, y);
			index = (y & 0xf0) + 14;
			buffer_4_mt(address, index); // ppu_address, index to the data
	}

	
	
	++scroll_count;
	scroll_count &= 3; //mask off top bits, keep it 0-3
}



// copy a new collision map to one of the 2 c_map arrays
void new_cmap(void){
	// copy a new collision map to one of the 2 c_map arrays
	room = ((scroll_y >> 8)); //high byte = room, one to the right
	
	map = room & 1; //even or odd?
	if(!map){
		memcpy (c_map, Rooms[room], 240);
	}
	else{
		memcpy (c_map2, Rooms[room], 240);
	}
}

