#include "gl.h"
#include <stdio.h>
#include "ctr_rend.h"

#ifdef _3DS
#include <3ds.h>
#endif

#if 0
#define DBGPRINT printf
#else
#define DBGPRINT(...) /**/  
#endif


void glTexEnvf(GLenum val0, GLenum val1, GLenum val2) {
	DBGPRINT("\n");
}


void glBlendFunc(GLenum a, GLenum b) {
	DBGPRINT("\n");
}


void glDepthMask(GLenum a) {
	DBGPRINT("\n");
}


void glPolygonMode(GLenum a, GLenum b) {
	DBGPRINT("\n");
}


void glCullFace(GLenum a) {
	DBGPRINT("\n");
}


void glDepthFunc(GLenum a) {
	DBGPRINT("\n");
}


void glDepthRange(GLfloat a, GLfloat b) {
	DBGPRINT("\n");
}


void glTexParameteri(GLenum target, GLenum b, GLint c) {
	CTR_TEXTURE *tx = ctr_handle_get(CTR_HANDLE_TEXTURE, ctr_state.bound_texture[ctr_state.client_texture_current]);
	if (target != GL_TEXTURE_2D || tx == 0) {
		return;
	}
	switch (b) {
	case GL_TEXTURE_MAG_FILTER:
	{
		switch (c) {
		case GL_NEAREST:
			tx->params &= ~GPU_TEXTURE_MAG_FILTER(GPU_LINEAR);
			break;
		case GL_LINEAR:
			tx->params |= GPU_TEXTURE_MAG_FILTER(GPU_LINEAR);
			break;
		}
	}
	break;
	case GL_TEXTURE_MIN_FILTER:
	{
		switch (c) {
		case GL_NEAREST:
			tx->params &= ~GPU_TEXTURE_MIN_FILTER(GPU_LINEAR);
			break;
		case GL_LINEAR:
			tx->params |= GPU_TEXTURE_MIN_FILTER(GPU_LINEAR);
			break;
		}
	}
	break;
	case GL_TEXTURE_WRAP_S:
	{
		tx->params &= ~GPU_TEXTURE_WRAP_S(3);
		switch (c) {
		case GL_CLAMP_TO_EDGE:
			tx->params |= GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE);
			break;
		case GL_REPEAT:
			tx->params |= GPU_TEXTURE_WRAP_S(GPU_REPEAT);
			break;
		}
	}
	break;
	case GL_TEXTURE_WRAP_T:
	{
		tx->params &= ~GPU_TEXTURE_WRAP_T(3);
		switch (c) {
		case GL_CLAMP_TO_EDGE:
			tx->params |= GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE);
			break;
		case GL_REPEAT:
			tx->params |= GPU_TEXTURE_WRAP_T(GPU_REPEAT);
			break;
		}
	}
	break;
	}
	DBGPRINT("\n");
}

void glTexParameterf(GLenum target, GLenum b, GLfloat c) {
	DBGPRINT("\n");
}

void glTexParameterfv(GLenum a, GLenum b, GLfloat *c) {
	DBGPRINT("\n");
}


void glReadPixels(GLenum a, GLenum b, GLenum c, GLenum d, GLenum e, GLenum f, void *p) {
	DBGPRINT("\n");
}


void glDrawBuffer(GLenum a) {
	DBGPRINT("\n");
}


void glClear(GLenum a) {
	DBGPRINT("\n");
}


void glAlphaFunc(GLenum a, GLfloat b) {
	DBGPRINT("\n");
}


void glClearColor(GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
	DBGPRINT("\n");
}


void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
	DBGPRINT("\n");
}


void glFinish() {
	DBGPRINT("\n");
}


void glClipPlane(GLenum plane, const GLdouble *equation) {
	DBGPRINT("\n");
}


void glClearStencil(GLenum a) {
	DBGPRINT("\n");
}


GLenum glGetError() {
	DBGPRINT("\n");
	return 0;
}


void glShadeModel(GLenum a) {
	DBGPRINT("\n");
}


void glColor4f(GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
	DBGPRINT("\n");
}


void glGetIntegerv(GLenum pname, GLint *params)
{
	switch (pname) {
	case GL_MAX_TEXTURE_SIZE:
		*params = 128;
		break;
	case GL_MAX_ACTIVE_TEXTURES:
		*params = 3;
		break;
	default:
		DBGPRINT("glGetIntegerv: %d\n", pname);
		waitforit("glGetIntegerv");
	}
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
	switch (pname) {
	case GL_MODELVIEW_MATRIX:
	{
		int depth = ctr_state.matrix_depth[0];
		matrix_4x4 *mat = &ctr_state.matrix[0][depth];
		memcpy(params, mat->m, sizeof(float) * 16);
	}
	break;
	case GL_PROJECTION_MATRIX:
	{
		int depth = ctr_state.matrix_depth[1];
		matrix_4x4 *mat = &ctr_state.matrix[1][depth];
		memcpy(params, mat->m, sizeof(float) * 16);
	}
	break;
	default:
		DBGPRINT("glGetFloatv: %d\n", pname);
		waitforit("glGetFloatv");
	}
}


void glColor4ubv(const GLubyte *v) {
	DBGPRINT("\n");
}


void glTexCoord2fv(GLfloat *a) {
	DBGPRINT("\n");
}


void glPolygonOffset(GLfloat factor, GLfloat units) {
	DBGPRINT("\n");
}


void glArrayElement(GLint i) {
	DBGPRINT("\n");
}

void glClearDepth(GLclampd depth) {
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
}


void glLineWidth(GLfloat width) {
}

void glStencilMask(GLuint mask) {
}

void glCallList(GLuint list) {
}
