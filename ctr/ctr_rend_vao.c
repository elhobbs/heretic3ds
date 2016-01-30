#include "ctr_rend.h"
#include <stdlib.h>

void ctr_attrib_set(CTR_ATTR *attr, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	if (attr->size != size ||
		attr->stride != stride ||
		attr->type != type) {
		ctr_state.dirty = true;
	}

	/*if (attr->ptr != pointer) {
	int num = attr - ctr_state.attr;
	if (num >= 0 && num < CTR_ATTR_NUM) {
	GPUCMD_AddWrite(GPUREG_ATTRIBBUFFER0_CONFIG0 + (num * 3), CTR_RENDER_OFFSET(pointer));
	}
	}*/
	attr->size = size;
	attr->stride = stride;
	attr->type = type;
	attr->ptr = pointer;
	attr->bound_array_buffer = ctr_state.bound_array_buffer;
	DBGPRINT("ctr_attrib_set: %08x\n", attr->bound_array_buffer);
}

u32 ctr_attr_format(CTR_ATTR *attr, int num, int index, u32 *mask, u64 *attr_perm, u64 *buff_perm, u32 *buff_offs, u16 *buff_strd) {

	if (mask) *mask |= (1L << index);

	if (attr_perm) *attr_perm |= (index << (4 * num));

	if (buff_perm) *buff_perm = index;
	if (buff_strd) *buff_strd = attr->stride;
	if (buff_offs) {
		if (attr->bound_array_buffer) {
			CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, attr->bound_array_buffer);
			if (buf) {
				*buff_offs = CTR_RENDER_OFFSET_ADD(buf->data,attr->ptr);
				DBGPRINT("adding offset\n");
			}
			else {
				*buff_offs = 0;
			}
		}
		else {
			*buff_offs = attr->ptr ? CTR_RENDER_OFFSET(attr->ptr) : 0;
		}
	}

	return GPU_ATTRIBFMT(num, attr->size, __ctr_data_type(attr->type));
}

void ctr_attrib_clear(CTR_ATTR *attr) {
	attr->size = 0;
	attr->stride = 0;
	attr->type = 0;
	attr->ptr = 0;
	ctr_state.dirty = 1;
}

void glEnableClientState(GLenum cap) {
	ctr_state.client_state_requested |= cap;
	ctr_state.dirty = 1;
}

void glDisableClientState(GLenum cap) {
	CTR_ATTR *attr;
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	if (vao == 0) {
		return;
	}
	switch (cap) {
	case GL_COLOR_ARRAY:
		attr = &vao->color;
		break;
	case GL_NORMAL_ARRAY:
		attr = &vao->normal;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		attr = &vao->texture[ctr_state.client_texture_current];
		break;
	case GL_VERTEX_ARRAY:
		attr = &vao->vertex;
		break;
	default:
		return;
	}

	ctr_state.client_state_requested &= ~cap;
	ctr_attrib_clear(attr);
	ctr_state.dirty = true;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	DBGPRINT("color: %d\n", ctr_state.bound_vao);
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	if (vao == 0) {
		return;
	}
	vao->bound_array_buffer = ctr_state.bound_array_buffer;
	ctr_attrib_set(&vao->color, size, type, stride, pointer);
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	DBGPRINT("texcoord: %d %d\n", ctr_state.bound_vao, ctr_state.client_texture_current);
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	if (vao == 0) {
		return;
	}
	vao->bound_array_buffer = ctr_state.bound_array_buffer;
	ctr_attrib_set(&vao->texture[ctr_state.client_texture_current], size, type, stride, pointer);
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	//void *p0 = __builtin_return_address(0);
	//DBGPRINT("glVertexPointer: %08x\n", p0);
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	if (vao == 0) {
		return;
	}
	DBGPRINT("vertex: %d %08x %d %08x\n", ctr_state.bound_vao, vao, stride, ctr_state.bound_array_buffer);
	vao->bound_array_buffer = ctr_state.bound_array_buffer;
	ctr_attrib_set(&vao->vertex, size, type, stride, pointer);
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
	DBGPRINT("normal: %d\n", ctr_state.bound_vao);
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	if (vao == 0) {
		return;
	}
	vao->bound_array_buffer = ctr_state.bound_array_buffer;
	ctr_attrib_set(&vao->normal, 3, type, stride, pointer);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *offset) {
	DBGPRINT("vap: %d %08x\n", index, ctr_state.bound_vao);
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	if (vao == 0) {
		return;
	}
	vao->bound_array_buffer = ctr_state.bound_array_buffer;
	ctr_attrib_set(&vao->attr[index], size, type, stride, offset);
}

#if 1 //VAO stuff
void glGenVertexArrays(GLsizei n, GLuint *arrays) {
	if (n <= 0 || arrays == 0) {
		return;
	}
	GLuint i;
	for (i = 0; i < n; i++) {
		CTR_VAO *vao = malloc(sizeof(*vao));
		if (vao == 0) {
			return;
		}
		memset(vao, 0, sizeof(*vao));
		arrays[i] = ctr_handle_new(CTR_HANDLE_VAO, vao);
	}
}

void glBindVertexArray(GLuint arrayid) {
	DBGPRINT("bind vao: %08x\n", arrayid);
	if (arrayid == 0) {
		ctr_state.bound_vao = 0;
		return;
	}
	if (ctr_handle_get(CTR_HANDLE_VAO, arrayid) != 0) {
		ctr_state.bound_vao = arrayid;
		DBGPRINT("bound vao: %08x\n", arrayid);
	}
}

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer) {
	if (ctr_state.bound_array_buffer == 0) {
		return;
	}
	CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, ctr_state.bound_array_buffer);
	if (buf == 0) {
		return;
	}
	switch (format) {
	case GL_V2F:
		break;
	case GL_V3F:
		break;
	case GL_C4UB_V2F:
		break;
	case GL_C4UB_V3F:
		break;
	case GL_C3F_V3F:
		break;
	case GL_N3F_V3F:
		break;
	case GL_C4F_N3F_V3F:
		break;
	case GL_T2F_V3F:
		break;
	case GL_T4F_V4F:
		break;
	case GL_T2F_C4UB_V3F:
		break;
	case GL_T2F_C3F_V3F:
		break;
	case GL_T2F_N3F_V3F:
		break;
	case GL_T2F_C4F_N3F_V3F:
		break;
	case GL_T4F_C4F_N3F_V4F:
		break;
	}
}
#endif

void ctr_rend_vao_init() {
	//setup vao 0 as default
	ctr_state.bound_vao = 0;
	CTR_VAO *vao = malloc(sizeof(*vao));
	memset(vao, 0, sizeof(*vao));
	ctr_handle_set(CTR_HANDLE_VAO, 0, vao);
}

void glLockArraysEXT(GLint first, GLsizei count) {
}

void glUnlockArraysEXT(void) {
}
