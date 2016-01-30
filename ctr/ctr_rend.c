#include "ctr_rend.h"
#ifdef _3DS
#include <3ds.h>
#endif

#include <string.h>
#include <stdlib.h>

extern int uLoc_projection, uLoc_modelView;

CTR_CLIENT_STATE ctr_state;

void ctr_rend_init() {
	memset(&ctr_state, 0, sizeof(ctr_state));
	ctr_state.dirty = 1;

	//no bound textures
	ctr_state.bound_texture[0] = 0;
	ctr_state.bound_texture[1] = 0;
	ctr_state.bound_texture[2] = 0;

	//no buffers bound
	ctr_state.bound_array_buffer = 0;
	ctr_state.bound_element_array_buffer = 0;

	ctr_rend_vao_init();

	ctr_rend_vbo_init();

	ctr_rend_matrix_init();

	ctr_rend_shader_init();

	ctr_rend_buffer_init();

}



static const u8 GPU_FORMATSIZE[4] = { 1, 1, 2, 4 };

void myGPU_SetAttributeBuffers(
	u8 totalAttributes, 
	u32* baseAddress, 
	u64 attributeFormats, 
	u16 attributeMask, 
	u64 attributePermutation, 
	u8 numBuffers, 
	u32 bufferOffsets[], 
	u64 bufferPermutations[],
	u16 bufferStrides[],
	u8 bufferNumAttributes[])
{
	u32 param[0x28];

	memset(param, 0x00, 0x28 * 4);

	param[0x0] = ((u32)baseAddress) >> 3;
	param[0x1] = attributeFormats & 0xFFFFFFFF;
	param[0x2] = ((totalAttributes - 1) << 28) | ((attributeMask & 0xFFF) << 16) | ((attributeFormats >> 32) & 0xFFFF);

	int i, j;
	u8 sizeTable[0xC];
	DBGPRINT("-------- %d\n", (u32)totalAttributes);
	for (i = 0; i<totalAttributes; i++)
	{
		u8 v = attributeFormats & 0xF;
		sizeTable[i] = GPU_FORMATSIZE[v & 3] * ((v >> 2) + 1);
		attributeFormats >>= 4;
		DBGPRINT("-------- %d %d %d\n", i, v, GPU_FORMATSIZE[v & 3] * ((v >> 2) + 1));
	}

	for (i = 0; i<numBuffers; i++)
	{
		u16 stride;
		param[3 * (i + 1) + 0] = bufferOffsets[i];
		param[3 * (i + 1) + 1] = bufferPermutations[i] & 0xFFFFFFFF;
		if (bufferStrides[i]) {
			stride = bufferStrides[i];
		} else {
			stride = 0;
			for (j = 0; j < bufferNumAttributes[i]; j++) {
				//stride += sizeTable[j & 0xF];
				DBGPRINT("--- %2d %2d\n", j & 0xf, sizeTable[j & 0xF]);
				stride += sizeTable[(bufferPermutations[i] >> (4 * j)) & 0xF];
				DBGPRINT("--- %2d %2d\n\n", (bufferPermutations[i] >> (4 * j)) & 0xF, sizeTable[(bufferPermutations[i] >> (4 * j)) & 0xF]);
			}
		}
		//DBGPRINT("--stride: %d %d %d\n", i, stride, bufferStrides[i]);
		param[3 * (i + 1) + 2] = (bufferNumAttributes[i] << 28) | ((stride & 0xFFF) << 16) | ((bufferPermutations[i] >> 32) & 0xFFFF);
	}
	//DBGPRINT("\n");
#ifdef _3DS

	GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFERS_LOC, param, 0x00000027);

	GPUCMD_AddMaskedWrite(GPUREG_VSH_INPUTBUFFER_CONFIG, 0xB, 0xA0000000 | (totalAttributes - 1));
	GPUCMD_AddWrite(GPUREG_VSH_NUM_ATTR, (totalAttributes - 1));

	GPUCMD_AddIncrementalWrites(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW, ((u32[]){attributePermutation & 0xFFFFFFFF, (attributePermutation >> 32) & 0xFFFF}), 2);
#endif
}

int compare(const void * a, const void * b)
{
	return (*(int*)a - *(int*)b);
}

void ctr_build_order(CTR_VAO *vao, int *order) {
	int i;
	for (i = 0; i < CTR_ATTR_NUM; i++) {
		if (vao->attr[i].size) {
			order[i] = (((u32)(vao->attr[i].ptr))<<4) + i;
		}
		else {
			order[i] = 0x0ffffff0 + i;
		}
	}

	qsort(order, CTR_ATTR_NUM, sizeof(int), compare);
	for (i = 0; i < CTR_ATTR_NUM; i++) {
		if ((order[i] & 0x0ffffff0) == 0x0ffffff0) {
			order[i] = -1;
		}
		else {
			order[i] &= 0xf;
		}
		DBGPRINT(" %d", order[i]);
	}
	DBGPRINT("\n");
}

//TODO - this is a mess...
void ctr_build_order2(CTR_VAO *vao, int *order) {
	int i,j;
	u32 ord[CTR_ATTR_NUM];
	for (i = 0; i < CTR_ATTR_NUM; i++) {
		if (vao->attr[i].size) {
			ord[i] = (u32)(vao->attr[i].ptr);
		}
		else {
			ord[i] = 0xfffffff0 + i;
		}
	}

	qsort(order, CTR_ATTR_NUM, sizeof(int), compare);

	for (i = 0; i < CTR_ATTR_NUM; i++) {
		order[i] = -1;
		for (j = 0; j < CTR_ATTR_NUM; j++) {
			if (((u32)vao->attr[i].ptr) == ord[j]) {
				DBGPRINT("ctr_build_order2: %08x\n", ord[j]);
				if ((ord[j] & 0xfffffff0) == 0xfffffff0) {
					order[i] = -1;
				}
				else {
					order[i] = j;
				}
				break;
			}
		}
	}
	DBGPRINT("\n");
}


int ctr_is_aos(CTR_VAO *vao) {
	int i, cb = 0, stride = 0, s;
	CTR_ATTR *attr = vao->attr;
	for (i = 0; i < CTR_ATTR_NUM; i++, attr++) {
		if (attr->size) {
			if (attr->stride == 0) {
				return 0;
			}
			s = GPU_FORMATSIZE[__ctr_data_type(attr->type)];
			DBGPRINT("%d %d\n", s, __ctr_data_type(attr->type));
			cb += (s * attr->size);
			//if tightly packed then single attr
			if (stride == 0) {
				stride = attr->stride;
			}
			else if (stride != attr->stride) {
				//different strides then not a single aos
				return 0;
			}
		}
	}
	DBGPRINT("cb: %d %d\n", cb, stride);
	return stride == cb ? 1 : 0;
}

void ctr_set_attr_buffers(int element_count, GLint basevertex) {
	DBGPRINT("ctr_state.bound_array_buffer == %08x\n", ctr_state.bound_array_buffer);
	DBGPRINT("ctr_state.bound_vao == %08x\n", ctr_state.bound_vao);
	//if (!ctr_state.dirty) {
	//	return;
	//}
	u32 attr_frmt = 0;
	int i;
	u16 stride = 0;
	u32 num_attr = 0;
	u32 num_buff = 0;
	u32 attr_mask = 0;
	u64 attr_perm = 0;
	u64 buff_perm[CTR_ATTR_NUM];
	u32 buff_offs[CTR_ATTR_NUM];
	u16 buff_strd[CTR_ATTR_NUM];
	u8  buff_attr[CTR_ATTR_NUM];
	int order[CTR_ATTR_NUM];
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	if (vao == 0) {
		return;
	}
	GLuint bound_array_buffer = ctr_state.bound_array_buffer;

	//if the vao has a bound array then use it
	if (vao->bound_array_buffer) {
		bound_array_buffer = vao->bound_array_buffer;
		DBGPRINT("bound buffer: %08x\n", vao->bound_array_buffer);
	}

	if (bound_array_buffer) {
		CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, bound_array_buffer);
		
		//try to figure out the order for each attribute
		ctr_build_order(vao, order);
		
		buff_strd[0] = 0;
		for (i = 0; i < CTR_ATTR_NUM && order[i] != -1; i++) {
			u16 attr_stride = 0;
			int index = order[i];
			attr_frmt |= ctr_attr_format(&vao->attr[index], num_attr, index, &attr_mask, &attr_perm, 0, 0, &attr_stride);
			stride += attr_stride;
			num_attr++;
			DBGPRINT("   %d %08x %016llx %d %d\n", i, attr_mask, attr_perm, attr_stride, stride);
		}

		//HACK HACK HACK
		stride = vao->attr[order[0]].stride;

		buff_offs[0] = CTR_RENDER_OFFSET(buf->data) + ((u32)basevertex * (u32)stride);
		buff_attr[0] = num_attr;
		buff_perm[0] = attr_perm;
		buff_strd[0] = stride;
		num_buff = 1;
	}
	else {
		//if this is a single attribute buffer that is not bound then
		//it needs to be copied to linear
		if (ctr_is_aos(vao)) {
			DBGPRINT("is AOS\n");
			//while (1);
			//try to figure out the order for each attribute
			ctr_build_order2(vao, order);
			stride = vao->attr[order[0]].stride;
			int cb = stride * element_count;
			GLubyte *data = ctr_rend_buffer_copy(vao->attr[order[0]].ptr, cb);
			DBGPRINT("%d %d %d %d\n", order[0], stride, element_count, cb);

			for (i = 0; i < CTR_ATTR_NUM && order[i] != -1; i++) {
				u16 attr_stride = 0;
				int index = order[i];

				DBGPRINT("attr: %d %d %d\n", i, vao->attr[index].size, index);

				attr_frmt |= ctr_attr_format(&vao->attr[index], num_attr, index, &attr_mask, &attr_perm, 0, 0, &attr_stride);
				num_attr++;
				DBGPRINT("   %d %08x %016llx %d %d\n", i, attr_mask, attr_perm, attr_stride, stride);
			}

			buff_offs[0] = CTR_RENDER_OFFSET(data);// buf->data);
			buff_attr[0] = num_attr;
			buff_perm[0] = attr_perm;
			buff_strd[0] = stride;
			num_buff = 1;
		}
		else {
			DBGPRINT("not AOS\n");
			DBGPRINT("program: %d\n", ctr_state.bound_program);
			//while (1);
			CTR_ATTR *attr = vao->attr;
			for (i = 0; i < CTR_ATTR_NUM; i++, attr++) {
				//this is a big hack
				//we are going to fake the missing attribute for program 0
				int clear = 0;
				if (attr->size == 0 && i < 4 && ctr_state.bound_program == 0) {
					DBGPRINT("hack %d %d %d\n", i, 4, attr->size);
					clear = 1;
					attr->bound_array_buffer = 0;
					attr->size = 4;
					attr->stride = 4;
					attr->type = GL_UNSIGNED_BYTE;
					attr->ptr = ctr_rend_buffer_alloc(element_count * 4);
					memset(attr->ptr, 0, element_count * 4);
					attr_frmt |= ctr_attr_format(attr, i, i, &attr_mask, &attr_perm, &buff_perm[i], &buff_offs[i], &buff_strd[i]);
					
					buff_attr[i] = 1;
					num_attr++;
					num_buff++;
				}
				else if (attr->ptr || attr->bound_array_buffer/* && ctr_state.client_state_requested & (1 << i)*/) {
					attr_frmt |= ctr_attr_format(attr, i, i, &attr_mask, &attr_perm, &buff_perm[i], &buff_offs[i], &buff_strd[i]);

					int calc_stride = GPU_FORMATSIZE[__ctr_data_type(attr->type)] * attr->size;
					if (vao->attr[i].stride == 0) {
						stride = calc_stride;
					} else {
						stride = attr->stride;
					}
					int cb = stride * element_count;
					GLubyte *data = ctr_rend_buffer_copy_stride(attr->ptr, element_count, calc_stride, stride);
					buff_offs[i] = CTR_RENDER_OFFSET(data);
					buff_strd[i] = calc_stride;

					DBGPRINT("attr: %d %d\n", i, calc_stride);



					buff_attr[i] = 1;
					num_attr++;
					num_buff++;
				}
				else {
					buff_attr[i] = 0;
				}
				//hack cleanup
				if (clear) {
					clear = 0;
					attr->size = 0;
					attr->type = 0;
					attr->ptr = 0;
				}
				//DBGPRINT("%d %08x %016llx\n  %016llx %08x\n", i, attr_mask, attr_perm, buff_perm[i], buff_offs[i]);
			}
		}
		//while (1);
	}

	//invert bits for attr_mask
	attr_mask = (~attr_mask) & 0xFFF;
	//printf("%08x\n", attr_mask);
	DBGPRINT("mask: %08x\n", attr_mask);
	DBGPRINT("num_attr: %d\n", num_attr);
	DBGPRINT("attr_frmt: %08x\n", attr_frmt);
	DBGPRINT("attr_perm: %016llx\n", attr_perm);
	DBGPRINT("num_buff: %d\n", num_buff);
	DBGPRINT("buff_attr:"); for (i = 0; i < num_buff; i++) DBGPRINT(" %d", buff_attr[i]); DBGPRINT("\n");
	DBGPRINT("buff_strd:"); for (i = 0; i < num_buff; i++) DBGPRINT(" %d", buff_strd[i]); DBGPRINT("\n");
	DBGPRINT("buff_perm:"); for (i = 0; i < num_buff; i++) DBGPRINT(" %016llx", buff_perm[i]); DBGPRINT("\n");
	DBGPRINT("buff_offs:"); for (i = 0; i < num_buff; i++) DBGPRINT(" %d", buff_offs[i]); DBGPRINT("\n");
	//while (1);

	myGPU_SetAttributeBuffers(
		num_attr, // Number of inputs per vertex
		(u32*)osConvertVirtToPhys((u32)__ctru_linear_heap), // Location of the VBO
		attr_frmt, // Format of the inputs
		attr_mask, // Unused attribute mask, in our case bits 0~2 are cleared since they are used
		attr_perm, // Attribute permutations (here it is the identity, passing each attribute in order)
		num_buff, // Number of buffers
		buff_offs, // Buffer offsets (placeholders)
		buff_perm, // Attribute permutations for each buffer
		buff_strd, // attribute strides for each buffer
		buff_attr); // Number of attributes for each buffer
}

#ifndef GPUREG_VERTEX_OFFSET
#define GPUREG_VERTEX_OFFSET 0x022A
#endif

static void gsSetUniformMatrix(u32 startreg, float* m)
{
	float param[16];

	param[0x0] = m[3]; //w
	param[0x1] = m[2]; //z
	param[0x2] = m[1]; //y
	param[0x3] = m[0]; //x

	param[0x4] = m[7];
	param[0x5] = m[6];
	param[0x6] = m[5];
	param[0x7] = m[4];

	param[0x8] = m[11];
	param[0x9] = m[10];
	param[0xa] = m[9];
	param[0xb] = m[8];

	param[0xc] = m[15];
	param[0xd] = m[14];
	param[0xe] = m[13];
	param[0xf] = m[12];

	GPU_SetFloatUniform(GPU_VERTEX_SHADER, startreg, (u32*)param, 4);
}

void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex) {
	DBGPRINT("glDrawElements %08x %d %08x\n", ctr_state.bound_texture[ctr_state.client_texture_current], count, indices);
	GLuint _mode = -1;
	switch (mode) {
	case GL_TRIANGLES:
		_mode = GPU_GEOMETRY_PRIM;
		break;
	default:
		return;
	}
	//TODO - only update what is needed
	ctr_set_attr_buffers(count, basevertex);

	if (type != GL_BYTE && type != GL_UNSIGNED_BYTE && type != GL_SHORT) {
		return;
	}
	CTR_VAO *vao = ctr_handle_get(CTR_HANDLE_VAO, ctr_state.bound_vao);
	//printf("%d\n", vao->bound_element_array_buffer);

	//if there is a bound element array buffer then use it
	//TODO use indices as offset
	if (vao->bound_element_array_buffer) {
		CTR_BUFFER *buf = ctr_handle_get(CTR_HANDLE_BUFFER, vao->bound_element_array_buffer);
		if (buf == 0 || buf->data == 0) {
			return;
		}
		indices = buf->data;
	}
	else {
		//this element array is not in gpu memory
		//use buffer space and copy data there
		int cb;
		switch (type) {
		case GL_SHORT:
			cb = count * 2;
			break;
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			cb = count;
			break;
		default:
			//bad data do nothing
			return;
		}
		indices = ctr_rend_buffer_copy(indices, cb);
		if (indices == 0) {
			return;
		}
	}
	//TODO need to translate to short?
	u32 format = 0;
	if (type == GL_SHORT) {
		format = 0x80000000;
	}
#ifdef _3DS
	int mmode;
	int depth;
	matrix_4x4 *mat;

	mmode = GL_MODELVIEW;
	depth = ctr_state.matrix_depth[mmode];
	mat = &ctr_state.matrix[mmode][depth];
	gsSetUniformMatrix(uLoc_modelView, mat->m);
	//GPU_SetFloatUniform(GPU_VERTEX_SHADER, uLoc_modelView, (u32*)mat->m, 4);

	mmode = GL_PROJECTION;
	depth = ctr_state.matrix_depth[mmode];
	mat = &ctr_state.matrix[mmode][depth];
	//GPU_SetFloatUniform(GPU_VERTEX_SHADER, uLoc_projection, (u32*)mat->m, 4);
	gsSetUniformMatrix(uLoc_projection, mat->m);

	//set primitive type
	GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x2, _mode);
	GPUCMD_AddMaskedWrite(GPUREG_RESTART_PRIMITIVE, 0x2, 0x00000001);
	//index buffer
	GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, format | CTR_RENDER_OFFSET(indices));
	//pass number of vertices
	GPUCMD_AddWrite(GPUREG_NUMVERTICES, count);

	GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, 0);

	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 0x2, 0x00000100);
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x2, 0x00000100);

	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 0x1, 0x00000000);
	GPUCMD_AddWrite(GPUREG_DRAWELEMENTS, 0x00000001);
	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 0x1, 0x00000001);
	GPUCMD_AddWrite(GPUREG_VTX_FUNC, 0x00000001);
#endif
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
	glDrawElementsBaseVertex(mode, count, type, indices, 0);
}

void glEnable(GLenum  cap) {
	switch (cap) {
	case GL_TEXTURE_2D:
		ctr_state.texture_units |= (1 << ctr_state.client_texture_current);
		GPU_SetTextureEnable(ctr_state.texture_units);
		break;
	case GL_DEPTH_TEST:
		GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
		break;
	case GL_CULL_FACE:
		GPU_SetFaceCulling(GPU_CULL_NONE);
		break;
	}
}

void glDisable(GLenum  cap) {
	switch (cap) {
	case GL_TEXTURE_2D:
		ctr_state.texture_units &= ~(1 << ctr_state.client_texture_current);
		GPU_SetTextureEnable(ctr_state.texture_units);
		break;
	case GL_DEPTH_TEST:
		GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, GPU_WRITE_ALL);
		break;
	case GL_CULL_FACE:
		GPU_SetFaceCulling(GPU_CULL_NONE);
	}
}


#if 0 //unused
void glEnableVertexAttribArray(GLuint index) {
	if (ctr_state.bound_array_buffer == 0) {
		return;
	}
	CTR_BUFFER *buf = ctr_state.buffers[ctr_state.bound_array_buffer];
	if (buf == 0) {
		return;
	}
}

#endif //end unused



