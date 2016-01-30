#ifndef __DS_TEXTURES_H__
#define __DS_TEXTURES_H__

typedef struct
{
	int		originx;	// block origin (allways UL), which has allready
	int		originy;	// accounted  for the patch's internal origin
	int		patch;
} texpatch_t;

// a maptexturedef_t describes a rectangular texture, which is composed of one
// or more mappatch_t structures that arrange graphic patches
typedef struct
{
	char		name[8];		// for switch changing, etc
	short		width;
	short		height;
	short		patchcount;
	texpatch_t	patches[1];		// [patchcount] drawn back to front
								//  into the cached texture
} texture_t;

typedef struct {
	int	name;
	int visframe;
	int status;
	int texnum;
} ds_texture_t;

typedef struct dstex_s
{
	int			name;
	short		width, height;
	short		block_width, block_height;
	short		zone,block;
	unsigned int	addr;
	int			width_scale,height_scale;
} dstex_t;

void ds_init_textures();
void ds_init_texture(int name,texture_t *tx,dstex_t *ds);
int ds_load_map_texture(int name,int flags);
int ds_load_sky_texture(int name,int flags);

void ds_init_flat(int name,dstex_t *ds);
int ds_load_map_flat(int name);

void ds_init_sprite(int name,patch_t *patch,dstex_t *ds);
int ds_load_sprite(int name);

int ds_load_blank_tex();

extern texture_t	**textures;
extern dstex_t		*textures_ds;
extern dstex_t		*flats_ds;
extern int		firstflat;

#endif