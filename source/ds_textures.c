#if 0

#include "DoomDef.h"
#include "ds_textures.h"

int tex_cb_flats = 0;
int tex_cb_sprites = 0;
int tex_cb_walls = 0;

#define USE_GL 1

#ifdef WIN32
#include <windows.h>
#include <gl\gl.h>
#define TEXGEN_TEXCOORD 0
#endif

#define TEXTURE_RESIDENT 1

#define TEXTURE_BANK_SIZE (256*256*2)

//DS_TEXTURE_BASE2 = (byte*)VRAM_A;
//DS_TEXTURE_BASE3 = (byte *)VRAM_C;
//DS_TEXTURE_BASE = DS_TEXTURE_BASE3 + (NUM_TEXTURE_BLOCKS3*TEXTURE_BLOCK_SIZE3);

//#define NUM_TEXTURE_BLOCKS 16 //we are going to use the last one for a few small things
#define NUM_TEXTURE_BLOCKS 40
#define TEXTURE_BLOCK_SIZE (64*64)
#define TEXTURE_PER_BANK (TEXTURE_BANK_SIZE/TEXTURE_BLOCK_SIZE)
#define TEXTURE_SIZE (NUM_TEXTURE_BLOCKS*TEXTURE_BLOCK_SIZE)

//256k
#define NUM_TEXTURE_BLOCKS2 16
#define TEXTURE_BLOCK_SIZE2 (128*64)
#define TEXTURE_PER_BANK2 (TEXTURE_BANK_SIZE/TEXTURE_BLOCK_SIZE2)
#define TEXTURE_SIZE2 (NUM_TEXTURE_BLOCKS2*TEXTURE_BLOCK_SIZE2)

#define NUM_TEXTURE_BLOCKS3 6
#define TEXTURE_BLOCK_SIZE3 (256*128)
#define TEXTURE_PER_BANK3 (TEXTURE_BANK_SIZE/TEXTURE_BLOCK_SIZE3)
#define TEXTURE_SIZE3 (NUM_TEXTURE_BLOCKS3*TEXTURE_BLOCK_SIZE3)

#define NUM_TEXTURE_BLOCKS4 32
#define TEXTURE_BLOCK_SIZE4 (32*32)
#define TEXTURE_PER_BANK4 (TEXTURE_BANK_SIZE/TEXTURE_BLOCK_SIZE4)
#define TEXTURE_SIZE4 (NUM_TEXTURE_BLOCKS4*TEXTURE_BLOCK_SIZE4)

#define NUM_TEXTURE_BLOCKS5 40
#define TEXTURE_BLOCK_SIZE5 (8*8)
#define TEXTURE_PER_BANK5 (TEXTURE_BANK_SIZE/TEXTURE_BLOCK_SIZE5)
#define TEXTURE_SIZE5 (NUM_TEXTURE_BLOCKS5*TEXTURE_BLOCK_SIZE5)



extern int		r_framecount;

byte *DS_TEXTURE_BASE;
byte *DS_TEXTURE_BASE2;
byte *DS_TEXTURE_BASE3;
byte *DS_TEXTURE_BASE4;
byte *DS_TEXTURE_BASE5;

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


#define DS_TEXTURE_MEM_SIZE (256*256*2*4)

ds_texture_t ds_textures[NUM_TEXTURE_BLOCKS];
ds_texture_t ds_textures2[NUM_TEXTURE_BLOCKS2];
ds_texture_t ds_textures3[NUM_TEXTURE_BLOCKS3];
ds_texture_t ds_textures4[NUM_TEXTURE_BLOCKS4];
ds_texture_t ds_textures5[NUM_TEXTURE_BLOCKS5];

byte ds_texture_cache[256*256*2*2];
int		ds_texture_cache_offset = 0;
int		ds_texture_cache_frame = 0;
byte	*ds_texture_cache_base;

void ds_texture_cache_load() {

#ifdef ARM9
	DC_FlushAll();

	if(ds_texture_cache_frame == 0) {
		vramSetBankA(VRAM_A_LCD);
		vramSetBankB(VRAM_B_LCD);
		dmaCopyWords(0,(uint32*)ds_texture_cache, (uint32*)ds_texture_cache_base, ds_texture_cache_offset);
		vramSetBankA(VRAM_A_TEXTURE);
		vramSetBankB(VRAM_B_TEXTURE);
	} else {
		vramSetBankC(VRAM_C_LCD);
		vramSetBankD(VRAM_D_LCD);
		dmaCopyWords(0,(uint32*)ds_texture_cache, (uint32*)ds_texture_cache_base, ds_texture_cache_offset);
		vramSetBankC(VRAM_C_TEXTURE);
		vramSetBankD(VRAM_D_TEXTURE);
	}
#endif

	ds_texture_cache_base = DS_TEXTURE_BASE + (ds_texture_cache_frame == 0 ? (256*256*2*2) : 0);
	ds_texture_cache_frame = !ds_texture_cache_frame;
	ds_texture_cache_offset = 0;
}

void ds_texture_cache_lock() {

#ifdef ARM9
	if(ds_texture_cache_frame == 0) {
		vramSetBankA(VRAM_A_TEXTURE);
		vramSetBankB(VRAM_B_TEXTURE);
	} else {
		vramSetBankC(VRAM_C_TEXTURE);
		vramSetBankD(VRAM_D_TEXTURE);
	}
#endif

}

void ds_texture_cache_unlock() {

#ifdef ARM9
	if(ds_texture_cache_frame == 0) {
		vramSetBankC(VRAM_C_LCD);
		vramSetBankD(VRAM_D_LCD);
	} else {
		vramSetBankA(VRAM_A_LCD);
		vramSetBankB(VRAM_B_LCD);
	}
#endif

	ds_texture_cache_base = DS_TEXTURE_BASE + (ds_texture_cache_frame == 0 ? (256*256*2*2) : 0);
	ds_texture_cache_frame = !ds_texture_cache_frame;
	ds_texture_cache_offset = 0;
}

typedef struct ds_texture_bank_s
{
	byte			*start;
	int				num_blocks;
	int				blocks_per_bank;
	int				block_size;
	int				last_free;
	ds_texture_t	*textures;
#ifdef WIN32
	int				*texnums;
#endif
} ds_texture_bank_t;

ds_texture_bank_t ds_blocks[5];

#ifdef WIN32
int texnums[NUM_TEXTURE_BLOCKS];
int texnums2[NUM_TEXTURE_BLOCKS2];
int texnums3[NUM_TEXTURE_BLOCKS3];
int texnums4[NUM_TEXTURE_BLOCKS4];
int texnums5[NUM_TEXTURE_BLOCKS5];
float ds_texture_width = 64.0f;
float ds_texture_height = 64.0f;
#endif

void ds_init_textures()
{
	host_basepal = (byte *)W_CacheLumpName ("PLAYPAL",PU_STATIC);
#ifdef WIN32
	DS_TEXTURE_BASE = (byte*)malloc(DS_TEXTURE_MEM_SIZE);
#endif

#ifdef ARM9
	DS_TEXTURE_BASE = (byte*)VRAM_A;
#endif

	ds_texture_cache_base = DS_TEXTURE_BASE;

	DS_TEXTURE_BASE2 = DS_TEXTURE_BASE + TEXTURE_SIZE;
	DS_TEXTURE_BASE3 = DS_TEXTURE_BASE2 + TEXTURE_SIZE2;
	DS_TEXTURE_BASE4 = DS_TEXTURE_BASE3 + TEXTURE_SIZE3;
	DS_TEXTURE_BASE5 = DS_TEXTURE_BASE4 + TEXTURE_SIZE4;


	memset(ds_textures,0,sizeof(ds_textures));
	memset(ds_textures2,0,sizeof(ds_textures2));
	memset(ds_textures3,0,sizeof(ds_textures3));
	memset(ds_textures4,0,sizeof(ds_textures4));

#ifdef WIN32
	memset(texnums,0,sizeof(texnums));
	memset(texnums2,0,sizeof(texnums2));
	memset(texnums3,0,sizeof(texnums3));
	memset(texnums4,0,sizeof(texnums4));
#endif

	ds_blocks[0].block_size = TEXTURE_BLOCK_SIZE;
	ds_blocks[0].blocks_per_bank = TEXTURE_PER_BANK;
	ds_blocks[0].num_blocks = NUM_TEXTURE_BLOCKS;
	ds_blocks[0].start = DS_TEXTURE_BASE;
	ds_blocks[0].last_free = -1;
	ds_blocks[0].textures = ds_textures;

	ds_blocks[1].block_size = TEXTURE_BLOCK_SIZE2;
	ds_blocks[1].blocks_per_bank = TEXTURE_PER_BANK2;
	ds_blocks[1].num_blocks = NUM_TEXTURE_BLOCKS2;
	ds_blocks[1].start = DS_TEXTURE_BASE2;
	ds_blocks[1].last_free = -1;
	ds_blocks[1].textures = ds_textures2;

	ds_blocks[2].block_size = TEXTURE_BLOCK_SIZE3;
	ds_blocks[2].blocks_per_bank = TEXTURE_PER_BANK3;
	ds_blocks[2].num_blocks = NUM_TEXTURE_BLOCKS3;
	ds_blocks[2].start = DS_TEXTURE_BASE3;
	ds_blocks[2].last_free = -1;
	ds_blocks[2].textures = ds_textures3;

	ds_blocks[3].block_size = TEXTURE_BLOCK_SIZE4;
	ds_blocks[3].blocks_per_bank = TEXTURE_PER_BANK4;
	ds_blocks[3].num_blocks = NUM_TEXTURE_BLOCKS4;
	ds_blocks[3].start = DS_TEXTURE_BASE4;
	ds_blocks[3].last_free = -1;
	ds_blocks[3].textures = ds_textures4;

	ds_blocks[4].block_size = TEXTURE_BLOCK_SIZE5;
	ds_blocks[4].blocks_per_bank = TEXTURE_PER_BANK5;
	ds_blocks[4].num_blocks = NUM_TEXTURE_BLOCKS5;
	ds_blocks[4].start = DS_TEXTURE_BASE5;
	ds_blocks[4].last_free = -1;
	ds_blocks[4].textures = ds_textures5;

#ifdef WIN32
	ds_blocks[0].texnums = texnums;
	ds_blocks[1].texnums = texnums2;
	ds_blocks[2].texnums = texnums3;
	ds_blocks[3].texnums = texnums4;
	ds_blocks[4].texnums = texnums5;
#endif
}

void ds_print_blocks()
{
	int i,j,in,used;

	for(i=0;i<5;i++)
	{
		in = used = 0;
		for(j=0;j<ds_blocks[i].num_blocks;j++)
		{
			if(r_framecount - ds_blocks[i].textures[j].visframe < 2)
			{
				used++;
			}
			if(ds_blocks[i].textures[j].name)
			{
				in++;
			}
		}
		printf("tex %d: used: %d in: %d all: %d\n",i,used,in,ds_blocks[i].num_blocks);

	}
	printf("s: %d t: %d f: %d\n",tex_cb_sprites,tex_cb_walls,tex_cb_flats);
	printf("total: %d\n",tex_cb_sprites+tex_cb_walls+tex_cb_flats);

	tex_cb_sprites = tex_cb_walls = tex_cb_flats = 0;
}

void ds_lock_bank(byte *addr)
{
#ifdef ARM9
    //vramSetMainBanks(VRAM_A_TEXTURE, VRAM_B_TEXTURE, VRAM_C_TEXTURE, VRAM_D_TEXTURE);
	if(addr >= (byte *)VRAM_A && addr < (byte *)VRAM_B)
	{
		vramSetBankA(VRAM_A_TEXTURE);
		return;
	}
	if(addr >= (byte *)VRAM_B && addr < (byte *)VRAM_C)
	{
		vramSetBankB(VRAM_B_TEXTURE);
		return;
	}
	if(addr >= (byte *)VRAM_C && addr < (byte *)VRAM_D)
	{
		vramSetBankC(VRAM_C_TEXTURE);
		return;
	}
	if(addr >= (byte *)VRAM_D && addr < (byte *)VRAM_E)
	{
		vramSetBankD(VRAM_D_TEXTURE);
		return;
	}
#endif
}
void ds_unlock_bank(byte *addr)
{
#ifdef ARM9
    //vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	if(addr >= (byte *)VRAM_A && addr < (byte *)VRAM_B)
	{
		vramSetBankA(VRAM_A_LCD);
		return;
	}
	if(addr >= (byte *)VRAM_B && addr < (byte *)VRAM_C)
	{
		vramSetBankB(VRAM_B_LCD);
		return;
	}
	if(addr >= (byte *)VRAM_C && addr < (byte *)VRAM_D)
	{
		vramSetBankC(VRAM_C_LCD);
		return;
	}
	if(addr >= (byte *)VRAM_D && addr < (byte *)VRAM_E)
	{
		vramSetBankD(VRAM_D_LCD);
		return;
	}
#endif
}

int ds_adjust_size(int x,int max)
{
	if(x > 128 && max > 128)
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

int ds_find_free_block(int zone,int name)
{
	int i;
	int oldest = -1;
	int diff,maxdiff = 1;
	ds_texture_bank_t *bank = &ds_blocks[zone];
	
	//names are in a hash list so they do not need strcmp
	for(i=0;i<bank->num_blocks;i++)
	{
		if(bank->textures[i].name == name)
			return i;
	}
	
	//do not start at beggining - might leave textures in longer
	//look for ones that have not been used in a couple frames
	for(i=0;i<bank->num_blocks;i++)
	{
		bank->last_free++;
		if(bank->last_free >= bank->num_blocks)
		{
			bank->last_free = 0;
		}
		
		//if(r_framecount - bank->textures[bank->last_free].visframe > 1)
		diff = r_framecount - bank->textures[bank->last_free].visframe;
		if(diff > maxdiff)
		{
			//return bank->last_free;
			maxdiff = diff;
			oldest = bank->last_free;
		}
	}
	return oldest;
}


int ds_is_texture_resident(dstex_t *tx)
{
	ds_texture_bank_t *bank = &ds_blocks[tx->zone];
	int block = tx->block%bank->num_blocks;
	if(bank->textures[block].name == tx->name)
		return block;
	return -1;
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
		out = outt;
		for (i=0 ; i<bw*bh ; i++) {
			switch(in[i]) {
			case 0xff:
				out[i] = 0;
				break;
			case 0:
				out[i] = 0xff;
				break;
			default:
				out[i] = in[i];
				break;
			}
		}
		return outt;
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
			switch(out[j]) {
			case 0xff:
				out[j] = 0;
				break;
			case 0:
				out[j] = 0xff;
				break;
			default:
				break;
			}
			frac += fracstep;
		}
	}
	return outt;
}

byte *ds_block_address(int zone, int block)
{
	ds_texture_bank_t *bank = &ds_blocks[zone];;
	return bank->start + (block * bank->block_size);
}

#ifdef ARM9
void memcpy32(void *dst, const void *src, uint wdcount) ITCM_CODE;
#endif

byte* ds_load_block(int zone,int block,byte *texture,int size)
{
	byte *addr = ds_block_address(zone,block);


#ifdef WIN32
	memcpy(addr,texture,size);
#endif

#ifdef ARM9
	{
		int ofs = 0;
		int cb;
		int i;
		vu16 vcount,v1,v2;
	
		//DC_FlushRange((uint32*)texture, size);
		DC_FlushAll();
		v1 = REG_VCOUNT;
		i=0;
		do {
			while(REG_VCOUNT > 260);

			//if(REG_VCOUNT < 191 || REG_VCOUNT > 213) {
				cb = MIN(512,size);
				while((REG_DISPSTAT&DISP_IN_HBLANK) != 0);
				while((REG_DISPSTAT&DISP_IN_HBLANK) == 0);
			//} else {
			//	cb = MIN(4096,size);
			//}
			
			vcount = REG_VCOUNT;
			ds_unlock_bank(addr+ofs);
			//memcpy32(addr+ofs,texture+ofs,cb/4);
			//vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
			dmaCopyWords(0,(uint32*)(texture+ofs), (uint32*)(addr+ofs) , cb);
			//vramSetMainBanks(VRAM_A_TEXTURE, VRAM_B_TEXTURE, VRAM_C_TEXTURE, VRAM_D_TEXTURE);
			ds_lock_bank(addr+ofs);
			if(vcount != REG_VCOUNT) {
				iprintf("%d %d %d missed blanking period\n",vcount,REG_VCOUNT,cb);
			}

			ofs += cb;
			i++;
		} while(ofs < size);
		v2 = REG_VCOUNT;
		iprintf("blk: %d %d %d %d\n",r_framecount,size,v2-v1,i);
	}
#endif
	
	return addr;
}

int ds_tex_size(int x)
{
	switch(x)
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

int ds_tex_parameters(	int sizeX, int sizeY,
						const byte* addr,
						int mode,
						unsigned int param)
{
//---------------------------------------------------------------------------------
	return param | (sizeX << 20) | (sizeY << 23) | (((unsigned int)addr >> 3) & 0xFFFF) | (mode << 26);
}

int ds_load_texture(dstex_t *ds,byte *buf,int trans,int flags)
{
	//ds_texture_bank_t *bank = &ds_blocks[ds->zone];
	int block,ofs;
	byte *addr;
	int w=ds->block_width, h = ds->block_height;

	ds->visframe = r_framecount;
	ofs = ds_texture_cache_offset;
	if((ds_texture_cache_offset + (w * h)) > (256*256*2*2)) {
		return 0;
	}
	ds_texture_cache_offset += (w * h);

#ifdef WIN32
	if(ds->addr == 0) {
		glGenTextures(1,&ds->addr);
	}
	glBindTexture(GL_TEXTURE_2D, ds->addr);
	//bank->textures[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,0,0);
	{
		unsigned int dest32[256*256],v,r,g,b;
		int i;
		addr = buf;
		for(i=0;i<(w*h);i++) {
			r = host_basepal[addr[i]*3 + 0];
			g = host_basepal[addr[i]*3 + 1];
			b = host_basepal[addr[i]*3 + 2];
			if(trans) {
				v = ((addr[i] == 0 ? 0 : 255)<<24) + (r<<0) + (g<<8) + (b<<16);
			} else {
				v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			}
			dest32[i] = v;
		}
		glTexImage2D (GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
#endif

#ifdef WIN32
	memcpy(ds_texture_cache+ofs,buf,w*h);
#endif

#ifdef ARM9
	//DC_FlushRange( (const void *)(((unsigned int)buf)&(~31)),(w*h)+32);
	//dmaCopyWords(0,(uint32*)buf, (uint32*)(ds_texture_cache_base+ofs), w*h);
	memcpy32(ds_texture_cache+ofs,buf,(w*h)/4);
	ds->addr = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),
		ds_texture_cache_base+ofs,GL_RGB256,flags|(trans  ? GL_TEXTURE_COLOR0_TRANSPARENT : 0));
#endif

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
	ds_texture_bank_t *bank = &ds_blocks[ds->zone];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,mask[512],col[512];

	if(ds->visframe == r_framecount) {
		return ds->addr;
	}

	size = texture->width * texture->height;
	w = texture->width;
	h = texture->height;
	ds_size = ds->block_width * ds->block_height;

	tex_cb_walls += ds_size;

	if(ds->data == 0) {
		src = (byte *)Z_Malloc (size,PU_STATIC,0);
		dst = (byte *)Z_Malloc (ds_size,PU_STATIC,&(ds->data));
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
		Z_ChangeTag (dst, PU_CACHE);
		Z_Free(src);
	} else {
		addr = (byte *)ds->data;
	}
#ifdef USE_GL
	ds->addr = texnum = ds_load_texture(ds,addr,1,flags|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
#endif
	return texnum;
}

int ds_load_map_texture(int name,int flags)
{
	column_t *column;
	texture_t		*texture = textures[name];
	dstex_t			*ds = &textures_ds[name];
	ds_texture_bank_t *bank = &ds_blocks[ds->zone];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,mask[512],col[512];

	if(ds->visframe == r_framecount) {
		return ds->addr;
	}
	ds->visframe = r_framecount;
 	size = texture->width * texture->height;
	w = texture->width;
	h = texture->height;
	ds_size = ds->block_width * ds->block_height;

	tex_cb_walls += ds_size;

	if(ds->data == 0) {
		//src = (byte *)Z_Malloc (size+ds_size,PU_STATIC,0);
		//dst = src + size;
		src = (byte *)Z_Malloc (size,PU_STATIC,0);
		dst = (byte *)Z_Malloc (ds_size,PU_STATIC,&(ds->data));
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
		Z_ChangeTag (dst, PU_CACHE);
		Z_Free(src);
	} else {
		addr = (byte *)ds->data;
	}
#ifdef USE_GL

	ds->addr = texnum = ds_load_texture(ds, addr, 1, flags | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);

#endif //  USE_GL


	return texnum;
}

int ds_load_map_flat(int name)
{
	//texture_t		*texture = textures[name];
	dstex_t			*ds = &flats_ds[name];
	ds_texture_bank_t *bank = &ds_blocks[ds->zone];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,mask[512],col[512];


	if(ds->visframe == r_framecount) {
		return ds->addr;
	}

	size = 64 * 64;
	w = 64;
	h = 64;
	ds_size = ds->block_width * ds->block_height;

	tex_cb_flats += ds_size;

	if(ds->data == 0) {
		src = (byte *)W_CacheLumpNum(firstflat + name, PU_STATIC);
		dst = (byte *)Z_Malloc (ds_size,PU_CACHE,&(ds->data));

		addr = ds_scale_texture(ds,w,h,src,dst);
		Z_ChangeTag (src, PU_CACHE);
		Z_ChangeTag (dst, PU_CACHE);
		//Z_Free(dst);
	} else {
		addr = (byte *)ds->data;
	}

#ifdef USE_GL
	texnum = ds_load_texture(ds, addr, 0, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);
#endif // USE_GL

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
	ds_texture_bank_t *bank = &ds_blocks[ds->zone];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	byte *src,*addr,*dst,*scol,*dcol,col[512];

	if(ds->visframe == r_framecount) {
		return ds->addr;
	}

	w = spritewidth[name]>>FRACBITS;
	h = spriteheight[name]>>FRACBITS;
	size = w * h;
	size = (size+3)&(~3L);
	ds_size = ds->block_width * ds->block_height;

	tex_cb_sprites += ds_size;

	if(ds->data == 0) {
		src = (byte *)Z_Malloc (size+16,PU_STATIC,0);
		dst = (byte *)Z_Malloc (ds_size+16,PU_STATIC,&(ds->data));
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
		Z_Free(src);
		Z_ChangeTag (dst, PU_CACHE);
		/*for(i=0;i<ds->width;i++) {
			addr[i] = 15;
			addr[ds->block_width*(ds->height-1) + i] = 9*16;
		}
		for(i=0;i<ds->height;i++) {
			addr[i*ds->block_width] = 15;
			addr[i*ds->block_width + ds->width - 1] = 9*16;
		}*/
	} else {
		addr = (byte *)ds->data;
	}
	//printf("sprite: %d %d %d\n",name,w,h);
	///for(i=0;i<32;i++) {
	//	printf("%02x",addr[i+(ds->block_width * ds->height/2)]);
	//}
	//printf("\n");

#ifdef USE_GL
	texnum = ds_load_texture(ds, addr, 1, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T);

#endif // USE_GL


	return texnum;
}

#if 0
/*
byte	crosstexture[8][8] =
{
	{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0xf,0xf,0xf,0xf,0xf,0xf,0x0},
	{0x0,0xf,0xf,0xf,0xf,0xf,0xf,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
};

byte	dottexture[8][8] =
{
	{0x0,0xf,0xf,0xf,0,0,0,0},
	{0xf,0xf,0xf,0xf,0xf,0,0,0},
	{0xf,0xf,0xf,0xf,0xf,0,0,0},
	{0xf,0xf,0xf,0xf,0xf,0,0,0},
	{0x0,0xf,0xf,0xf,0,0,0,0},
	{0x0,0x0,0x0,0x0,0,0,0,0},
	{0x0,0x0,0x0,0x0,0,0,0,0},
	{0x0,0x0,0x0,0x0,0,0,0,0},
};

int ds_load_particle_texture(dstex_t *ds)
{
	int handle, length, size, block, w, h;
	byte *buf,*addr,*dst;

#ifdef WIN32
	ds_texture_width = ds->width*16.0f;
	ds_texture_height = ds->height*16.0f;
#endif

	block = ds_is_texture_resident(ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums[block]);
#endif
		ds_textures[block].visframe = r_framecount;
		return ds_textures[block].texnum;
	}
	//Con_DPrintf("%s %d %d\n",ds->name,ds->width,ds->height);
	return ds_loadTexture(ds,8,8,&dottexture[0][0],1);
}
int ds_load_crosshair_texture(dstex_t *ds)
{
	int handle, length, size, block, w, h;
	byte *buf,*addr,*dst;


	block = ds_is_texture_resident(ds);
	if(block != -1)
	{
		ds_textures[block].visframe = r_framecount;
		return ds_textures[block].texnum;
	}
	//Con_DPrintf("%s %d %d\n",ds->name,ds->width,ds->height);
	return ds_loadTexture(ds,8,8,&crosstexture[0][0],1);
}
*/
#endif
/*
#define TEXTURE_BLOCK_SIZE2 (128*64)
#define TEXTURE_BLOCK_SIZE3 (256*128)
#define TEXTURE_BLOCK_SIZE (64*64)
*/
void ds_init_texture(int name,texture_t *tx,dstex_t *ds)
{
	int max[2];
	ds->block = -1;
	ds->name = name;
	if(tx->width <= 64 && tx->height <= 64)
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
	}

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
	if(patch->width <= 32 && patch->height <= 32)
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
	}

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
	dstex_t			*ds = &ds_blank_tex;
	ds_texture_bank_t *bank = &ds_blocks[ds->zone];
	int handle, length, size,ds_size, block, w, h, i, j,lump, texnum;
	//byte *src,*addr,*dst,*scol,*dcol,col[512];

#ifdef WIN32
	ds_texture_width = ds->width*16.0f;
	ds_texture_height = ds->height*16.0f;
#endif

	return 0;

	block = ds_is_texture_resident(ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,bank->texnums[block]);
#endif
		bank->textures[block].visframe = r_framecount;
		return bank->textures[block].texnum;
	}
	//Con_DPrintf("%s %d %d\n",texture->ds.name,texture->ds.width,texture->ds.height);

#ifdef USE_GL
	texnum = ds_load_texture(ds,&ds_blank_texture[0][0],1,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
#endif

	return texnum;
}

#endif