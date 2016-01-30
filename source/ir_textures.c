#if 1

#include "DoomDef.h"
#include "ds_textures.h"

int tex_cb_flats = 0;
int tex_cb_sprites = 0;
int tex_cb_walls = 0;

#define USE_GL 1

#ifdef WIN32
#include <windows.h>
#include <gl\gl.h>
#endif

#ifdef _3DS
#include "gl.h"
#endif

extern int		r_framecount;

byte *host_basepal;

byte	ds_blank_texture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0}
};
byte	ds_blank_texture2[8][8] =
{
	{18,18,18,18,18,18,18,18},
	{18,18,18,18,18,18,18,18},
	{18,18,18,18,18,18,18,18},
	{18,18,18,18,18,18,18,18},
	{0,0,0,0,0,0,0,0},
	{18,18,18,18,18,18,18,18},
	{18,18,18,18,18,18,18,18},
	{18,18,18,18,18,18,18,18}
};
dstex_t ds_blank_tex = {66666,8,8,8,8,4,0};



void ds_init_textures()
{
	host_basepal = (byte *)W_CacheLumpName ("PLAYPAL",PU_STATIC);
}

int ds_tex_size(int x)
{
	switch (x)
	{
	case 512:
		return 6;
	case 256:
		return 5;
	case 128:
		return 4;
	case 64:
		return 3;
	case 32:
		return 2;
	case 16:
		return 1;
	case 8:
		return 0;
	}
	return 0;
}

int ds_adjust_size(int x,int max)
{
	if (x > 256 && max > 256)
	{
		return 512;
	}
	if (x > 128 && max > 128)
	{
		return 256;
	}
	if(x > 64 && max > 64)
	{
		return 128;
	}
	if(x > 32 && max > 32)
	{
		return 64;
	}
	if(x > 16 && max > 16)
	{
		return 32;
	}
	if(x > 8 && max > 8)
	{
		return 16;
	}
	return 8;
}

byte* ds_scale_texture(dstex_t *ds,int inwidth,int inheight,byte *in,byte *outt)
{
	int width,height,i,j,bw,bh;
	//int inwidth, inheight;
	unsigned	frac, fracstep;
	byte *inrow,*out;
	//int has255 = 0;
	width = ds->width;
	height = ds->height;

	bw = ds->block_width;
	bh = ds->block_height;

	if(inwidth == bw && inheight == bh)
	{
		return in;
	}

	out = outt;
	fracstep = (inwidth<<16)/width;
	for (i=0 ; i<height ; i++, out += bw)
	{
		inrow = (in + inwidth*(i*inheight/height));
		frac = fracstep >> 1;
		for (j=0 ; j<width ; j++)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
		}
	}
	return outt;
}


int ds_load_texture(dstex_t *ds,byte *buf,int trans,int flags)
{
	//ds_texture_bank_t *bank = &ds_blocks[ds->zone];
	int block,ofs;
	byte *addr;
	int w=ds->block_width, h = ds->block_height;

	if(ds->addr == 0) {
		glGenTextures(1,&ds->addr);
	}
	glBindTexture(GL_TEXTURE_2D, ds->addr);
	//bank->textures[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,0,0);
	{
		unsigned int v, r, g, b;
		unsigned int *dest32 = malloc(256 * 256 * 4);
		int i;
		addr = buf;
		for(i=0;i<(w*h);i++) {
			r = host_basepal[addr[i]*3 + 0];
			g = host_basepal[addr[i]*3 + 1];
			b = host_basepal[addr[i]*3 + 2];
			if(trans) {
				v = ((addr[i] == 255 ? 0 : 255)<<24) + (r<<0) + (g<<8) + (b<<16);
			} else {
				v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			}
			dest32[i] = v;
		}
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		free(dest32);
	}

	return ds->addr;
}

byte *R_GetColumn (int tex, int col);
extern short		**texturecolumnlump;
void R_DrawColumnInCacheGL
( column_t*	patch,
  byte*		cache,
  //byte*		mask,
  int		originy,
  int		cacheheight );

int ds_load_sky_texture(int name,int flags)
{
	column_t *column;
	texture_t		*texture = textures[name];
	dstex_t			*ds = &textures_ds[name];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,mask[512],col[512];

	if (ds->addr) {
		glBindTexture(GL_TEXTURE_2D, ds->addr);
		return ds->addr;
	}

	size = texture->width * texture->height;
	w = texture->width;
	h = texture->height;
	ds_size = ds->block_width * ds->block_height;

	tex_cb_walls += ds_size;

	src = (byte *)Z_Malloc (size,PU_STATIC,0);
	dst = (byte *)Z_Malloc (ds_size,PU_STATIC,0);
	memset(dst,0xff,ds_size);

	for(i=0;i<w;i++)
	{
		scol = R_GetColumn(name,i)+72;
		dcol = &src[i];
		for(j=0;j<h;j++)
		{
			*dcol = *scol++;
			dcol += w;
		}
	}
	addr = ds_scale_texture(ds,w,h,src,dst);
	ds->addr = texnum = ds_load_texture(ds,addr,1,flags|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
	Z_Free(dst);
	Z_Free(src);

	return texnum;
}

int ds_load_map_texture(int name,int flags)
{
	column_t *column;
	texture_t		*texture = textures[name];
	dstex_t			*ds = &textures_ds[name];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,mask[512],col[512];

	if (ds->addr) {
		glBindTexture(GL_TEXTURE_2D, ds->addr);
		return ds->addr;
	}

 	size = texture->width * texture->height;
	w = texture->width;
	h = texture->height;
	ds_size = ds->block_width * ds->block_height;

	tex_cb_walls += ds_size;

	src = (byte *)Z_Malloc (size,PU_STATIC,0);
	dst = (byte *)Z_Malloc (ds_size,PU_STATIC,0);
	memset(dst,0xff,ds_size);

	for(i=0;i<w;i++)
	{
		if(name != skytexture && texturecolumnlump[name][i] > 0) {
			memset(col,0xff,h);
			column = (column_t *)((byte *)R_GetColumn(name,i) - 3);
			R_DrawColumnInCacheGL(column,col,0,texture->height);
			scol = &col[0];
		} else {
			scol = R_GetColumn(name,i);
		}
		dcol = &src[i];
		for(j=0;j<h;j++)
		{
			*dcol = *scol++;
			dcol += w;
		}
	}
	addr = ds_scale_texture(ds,w,h,src,dst);
	ds->addr = texnum = ds_load_texture(ds, addr, 1, flags | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);
	Z_Free(dst);
	Z_Free(src);

	return texnum;
}

int ds_load_map_flat(int name)
{
	//texture_t		*texture = textures[name];
	dstex_t			*ds = &flats_ds[name];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,mask[512],col[512];

	if (ds->addr) {
		glBindTexture(GL_TEXTURE_2D, ds->addr);
		return ds->addr;
	}

	size = 64 * 64;
	w = 64;
	h = 64;
	ds_size = ds->block_width * ds->block_height;

	tex_cb_flats += ds_size;

	src = (byte *)W_CacheLumpNum(firstflat + name, PU_STATIC);
	dst = (byte *)Z_Malloc (ds_size, PU_STATIC, 0);

	addr = ds_scale_texture(ds,w,h,src,dst);
	texnum = ds_load_texture(ds, addr, 0, GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);
	Z_Free(src);
	Z_Free(dst);

	return texnum;
}

void
R_DrawColumnInCacheGL
( column_t*	patch,
  byte*		cache,
  //byte*		mask,
  int		originy,
  int		cacheheight )
{
    int		count;
    int		position;
    byte*	source;
    byte*	dest;
	
    dest = (byte *)cache + 3;
	
    while (patch->topdelta != 0xff)
    {
		source = (byte *)patch + 3;
		count = patch->length;
		position = originy + patch->topdelta;

		if (position < 0)
		{
			count += position;
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		if (count > 0)
		{
			memcpy (cache + position, source, count);
			//memset (mask + position, -1, count);
		}
		
		patch = (column_t *)(  (byte *)patch + patch->length + 4); 
    }
}

extern fixed_t		*spritewidth;		// needed for pre rendering
extern fixed_t		*spriteheight;		// needed for pre rendering
extern dstex_t		*sprites_ds;
extern int		firstspritelump, lastspritelump, numspritelumps;


int ds_load_sprite(int name)
{
	column_t *column;
	patch_t *patch = (patch_t *)W_CacheLumpNum (firstspritelump+name, PU_CACHE);
	dstex_t			*ds = &sprites_ds[name];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,col[512];

	if (ds->addr) {
		glBindTexture(GL_TEXTURE_2D, ds->addr);
		return ds->addr;
	}

	w = spritewidth[name]>>FRACBITS;
	h = spriteheight[name]>>FRACBITS;
	size = w * h;
	size = (size+3)&(~3L);
	ds_size = ds->block_width * ds->block_height;

	tex_cb_sprites += ds_size;

	src = (byte *)Z_Malloc (size+16,PU_STATIC,0);
	dst = (byte *)Z_Malloc (ds_size+16,PU_STATIC,0);
	//dst = src + size;
	memset(dst,0,ds_size);

	for(i=0;i<w;i++)
	{
		memset(col,0xff,h);
		column = (column_t *) ((byte *)patch + (patch->columnofs[i]));
		R_DrawColumnInCacheGL(column,col,0,patch->height);
		scol = &col[0];
		dcol = &src[i];
		for(j=0;j<h;j++)
		{
			*dcol = *scol++;
			dcol += w;
		}
	}
	addr = ds_scale_texture(ds,w,h,src,dst);
	texnum = ds_load_texture(ds, addr, 1, GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);
	Z_Free(src);
	Z_Free(dst);

	return texnum;
}

void ds_init_texture(int name,texture_t *tx,dstex_t *ds)
{
	int max[2];
	ds->block = -1;
	ds->name = name;
	/*if(tx->width <= 64 && tx->height <= 64)
	{
		ds->zone = 0;
		max[0] = max[1] = 64;
	}
	else if(tx->width > 128 || tx->height > 128)
	{
		ds->zone = 2;
		max[0] = 256;
		max[1] = 128;
	}
	else
	{
		ds->zone = 1;
		max[0] = 128;
		max[1] = 64;
	}*/

	max[1] = max[0] = 512;

	ds->width = ds_adjust_size(tx->width,max[0]);
	ds->height = ds_adjust_size(tx->height,max[0]);
	if(tx->width >= tx->height && ds->height > max[1])
	{
		ds->height = max[1];
	}
	else if(tx->height >= tx->width && ds->width > max[1])
	{
		ds->width = max[1];
	}
	ds->block_width = ds->width;
	ds->block_height = ds->height;
	ds->width_scale = (ds->width<<16)/tx->width;
	ds->height_scale = (ds->height<<16)/tx->height;
}
void ds_init_flat(int name,dstex_t *ds)
{
	ds->block = -1;
	ds->name = name;
	ds->zone = 0;

	ds->block_width = ds->width = 64;
	ds->block_height = ds->height = 64;
}
void ds_init_sprite(int name,patch_t *patch,dstex_t *ds)
{
	int max[2];
	ds->block = -1;
	ds->name = name+firstspritelump;
	/*if(patch->width <= 32 && patch->height <= 32)
	{
		ds->zone = 3;
		max[0] = max[1] = 32;
	}
	else if(patch->width <= 64 && patch->height <= 64)
	{
		ds->zone = 0;
		max[0] = max[1] = 64;
	}
	else if(patch->width > 128 || patch->height > 128)
	{
		ds->zone = 2;
		max[0] = 256;
		max[1] = 128;
	}
	else
	{
		ds->zone = 1;
		max[0] = 128;
		max[1] = 64;
	}*/
	max[1] = max[0] = 512;

	ds->block_width = ds_adjust_size(patch->width,max[0]);
	ds->block_height = ds_adjust_size(patch->height,max[0]);
	if(patch->width >= patch->height && ds->block_height > max[1])
	{
		ds->block_height = max[1];
	}
	else if(patch->height >= patch->width && ds->block_width > max[1])
	{
		ds->block_width = max[1];
	}
	ds->width = MIN(patch->width,ds->block_width);
	ds->height = MIN(patch->height,ds->block_height);
}

int ds_load_blank_tex()
{
	return 0;
}

#endif