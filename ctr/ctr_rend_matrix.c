#include "ctr_rend.h"
#include "gpu.h"

#if 1 //MATRIX stuff 

void glMatrixMode(GLenum mode) {
	if (mode < GL_MODELVIEW || mode > GL_TEXTURE) {
		return;
	}
	ctr_state.matrix_current = mode;
}

void glLoadIdentity() {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (mat == 0 || depth < 0 || depth > 31) {
		return;
	}
	m4x4_identity(mat);
}


void glPushMatrix() {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (mat == 0 || depth < 0 || depth >= 30) {
		return;
	}
	m4x4_copy(mat + 1, mat);
	ctr_state.matrix_depth[mode]++;
}

void glPopMatrix() {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (mat == 0 || depth < 1 || depth > 31) {
		return;
	}
	ctr_state.matrix_depth[mode]--;
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}
	m4x4_translate(mat, x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}
	m4x4_scale(mat, x, y, z);
}

void glRotateX(float angle) {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}
	m4x4_rotate_x(mat, angle*M_PI / 180.0f, true);
}

void glRotateY(float angle) {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}
	m4x4_rotate_y(mat, angle*M_PI / 180.0f, true);
}

void glRotateZ(float angle) {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}
	m4x4_rotate_z(mat, angle*M_PI / 180.0f, true);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}
	m4x4_rotate(mat, angle, x, y, z, true);
}

void glLoadMatrixf(const GLfloat *m) {
	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}
	memcpy(mat->m,m,sizeof(float)*16);
}

#endif //end MATRX stuff

void ctr_rend_matrix_init() {
	//initialize matrix stack
	ctr_state.matrix_depth[0] = 0;
	ctr_state.matrix_depth[1] = 0;
	ctr_state.matrix_depth[2] = 0;
	ctr_state.matrix_current = GL_MODELVIEW;
	m4x4_identity(&ctr_state.matrix[0][0]);
	m4x4_identity(&ctr_state.matrix[1][0]);
	m4x4_identity(&ctr_state.matrix[2][0]);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	matrix_4x4 mp;
	m4x4_zeros(&mp);

	int mode = ctr_state.matrix_current;
	int depth = ctr_state.matrix_depth[mode];
	matrix_4x4 *mat = &ctr_state.matrix[mode][depth];
	if (!mat) {
		return;
	}

	// Build standard orthogonal projection matrix
	mp.r[0].x = 2.0f / (right - left);
	mp.r[0].w = (left + right) / (left - right);
	mp.r[1].y = 2.0f / (top - bottom);
	mp.r[1].w = (bottom + top) / (bottom - top);
	mp.r[2].z = 2.0f / (zNear - zFar);
	mp.r[2].w = (zFar + zNear) / (zFar - zNear);
	mp.r[3].w = 1.0f;

	// Fix depth range to [-1, 0]
	matrix_4x4 mp2, mp3;
	m4x4_identity(&mp2);
	mp2.r[2].z = 0.5;
	mp2.r[2].w = -0.5;
	m4x4_multiply(&mp3, &mp2, &mp);

	// Fix the 3DS screens' orientation by swapping the X and Y axis
	m4x4_identity(&mp2);
	mp2.r[0].x = 0.0;
	mp2.r[0].y = 1.0;
	mp2.r[1].x = -1.0; // flipped
	mp2.r[1].y = 0.0;
	m4x4_multiply(mat, &mp2, &mp3);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	DBGPRINT("view: %d %d %d %d\n", x, y, width, height);
	gpuViewPort(x, y, width, height);
}
