#include "3dmath.h"

void m4x4_print(matrix_4x4* in) {
	int i;
	for (i = 0; i < 16; i+=2) {
		printf("%2d: %-4.6f %-4.6f\n", i, in->m[i], in->m[i+1]);
	}

}
void m4x4_identity(matrix_4x4* out)
{
	m4x4_zeros(out);
	out->m[0] = out->m[5] = out->m[10] = out->m[15] = 1.0f;
}

void m4x4_multiply(matrix_4x4* out, const matrix_4x4* a, const matrix_4x4* b)
{
	int i, j;
	for (i = 0; i < 4; i ++)
		for (j = 0; j < 4; j ++)
			out->m[i + j * 4] =
			(a->m[0 + j * 4] * b->m[i + 0 * 4]) +
			(a->m[1 + j * 4] * b->m[i + 1 * 4]) +
			(a->m[2 + j * 4] * b->m[i + 2 * 4]) +
			(a->m[3 + j * 4] * b->m[i + 3 * 4]);
}

void m4x4_translate(matrix_4x4* mtx, float x, float y, float z)
{
	matrix_4x4 tm, om;

	m4x4_identity(&tm);
	tm.m[3] = x;
	tm.m[7] = y;
	tm.m[11] = z;

	m4x4_multiply(&om, mtx, &tm);
	m4x4_copy(mtx, &om);
}

void m4x4_scale(matrix_4x4* mtx, float x, float y, float z)
{
	float *tm = mtx->m;
	tm[0] *= x; tm[4] *= x; tm[8] *= x; tm[12] *= x;
	tm[1] *= y; tm[5] *= y; tm[9] *= y; tm[13] *= y;
	tm[2] *= z; tm[6] *= z; tm[10] *= z; tm[14] *= z;
}

void m4x4_rotate_x(matrix_4x4* mtx, float angle, bool bRightSide)
{
	matrix_4x4 rm, om;

	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);

	m4x4_zeros(&rm);
	rm.m[0] = 1.0f;
	rm.m[5] = cosAngle;
	rm.m[6] = sinAngle;
	rm.m[9] = -sinAngle;
	rm.m[10] = cosAngle;
	rm.m[15] = 1.0f;

	if (bRightSide) m4x4_multiply(&om, mtx, &rm);
	else            m4x4_multiply(&om, &rm, mtx);
	m4x4_copy(mtx, &om);
}

void m4x4_rotate_y(matrix_4x4* mtx, float angle, bool bRightSide)
{
	matrix_4x4 rm, om;

	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);

	m4x4_zeros(&rm);
	rm.m[0] = cosAngle;
	rm.m[2] = sinAngle;
	rm.m[5] = 1.0f;
	rm.m[8] = -sinAngle;
	rm.m[10] = cosAngle;
	rm.m[15] = 1.0f;

	if (bRightSide) m4x4_multiply(&om, mtx, &rm);
	else            m4x4_multiply(&om, &rm, mtx);
	m4x4_copy(mtx, &om);
}

void m4x4_rotate_z(matrix_4x4* mtx, float angle, bool bRightSide)
{
	matrix_4x4 rm, om;

	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);

	m4x4_zeros(&rm);
	rm.m[0] = cosAngle;
	rm.m[1] = sinAngle;
	rm.m[4] = -sinAngle;
	rm.m[5] = cosAngle;
	rm.m[10] = 1.0f;
	rm.m[15] = 1.0f;

	if (bRightSide) m4x4_multiply(&om, mtx, &rm);
	else            m4x4_multiply(&om, &rm, mtx);
	m4x4_copy(mtx, &om);
}

void m4x4_rotate(matrix_4x4* mtx, float angle, float x, float y, float z, bool bRightSide)
{
	float axis[3];
	float sine = sinf(angle);
	float cosine = cosf(angle);
	float one_minus_cosine = 1.0f - cosine;
	matrix_4x4 rm, om;
	vector_4f vec = { { 1.0f, z, y, x } };
	v4f_norm4(&vec);
	axis[0] = vec.x;
	axis[1] = vec.y;
	axis[2] = vec.z;

	m4x4_zeros(&rm);

	rm.r[0].x = cosine + (one_minus_cosine * axis[0] * axis[0]);
	rm.r[0].y = (one_minus_cosine * axis[0] *  axis[1]) - (axis[2] * sine);
	rm.r[0].z = (one_minus_cosine * axis[0] * axis[2]) + (axis[1] * sine);

	rm.r[1].x = (one_minus_cosine * axis[0] * axis[1]) + (axis[2] * sine);
	rm.r[1].y = cosine + (one_minus_cosine * axis[1] * axis[1]);
	rm.r[1].z = (one_minus_cosine * axis[1] * axis[2]) - (axis[0] * sine);

	rm.r[2].x = (one_minus_cosine * axis[0] * axis[2]) - (axis[1] * sine);
	rm.r[2].y = (one_minus_cosine * axis[1] * axis[2]) + (axis[0] * sine);
	rm.r[2].z = cosine + (one_minus_cosine * axis[2] * axis[2]);
	
	rm.r[3].w = 1.0f;

	if (bRightSide) m4x4_multiply(&om, mtx, &rm);
	else            m4x4_multiply(&om, &rm, mtx);
	m4x4_copy(mtx, &om);
}

void m4x4_ortho_tilt(matrix_4x4* mtx, float left, float right, float bottom, float top, float near, float far)
{
	matrix_4x4 mp;
	m4x4_zeros(&mp);

	// Build standard orthogonal projection matrix
	mp.r[0].x = 2.0f / (right - left);
	mp.r[0].w = (left + right) / (left - right);
	mp.r[1].y = 2.0f / (top - bottom);
	mp.r[1].w = (bottom + top) / (bottom - top);
	mp.r[2].z = 2.0f / (near - far);
	mp.r[2].w = (far + near) / (far - near);
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
	m4x4_multiply(mtx, &mp2, &mp3);
}

void m4x4_persp_tilt(matrix_4x4* mtx, float fovx, float invaspect, float near, float far)
{
	// Notes:
	// We are passed "fovy" and the "aspect ratio". However, the 3DS screens are sideways,
	// and so are these parameters -- in fact, they are actually the fovx and the inverse
	// of the aspect ratio. Therefore the formula for the perspective projection matrix
	// had to be modified to be expressed in these terms instead.

	// Notes:
	// fovx = 2 atan(tan(fovy/2)*w/h)
	// fovy = 2 atan(tan(fovx/2)*h/w)
	// invaspect = h/w

	// a0,0 = h / (w*tan(fovy/2)) =
	//      = h / (w*tan(2 atan(tan(fovx/2)*h/w) / 2)) =
	//      = h / (w*tan( atan(tan(fovx/2)*h/w) )) =
	//      = h / (w * tan(fovx/2)*h/w) =
	//      = 1 / tan(fovx/2)

	// a1,1 = 1 / tan(fovy/2) = (...) = w / (h*tan(fovx/2))

	float fovx_tan = tanf(fovx / 2);
	matrix_4x4 mp;
	m4x4_zeros(&mp);

	// Build standard perspective projection matrix
	mp.r[0].x = 1.0f / fovx_tan;
	mp.r[1].y = 1.0f / (fovx_tan*invaspect);
	mp.r[2].z = (near + far) / (near - far);
	mp.r[2].w = (2 * near * far) / (near - far);
	mp.r[3].z = -1.0f;

	// Fix depth range to [-1, 0]
	matrix_4x4 mp2;
	m4x4_identity(&mp2);
	mp2.r[2].z = 0.5;
	mp2.r[2].w = -0.5;
	m4x4_multiply(mtx, &mp2, &mp);

	// Rotate the matrix one quarter of a turn CCW in order to fix the 3DS screens' orientation
	m4x4_rotate_z(mtx, M_PI / 2, true);
}
