#include "ctr_rend.h"
#include <stdlib.h>
#ifdef _3DS
#include <3ds.h>
#endif

void glGenTextures(GLsizei n, GLuint * textures) {
	if (n <= 0 || textures == 0) {
		return;
	}
	GLuint i;
	for (i = 0; i < n; i++) {
		CTR_TEXTURE *buf = malloc(sizeof(*buf));
		if (buf == 0) {
			return;
		}
		memset(buf, 0, sizeof(*buf));
		buf->params = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_WRAP_S(GPU_REPEAT) | GPU_TEXTURE_WRAP_T(GPU_REPEAT);
		textures[i] = ctr_handle_new(CTR_HANDLE_TEXTURE, buf);
	}
}

void glDeleteTextures(GLsizei n, const GLuint  *textures) {
	if (n <= 0 || textures == 0) {
		return;
	}
	GLuint i;
	for (i = 0; i < n; i++) {
		CTR_TEXTURE *buf = ctr_handle_get(CTR_HANDLE_TEXTURE, textures[i]);
		if (buf == 0) {
			continue;
		}
		if (buf->data) {
			linearFree(buf->data);
		}
		ctr_handle_remove(CTR_HANDLE_TEXTURE, textures[i]);
		free(buf);
	}
}

static GPU_TEXUNIT __tmu[3] = { GPU_TEXUNIT0, GPU_TEXUNIT1, GPU_TEXUNIT2 };
static void myGPU_SetTexture(GPU_TEXUNIT unit, u32 data, u16 width, u16 height, u32 param, GPU_TEXCOLOR colorType) {
	u32 iunit = unit;
	u32 iwidth = width;
	u32 iheight = height;
	u32 icolor = colorType;
	DBGPRINT("set: %d %d %d %d %08x %08x\n", ctr_state.client_texture_current, iwidth, iheight, icolor, data, data >> 3);
	//svcSleepThread(1000000000);
#ifdef _3DS
	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR), // RGB channels
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR), // Alpha
		GPU_TEVOPERANDS(0, 0, 0), // RGB
		GPU_TEVOPERANDS(0, 0, 0), // Alpha
		GPU_MODULATE, GPU_MODULATE, // RGB, Alpha
		0xFFFFFFFF);
	switch (unit)
	{
	case GPU_TEXUNIT0:
		GPUCMD_AddWrite(GPUREG_TEXUNIT0_TYPE, icolor);
		GPUCMD_AddWrite(GPUREG_TEXUNIT0_ADDR1, (data) >> 3);
		GPUCMD_AddWrite(GPUREG_TEXUNIT0_DIM, (iwidth << 16) | iheight);
		GPUCMD_AddWrite(GPUREG_TEXUNIT0_PARAM, param);
		break;

	case GPU_TEXUNIT1:
		GPUCMD_AddWrite(GPUREG_TEXUNIT1_TYPE, icolor);
		GPUCMD_AddWrite(GPUREG_TEXUNIT1_ADDR, (data) >> 3);
		GPUCMD_AddWrite(GPUREG_TEXUNIT1_DIM, (iwidth << 16) | iheight);
		GPUCMD_AddWrite(GPUREG_TEXUNIT1_PARAM, param);
		break;

	case GPU_TEXUNIT2:
		GPUCMD_AddWrite(GPUREG_TEXUNIT2_TYPE, icolor);
		GPUCMD_AddWrite(GPUREG_TEXUNIT2_ADDR, (data) >> 3);
		GPUCMD_AddWrite(GPUREG_TEXUNIT2_DIM, (iwidth << 16) | iheight);
		GPUCMD_AddWrite(GPUREG_TEXUNIT2_PARAM, param);
		break;
	}
#endif
}
void glBindTexture(GLenum target, GLuint texture) {
	DBGPRINT("bind texture: %08x\n", texture);
	if (texture == 0) {
		ctr_state.bound_texture[ctr_state.client_texture_current] = 0;
		return;
	}
	CTR_TEXTURE *tx = ctr_handle_get(CTR_HANDLE_TEXTURE, texture);
	DBGPRINT("bound texture: %d %08x %08x\n", ctr_state.client_texture_current, texture, tx);
	if (tx == 0) {
		tx = malloc(sizeof(*tx));
		if (tx == 0) {
			return;
		}
		memset(tx, 0, sizeof(*tx));
		tx->params = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_WRAP_S(GPU_REPEAT) | GPU_TEXTURE_WRAP_T(GPU_REPEAT);
		ctr_handle_set(CTR_HANDLE_TEXTURE, texture, tx);
	}
	ctr_state.bound_texture[ctr_state.client_texture_current] = texture;
#ifdef _3DS
	if (tx->data) {
		myGPU_SetTexture(
			__tmu[ctr_state.client_texture_current],
			osConvertVirtToPhys((u32)tx->data),
			tx->width, // Width
			tx->height, // Height
			tx->params, // Flags
			tx->format // Pixel format
			);
	}
#endif
}

void glClientActiveTexture(GLenum texture) {
	switch (texture) {
	case GL_TEXTURE0:
	case GL_TEXTURE1:
	case GL_TEXTURE2:
		ctr_state.client_texture_current = texture - GL_TEXTURE0;
		break;
	}
}

void glActiveTexture(GLenum texture) {
	switch (texture) {
	case GL_TEXTURE0:
	case GL_TEXTURE1:
	case GL_TEXTURE2:
		ctr_state.client_texture_current = texture - GL_TEXTURE0;
		break;
	}
}

int is_supported_size(int size) {
	switch (size) {
	case 1024:
	case 512:
	case 256:
	case 128:
	case 64:
	case 32:
	case 16:
	case 8:
		return 1;
	}
	return 0;
}


void tex_copy(u32 *inaddr, u32 *outaddr, u32 indim, u32 outdim, u32 flags, u32 size, u32 inlinewidth, u32 outlinewidth) {
	*((u32 *)0x1EF00C00) = ((u32)osConvertVirtToPhys((u32)inaddr)) >> 3;
	*((u32 *)0x1EF00C04) = ((u32)osConvertVirtToPhys((u32)outaddr)) >> 3;
	*((u32 *)0x1EF00C08) = outdim;
	*((u32 *)0x1EF00C0C) = indim;
	*((u32 *)0x1EF00C10) = flags;

	*((u32 *)0x1EF00C20) = size;
	*((u32 *)0x1EF00C24) = inlinewidth;
	*((u32 *)0x1EF00C28) = outlinewidth;

	*((u32 *)0x1EF00C14) = 0;
	*((u32 *)0x1EF00C18) |= 1;
}

static u8 tileOrder[] = {
	 0,  1,  8,  9,  2,  3, 10, 11,
	16, 17, 24, 25, 18, 19, 26, 27,
	 4,  5, 12, 13,  6,  7, 14, 15,
	20, 21, 28, 29, 22, 23, 30, 31,
	32, 33, 40, 41, 34, 35, 42, 43, 
	48, 49, 56, 57, 50, 51, 58, 59,
	36, 37, 44, 45, 38, 39, 46, 47,
	52, 53, 60, 61, 54, 55, 62, 63 };
static unsigned long htonl(unsigned long v)
{
	u8* v2 = (u8*)&v;
	return (v2[0] << 24) | (v2[1] << 16) | (v2[2] << 8) | (v2[3]);
}
extern const u8 kitten_bin[];
extern const u32 kitten_bin_size;
#define RGBA5551(r,g,b,a) (\
(((r>>3) & 0x1F) << 11)\
| (((g>>3) & 0x1F) << 6)\
| (((b>>3) & 0x1F) << 1)\
| (a))\

void tileImage32(u32* src, u32* dst, int width, int height)
{
	if (!src || !dst)return;
	int i, j, k, l;
	int r, g, b;
	u16 *dst2 = (u16*)dst;
	l = 0;
	for (j = 0; j<height; j += 8)
	{
		for (i = 0; i<width; i += 8)
		{
			for (k = 0; k<8 * 8; k++)
			{
				int x = i + tileOrder[k] % 8;
				int y = j + (tileOrder[k] - (x - i)) / 8;
				u32 v = src[x + (height - 1 - y)*width];
				int a = (v >> 24) & 0xff;
				int b = (v >> 16) & 0xff;
				int g = (v >> 8) & 0xff;
				int r = (v >> 0) & 0xff;
				dst2[l++] = RGBA5551(r, g, b,(a ? 1 : 0));
			}
		}
	}
}

#if 0
void copy_tex_block_rgba_5551(u32 *src, u16 *dst, int width, int height, int dst_stride, int row_stride) {
	int w, h;
	u32 *row_src = src;
	u16 *row_dst = dst;
	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			u32 v = row_src[w];
			int a = (v >> 24) & 0xff;
			int b = (v >> 16) & 0xff;
			int g = (v >> 8) & 0xff;
			int r = (v >> 0) & 0xff;
			row_dst[w] = RGBA5551(r, g, b, (a ? 1 : 0));
		}
		row_src += row_stride;
		row_dst += dst_stride;
	}
}

void copy_tex_rgba_5551(u32 *src, u16 *dst, int width, int height) {
	int w, h;
	u32 *row_src = src;
	u16 *row_dst = dst;
	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			int x = w + tileOrder[k] % 8;
			int y = j + (tileOrder[k] - (x - w)) / 8;
			u32 v = src[x + (height - 1 - y)*width];
			row_src += (8 * 8);
		}
	}
}

#endif // 0

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data) {
	//TODO support mipmaps
	if (level != 0) {
		return;
	}
	DBGPRINT("bound texture: %d %08x\n", ctr_state.client_texture_current, ctr_state.bound_texture[ctr_state.client_texture_current]);
	CTR_TEXTURE *tx = ctr_handle_get(CTR_HANDLE_TEXTURE, ctr_state.bound_texture[ctr_state.client_texture_current]);
	if (target != GL_TEXTURE_2D || tx == 0) {
		return;
	}


	//verify supported dimensions
	if (!is_supported_size(width) || !is_supported_size(height)) {
		return;
	}

	//verify supported formats
	int _format = -1;
	int _size = -1;
	switch (type) {
	case GL_UNSIGNED_BYTE:
		switch (format) {
		case GL_RGB:
			_format = GPU_RGB8;
			_size = 3;
			break;
		case GL_RGBA:
			_format = GPU_RGBA8;
			_size = 4;
			break;
		case GL_LUMINANCE:
			_format = GPU_L8;
			_size = 1;
			break;
		case GL_LUMINANCE_ALPHA:
			_format = GPU_LA8;
			_size = 1;
			break;
		}
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		_format = GPU_RGB565;
		_size = 2;
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		_format = GPU_RGBA4;
		_size = 2;
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		_format = GPU_RGBA5551;
		_size = 2;
		break;
	}
	//unsupported format
	if (_size == -1 || format == -1) {
		return;
	}

	_size *= (width * height);

	//release the old buffer if it is different size
	if (tx->data && (tx->size != _size)) {
		linearFree(tx->data);
		tx->data = 0;
		tx->size = 0;
		DBGPRINT("===================\n  freeing texture\n");
	}

	//create a new buffer if needed
	if (tx->data == 0) {
		tx->data = (GLubyte *)linearMemAlign(_size, 0x80);
		if (tx->data == 0) {
			return;
		}
		tx->size = _size;
		DBGPRINT("===================\n  new buffer\n");
	}
	tx->width = width;
	tx->height = height;
	tx->format = _format;

	if (data == 0) {
		return;
	}

	switch (type) {
	case GL_UNSIGNED_BYTE:
		switch (format) {
		case GL_RGB:
			copy_tex_rgb_rgb(tx, data, width, height);
			break;
		case GL_RGBA:
			copy_tex_rgba_rgba(tx, data, width, height);
			break;
		case GL_LUMINANCE:
			copy_tex_8_8(tx, data, width, height);
			break;
		case GL_LUMINANCE_ALPHA:
			copy_tex_8_8(tx, data, width, height);
			break;
		}
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		break;
	}
	//copy data if provided
	//if (data) {
		//memcpy(tx->data, data, _size);
		//tileImage32(data, tx->data, width, height);
	//}
#ifdef _3DS
	myGPU_SetTexture(
		__tmu[ctr_state.client_texture_current],
		osConvertVirtToPhys((u32)tx->data),
		tx->width, // Width
		tx->height, // Height
		tx->params, // Flags
		tx->format // Pixel format
		);
#endif
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
	//TODO support mipmaps
	if (level != 0) {
		return;
	}
	DBGPRINT("bound texture: %d %08x\n", ctr_state.client_texture_current, ctr_state.bound_texture[ctr_state.client_texture_current]);
	CTR_TEXTURE *tx = ctr_handle_get(CTR_HANDLE_TEXTURE, ctr_state.bound_texture[ctr_state.client_texture_current]);
	if (target != GL_TEXTURE_2D || tx == 0) {
		return;
	}


	//verify supported formats
	int _format = -1;
	int _size = -1;
	switch (type) {
	case GL_UNSIGNED_BYTE:
		switch (format) {
		case GL_RGB:
			_format = GPU_RGB8;
			_size = 3;
			break;
		case GL_RGBA:
			_format = GPU_RGBA8;
			_size = 4;
			break;
		case GL_LUMINANCE:
			_format = GPU_L8;
			_size = 1;
			break;
		case GL_LUMINANCE_ALPHA:
			_format = GPU_LA8;
			_size = 1;
			break;
		}
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		_format = GPU_RGB565;
		_size = 2;
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		_format = GPU_RGBA4;
		_size = 2;
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		_format = GPU_RGBA5551;
		_size = 2;
		break;
	}
	//unsupported format
	if (_size == -1 || format == -1 || tx->data == 0) {
		return;
	}

#if 1
	switch (type) {
	case GL_UNSIGNED_BYTE:
		switch (format) {
		case GL_RGB:
			copy_tex_sub_rgb_rgb(tx, pixels, xoffset, yoffset, width, height);
			break;
		case GL_RGBA:
			copy_tex_sub_rgba_rgba(tx, pixels, xoffset, yoffset, width, height);
			break;
		case GL_LUMINANCE:
			copy_tex_sub_8_8(tx, pixels, xoffset, yoffset, width, height);
			break;
		case GL_LUMINANCE_ALPHA:
			copy_tex_sub_8_8(tx, pixels, xoffset, yoffset, width, height);
			break;
		}
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		break;
	}


#else
	tileImage32(pixels, tx->data, width, height);
#endif
#ifdef _3DS
	myGPU_SetTexture(
		__tmu[ctr_state.client_texture_current],
		osConvertVirtToPhys((u32)tx->data),
		tx->width, // Width
		tx->height, // Height
		tx->params, // Flags
		tx->format // Pixel format
		);
#endif
}
