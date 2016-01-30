#include "ctr_rend.h"
#ifdef _3DS
#include <3ds/gpu/shaderProgram.h>
#endif

#include <stdlib.h>
#include "gpu.h"

#ifdef _3DS
#include "vshader_shbin.h"
#endif

GLuint glCreateProgram(void) {
	shaderProgram_s *shaderprogram = (shaderProgram_s *)malloc(sizeof(*shaderprogram));
	if (shaderprogram == 0) {
		return 0;
	}
	shaderProgramInit(shaderprogram);
	GLuint id = ctr_handle_new(CTR_HANDLE_PROGRAM, shaderprogram);

	return id;
}

GLuint glCreateShader(GLenum shaderType) {
	GLuint id = ctr_handle_new(CTR_HANDLE_SHADER, 0);

	return id;
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
	if (count != 1) {
		return;
	}
	DVLB_s *dvlb = DVLB_ParseFile((u32*)string, *length);
	ctr_handle_set(CTR_HANDLE_SHADER, shader, dvlb);
}

void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length) {
	if (count != 1 || shaders == 0 || binary == 0 || length <= 0) {
		return;
	}
	DVLB_s *dvlb = DVLB_ParseFile((u32*)binary, length);
	ctr_handle_set(CTR_HANDLE_SHADER, *shaders, dvlb);
}

void glCompileShader(GLuint shader) {
	//do something???
}

void glAttachShader(GLuint program, GLuint shader) {
	DVLB_s *dvlb = (DVLB_s *)ctr_handle_get(CTR_HANDLE_SHADER, shader);
	shaderProgram_s *shaderprogram = (shaderProgram_s *)ctr_handle_get(CTR_HANDLE_PROGRAM, program);
	if (dvlb == 0 || shaderprogram == 0) {
		return;
	}
#ifdef _3DS
	//printf("dvlb->numDVLE: %d %d\n", dvlb->numDVLE, dvlb->DVLE[0].type);
	//while (1);
	for (int i = 0; i < dvlb->numDVLE; i++) {
		switch (dvlb->DVLE[i].type) {
		case VERTEX_SHDR:
			shaderProgramSetVsh(shaderprogram, &dvlb->DVLE[i]);
			break;
		case GEOMETRY_SHDR:
			shaderProgramSetGsh(shaderprogram, &dvlb->DVLE[i], 1); //TODO: fix this
			break;
		}
	}
#endif
}

void glLinkProgram(GLuint program) {
	//do something???
}

void glUseProgram(GLuint program) {
	shaderProgram_s *shaderprogram = (shaderProgram_s *)ctr_handle_get(CTR_HANDLE_PROGRAM, program);
	if(shaderprogram == 0) {
		ctr_state.bound_program = 0;
		return;
	}
	if (shaderProgramUse(shaderprogram) == 0) {
		ctr_state.bound_program = program;
	}
}

GLint glGetUniformLocation(GLuint program, const GLchar *name) {
	shaderProgram_s *shaderprogram = (shaderProgram_s *)ctr_handle_get(CTR_HANDLE_PROGRAM, program);
	if (shaderprogram == 0) {
		return 0;
	}
	GLint result = shaderInstanceGetUniformLocation(shaderprogram->vertexShader, name);
	return result;
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
#ifdef _3DS
	GPU_SetFloatUniformMatrix(GPU_VERTEX_SHADER, location, (matrix_4x4*)value);
#endif
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
#ifdef _3DS
	GPU_SetFloatUniform(GPU_VERTEX_SHADER, location, (u32*)value, count);
#endif
}

int uLoc_projection, uLoc_modelView;
int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;

matrix_4x4 material =
{
	.r =
	{
		{ { 0.0f, 0.2f, 0.2f, 0.2f } }, // Ambient
		{ { 0.0f, 0.4f, 0.4f, 0.4f } }, // Diffuse
		{ { 0.0f, 0.8f, 0.8f, 0.8f } }, // Specular
		{ { 1.0f, 0.0f, 0.0f, 0.0f } }, // Emission
	}
};

void ctr_rend_shader_init() {
	//manually create the program so we can use id 0
	GLuint prog_id = 0;
	shaderProgram_s *shaderprogram = (shaderProgram_s *)malloc(sizeof(*shaderprogram));
	if (shaderprogram == 0) {
		return;
	}
	ctr_handle_set(CTR_HANDLE_PROGRAM, prog_id, shaderprogram);

#ifdef _3DS
	shaderProgramInit(shaderprogram);
	//manually create the shader so we can use id 0
	GLuint vsh_id = 0;
	glShaderBinary(1, &vsh_id, 0, vshader_shbin, vshader_shbin_size);
	glAttachShader(prog_id, vsh_id);
	glLinkProgram(prog_id);

	// Get the location of the uniforms
	uLoc_projection = glGetUniformLocation(prog_id, "projection");
	uLoc_modelView = glGetUniformLocation(prog_id, "modelView");
	/*uLoc_lightVec = glGetUniformLocation(prog_id, "lightVec");
	uLoc_lightHalfVec = glGetUniformLocation(prog_id, "lightHalfVec");
	uLoc_lightClr = glGetUniformLocation(prog_id, "lightClr");
	uLoc_material = glGetUniformLocation(prog_id, "material");*/

	//set the default shader
	shaderProgramUse(shaderprogram);

	/*glUniformMatrix4fv(uLoc_material, 1, 0, material.m);
	glUniform4fv(uLoc_lightVec, 1, (float[]){ 0.0f, -1.0f, 0.0f, 0.0f });
	glUniform4fv(uLoc_lightHalfVec, 1, (float[]){ 0.0f, -1.0f, 0.0f, 0.0f });
	glUniform4fv(uLoc_lightClr, 1, (float[]){ 1.0f, 1.0f, 1.0f, 1.0f });*/
#endif

	ctr_state.bound_program = prog_id;

}
