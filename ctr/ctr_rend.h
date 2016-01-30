#pragma once

#ifdef _3DS
#include <3ds\types.h>
#include <3ds\gpu\gpu.h>
#include <3ds/linear.h>
#endif

#ifdef _WIN32
#include "wintypes.h"
#endif

#include "3dmath.h"
#include "gl.h"
#include "ctr_lists.h"

#if 0
#define DBGPRINT printf
#else
#define DBGPRINT(...) /**/  
#endif
extern u32 __ctru_linear_heap;
#define CTR_RENDER_OFFSET(x) ( ((u32)(x)) -  __ctru_linear_heap)
#define CTR_RENDER_OFFSET_ADD(x,o) (CTR_RENDER_OFFSET((x))+((u32)(o)))


typedef struct {
	union {
		float v[4];
		struct {
			float x, y, z, w;
		};
	};
} vec4f;

typedef struct {
	union {
		float v[3];
		struct {
			float x, y, z;
		};
	};
} vec3f;

typedef struct {
	union {
		float v[2];
		struct {
			float s, t;
		};
	};
} vec2f;

	//GL_UNSIGNED_SHORT,
	//GL_INT = 0,
	//GL_DOUBLE,

static __inline GLenum __ctr_data_type(GLenum type) {
	switch (type) {
	case GL_BYTE:			return GPU_BYTE;
	case GL_UNSIGNED_BYTE:	return GPU_UNSIGNED_BYTE;
	case GL_SHORT:			return GPU_SHORT;
	case GL_FLOAT:			return GPU_FLOAT;
	}
	return -1;
}

static __inline GLenum __ctr_texture_type(GLenum type) {
	switch (type) {
	case GL_RGB:				return GPU_RGB8;
	case GL_RGBA:				return GPU_RGBA8;
	case GL_LUMINANCE:			return GPU_L8;
	case GL_LUMINANCE_ALPHA:	return GPU_LA8;
	}
	return -1;
}


typedef struct {
	GLint size;
	GLenum type;
	GLsizei stride;
	const GLvoid *ptr;
	const u32 reg_config;
	GLuint bound_array_buffer;
} CTR_ATTR;


#define CTR_ATTR_NUM 12

typedef struct {
	GLuint bound_array_buffer;
	GLuint bound_element_array_buffer;
	union {
		CTR_ATTR attr[CTR_ATTR_NUM];
		struct {
			CTR_ATTR vertex;
			CTR_ATTR normal;
			CTR_ATTR color;
			CTR_ATTR texture[3];
		};
	};
} CTR_VAO;

typedef struct {
	//u32 mask;
	GLuint len;
	GLubyte *data;
} CTR_BUFFER;

typedef struct {
	GLshort width;
	GLshort height;
	GPU_TEXCOLOR format;
	GLuint size;
	GLubyte *data;
	GLuint params;
} CTR_TEXTURE;

typedef struct {
	u32 client_state_requested;
	u32 client_state_current;
	u32 client_texture_current;

	u32 texture_units;

	u32 dirty;

	GLuint bound_vao;
	GLuint bound_array_buffer;
	GLuint bound_element_array_buffer;
	GLuint bound_texture[3];
	GLuint bound_program;

	//this is used to move client data to linear
	GLubyte	*buffer;
	GLint	buffer_pos;
	GLint	buffer_len;

	int matrix_current;
	int matrix_depth[3];
	bool matrix_dirty[3];
	matrix_4x4 matrix[3][32];

} CTR_CLIENT_STATE;

u32 ctr_attr_format(CTR_ATTR *attr, int num, int index, u32 *mask, u64 *attr_perm, u64 *buff_perm, u32 *buff_offs, u16 *buff_strd);

void ctr_rend_buffer_reset();
void* ctr_rend_buffer_alloc(int len);
void* ctr_rend_buffer_copy(void *p, int len);
void* ctr_rend_buffer_copy_stride(void *p, int count, int size, int stride);

void ctr_rend_shader_init();
void ctr_rend_vao_init();
void ctr_rend_vbo_init();
void ctr_rend_matrix_init();
void ctr_rend_buffer_init();
void ctr_rend_init();
extern CTR_CLIENT_STATE ctr_state;


void copy_tex_rgb_rgb(CTR_TEXTURE *dst, u8 *src, int width, int height);
void copy_tex_rgba_rgba(CTR_TEXTURE *dst, u8 *src, int width, int height);
void copy_tex_rgb_5551(CTR_TEXTURE *dst, u8 *src, int width, int height);
void copy_tex_rgba_5551(CTR_TEXTURE *dst, u8 *src, int width, int height);
void copy_tex_rgba_4444(CTR_TEXTURE *dst, u8 *src, int width, int height);
void copy_tex_8_8(CTR_TEXTURE *dst, u8 *src, int width, int height);

void copy_tex_sub_rgb_rgb(CTR_TEXTURE *dst, u8 *src, int x, int y, int width, int height);
void copy_tex_sub_rgb_5551(CTR_TEXTURE *dst, u8 *src, int x, int y, int width, int height);
void copy_tex_sub_8_8(CTR_TEXTURE *dst, u8 *src, int x, int y, int width, int height);
