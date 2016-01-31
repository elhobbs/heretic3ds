#include <string.h>
#include "keyboard.h"

#include <3ds.h>
#include "gpu.h"
#include "DOOMDEF.H"
#include "gl.h"

#include "heretic_shbin.h"

typedef unsigned char byte;

byte *host_basepal;
int r_framecount = 0;


int ibm_main(int argc, char **argv);

void Pause(u32 ms)
{
	svcSleepThread(ms * 1000000LL);
}

int ms_to_next_tick;

int I_GetTime(void)
{
	int t = osGetTime();
	int i = t*(TICRATE / 5) / 200;
	ms_to_next_tick = (i + 1) * 200 / (TICRATE / 5) - t;
	if (ms_to_next_tick > 1000 / TICRATE || ms_to_next_tick<1) ms_to_next_tick = 1;
	return i;
}
int heretic_prog;

void __gspWaitForEvent(GSPGPU_Event id, bool nextEvent);

void _3ds_shutdown() {
	gpuExit();
	gfxExit();
}


int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, 0);
	gpuInit();
	keyboard_init();

	atexit(_3ds_shutdown);

#if 0
	ctr_rend_init();
	GPU_FinishDrawing();
	GPUCMD_Finalize();
	GPUCMD_FlushAndRun();
	__gspWaitForEvent(GSPGPU_EVENT_P3D,false); // Wait for the rendering to complete


	heretic_prog = glCreateProgram();
	GLuint shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderBinary(1, &shader, 0, heretic_shbin, heretic_shbin_size);
	glAttachShader(heretic_prog, shader);
	glLinkProgram(heretic_prog);

#endif // 0

	ibm_main(argc, argv);

	return 0;
}
