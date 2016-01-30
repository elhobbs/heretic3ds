#include "gpu.h"
#include <stdio.h>

#define RGBA8(r,g,b,a) (\
(((r) & 0xFF) << 24)\
| (((g) & 0xFF) << 16)\
| (((b) & 0xFF) << 8)\
| ((a) & 0xFF))\

extern Handle gspEvents[GSPGPU_EVENT_MAX];
void __gspWaitForEvent(GSPGPU_Event id, bool nextEvent) {
	if (id >= GSPGPU_EVENT_MAX)return;
	if (nextEvent)
		svcClearEvent(gspEvents[id]);
	Result ret = svcWaitSynchronization(gspEvents[id], 200 * 1000 * 1000);
	if (ret == 0) {
		if (!nextEvent)
			svcClearEvent(gspEvents[id]);
	}
	else {
		printf("WaitForEvent timeout: %d\n", id);
	}
}

#define __gspWaitForPSC0() __gspWaitForEvent(GSPGPU_EVENT_PSC0, false)
#define __gspWaitForPSC1() __gspWaitForEvent(GSPGPU_EVENT_PSC1, false)
#define __gspWaitForVBlank() __gspWaitForEvent(GSPGPU_EVENT_VBlank0, true)
#define __gspWaitForVBlank0() __gspWaitForEvent(GSPGPU_EVENT_VBlank0, true)
#define __gspWaitForVBlank1() __gspWaitForEvent(GSPGPU_EVENT_VBlank1, true)
#define __gspWaitForPPF() __gspWaitForEvent(GSPGPU_EVENT_PPF, false)
#define __gspWaitForP3D() __gspWaitForEvent(GSPGPU_EVENT_P3D, false)
#define __gspWaitForDMA() __gspWaitForEvent(GSPGPU_EVENT_DMA, false)


#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static u32 *colorBuf, *depthBuf;
static u32 *cmdBuf;
static u32 cmdBufSize = 0x40000;
static u32 gsBackgroundColor;

void gpuInit(void)
{
	colorBuf = vramAlloc(400*240*8);
	depthBuf = vramAlloc(400*240*8);
	cmdBuf = linearAlloc(cmdBufSize *4);

	GPU_Init(NULL);
	GPU_Reset(NULL, cmdBuf, cmdBufSize);

	ctr_rend_init();
	// Bind the shader program
	glUseProgram(0);

	gsBackgroundColor = RGBA8(0x00, 0x00, 0x45, 0xFF);


	gpuClearBuffers(gsBackgroundColor);

	gpuFrameBegin();
	// Configure the first fragment shading substage to blend the texture color with
	// the vertex color (calculated by the vertex shader using a lighting algorithm)
	// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR), // RGB channels
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR), // Alpha
		GPU_TEVOPERANDS(0, 0, 0), // RGB
		GPU_TEVOPERANDS(0, 0, 0), // Alpha
		GPU_MODULATE, GPU_MODULATE, // RGB, Alpha
		0xFFFFFFFF);
	GPU_SetDummyTexEnv(1);
	GPU_SetDummyTexEnv(2);
	GPU_SetDummyTexEnv(3);
	GPU_SetDummyTexEnv(4);
	GPU_SetDummyTexEnv(5);
}

void gpuExit(void)
{
	linearFree(cmdBuf);
	vramFree(depthBuf);
	vramFree(colorBuf);
}

void gpuClearBuffers(u32 clearColor)
{
	GX_MemoryFill(
		colorBuf, clearColor, &colorBuf[240*400], GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH,
		depthBuf, 0,          &depthBuf[240*400], GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH);
	__gspWaitForPSC0(); // Wait for the fill to complete
}

void gpuViewPort(float x, float y, float width, float height) {
	GPU_SetViewport(
		(u32*)osConvertVirtToPhys((u32)depthBuf),
		(u32*)osConvertVirtToPhys((u32)colorBuf),
		x, y, height, width); // The top screen is physically 240x400 pixels
}


void gpuFrameBegin(void)
{
	// Configure the viewport and the depth linear conversion function
	gpuViewPort(0, 0, 400, 240); // The top screen is physically 240x400 pixels
	GPU_DepthMap(-1.0f, 0.0f); // calculate the depth value from the Z coordinate in the following way: -1.0*z + 0.0

	// Configure some boilerplate
	GPU_SetFaceCulling(GPU_CULL_NONE);
	GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
	GPU_SetStencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
	GPU_SetBlendingColor(0,0,0,0);
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);


	// This is unknown
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x1, 0);
	GPUCMD_AddWrite(GPUREG_EARLYDEPTH_TEST2, 0);
	//u32 zero = 0;
	//GPUCMD_Add(0x00010062, &zero, 1);
	//GPUCMD_Add(0x000F0118, &zero, 1);

	// Configure alpha blending and test
	GPU_SetAlphaBlending(
		GPU_BLEND_ADD, 
		GPU_BLEND_ADD, 
		GPU_SRC_ALPHA, 
		GPU_ONE_MINUS_SRC_ALPHA, 
		GPU_SRC_ALPHA, 
		GPU_ONE_MINUS_SRC_ALPHA);
	GPU_SetAlphaTest(false, GPU_ALWAYS, 0x00);

/*	GPU_SetTextureEnable(GPU_TEXUNIT0);

	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF);
	GPU_SetDummyTexEnv(1);
	GPU_SetDummyTexEnv(2);
	GPU_SetDummyTexEnv(3);
	GPU_SetDummyTexEnv(4);
	GPU_SetDummyTexEnv(5);*/

}


void gpuFrameEnd(void)
{
	// Finish rendering
	GPU_FinishDrawing();
	GPUCMD_Finalize();
	GPUCMD_FlushAndRun();
	__gspWaitForP3D(); // Wait for the rendering to complete

	// Transfer the GPU output to the framebuffer
	GX_DisplayTransfer(colorBuf, GX_BUFFER_DIM(240, 400),
		(u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), GX_BUFFER_DIM(240, 400),
		DISPLAY_TRANSFER_FLAGS);
	__gspWaitForPPF(); // Wait for the transfer to complete


	//__gspWaitForVBlank();
	gfxSwapBuffersGpu();

	GPUCMD_SetBufferOffset(0);

	ctr_rend_buffer_reset();
	//printf("--------end frame -------\n\n");

	gpuClearBuffers(gsBackgroundColor);


};

void GPU_SetDummyTexEnv(int id)
{
	GPU_SetTexEnv(id,
		GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
		GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_REPLACE,
		GPU_REPLACE,
		0xFFFFFFFF);
}
