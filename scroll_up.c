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
	clear_vram_buffer();
	
	load_room();
	
	scroll_y = MAX_SCROLL; // start at MAX, downward to zero
	set_scroll_y(scroll_y);
	
	ppu_on_all(); // turn on screen
	

	while (1){
		// infinite loop
		ppu_wait_nmi(); // wait till beginning of the frame
		
		pad1 = pad_poll(0); // read the first controller
		
		clear_vram_buffer(); // do at the beginning of each frame
		
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
			clear_vram_buffer(); // do each frame, and before putting anything in the buffer
			
			address = get_ppu_addr(temp1, x, y);
			index = (y & 0xf0) + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			flush_vram_update_nmi();
			if (x == 0xe0) break;
		}
		if (y == 0xe0) break;
	}
	
	
	temp1 = temp1 ^ 2; // flip that 0000 0010 bit
	// a little bit in the other room
	set_data_pointer(Rooms[MAX_ROOMS-1]);
	for(x=0; ;x+=0x20){
		y = 0xe0;
		clear_vram_buffer(); // do each frame, and before putting anything in the buffer
		address = get_ppu_addr(temp1, x, y);
		index = (y & 0xf0) + (x >> 4);
		buffer_4_mt(address, index); // ppu_address, index to the data
		flush_vram_update_nmi();
		if (x == 0xe0) break;
	}
	clear_vram_buffer();
	
	//copy the room to the collision map
	memcpy (c_map, Rooms[MAX_ROOMS], 240);
}




void draw_sprites(void){
	// clear all sprites from sprite buffer
	oam_clear();

	// reset index into the sprite buffer
	sprid = 0;
	
	// draw 1 metasprite
	if(direction == LEFT) {
		sprid = oam_meta_spr(high_byte(BoxGuy1.x), high_byte(BoxGuy1.y), sprid, RoundSprL);
	}
	else{
		sprid = oam_meta_spr(high_byte(BoxGuy1.x), high_byte(BoxGuy1.y), sprid, RoundSprR);
	}
}
	

	
	
void movement(void){
	
// handle x

	old_x = BoxGuy1.x;
	
	if(pad1 & PAD_LEFT){
		direction = LEFT;
		if(BoxGuy1.x <= 0x100) {
			BoxGuy1.vel_x = 0;
			BoxGuy1.x = 0x100;
		}
		else if(BoxGuy1.x < 0x400) { // don't want to wrap around to the other side
			BoxGuy1.vel_x = -0x100;
		}
		else {
			BoxGuy1.vel_x = -SPEED;
		}
	}
	else if (pad1 & PAD_RIGHT){
		direction = RIGHT;
		if(BoxGuy1.x >= 0xf100) {
			BoxGuy1.vel_x = 0;
			BoxGuy1.x = 0xf100;
		}
		else if(BoxGuy1.x > 0xed00) { // don't want to wrap around to the other side
			BoxGuy1.vel_x = 0x100;
		}
		else {
			BoxGuy1.vel_x = SPEED;
		}
	}
	else { // nothing pressed
		BoxGuy1.vel_x = 0;
	}
	
	BoxGuy1.x += BoxGuy1.vel_x;
	
	if((BoxGuy1.x < 0x100)||(BoxGuy1.x > 0xf800)) { // make sure no wrap around to the other side
		BoxGuy1.x = 0x100;
	} 
	
	L_R_switch = 1; // shinks the y values in bg_coll, less problems with head / feet collisions
	
	Generic.x = high_byte(BoxGuy1.x); // this is much faster than passing a pointer to BoxGuy1
	Generic.y = high_byte(BoxGuy1.y);
	Generic.width = HERO_WIDTH;
	Generic.height = HERO_HEIGHT;
	bg_collision();
	if(collision_R && collision_L){ // if both true, probably half stuck in a wall
		BoxGuy1.x = old_x;
	}
	else if(collision_L) {
		high_byte(BoxGuy1.x) = high_byte(BoxGuy1.x) - eject_L;
		
	}
	else if(collision_R) {
		high_byte(BoxGuy1.x) = high_byte(BoxGuy1.x) - eject_R;
	} 


	
// handle y
	old_y = BoxGuy1.y; // didn't end up using the old value
	
	if(pad1 & PAD_UP){

			BoxGuy1.vel_y = -SPEED;

	}
	else if (pad1 & PAD_DOWN){
		if(BoxGuy1.y >= 0xe000) {
			BoxGuy1.vel_y = 0;
			BoxGuy1.y = 0xe000;
		}
		else if(BoxGuy1.y > 0xdc00) { // don't want to wrap around to the other side
			BoxGuy1.vel_y = 0x100;
		}
		else {
			BoxGuy1.vel_y = SPEED;
		}
	}
	else { // nothing pressed
		BoxGuy1.vel_y = 0;
	}
	
	BoxGuy1.y += BoxGuy1.vel_y;
	
	if((BoxGuy1.y < 0x100)||(BoxGuy1.y > 0xf000)) { // make sure no wrap around to the other side
		BoxGuy1.y = 0x100;
	} 
	
	L_R_switch = 0; // shinks the y values in bg_coll, less problems with head / feet collisions
	
	Generic.x = high_byte(BoxGuy1.x); // this is much faster than passing a pointer to BoxGuy1
	Generic.y = high_byte(BoxGuy1.y);
//	Generic.width = HERO_WIDTH;
//	Generic.height = HERO_HEIGHT;
	bg_collision();
	if(collision_U && collision_D){ // if both true, probably half stuck in a wall
		BoxGuy1.y = old_y;
	}
	else if(collision_U) {
		high_byte(BoxGuy1.y) = high_byte(BoxGuy1.y) - eject_U;
		
	}
	else if(collision_D) {
		high_byte(BoxGuy1.y) = high_byte(BoxGuy1.y) - eject_D;
	}
	
	
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




void bg_collision(void){
	// note, !0 = collision
	// sprite collision with backgrounds
	// load the object's x,y,width,height to Generic, then call this
	

	collision_L = 0;
	collision_R = 0;
	collision_U = 0;
	collision_D = 0;
	
	
	
	temp3 = Generic.y;
	if(L_R_switch) temp3 += 2; // fix bug, walking through walls
	
	if(temp3 >= 0xf0) return;
	
	temp5 = add_scroll_y(temp3, scroll_y); // upper left
	temp2 = temp5 >> 8; // high byte y
	temp3 = temp5 & 0xff; // low byte y

	temp1 = Generic.x; // x left
	
	eject_L = temp1 | 0xf0;
	eject_U = temp3 | 0xf0;
	
	bg_collision_sub();
	
	if(collision){ // find a corner in the collision map
		++collision_L;
		++collision_U;
	}
	
	temp1 += Generic.width; // x right
	
	eject_R = (temp1 + 1) & 0x0f;
	
	// temp2,temp3 is unchanged
	bg_collision_sub();
	
	if(collision){ // find a corner in the collision map
		++collision_R;
		++collision_U;
	}
	
	
	// again, lower
	
	// bottom right, x hasn't changed

	temp3 = Generic.y + Generic.height; // y bottom
	if(L_R_switch) temp3 -= 2; // fix bug, walking through walls
	temp5 = add_scroll_y(temp3, scroll_y); // upper left
	temp2 = temp5 >> 8; // high byte y
	temp3 = temp5 & 0xff; // low byte y
	
	eject_D = (temp3 + 1) & 0x0f;
	if(temp3 >= 0xf0) return;
	
	bg_collision_sub();
	
	if(collision){ // find a corner in the collision map
		++collision_R;
		++collision_D;
	}
	
	// bottom left
	temp1 = Generic.x; // x left
	
	//temp2,temp3 is unchanged

	bg_collision_sub();
	
	if(collision){ // find a corner in the collision map
		++collision_L;
		++collision_D;
	}
}



void bg_collision_sub(void){
	coordinates = (temp1 >> 4) + (temp3 & 0xf0); // upper left

	map = temp2&1; // high byte
	if(!map){
		collision = c_map[coordinates];
	}
	else{
		collision = c_map2[coordinates];
	}
}



void draw_screen_U(void){
	pseudo_scroll_y = sub_scroll_y(0x20,scroll_y);
	
	temp1 = pseudo_scroll_y >> 8;
	
	set_data_pointer(Rooms[temp1]);
	nt = (temp1 & 1) << 1; // 0 or 2
	y = pseudo_scroll_y & 0xff;
	
	// important that the main loop clears the vram_buffer
	
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