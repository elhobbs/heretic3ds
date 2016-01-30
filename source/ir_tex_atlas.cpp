#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include "DOOMDEF.H"
#include "MaxRectsBinPack.h"
#ifdef WIN32
#include <windows.h>
#include <gl\gl.h>
#endif

#ifdef _3DS
#include "gl.h"
#endif

static int ir_atlas_count = 0;
static GLuint ir_atlas = 0;
static int ir_atlas_create = 1;
static rbp::MaxRectsBinPack *ir_bin = 0;

#define IR_ATLAS_WIDTH 256

static u32 ir_data[IR_ATLAS_WIDTH*IR_ATLAS_WIDTH];

extern "C" int ir_add_atlas(int width, int height, int *rect, u32 *data);

int ir_add_atlas(int width, int height, int *rect, u32 *data) {
	do {
		if (ir_atlas_create) {

			if (ir_bin) {
				delete ir_bin;
			}
			ir_bin = new rbp::MaxRectsBinPack;
			ir_bin->Init(IR_ATLAS_WIDTH, IR_ATLAS_WIDTH);
			glGenTextures(1, &ir_atlas);
			glBindTexture(GL_TEXTURE_2D, ir_atlas);
			memset(ir_data, 0, sizeof(ir_data));
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IR_ATLAS_WIDTH, IR_ATLAS_WIDTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, ir_data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			ir_atlas_create = 0;
			ir_atlas_count++;
		}
		rbp::Rect r = ir_bin->Insert(width, height, rbp::MaxRectsBinPack::RectBestShortSideFit);
		if (r.height > 0) {
			//printf("%4d: %d %d %d %d\n", i, r.x, r.y, r.width, r.height);
			if (r.x < 0 || (r.x + r.width) > IR_ATLAS_WIDTH ||
				r.y < 0 || (r.y + r.height) > IR_ATLAS_WIDTH) {
				printf("DIM: %d %d\n", width, height);
				printf("BAD: %d %d %d %d\n", r.x, r.y, r.width, r.height);
#ifdef _3DS
				do {
					gspWaitForVBlank();
					hidScanInput();
				} while ((hidKeysUp() & KEY_A) == 0);
#endif
			}
			if(r.height == height)
			{
				int x = r.x;
				int y = r.y;
				int w = width;
				int h = height;
				u32 *dest = ir_data + IR_ATLAS_WIDTH * y + x;
				u32 *src = data;
				for (int i = 0; i < h; i++) {
					for (int j = 0; j < w; j++) {
						dest[j] = src[j];
					}
					src += w;
					dest += IR_ATLAS_WIDTH;
				}
			}
			else {
				int x = r.x;
				int y = r.y;
				int w = width;
				int h = height;
				u32 *dest = ir_data + IR_ATLAS_WIDTH * y + x;
				u32 *src = data;
				for (int i = 0; i < h; i++) {
					for (int j = 0; j < w; j++) {
						dest[j*IR_ATLAS_WIDTH] = src[j];
					}
					src += w;
					dest ++;
				}
			}
			rect[0] = r.x;
			rect[1] = r.y;
			rect[2] = r.width;
			rect[3] = r.height;
			glBindTexture(GL_TEXTURE_2D, ir_atlas);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IR_ATLAS_WIDTH, IR_ATLAS_WIDTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, ir_data);
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IR_ATLAS_WIDTH, IR_ATLAS_WIDTH, GL_RGBA, GL_UNSIGNED_BYTE, ir_data);
			break;
		}
		else {
			ir_atlas_create = 1;
		}
	} while (ir_atlas_create);

	return ir_atlas;
}

typedef struct {
	float xyz[3];
	float n[3];
	float c[4];
	float st[2];
} ir_vert_t;

extern "C" void ir_render_atlas();

void ir_render_atlas() {
	return;
	ir_vert_t verts[4];
	short indexes[6] = { 0,1,2,0,2,3 };
	float light[4] = { 1, 1, 1, 1 };
	int x = 100;
	int y = 500;

	printf("atlas: %d\n", ir_atlas);

	glBindTexture(GL_TEXTURE_2D, ir_atlas);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(ir_vert_t), verts->xyz);
	glNormalPointer(GL_FLOAT, sizeof(ir_vert_t), verts->n);
	glColorPointer(4, GL_FLOAT, sizeof(ir_vert_t), verts->c);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ir_vert_t), verts->st);


	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = 0;// sprite->ul;
	verts[0].st[1] = 0;//sprite->vt;
	memcpy(verts[0].c, light, 16);

	verts[1].xyz[0] = x + IR_ATLAS_WIDTH;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = 1;// sprite->ur;
	verts[1].st[1] = 0;//sprite->vt;
	memcpy(verts[1].c, light, 16);

	verts[2].xyz[0] = x + IR_ATLAS_WIDTH;
	verts[2].xyz[1] = y + IR_ATLAS_WIDTH;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = 1;// sprite->ur;
	verts[2].st[1] = 1;//sprite->vb;
	memcpy(verts[2].c, light, 16);

	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + IR_ATLAS_WIDTH;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = 0;// sprite->ul;
	verts[3].st[1] = 1;//sprite->vb;
	memcpy(verts[3].c, light, 16);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexes);
}