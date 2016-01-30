#include "ctr_rend.h"
#include <stdlib.h>

#if 1 //VBO stuff

void glGenBuffers(GLsizei n, GLuint *buffers) {
	if (n <= 0 || buffers == 0) {
		return;
	}
	GLuint i;
	for (i = 0; i < n; i++) {
		CTR_BUFFER *buf = malloc(sizeof(*buf));
		if (buf == 0) {
			return;
		}
		memset(buf, 0, sizeof(*buf));
		buffers[i] = ctr_handle_new(CTR_HANDLE_BUFFER, buf);
	}
}

void glBindBuffer(GLenum target, GLuint buffer) {
	DBGPRINT("bind buffer: %08x\n", buffer);
	CTR_VAO *vao;
	if (buffer == 0) {
		switch (target) {
		case GL_ARRAY_BUFFER:
			if ((vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao)) != 0) {
				vao->bound_array_buffer = 0;
			}
			ctr_state.bound_array_buffer = 0;
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			if ((vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao)) != 0) {
				vao->bound_element_array_buffer = 0;
			}
			ctr_state.bound_element_array_buffer = 0;
			break;
		}
		return;
	}
	if (ctr_handle_get(CTR_HANDLE_BUFFER, buffer) != 0) {
		switch (target) {
		case GL_ARRAY_BUFFER:
			ctr_state.bound_array_buffer = buffer;
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			if((vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao)) != 0) {
				vao->bound_element_array_buffer = buffer;
			}
			ctr_state.bound_element_array_buffer = buffer;
			break;
		}
		DBGPRINT("bound buffer: %08x\n", buffer);
	}
}

void glBufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage) {
	int id = 0;
	switch (target) {
	case GL_ARRAY_BUFFER:
		id = ctr_state.bound_array_buffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		id = ctr_state.bound_element_array_buffer;
		break;
	}
	if (id == 0) {
		return;
	}
	CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, id);
	if (buf == 0) {
		return;
	}
	if (buf->data) {
		linearFree(buf->data);
		buf->data = 0;
	}
	GLubyte *buf_data = (GLubyte *)linearAlloc(size);
	if (buf_data == 0) {
		return;
	}
	buf->len = size;
	if (data) {
		memcpy(buf_data, data, size);
	}
	buf->data = buf_data;
	DBGPRINT("data: %08x %d %08x\n", id, size, buf_data);
}

void *glMapBuffer(GLenum target, GLenum access) {
	int id = 0;
	switch (target) {
	case GL_ARRAY_BUFFER:
		id = ctr_state.bound_array_buffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		id = ctr_state.bound_element_array_buffer;
		break;
	}
	if (id == 0) {
		return 0;
	}
	CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, id);
	if (buf == 0) {
		return 0;
	}
	return buf->data;
}

void *glMapBufferRange(GLenum target, GLuint offset, GLuint length, GLenum access) {
	int id = 0;
	switch (target) {
	case GL_ARRAY_BUFFER:
		id = ctr_state.bound_array_buffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		id = ctr_state.bound_element_array_buffer;
		break;
	}
	if (id == 0) {
		return 0;
	}
	CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, id);
	if (buf == 0) {
		return 0;
	}
	return (buf->data + (u32)offset);
}

GLboolean glUnmapBuffer(GLenum target) {

}


void glDeleteBuffers(GLsizei n, const GLuint *buffers) {
	if (n <= 0 || buffers == 0) {
		return;
	}
	GLuint i;
	for (i = 0; i < n; i++) {
		CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, buffers[i]);
		if (buf == 0) {
			continue;
		}
		if (buf->data) {
			linearFree(buf->data);
		}
		ctr_handle_remove(CTR_HANDLE_BUFFER, buffers[i]);
		free(buf);
	}
}

#endif //end VBO stuff

void ctr_rend_vbo_init() {
}
