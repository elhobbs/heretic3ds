
#ifdef _3DS
#include <3ds.h>
#include <3ds/types.h>
#include "gl.h"

#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include "DoomDef.h"
#include "R_local.h"
#include "map.h"
#include "keyboard.h"

touchPosition	g_lastTouch = { 0, 0 };
touchPosition	g_currentTouch = { 0, 0 };

//0=DS Bit,1=game key, 2=menu key
u32 keys3ds[32][3] = {
	{ KEY_A,		KEY_RCTRL,		KEY_ENTER }, //bit 00
	{ KEY_B,		' ',			KEY_ESCAPE }, //bit 01
	{ KEY_SELECT,	KEY_ENTER,		0 }, //bit 02
	{ KEY_START,	KEY_ESCAPE,	0 }, //bit 03
	{ KEY_DRIGHT,	KEY_RIGHTARROW,0 }, //bit 04
	{ KEY_DLEFT,	KEY_LEFTARROW, 0 }, //bit 05
	{ KEY_DUP,		KEY_UPARROW,	0 }, //bit 06
	{ KEY_DDOWN,	KEY_DOWNARROW, 0 }, //bit 07
	{ KEY_R,		'.',			0 }, //bit 08
	{ KEY_L,		',',			0 }, //bit 09
	{ KEY_X,		'x',			0 }, //bit 10
	{ KEY_Y,		'y',			'y' }, //bit 11
	{ 0, 0, 0 }, //bit 12
	{ 0, 0, 0 }, //bit 13
	{ KEY_ZL,		'[',			0 }, //bit 14
	{ KEY_ZR,		']',			0 }, //bit 15
	{ 0, 0, 0 }, //bit 16
	{ 0, 0, 0 }, //bit 17
	{ 0, 0, 0 }, //bit 18
	{ 0, 0, 0 }, //bit 19
	{ 0, 0, 0 }, //bit 20
	{ 0, 0, 0 }, //bit 21
	{ 0, 0, 0 }, //bit 22
	{ 0, 0, 0 }, //bit 23
#if 0
	{ KEY_CSTICK_RIGHT,	KEYD_CSTICK_RIGHT,	0 }, //bit 24
	{ KEY_CSTICK_LEFT,	KEYD_CSTICK_LEFT,	0 }, //bit 25
	{ KEY_CSTICK_UP,	KEYD_CSTICK_UP,		0 }, //bit 26
	{ KEY_CSTICK_DOWN,	KEYD_CSTICK_DOWN,	0 }, //bit 27
	{ KEY_CPAD_RIGHT,	KEYD_CPAD_RIGHT,	0 }, //bit 28
	{ KEY_CPAD_LEFT,	KEYD_CPAD_LEFT,		0 }, //bit 29
	{ KEY_CPAD_UP,		KEYD_CPAD_UP,		0 }, //bit 30
	{ KEY_CPAD_DOWN,	KEYD_CPAD_DOWN,		0 }, //bit 31
#endif
};

extern boolean setup_select; // changing an item

void _3ds_controls(void) {
	touchPosition touch;
	circlePosition nubPos = { 0, 0 };
	circlePosition cstickPos = { 0, 0 };
	int dx, dy;
	event_t ev;

	scanKeys();	// Do DS input housekeeping
	u32 keys = keysDown();
	u32 held = keysHeld();
	u32 up = keysUp();
	int i;
	boolean altmode = false;// menuactive && !setup_select;

	if (held & KEY_TOUCH) {
		touchRead(&touch);
	}

	for (i = 0; i<24; i++) {
		//send key down
		if (keys3ds[i][0] & keys) {
			event_t ev;
			ev.type = ev_keydown;
			ev.data1 = keys3ds[i][(altmode && keys3ds[i][2]) ? 2 : 1];
			//printf("key down: %d\n", ev.data1);
			D_PostEvent(&ev);
		}
		//send key up
		if (keys3ds[i][0] & up) {
			event_t ev;
			ev.type = ev_keyup;
			ev.data1 = keys3ds[i][(altmode && keys3ds[i][2]) ? 2 : 1];
			//printf("key up: %d\n", ev.data1);
			D_PostEvent(&ev);
		}
	}

	if (keysDown() & KEY_TOUCH)
	{
		touchRead(&g_lastTouch);
		g_lastTouch.px <<= 7;
		g_lastTouch.py <<= 7;
	}

	if (keysHeld() & KEY_TOUCH) // this is only for x axis
	{

		touchRead(&g_currentTouch);// = touchReadXY();
								   // let's use some fixed point magic to improve touch smoothing accuracy
		g_currentTouch.px <<= 7;
		g_currentTouch.py <<= 7;

		dx = (g_currentTouch.px - g_lastTouch.px) >> 6;
		dy = (g_currentTouch.py - g_lastTouch.py) >> 6;

		ev.type = ev_mouse;
		ev.data1 = 0;
		ev.data2 = (dx << 3) * (mouseSensitivity + 5) / 10;
		ev.data3 = (dy >> 1) * (mouseSensitivity + 5) / 10;
		D_PostEvent(&ev);

		g_lastTouch.px = (g_lastTouch.px + g_currentTouch.px) / 2;
		g_lastTouch.py = (g_lastTouch.py + g_currentTouch.py) / 2;
	}

	irrstCstickRead(&nubPos);
	if (abs(nubPos.dx) > 20 || abs(nubPos.dy) > 20) {
		dx = 0;
		dy = 0;
		if (abs(nubPos.dx) > 20) {
			dx = (nubPos.dx) * (nubSensitivity + 5) / 10;
		}
		if (abs(nubPos.dy) > 20) {
			dy = -(nubPos.dy) * (nubSensitivity + 5) / 10;
		}

		ev.type = ev_nub;
		ev.data1 = 0;
		ev.data2 = dx;
		ev.data3 = dy;
		D_PostEvent(&ev);
	}

	circleRead(&cstickPos);
	if (abs(cstickPos.dx) > 20 || abs(cstickPos.dy) > 20) {
		dx = 0;
		dy = 0;
		if (abs(cstickPos.dx) > 20) {
			dx = (cstickPos.dx >> 2) * (cstickSensitivity + 5) / 10;
		}
		if (abs(cstickPos.dy) > 20) {
			dy = (cstickPos.dy >> 2) * (cstickSensitivity + 5) / 10;
		}

		ev.type = ev_cstick;
		ev.data1 = 0;
		ev.data2 = dx;
		ev.data3 = dy;
		D_PostEvent(&ev);
	}

	keyboard_input();
}
#endif
