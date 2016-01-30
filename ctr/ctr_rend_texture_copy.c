#include "ctr_rend.h"

u32 morton_lut[] = {
	0, 1, 4, 5, 16, 17, 20, 21,
	2, 3, 6, 7, 18, 19, 22, 23,
	8, 9, 12, 13, 24, 25, 28, 29,
	10, 11, 14, 15, 26, 27, 30, 31,
	32, 33, 36, 37, 48, 49, 52, 53,
	34, 35, 38, 39, 50, 51, 54, 55,
	40, 41, 44, 45, 56, 57, 60, 61,
	42, 43, 46, 47, 58, 59, 62, 63,
};

#define RGBA5551(r,g,b,a) (\
(((r>>3) & 0x1F) << 11)\
| (((g>>3) & 0x1F) << 6)\
| (((b>>3) & 0x1F) << 1)\
| (a))\

#define RGBA4444(r,g,b,a) (\
(((r>>4) & 0xF) << 12)\
| (((g>>4) & 0xF) << 8)\
| (((b>>4) & 0xF) << 4)\
| ((a) & 0xF))\

#define copy_rgb_5551(_d,_s) \
		((u16 *)_d)[z] = RGBA5551(_s[0],_s[1],_s[2],1); \
		_s+=3;

#define copy_rgba_5551(_d,_s) \
		((u16 *)_d)[z] = RGBA5551(_s[0],_s[1],_s[2],_s[3]); \
		_s+=3;

#define copy_rgba_4444(_d,_s) \
		((u16 *)_d)[z] = RGBA4444(_s[0],_s[1],_s[2],_s[3]); \
		_s+=4;

#define copy_rgb_rgb(_d,_s) \
		_d[z + 2] = *_s++; \
		_d[z + 1] = *_s++; \
		_d[z + 0] = *_s++;

#define copy_rgb_rgba(_d,_s) \
		_d[z + 3] = 255; \
		_d[z + 2] = *_s++; \
		_d[z + 1] = *_s++; \
		_d[z + 0] = *_s++;

#define copy_rgba_rgba(_d,_s) \
		_d[z + 3] = *_s++; \
		_d[z + 2] = *_s++; \
		_d[z + 1] = *_s++; \
		_d[z + 0] = *_s++;

#define copy_8_8(_d,_s) \
		_d[z] = *_s++;

void copy_tex_sub_tex_rgb_rgb(u8 *dst, int dst_width, int dst_height, int src_x, int src_y, int dst_x, int dst_y, u8 *src, int src_width) {
	u8 *src_blk;
	u8 *dst_blk;
	int dst_x_blk;
	int dst_y_blk;
	int z;

	//flip dst_y
	dst_y = dst_height - 1 - dst_y;

	//start of dst block
	dst_x_blk = dst_x & ~7;
	dst_y_blk = dst_y / 8;

	src_blk = &src[(src_y * src_width + src_x) * 3];
	dst_blk = &dst[(dst_y_blk * dst_width + dst_x_blk) * 3 * 8];
	z = 3 * morton_lut[((dst_y & 7) << 3) | (dst_x & 7)];
	copy_rgb_rgb(dst_blk, src_blk);
}

void copy_tex_sub_rgb_rgb(CTR_TEXTURE *dst, u8 *src, int x, int y, int width, int height) {
	int w, h;
	u8 *dst_data = dst->data;
	int dst_width = dst->width;
	int dst_height = dst->height;
	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			copy_tex_sub_tex_rgb_rgb(dst_data, dst_width, dst_height, w, h, x + w, y + h, src, width);
		}
	}
}

void copy_tex_sub_tex_rgba_rgba(u8 *dst, int dst_width, int dst_height, int src_x, int src_y, int dst_x, int dst_y, u8 *src, int src_width) {
	u8 *src_blk;
	u8 *dst_blk;
	int dst_x_blk;
	int dst_y_blk;
	int z;

	//flip dst_y
	dst_y = dst_height - 1 - dst_y;

	//start of dst block
	dst_x_blk = dst_x & ~7;
	dst_y_blk = dst_y / 8;

	src_blk = &src[(src_y * src_width + src_x) * 4];
	dst_blk = &dst[(dst_y_blk * dst_width + dst_x_blk) * 4 * 8];
	z = 4 * morton_lut[((dst_y & 7) << 3) | (dst_x & 7)];
	copy_rgba_rgba(dst_blk, src_blk);
}

void copy_tex_sub_rgba_rgba(CTR_TEXTURE *dst, u8 *src, int x, int y, int width, int height) {
	int w, h;
	u8 *dst_data = dst->data;
	int dst_width = dst->width;
	int dst_height = dst->height;
	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			copy_tex_sub_tex_rgba_rgba(dst_data, dst_width, dst_height, w, h, x + w, y + h, src, width);
		}
	}
}

void copy_tex_sub_tex_rgb_5551(u8 *dst, int dst_width, int dst_height, int src_x, int src_y, int dst_x, int dst_y, u8 *src, int src_width) {
	u8 *src_blk;
	u8 *dst_blk;
	int dst_x_blk;
	int dst_y_blk;
	int z;

	//flip dst_y
	dst_y = dst_height - 1 - dst_y;

	//start of dst block
	dst_x_blk = dst_x & ~7;
	dst_y_blk = dst_y / 8;

	src_blk = &src[(src_y * src_width + src_x) * 3];
	dst_blk = &dst[(dst_y_blk * dst_width + dst_x_blk) * 2 * 8];
	z = morton_lut[((dst_y & 7) << 3) | (dst_x & 7)];
	copy_rgb_5551(dst_blk, src_blk);
}

void copy_tex_sub_rgb_5551(CTR_TEXTURE *dst, u8 *src, int x, int y, int width, int height) {
	int w, h;
	u8 *dst_data = dst->data;
	int dst_width = dst->width;
	int dst_height = dst->height;
	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			copy_tex_sub_tex_rgb_5551(dst_data, dst_width, dst_height, w, h, x + w, y + h, src, width);
		}
	}
}

void copy_tex_sub_tex_8_8(u8 *dst, int dst_width, int dst_height, int src_x, int src_y, int dst_x, int dst_y, u8 *src, int src_width) {
	u8 *src_blk;
	u8 *dst_blk;
	int dst_x_blk;
	int dst_y_blk;
	int z;

	//flip dst_y
	dst_y = dst_height - 1 - dst_y;

	//start of dst block
	dst_x_blk = dst_x & ~7;
	dst_y_blk = dst_y / 8;

	src_blk = &src[(src_y * src_width + src_x)];
	dst_blk = &dst[(dst_y_blk * dst_width + dst_x_blk) * 8];
	z = morton_lut[((dst_y & 7) << 3) | (dst_x & 7)];
	copy_8_8(dst_blk, src_blk);
}

void copy_tex_sub_8_8(CTR_TEXTURE *dst, u8 *src, int x, int y, int width, int height) {
	int w, h;
	u8 *dst_data = dst->data;
	int dst_width = dst->width;
	int dst_height = dst->height;
	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			copy_tex_sub_tex_8_8(dst_data, dst_width, dst_height, w, h, x + w, y + h, src, width);
		}
	}
}

#if 1
u32 morton_lut_flip[] = {
	42, 43, 46, 47, 58, 59, 62, 63,
	40, 41, 44, 45, 56, 57, 60, 61,
	34, 35, 38, 39, 50, 51, 54, 55,
	32, 33, 36, 37, 48, 49, 52, 53,
	10, 11, 14, 15, 26, 27, 30, 31,
	8, 9, 12, 13, 24, 25, 28, 29,
	2, 3, 6, 7, 18, 19, 22, 23,
	0, 1, 4, 5, 16, 17, 20, 21,
};


void copy_tex_block_rgb_rgb(u8 *src, u8 *_dst, int stride) {
	int x, y, z;
	u8 dst[64 * 3];
	for (y = 0; y < 8; y++) {
		u8 *row_src = src;

		//0
		z = morton_lut_flip[(y << 3) | 0] * 3;
		copy_rgb_rgb(dst, row_src);

		//1
		z = morton_lut_flip[(y << 3) | 1] * 3;
		copy_rgb_rgb(dst, row_src);

		//2
		z = morton_lut_flip[(y << 3) | 2] * 3;
		copy_rgb_rgb(dst, row_src);

		//3
		z = morton_lut_flip[(y << 3) | 3] * 3;
		copy_rgb_rgb(dst, row_src);

		//4
		z = morton_lut_flip[(y << 3) | 4] * 3;
		copy_rgb_rgb(dst, row_src);

		//5
		z = morton_lut_flip[(y << 3) | 5] * 3;
		copy_rgb_rgb(dst, row_src);

		//6
		z = morton_lut_flip[(y << 3) | 6] * 3;
		copy_rgb_rgb(dst, row_src);

		//7
		z = morton_lut_flip[(y << 3) | 7] * 3;
		copy_rgb_rgb(dst, row_src);

		src += stride;
	}
	memcpy(_dst, dst, 64 * 3);
}

void copy_tex_rgb_rgb(CTR_TEXTURE *dst, u8 *src, int width, int height) {
	int x, y;
	u8 *dst_data = dst->data;
	u8 *src_row = &src[((height - 8) * width) * 3];
	for (y = height - 8; y >= 0; y -= 8) {
		for (x = 0; x < width; x += 8) {
			copy_tex_block_rgb_rgb(src_row + x * 3, dst_data, width * 3);
			dst_data += (64 * 3);
		}
		src_row -= (8 * width * 3);
	}
}

void copy_tex_block_rgba_rgba(u8 *src, u8 *_dst, int stride) {
	int x, y, z;
	u8 dst[64 * 4];
	for (y = 0; y < 8; y++) {
		u8 *row_src = src;

		//0
		z = morton_lut_flip[(y << 3) | 0] * 4;
		copy_rgba_rgba(dst, row_src);

		//1
		z = morton_lut_flip[(y << 3) | 1] * 4;
		copy_rgba_rgba(dst, row_src);

		//2
		z = morton_lut_flip[(y << 3) | 2] * 4;
		copy_rgba_rgba(dst, row_src);

		//3
		z = morton_lut_flip[(y << 3) | 3] * 4;
		copy_rgba_rgba(dst, row_src);

		//4
		z = morton_lut_flip[(y << 3) | 4] * 4;
		copy_rgba_rgba(dst, row_src);

		//5
		z = morton_lut_flip[(y << 3) | 5] * 4;
		copy_rgba_rgba(dst, row_src);

		//6
		z = morton_lut_flip[(y << 3) | 6] * 4;
		copy_rgba_rgba(dst, row_src);

		//7
		z = morton_lut_flip[(y << 3) | 7] * 4;
		copy_rgba_rgba(dst, row_src);

		src += stride;
	}
	memcpy(_dst, dst, 64 * 4);
}

void copy_tex_rgba_rgba(CTR_TEXTURE *dst, u8 *src, int width, int height) {
	int x, y;
	u8 *dst_data = dst->data;
	u8 *src_row = &src[((height - 8) * width) * 4];
	for (y = height - 8; y >= 0; y -= 8) {
		for (x = 0; x < width; x += 8) {
			copy_tex_block_rgba_rgba(src_row + x * 4, dst_data, width * 4);
			dst_data += (64 * 4);
		}
		src_row -= (8 * width * 4);
	}
}
#endif

void copy_tex_block_rgb_5551(u8 *src, u8 *_dst, int stride) {
	int x, y, z;
	u8 dst[64 * 2];
	for (y = 0; y < 8; y++) {
		u8 *row_src = src;

		//0
		z = morton_lut_flip[(y << 3) | 0];
		copy_rgb_5551(dst, row_src);

		//1
		z = morton_lut_flip[(y << 3) | 1];
		copy_rgb_5551(dst, row_src);

		//2
		z = morton_lut_flip[(y << 3) | 2];
		copy_rgb_5551(dst, row_src);

		//3
		z = morton_lut_flip[(y << 3) | 3];
		copy_rgb_5551(dst, row_src);

		//4
		z = morton_lut_flip[(y << 3) | 4];
		copy_rgb_5551(dst, row_src);

		//5
		z = morton_lut_flip[(y << 3) | 5];
		copy_rgb_5551(dst, row_src);

		//6
		z = morton_lut_flip[(y << 3) | 6];
		copy_rgb_5551(dst, row_src);

		//7
		z = morton_lut_flip[(y << 3) | 7];
		copy_rgb_5551(dst, row_src);

		src += stride;
	}
	memcpy(_dst, dst, 64 * 2);
}

void copy_tex_rgb_5551(CTR_TEXTURE *dst, u8 *src, int width, int height) {
	int x, y;
	u8 *dst_data = dst->data;
	u8 *src_row = &src[((height - 8) * width) * 3];
	for (y = height - 8; y >= 0; y -= 8) {
		for (x = 0; x < width; x += 8) {
			copy_tex_block_rgb_5551(src_row + x * 3, dst_data, width * 3);
			dst_data += (64 * 2);
		}
		src_row -= (8 * width * 3);
	}
}

void copy_tex_block_rgba_5551(u8 *src, u8 *_dst, int stride) {
	int x, y, z;
	u8 dst[64 * 2];
	for (y = 0; y < 8; y++) {
		u8 *row_src = src;

		//0
		z = morton_lut_flip[(y << 3) | 0];
		copy_rgba_5551(dst, row_src);

		//1
		z = morton_lut_flip[(y << 3) | 1];
		copy_rgba_5551(dst, row_src);

		//2
		z = morton_lut_flip[(y << 3) | 2];
		copy_rgba_5551(dst, row_src);

		//3
		z = morton_lut_flip[(y << 3) | 3];
		copy_rgba_5551(dst, row_src);

		//4
		z = morton_lut_flip[(y << 3) | 4];
		copy_rgba_5551(dst, row_src);

		//5
		z = morton_lut_flip[(y << 3) | 5];
		copy_rgba_5551(dst, row_src);

		//6
		z = morton_lut_flip[(y << 3) | 6];
		copy_rgba_5551(dst, row_src);

		//7
		z = morton_lut_flip[(y << 3) | 7];
		copy_rgba_5551(dst, row_src);

		src += stride;
	}
	memcpy(_dst, dst, 64 * 2);
}

void copy_tex_rgba_5551(CTR_TEXTURE *dst, u8 *src, int width, int height) {
	int x, y;
	u8 *dst_data = dst->data;
	u8 *src_row = &src[((height - 8) * width) * 4];
	for (y = height - 8; y >= 0; y -= 8) {
		for (x = 0; x < width; x += 8) {
			copy_tex_block_rgba_5551(src_row + x * 4, dst_data, width * 3);
			dst_data += (64 * 2);
		}
		src_row -= (8 * width * 4);
	}
}

void copy_tex_block_rgba_4444(u8 *src, u8 *_dst, int stride) {
	int x, y, z;
	u8 dst[64 * 2];
	for (y = 0; y < 8; y++) {
		u8 *row_src = src;

		//0
		z = morton_lut_flip[(y << 3) | 0];
		copy_rgba_4444(dst, row_src);

		//1
		z = morton_lut_flip[(y << 3) | 1];
		copy_rgba_4444(dst, row_src);

		//2
		z = morton_lut_flip[(y << 3) | 2];
		copy_rgba_4444(dst, row_src);

		//3
		z = morton_lut_flip[(y << 3) | 3];
		copy_rgba_4444(dst, row_src);

		//4
		z = morton_lut_flip[(y << 3) | 4];
		copy_rgba_4444(dst, row_src);

		//5
		z = morton_lut_flip[(y << 3) | 5];
		copy_rgba_4444(dst, row_src);

		//6
		z = morton_lut_flip[(y << 3) | 6];
		copy_rgba_4444(dst, row_src);

		//7
		z = morton_lut_flip[(y << 3) | 7];
		copy_rgba_4444(dst, row_src);

		src += stride;
	}
	memcpy(_dst, dst, 64 * 2);
}

void copy_tex_rgba_4444(CTR_TEXTURE *dst, u8 *src, int width, int height) {
	int x, y;
	u8 *dst_data = dst->data;
	u8 *src_row = &src[((height - 8) * width) * 4];
	for (y = height - 8; y >= 0; y -= 8) {
		for (x = 0; x < width; x += 8) {
			copy_tex_block_rgba_4444(src_row + x * 4, dst_data, width * 3);
			dst_data += (64 * 2);
		}
		src_row -= (8 * width * 4);
	}
}

void copy_tex_block_8_8(u8 *src, u8 *_dst, int stride) {
	int x, y, z;
	u8 dst[64];
	for (y = 0; y < 8; y++) {
		u8 *row_src = src;

		//0
		z = morton_lut_flip[(y << 3) | 0];
		copy_8_8(dst, row_src);

		//1
		z = morton_lut_flip[(y << 3) | 1];
		copy_8_8(dst, row_src);

		//2
		z = morton_lut_flip[(y << 3) | 2];
		copy_8_8(dst, row_src);

		//3
		z = morton_lut_flip[(y << 3) | 3];
		copy_8_8(dst, row_src);

		//4
		z = morton_lut_flip[(y << 3) | 4];
		copy_8_8(dst, row_src);

		//5
		z = morton_lut_flip[(y << 3) | 5];
		copy_8_8(dst, row_src);

		//6
		z = morton_lut_flip[(y << 3) | 6];
		copy_8_8(dst, row_src);

		//7
		z = morton_lut_flip[(y << 3) | 7];
		copy_8_8(dst, row_src);

		src += stride;
	}
	memcpy(_dst, dst, 64);
}

void copy_tex_8_8(CTR_TEXTURE *dst, u8 *src, int width, int height) {
	int x, y;
	u8 *dst_data = dst->data;
	u8 *src_row = &src[((height - 8) * width)];
	for (y = height - 8; y >= 0; y -= 8) {
		for (x = 0; x < width; x += 8) {
			copy_tex_block_8_8(src_row + x, dst_data, width);
			dst_data += 64;
		}
		src_row -= (8 * width);
	}
}