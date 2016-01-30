#include "ctr_rend.h"
#include <stdlib.h>
#ifdef _3DS
#include <3ds/linear.h>
#endif

#define CTR_REND_BUFFER_MAX (2 * 1024 * 1024)
static GLubyte *ctr_buffer = 0;
static int ctr_buffer_frame = 0;

void ctr_rend_buffer_init() {
	ctr_state.buffer = ctr_buffer = linearAlloc(CTR_REND_BUFFER_MAX * 2);
	if (ctr_state.buffer == 0) {
		printf("-------------\n\nctr_rend_buffer_copy_stride Out of Memory\n\n-------------\n");
		while (1);
	}
	ctr_state.buffer_len = CTR_REND_BUFFER_MAX;
	ctr_state.buffer_pos = 0;
	ctr_buffer_frame = 0;
}

void* ctr_rend_buffer_alloc(int len) {
	int cb = (len + 127) & (~127);
	if (ctr_state.buffer_pos + cb > CTR_REND_BUFFER_MAX) {
		printf("-------------\n\nctr_rend_buffer_alloc Out of Memory\n\n-------------\n");
		while (1);
		return 0;
	}
	GLubyte *addr = &ctr_state.buffer[ctr_state.buffer_pos];
	ctr_state.buffer_pos += cb;
	return addr;
}

void* ctr_rend_buffer_copy(void *p, int len) {
	int cb = (len + 127) & ( ~127 );
	if (ctr_state.buffer_pos + cb > CTR_REND_BUFFER_MAX) {
		printf("-------------\n\nctr_rend_buffer_copy Out of Memory\n\n-------------\n");
		while (1);
		return 0;
	}
	GLubyte *addr = &ctr_state.buffer[ctr_state.buffer_pos];
	memcpy(addr, p, len);
	ctr_state.buffer_pos += cb;

	return addr;
}

void* ctr_rend_buffer_copy_stride(void *p, int count, int size, int stride) {
	int len = size * count;
	int cb = (len + 15) & (~15);
	if (ctr_state.buffer_pos + cb > CTR_REND_BUFFER_MAX) {
		printf("-------------\n\nctr_rend_buffer_copy_stride Out of Memory\n\n-------------\n");
		while (1);
		return 0;
	}
	GLubyte *addr = &ctr_state.buffer[ctr_state.buffer_pos];
	u8 *src = p;
	u8 *dst = addr;
	int i,j;
	//printf("copy: %08x %d %d %d\n", p, count, size, stride);
	for (i = 0; i < count; i++) {
		memcpy(dst, src, size);
		//for (j = 0; j < size; j++) {
		//	printf("%02x", src[j]);
		//}
		//printf("%s", (i&1) ? "\n" : " ");
		src += stride;
		dst += size;
	}
	//if(i&1) printf("\n");
	ctr_state.buffer_pos += cb;

	return addr;
}

void ctr_rend_buffer_reset() {
	//printf("ctr_state.buffer_pos: %8d %10d\n", ctr_state.buffer_pos, linearSpaceFree());
	ctr_state.buffer_pos = 0;
	ctr_buffer_frame = ctr_buffer_frame ? 0 : 1;
	ctr_state.buffer = ctr_buffer;// +ctr_buffer_frame * CTR_REND_BUFFER_MAX;
}