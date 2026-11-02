#pragma once

#define BUTTON_DOWN 1

typedef struct kbutton_s
{
	// key nums holding it down
	int down[2]; // key nums holding it down
	// low bit is down state
	int state;	 // low bit is down state
} kbutton_t;
