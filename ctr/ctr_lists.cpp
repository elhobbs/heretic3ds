#include <map>
#include "ctr_lists.h"

extern "C" {

#if USE_MAP

	static int ctr_map_next[CTR_HANDLE_MAX] = { 0x10000000, 0x20000000, 0x30000000, 0x40000000, 0x50000000 };
	static std::map <int, void *> ctr_maps[CTR_HANDLE_MAX];

	void ctr_handle_set(int type, int id, void * ptr) {
		switch (type) {
		case CTR_HANDLE_VAO:
		case CTR_HANDLE_BUFFER:
		case CTR_HANDLE_TEXTURE:
		case CTR_HANDLE_PROGRAM:
		case CTR_HANDLE_SHADER:
			ctr_maps[type][id] = ptr;
			break;
		}
	}

	int ctr_handle_new(int type, void * ptr) {
		switch (type) {
		case CTR_HANDLE_VAO:
		case CTR_HANDLE_BUFFER:
		case CTR_HANDLE_TEXTURE:
		case CTR_HANDLE_PROGRAM:
		case CTR_HANDLE_SHADER:
			ctr_maps[type][ctr_map_next[type]] = ptr;
			return ctr_map_next[type]++;
			break;
		}
		return 0;
	}

	void *ctr_handle_get(int type, int id) {
		std::map<int, void *>::iterator it;
		switch (type) {
		case CTR_HANDLE_VAO:
		case CTR_HANDLE_BUFFER:
		case CTR_HANDLE_TEXTURE:
		case CTR_HANDLE_PROGRAM:
		case CTR_HANDLE_SHADER:
			it = ctr_maps[type].find(id);
			if (it != ctr_maps[type].end()) {
				return it->second;
			}
			break;
		}
		return 0;
	}

	void ctr_handle_remove(int type, int id) {
		std::map<int, void *>::iterator it;
		switch (type) {
		case CTR_HANDLE_VAO:
		case CTR_HANDLE_BUFFER:
		case CTR_HANDLE_TEXTURE:
		case CTR_HANDLE_PROGRAM:
		case CTR_HANDLE_SHADER:
			it = ctr_maps[type].find(id);
			if (it != ctr_maps[type].end()) {
				ctr_maps[type].erase(it);
			}
			break;
		}
	}

#else

	static void *ctr_lists[CTR_HANDLE_MAX][CTR_MAX_COUNT] = { 
		{ 0 },
		{ 0 },
		{ 0 },
	};


void ctr_handle_set(int type, int id, void * ptr) {
	switch (type) {
	case CTR_HANDLE_VAO:
	case CTR_HANDLE_BUFFER:
	case CTR_HANDLE_TEXTURE:
	case CTR_HANDLE_PROGRAM:
	case CTR_HANDLE_SHADER:
		ctr_lists[type][id] = ptr;
		break;
	}
}

int __ctr_get_new(int max,void *list[]) {
	int j;

	for (j = 1; j < max; j++) {
		if (list[j] == 0) {
			return j;
		}
	}
	return 0;
}

int ctr_handle_new(int type, void * ptr) {
	int id;
	switch (type) {
	case CTR_HANDLE_VAO:
	case CTR_HANDLE_BUFFER:
	case CTR_HANDLE_TEXTURE:
	case CTR_HANDLE_PROGRAM:
	case CTR_HANDLE_SHADER:
		id = __ctr_get_new(CTR_MAX_COUNT, ctr_lists[type]);
		if(id) {
			ctr_lists[type][id] = ptr;
		}
		return id;
	}
	return 0;
}

void *ctr_handle_get(int type, int id) {
	switch (type) {
	case CTR_HANDLE_VAO:
	case CTR_HANDLE_BUFFER:
	case CTR_HANDLE_TEXTURE:
	case CTR_HANDLE_PROGRAM:
	case CTR_HANDLE_SHADER:
		return ctr_lists[type][id];
	}
	return 0;
}

void ctr_handle_remove(int type, int id) {
	switch (type) {
	case CTR_HANDLE_VAO:
	case CTR_HANDLE_BUFFER:
	case CTR_HANDLE_TEXTURE:
	case CTR_HANDLE_PROGRAM:
	case CTR_HANDLE_SHADER:
		ctr_lists[type][id] = 0;
		break;
	}
}

#endif

};
