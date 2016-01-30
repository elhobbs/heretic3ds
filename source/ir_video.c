#ifdef _3DS
#include <3ds.h>
#include <3ds/types.h>
#include "gl.h"
#include "3dmath.h"
#define GL_STREAM_DRAW 0
#define GL_MAP_WRITE_BIT 0
#endif

#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include "DoomDef.h"
#include "R_local.h"
#include "map.h"
#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#include "..\win32\glext.h"
extern HWND openGL;
#endif

typedef struct {
	int id;
	short leftoffset, topoffset;
	short width, height;
	int rotated;
	float st[2][2];
} ir_patch_t;

typedef struct {
	float xyz[3];
	float n[3];
	float c[4];
	float st[2];
} ir_vert_t;

#define IR_ATLAS_WIDTH 256

void **ir_patches;

extern byte *host_basepal;

int ir_add_atlas(int width, int height, int *rect, u32 *data);

void *ir_copy_patch(patch_t *patch, int *out_width, int *out_height, int scale) {
	u32 *desttop, *buffer;
	u32 *dest;
	byte *source;
	column_t *column;
	int w, h;
	int count;
	int col;
	unsigned int v, r, g, b;
	int stride;
	int cb;
	
	w = SHORT(patch->width);
	h = SHORT(patch->height);
	
	if (scale) {
		stride = *out_width = next2(patch->width);
		*out_height = next2(patch->height);
		cb = *out_width * *out_height * 4;
	}
	else {
		stride = *out_width = w;
		*out_height = h;
		cb = w * h * 4;
	}

	desttop = buffer = (u32 *)calloc(1, cb);
	for (col = 0; col < w; col++, desttop++)
	{
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
		// Step through the posts in a column
		while (column->topdelta != 0xff)
		{
			source = (byte *)column + 3;
			dest = desttop + column->topdelta*stride;
			count = column->length;
			while (count--)
			{
				//*dest = *source++;
				r = host_basepal[*source * 3 + 0];
				g = host_basepal[*source * 3 + 1];
				b = host_basepal[*source * 3 + 2];
				v = ((*source == 255 ? 0 : 255) << 24) + (r << 0) + (g << 8) + (b << 16);
				*dest = v;
				dest += stride;
				source++;
			}
			column = (column_t *)((byte *)column + column->length + 4);
		}
	}
	return buffer;
}

int next2(int x) {
	if (x > 512) {
		return 1024;
	}
	if (x > 256) {
		return 512;
	}
	if (x > 128) {
		return 256;
	}
	if (x > 64) {
		return 128;
	}
	return 64;
}

void *ir_copy_screen(int w, int h, int stride, int block_height,  byte *source) {
	unsigned int v, r, g, b;
	u32 *buffer = calloc(1, stride * block_height * 4);
	if (buffer) {
		int i, j;
		for (i = 0; i < h; i++) {
			u32 *dst = &buffer[i*stride];
			for (j = 0; j < w; j++) {
				r = host_basepal[*source * 3 + 0];
				g = host_basepal[*source * 3 + 1];
				b = host_basepal[*source * 3 + 2];
				v = ((*source == 255 ? 0 : 255) << 24) + (r << 0) + (g << 8) + (b << 16);
				dst[j] = v;
				source++;
			}
		}
	}
	return buffer;
}

void ir_make_screen(ir_patch_t *ir_patch, int in_width, int in_height, void *data) {

	int w = next2(in_width);
	int h = next2(in_height);
	void *p = ir_copy_screen(in_width, in_height, w, h, data);

	if (p) {
		glGenTextures(1, &ir_patch->id);
		glBindTexture(GL_TEXTURE_2D, ir_patch->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		free(p);
	}
	else {
		ir_patch->id = 0;
	}
	ir_patch->st[0][0] = 0;
	ir_patch->st[0][1] = 0;
	ir_patch->st[1][0] = (float)in_width / (float)w;
	ir_patch->st[1][1] = (float)in_height / (float)h;
	ir_patch->width = in_width;
	ir_patch->height = in_height;
	ir_patch->leftoffset = 0;
	ir_patch->topoffset = 0;

}

void ir_make_block(ir_patch_t *ir_patch, patch_t *patch) {

	int w;
	int h;
	void *p = ir_copy_patch(patch, &w, &h, 1);
	
	if (p) {
		glGenTextures(1, &ir_patch->id);
		glBindTexture(GL_TEXTURE_2D, ir_patch->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		free(p);
	}
	else {
		ir_patch->id = 0;
	}
	ir_patch->st[0][0] = 0;
	ir_patch->st[0][1] = 0;
	ir_patch->st[1][0] = (float)patch->width / (float)w;
	ir_patch->st[1][1] = (float)patch->height / (float)h;
	ir_patch->width = patch->width;
	ir_patch->height = patch->height;
	ir_patch->leftoffset = patch->leftoffset;
	ir_patch->topoffset = patch->topoffset;
	ir_patch->rotated = 0;

}

void ir_make_atlas(ir_patch_t *ir_patch, patch_t *patch) {
	int rect[4];
	int w;
	int h;
	u32 *p = ir_copy_patch(patch, &w, &h, 0);
	if (p) {
		ir_patch->id = ir_add_atlas(w, h, rect, p);
		if (rect[3] == h) {
			ir_patch->rotated = 0;
		}
		else {
			ir_patch->rotated = 1;
		}

		free(p);
	}

	ir_patch->st[0][0] = (float)rect[0] / (float)IR_ATLAS_WIDTH;
	ir_patch->st[0][1] = (float)rect[1] / (float)IR_ATLAS_WIDTH;
	ir_patch->st[1][0] = (float)(rect[0] + rect[2]) / (float)IR_ATLAS_WIDTH;
	ir_patch->st[1][1] = (float)(rect[1] + rect[3]) / (float)IR_ATLAS_WIDTH;
	ir_patch->width = w;
	ir_patch->height = h;
	ir_patch->leftoffset = patch->leftoffset;
	ir_patch->topoffset = patch->topoffset;
}

void ir_load_patch(ir_patch_t *ir_patch, patch_t *patch) {
	if (!ir_patch || !patch) {
		return;
	}

	int w = SHORT(patch->width);
	int h = SHORT(patch->height);

	if (w > IR_ATLAS_WIDTH || h > IR_ATLAS_WIDTH) {
		ir_make_block(ir_patch, patch);
	}
	else {
		ir_make_atlas(ir_patch, patch);
	}

}

ir_patch_t* ir_patch_num(int lump, int tag) {
	byte *ptr;
	ir_patch_t *ir_patch;
	patch_t *patch;

	if ((unsigned)lump >= numlumps)
		I_Error("ir_patch_num: %i >= numlumps", lump);

	if (!ir_patches[lump])
	{	// read the lump in
		//printf ("cache miss on lump %i %d\n",lump,W_LumpLength (lump));
		ir_patch = (ir_patch_t *)Z_Malloc(sizeof(ir_patch_t), tag, &ir_patches[lump]);
		patch = W_CacheLumpNum(lump, tag);
		ir_load_patch(ir_patch, patch);
	}

	return (ir_patch_t *)ir_patches[lump];
	}

ir_patch_t* ir_screen_num(int lump, int tag) {
	byte *ptr;
	ir_patch_t *ir_patch;
	void *p;

	if ((unsigned)lump >= numlumps)
		I_Error("ir_patch_num: %i >= numlumps", lump);

	if (!ir_patches[lump])
	{	// read the lump in
		//printf ("cache miss on lump %i %d\n",lump,W_LumpLength (lump));
		ir_patch = (ir_patch_t *)Z_Malloc(sizeof(ir_patch_t), tag, &ir_patches[lump]);
		p = W_CacheLumpNum(lump, tag);
		ir_make_screen(ir_patch, SCREENWIDTH, SCREENHEIGHT, p);
	}

	return (ir_patch_t *)ir_patches[lump];
}

#ifdef _3DS
#define YPOS(_y) (SCREEN_HEIGHT - (_y))
#else
#define YPOS(_y) (_y)
#endif

int ir_adust_position = 0;

void ir_draw_patch(int x, int y, ir_patch_t *ir_patch) {
	ir_vert_t verts[4];
	short indexes[6] = { 0,1,2,0,2,3 };
	float light[4] = { 1, 1, 1, 1 };
	int w, h;

	if (!ir_patch) {
		return;
	}

	x -= ir_patch->leftoffset;
	y -= ir_patch->topoffset;

	w = ir_patch->width;
	h = ir_patch->height;

	switch (ir_adust_position) {
	case 0:
		//top center
		x += (SCREEN_WIDTH - SCREENWIDTH) / 2;
		break;
	case 1:
		//center
		x += (SCREEN_WIDTH - SCREENWIDTH) / 2;
		y += (SCREEN_HEIGHT - SCREENHEIGHT) / 2;
		break;
	case 2:
		//bottom center
		x += (SCREEN_WIDTH - SCREENWIDTH) / 2;
		y = (SCREEN_HEIGHT - (SCREENHEIGHT - y));
		break;
	case 3:
		//bottom left
		y = (SCREEN_HEIGHT - (SCREENHEIGHT - y));
		break;
	default:
		//do nothing - top left
		break;
	}


	glBindTexture(GL_TEXTURE_2D, ir_patch->id);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(ir_vert_t), verts->xyz);
	glNormalPointer(GL_FLOAT, sizeof(ir_vert_t), verts->n);
	glColorPointer(4, GL_FLOAT, sizeof(ir_vert_t), verts->c);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ir_vert_t), verts->st);

	verts[0].xyz[0] = x;
	verts[0].xyz[1] = YPOS(y);
	verts[0].xyz[2] = 0;
	verts[0].st[0] = ir_patch->st[0][0];// sprite->ul;
	verts[0].st[1] = ir_patch->st[0][1];//sprite->vt;
	memcpy(verts[0].c, light, 16);

	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = YPOS(y);
	verts[1].xyz[2] = 0;
	if (ir_patch->rotated) {
		verts[1].st[0] = ir_patch->st[0][0];// sprite->ur;
		verts[1].st[1] = ir_patch->st[1][1];//sprite->vt;
	}
	else {
		verts[1].st[0] = ir_patch->st[1][0];// sprite->ur;
		verts[1].st[1] = ir_patch->st[0][1];//sprite->vt;
	}
	memcpy(verts[1].c, light, 16);

	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = YPOS(y + h);
	verts[2].xyz[2] = 0;
	verts[2].st[0] = ir_patch->st[1][0];// sprite->ur;
	verts[2].st[1] = ir_patch->st[1][1];//sprite->vb;
	memcpy(verts[2].c, light, 16);

	verts[3].xyz[0] = x;
	verts[3].xyz[1] = YPOS(y + h);
	verts[3].xyz[2] = 0;
	if (ir_patch->rotated) {
		verts[3].st[0] = ir_patch->st[1][0];// sprite->ul;
		verts[3].st[1] = ir_patch->st[0][1];//sprite->vb;
	}
	else {
		verts[3].st[0] = ir_patch->st[0][0];// sprite->ul;
		verts[3].st[1] = ir_patch->st[1][1];//sprite->vb;
	}
	memcpy(verts[3].c, light, 16);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexes);
}

void ir_draw_patch_num(int x, int y, int lump, int tag) {
	ir_patch_t *ir_patch = ir_patch_num(lump, tag);
	ir_draw_patch(x, y, ir_patch);
}

void ir_draw_shadow_patch_num(int x, int y, int lump, int tag) {
	ir_patch_t *ir_patch = ir_patch_num(lump, tag);
	ir_draw_patch(x, y, ir_patch);
}

void ir_draw_fuzz_patch_num(int x, int y, int lump, int tag) {
	ir_patch_t *ir_patch = ir_patch_num(lump, tag);
	ir_draw_patch(x, y, ir_patch);
}

void ir_draw_sreen_num(int lump, int tag) {
	ir_patch_t *ir_patch = ir_screen_num(lump, tag);
	ir_draw_patch(0, 0, ir_patch);
}

void ir_video_init() {
	ir_patches = (ir_patch_t **)malloc(sizeof(ir_patch_t *)*numlumps);
	memset(ir_patches,0, sizeof(ir_patch_t *)*numlumps);
}
