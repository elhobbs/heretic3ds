#include "glwindow.h"
#include <stdio.h>
#include <math.h>

#include "r_local.h"

CGLWindow gl;
double frame_time = 0.0;
typedef bool	qboolean;

extern "C" {
HWND openGL;
double Sys_FloatTime (void);
void Key_Event (int key, qboolean down);
//#include "keys.h"
};


#include <stdio.h>

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

//
// these are the key numbers that should be passed to Key_Event
//
#define K_CAPS			1
#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32


// normal keys should be passed as lowercased ascii

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131

#define	K_ALT			132
#define	K_CTRL			133
#define	K_SHIFT			134
#define	K_F1			135
#define	K_F2			136
#define	K_F3			137
#define	K_F4			138
#define	K_F5			139
#define	K_F6			140
#define	K_F7			141
#define	K_F8			142
#define	K_F9			143
#define	K_F10			144
#define	K_F11			145
#define	K_F12			146
#define	K_INS			147
#define	K_DEL			148
#define	K_PGDN			149
#define	K_PGUP			150
#define	K_HOME			151
#define	K_END			152

//
// mouse buttons generate virtual keys
//
#define	K_MOUSE1		200
#define	K_MOUSE2		201
#define	K_MOUSE3		202

// JACK: Intellimouse(c) Mouse Wheel Support

#define K_MWHEELUP		239
#define K_MWHEELDOWN	240

#define K_NDS_START		241
#define K_NDS_SELECT	242
#define K_NDS_A			243
#define K_NDS_B			244
#define K_NDS_X			245
#define K_NDS_Y			246
#define K_NDS_L			247
#define K_NDS_R			248
#define K_NDS_LEFT		249
#define K_NDS_RIGHT		250
#define K_NDS_UP		251
#define K_NDS_DOWN		252
#define K_NDS_F1		253
#define K_NDS_TAP		254

#define K_PAUSE			255

byte        scantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME, 
	K_UPARROW,K_PGUP,'-',K_LEFTARROW,'5',K_RIGHTARROW,'+',K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
}; 
/*
=======
MapKey

Map from windows to quake keynums
=======
*/
int MapKey (int key)
{
	key = (key>>16)&255;
	if (key > 127)
		return 0;

	return scantokey[key];
}
/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	static int mouse_oldbuttonstate = 0;
	int	i;
	int mouse_buttons = 3;

	//if (mouseactive)
	//{
	// perform button actions
		for (i=0 ; i<mouse_buttons ; i++)
		{
			if ( (mstate & (1<<i)) &&
				!(mouse_oldbuttonstate & (1<<i)) )
			{
				Key_Event (K_MOUSE1 + i, true);
			}

			if ( !(mstate & (1<<i)) &&
				(mouse_oldbuttonstate & (1<<i)) )
			{
				Key_Event (K_MOUSE1 + i, false);
			}
		}	
			
		mouse_oldbuttonstate = mstate;
	//}
}

static int MapH2Key(int winKey)
{
#define	KEY_RIGHTARROW		0xae
#define	KEY_LEFTARROW		0xac
#define	KEY_UPARROW			0xad
#define	KEY_DOWNARROW		0xaf
#define	KEY_ESCAPE			27
#define	KEY_ENTER			13
#define	KEY_F1				(0x80+0x3b)
#define	KEY_F2				(0x80+0x3c)
#define	KEY_F3				(0x80+0x3d)
#define	KEY_F4				(0x80+0x3e)
#define	KEY_F5				(0x80+0x3f)
#define	KEY_F6				(0x80+0x40)
#define	KEY_F7				(0x80+0x41)
#define	KEY_F8				(0x80+0x42)
#define	KEY_F9				(0x80+0x43)
#define	KEY_F10				(0x80+0x44)
#define	KEY_F11				(0x80+0x57)
#define	KEY_F12				(0x80+0x58)
#define KEY_TAB				9
#define	KEY_RSHIFT			(0x80+0x36)
#define	KEY_RCTRL			(0x80+0x1d)
#define	KEY_RALT			(0x80+0x38)

	if (winKey == VK_UP)
	{
		return KEY_UPARROW;
	}
	if (winKey == VK_DOWN)
	{
		return KEY_DOWNARROW;
	}
	if (winKey == VK_LEFT)
	{
		return KEY_LEFTARROW;
	}
	if (winKey == VK_RIGHT)
	{
		return KEY_RIGHTARROW;
	}
	if (winKey == VK_RETURN)
	{
		return KEY_ENTER;
	}
	if (winKey == VK_ESCAPE)
	{
		return KEY_ESCAPE;
	}
	if (winKey == VK_F11)
	{
		return KEY_F11;
	}
	if (winKey == VK_TAB)
	{
		return KEY_TAB;
	}
	if (winKey == VK_RCONTROL || winKey == VK_CONTROL)
	{
		return KEY_RCTRL;
	}
	if (winKey == VK_RSHIFT || winKey == VK_SHIFT)
	{
		return KEY_RSHIFT;
	}
	if (winKey == VK_OEM_2)
	{
		return '/';
	}
	if (winKey == 77) {
		return 'm';
	}
	if (winKey == 66) {
		return 'b';
	}
	if (winKey == 76)
	{
		return 'l';
	}
	if (winKey == 221)
	{
		return ']';
	}
	if (winKey == 89)
	{
		return 'y';
	}

	return tolower(winKey);
}

extern "C" {
void ibm_handlewinkey(int key, int up);
//extern int draw_windings;
};
LRESULT CALLBACK WndProcGL(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
	case WM_SIZE:
		{
			RECT rc;
			GetWindowRect(hwnd,&rc);
			gl.Size(rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top);
		}
		break;
	case WM_CLOSE:
		gl.ShutDown();
		break;
	case WM_DESTROY:
         PostQuitMessage(0);
         return 0;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
			/*if(wParam == 'W')
			{
				draw_windings = !draw_windings;
			}*/
			ibm_handlewinkey(MapH2Key(wParam), 0);
		//Key_Event (MapKey(lParam), true);
		break;
		
	case WM_KEYUP:
	case WM_SYSKEYUP:
			ibm_handlewinkey(MapH2Key(wParam), 1);
		//Key_Event (MapKey(lParam), false);
		break;
	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			{
			int temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			IN_MouseEvent (temp);
			}
		return 0;
	default:
		break;
	}
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

extern "C" {
int initDisplay()
{
	WNDCLASSEX wnd;
	RECT rc;
/*
	DEVMODE dmScreenSettings;					// Device Mode
	memset(&dmScreenSettings,0,sizeof(dmScreenSettings));		// Makes Sure Memory's Cleared
	dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
	dmScreenSettings.dmPelsWidth	= DS_SCREEN_WIDTH;			// Selected Screen Width
	dmScreenSettings.dmPelsHeight	= DS_SCREEN_HEIGHT;			// Selected Screen Height
	dmScreenSettings.dmBitsPerPel	= 32;				// Selected Bits Per Pixel
	dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

	long val = ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN);
*/
	wnd.cbSize        = sizeof(wnd);
    wnd.style         = CS_HREDRAW | CS_VREDRAW;
    wnd.lpfnWndProc   = WndProcGL;
    wnd.cbClsExtra    = 0;
    wnd.cbWndExtra    = 0;
    wnd.hInstance     = (HINSTANCE)GetModuleHandle(0);
    wnd.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
    wnd.hCursor       = LoadCursor(NULL, IDC_WAIT);
    wnd.hbrBackground =(HBRUSH) GetStockObject(BLACK_BRUSH);
    wnd.lpszMenuName  = NULL;
    wnd.lpszClassName = "cquake";
    wnd.hIconSm       = LoadIcon(NULL, IDI_WINLOGO);

    RegisterClassEx(&wnd);


    openGL = CreateWindow("cquake",            // window class name
	                    "cquake",              // window caption
                        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,  // window style
                        300,100,                  // initial x & y position
                        SCREEN_WIDTH,SCREEN_HEIGHT,
                        NULL,                  // parent window handle
                        NULL,                  // window menu handle
                        (HINSTANCE)GetModuleHandle(0),             // program instance handle
                        NULL);		          // creation parameters

    
	// Make the window visible & update its client area
    ShowWindow(openGL, SW_SHOWNORMAL);// Show the window
    UpdateWindow(openGL);        // Sends WM_PAINT message


	gl.Link(openGL);
	gl.SetFormat(24,24,0,0);
	
	glMTexCoord2fSGIS = (lpMTexFUNC) wglGetProcAddress("glMultiTexCoord2fARB");
	glSelectTextureSGIS = (lpSelTexFUNC) wglGetProcAddress("glActiveTextureARB");
	if(glMTexCoord2fSGIS == NULL)
		glMTexCoord2fSGIS = (lpMTexFUNC) wglGetProcAddress("glMTexCoord2fSGIS");

	/*GetWindowRect(openGL,&rc);
	ClipCursor(&rc);
	ShowCursor(FALSE);
	GLWindow.Size(rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top);
	*/

	glColor3f(1,1,1);

	glViewport(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(73.74, 256.0 / 192.0, 0.005, 40.0);
	gluPerspective(68.3, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.005, 40.0);
	//gluPerspective(73.74, 256.0 / 192.0, 5, 10000.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER, 0.666);
	//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	return 0;
}

int frameBegin()
{
	//glDisable(GL_TEXTURE_2D);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor3b(255,255,255);

	// Set the current matrix to be the model matrix
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	/*
	glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up
    glRotatef (-r_angles[2],  1, 0, 0);
    glRotatef (-r_angles[0],  0, 1, 0);
    glRotatef (-r_angles[1],  0, 0, 1);
	glTranslatef(-(r_origin[0]*scale),-(r_origin[1]*scale),-(r_origin[2]*scale));*/

	
	return 0;
}

#ifdef WIN32
void glRotateX(float x) {
    glRotatef (x,  1.0f, 0, 0);
}

void glRotateY(float x) {
    glRotatef (x,  0, 1.0f, 0);
}

void glRotateZ(float x) {
    glRotatef (x,  0, 0, 1.0f);
}
#endif

void glTranslate3f32(int x,int y, int z) {
	//glTranslatef(x/4096.0f,y/4096.0f,z/4096.0f);
	glTranslatef(x,y,z);
}

//#define RGB15(r,g,b)  ((r)|((g)<<5)|((b)<<10))
void DS_COLOR(unsigned short c) {
	float r,g,b;
	r = c&31;
	g = (c>>5)&31;
	b = (c>>10)&31;
	glColor3f(r/31.0f,g/31.0f,b/31.0f);
}

void DS_NORMAL(int n) {
//#define NORMAL_PACK(x,y,z)   (((x) & 0x3FF) | (((y) & 0x3FF) << 10) | ((z) << 20)) /*!< \brief Pack 3 v10 normals into a 32bit value */
	float x,y,z;

	x = (n & 0x3FF)/(float)(1<<9);
	y = ((n>>10) & 0x3FF)/(float)(1<<9);
	z = ((n>>20) & 0x3FF)/(float)(1<<9);
	glNormal3f(x,y,z);
}

void DS_TEXCOORD2T16(int x, int y) {
//extern float ds_texture_width;
//extern float ds_texture_height;
	//glTexCoord2f(x/ds_texture_width,y/ds_texture_height);
}

void aDS_VERTEX3V16(int x, int y, int z) {
	glVertex3f(x/4096.0f,y/4096.0f,z/4096.0f);
}

int frameEnd()
{
		gl.SwapBuffers();
	return 0;
}

void ds_get_window_pos(RECT *rc) {
	gl.GetPos(rc);
}
void Key_Event (int key, qboolean down)
{
}

};
