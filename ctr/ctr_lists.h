#pragma once

enum {
	CTR_HANDLE_VAO = 0,
	CTR_HANDLE_BUFFER = 1,
	CTR_HANDLE_TEXTURE = 2,
	CTR_HANDLE_SHADER = 3,
	CTR_HANDLE_PROGRAM = 4,
	CTR_HANDLE_MAX = 5
};

#define USE_MAP 1

#define CTR_MAX_COUNT 1000

#ifdef __cplusplus
extern "C" {
#endif

void ctr_handle_set(int type, int id, void * ptr);
int ctr_handle_new(int type, void * ptr);
void *ctr_handle_get(int type, int id);
void ctr_handle_remove(int type, int id);

#ifdef __cplusplus
};
#endif