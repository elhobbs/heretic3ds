#pragma once

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef char GLchar;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

#define	GL_INVALID -1
#define	GL_BYTE						0x1000/*GPU_BYTE*/
#define	GL_UNSIGNED_BYTE			0x1001/*GPU_UNSIGNED_BYTE*/
#define	GL_SHORT					0x1002/*GPU_SHORT*/
#define	GL_UNSIGNED_SHORT			0x1002/*GPU_SHORT*/
#define	GL_FLOAT					0x1003/*GPU_FLOAT*/
#define	GL_UNSIGNED_SHORT_5_6_5		0x1004/*GPU_RGB565*/
#define	GL_UNSIGNED_SHORT_4_4_4_4	0x1005/*GPU_RGBA4*/
#define	GL_UNSIGNED_SHORT_5_5_5_1	0x1006/*GPU_RGBA5551*/

#define	GL_RGB						0x1007/*GPU_RGB8*/
#define	GL_RGB8						0x1007/*GPU_RGB8*/
#define	GL_RGBA						0x1008/*GPU_RGBA8*/
#define	GL_RGBA8					0x1008/*GPU_RGBA8*/
#define	GL_LUMINANCE				0x1009/*GPU_L8*/
#define	GL_LUMINANCE_ALPHA			0x100A/*GPU_LA8*/

#define	GL_TEXTURE_2D				0x100B
#define GL_ARRAY_BUFFER				0x100C
#define GL_ELEMENT_ARRAY_BUFFER		0x100D

#define GL_VERTEX_SHADER			0x100E

enum {
	GL_TEXTURE0 = 0x100F,
	GL_TEXTURE1 = 0x1010,
	GL_TEXTURE2 = 0x1011
};

enum {
	GL_VERTEX_ARRAY = 1,
	GL_NORMAL_ARRAY = 2,
	GL_COLOR_ARRAY = 4,
	GL_TEXTURE_COORD_ARRAY = 8 //16 32
};

enum {
	GL_TRIANGLES = 0x0000,
	GL_TRIANGLE_STRIP = 0x0100,
	GL_TRIANGLE_FAN = 0x0200
};

enum {
	GL_MODELVIEW = 0,
	GL_PROJECTION = 1,
	GL_TEXTURE = 2
};

enum {
	GL_V2F = 0x4000,
	GL_V3F,
	GL_C4UB_V2F,
	GL_C4UB_V3F,
	GL_C3F_V3F,
	GL_N3F_V3F,
	GL_C4F_N3F_V3F,
	GL_T2F_V3F,
	GL_T4F_V4F,
	GL_T2F_C4UB_V3F,
	GL_T2F_C3F_V3F,
	GL_T2F_N3F_V3F,
	GL_T2F_C4F_N3F_V3F,
	GL_T4F_C4F_N3F_V4F
};

#ifdef __cplusplus
extern "C" {
#endif

	void glEnable(GLenum  cap);

	void glMatrixMode(GLenum mode);
	void glLoadIdentity(void);
	void glPushMatrix(void);
	void glPopMatrix(void);
	void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
	void glScalef(GLfloat x, GLfloat y, GLfloat z);
	void glRotateX(float angle);
	void glRotateY(float angle);
	void glRotateZ(float angle);
	void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void glLoadMatrixf(const GLfloat *m);

	void glEnableClientState(GLenum cap);
	void glDisableClientState(GLenum cap);

#define BUFFER_OFFSET(x) ((const void*)(x))
#define BUFFER_OFFSET_ADD(x,o) ((const void*)(((u32)(x))+((u32)(o))))

	void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
	void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *offset);


	void glClientActiveTexture(GLenum texture);

	void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
	void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex);

	void glGenBuffers(GLsizei n, GLuint *buffers);
	void glBindBuffer(GLenum target, GLuint buffer);
	void glBufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage);
	void glDeleteBuffers(GLsizei n, const GLuint *buffers);
	void *glMapBuffer(GLenum target, GLenum access);
	void *glMapBufferRange(GLenum target, GLuint offset, GLuint length, GLenum access);
	GLboolean glUnmapBuffer(GLenum target);

	void glGenVertexArrays(GLsizei n, GLuint *arrays);
	void glBindVertexArray(GLuint array);
	void glLockArraysEXT(GLint first, GLsizei count);
	void glUnlockArraysEXT(void);

	void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);

	void glGenTextures(GLsizei n, GLuint *textures);
	void glDeleteTextures(GLsizei n, const GLuint *textures);
	void glBindTexture(GLenum target, GLuint texture);
	void glActiveTexture(GLenum texture);
	void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);

	GLuint glCreateProgram(void);
	GLuint glCreateShader(GLenum shaderType);
	void glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
	void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length);
	void glCompileShader(GLuint shader);
	void glAttachShader(GLuint program, GLuint shader);
	void glLinkProgram(GLuint program);
	void glUseProgram(GLuint program);
	GLint glGetUniformLocation(GLuint program, const GLchar *name);
	void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	void glUniform4fv(GLint location, GLsizei count, const GLfloat *value);

#if 0 //unused
	void glEnableVertexAttribArray(GLuint index);
#endif //end unused

#define GL_TRUE					1
#define GL_FALSE				0

	/*unimplemented*/
	void glDisable(GLenum val);
	void glTexEnvf(GLenum val0, GLenum val1, GLenum val2);
	void glBlendFunc(GLenum a, GLenum b);
	void glDepthMask(GLenum a);
	void glPolygonMode(GLenum a, GLenum b);
	void glCullFace(GLenum a);
	void glDepthFunc(GLenum a);
	void glDepthRange(GLfloat a, GLfloat b);
	void glTexParameteri(GLenum a, GLenum b, GLint c);
	void glTexParameterf(GLenum a, GLenum b, GLfloat c);
	void glTexParameterfv(GLenum a, GLenum b, GLfloat *c);
	void glReadPixels(GLenum a, GLenum b, GLenum c, GLenum d, GLenum e, GLenum f, void *p);
	void glDrawBuffer(GLenum a);
	void glClear(GLenum a);
	void glAlphaFunc(GLenum a, GLfloat b);
	void glClearColor(GLfloat a, GLfloat b, GLfloat c, GLfloat d);
	void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
	void glFinish();
	void glClipPlane(GLenum plane, const GLdouble *equation);
	void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void glClearStencil(GLenum a);
	GLenum glGetError();
	void glShadeModel(GLenum a);
	void glColor4f(GLfloat a, GLfloat b, GLfloat c, GLfloat d);
	void glGetIntegerv(GLenum a, GLint *b);
	void glBegin(GLenum a);
	void glVertex3fv(GLfloat *a);
	void glEnd();
	void glColor4ubv(const GLubyte *v);
	void glTexCoord2fv(GLfloat *a);
	void glPolygonOffset(GLfloat factor, GLfloat units);
	void glArrayElement(GLint i);
	void glGetFloatv(GLenum pname, GLfloat *params);

#ifdef __cplusplus
};
#endif

#define GL_CULL_FACE			0x100F
#define GL_FRONT				0x1010
#define GL_MODULATE				0x1011
#define GL_REPLACE				0x1012
#define GL_DECAL				0x1013
#define GL_ADD					0x1014
#define GL_EQUAL				0x1015
#define GL_ZERO					0x1016
#define GL_ONE					0x1017
#define GL_ONE_MINUS_SRC_COLOR	0x1018
#define GL_SRC_ALPHA			0x1019
#define GL_DST_ALPHA			0x101A
#define GL_ONE_MINUS_DST_COLOR	0x101B
#define GL_SRC_ALPHA_SATURATE	0x101C
#define GL_SRC_COLOR			0x101D
#define GL_BLEND				0x101E
#define GL_DST_COLOR			0x101F
#define GL_BACK					0x1020
#define GL_TEXTURE_ENV			0x1021
#define GL_TEXTURE_ENV_MODE		0x1022
#define GL_ONE_MINUS_SRC_ALPHA	0x1024
#define GL_ONE_MINUS_DST_ALPHA	0x1025
#define GL_FRONT_AND_BACK		0x1026
#define GL_FILL					0x1027
#define GL_DEPTH_TEST			0x1028
#define GL_ALPHA_TEST			0x1029
#define GL_GREATER				0x102A
#define GL_LESS					0x102B
#define GL_LEQUAL				0x102C
#define GL_COLOR_BUFFER_BIT		0x1
#define GL_DEPTH_BUFFER_BIT		0x2
#define GL_STENCIL_BUFFER_BIT	0x4
#define GL_LINE					0x102D
#define GL_GEQUAL				0x102E
#define GL_CLIP_PLANE0			0x102F
#define GL_STENCIL_INDEX		0x1030
#define GL_QUADS				0x1031
#define GL_TEXTURE_MIN_FILTER	0x1032
#define GL_LINEAR				0x1033
#define GL_TEXTURE_MAG_FILTER	0x1034
#define GL_TEXTURE_WRAP_S		0x1035
#define GL_TEXTURE_WRAP_T		0x1036
#define GL_CLAMP				0x1037
#define GL_BACK_LEFT			0x1038
#define GL_STENCIL_TEST			0x1039
#define GL_BACK_RIGHT			0x103B
#define GL_KEEP					0x103C
#define GL_INCR					0x103D
#define GL_ALWAYS				0x103E
#define GL_DEPTH_COMPONENT		0x103F
#define GL_LINEAR_MIPMAP_NEAREST	0x1040
#define GL_NEAREST					0x1041
#define GL_NEAREST_MIPMAP_NEAREST	0x1042
#define GL_NEAREST_MIPMAP_LINEAR	0x1043
#define GL_REPEAT					0x1044
#define GL_BORDER_COLOR				0x1045
#define GL_LINEAR_MIPMAP_LINEAR		0x1046
#define GL_RGBA4					0x1047
#define GL_RGBA5					0x1048
#define GL_RGB5						0x1049
#define GL_TEXTURE_BORDER_COLOR		0x104A
#define GL_SCISSOR_TEST				0x104B
#define GL_SMOOTH					0x104C
#define GL_MAX_TEXTURE_SIZE			0x104D
#define GL_POLYGON					0x104E
#define GL_POLYGON_OFFSET_FILL		0x104F
#define GL_UNSIGNED_INT				0x1050
#define GL_LINES					0x1051
#define GL_DECR						0x1052
#define GL_NOTEQUAL					0x1053
#define GL_MAX_ACTIVE_TEXTURES		0x1054
#define GL_MODELVIEW_MATRIX			0x1055
#define GL_PROJECTION_MATRIX		0x1056
#define GL_CLAMP_TO_EDGE			0x1057

#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505
