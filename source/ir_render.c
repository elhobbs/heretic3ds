#if 1
#ifdef _3DS
#include <3ds.h>
#include <3ds/types.h>
#include "gl.h"
#include "3dmath.h"
#define GL_STREAM_DRAW 0
#define GL_MAP_WRITE_BIT 0
#endif

#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include "DoomDef.h"
#include "R_local.h"
#include "map.h"
#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#include "..\win32\glext.h"
extern HWND openGL;
#endif

#include "geometry.h"
#include "ds_textures.h"
#include <limits.h>

typedef struct {
	float xyz[3];
	float n[3];
	float c[4];
	float st[2];
} ir_vert_t;

int ir_vert_count = 0;
int ir_vert_count_max = 0;

#define F_FLOOR(__x) ((__x)>>16)
#define F_CEIL(__x) (((__x)+(1<<15))>>16)

void IR_DrawPlayerSprites (void);
void IR_ResetOccluder();
int IR_AddOccluder(fixed_t x0, fixed_t y0, fixed_t x1, fixed_t y1);
int IR_IsOccluded(fixed_t x0, fixed_t y0, fixed_t x1, fixed_t y1);
int ir_occlude_total = 0;
int ir_occlude_cull = 0;

int ir_vbo = 0;
int ir_vbo_base_vertex = 0;

// get the screen space vector for sprites
float	ir_yaws;
float	ir_yawc;


void ir_frame_start();
void ir_frame_end();

#ifdef WIN32
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex;
#endif


void glRotateZ(float x);
void glRotateX(float x);
void glRotateY(float x);

static float roll     = 0.0f;
/* JDC static */ float yaw      = 0.0f;
static float inv_yaw  = 0.0f;
static float pitch    = 0.0f;

#define __glPi 3.14159265358979323846


int viewwidth = SCREEN_WIDTH;
int viewwidthf = SCREEN_WIDTH<<16;
int viewheight = SCREEN_HEIGHT;
int viewheightfrac;
int viewwindowx = 0;
int viewwindowy = 0;

// counters
int		c_occludedSprites;
int		c_sectors;
int		c_subsectors;

int		r_framecount;

extern int			validcount;		// increment every time a check is made


extern int screenblocks;
extern int			extralight;			// bumped light from gun blasts
extern lighttable_t	*fixedcolormap;

extern fixed_t		viewx, viewy, viewz;
extern angle_t		viewangle;
extern fixed_t		viewcos, viewsin;
extern player_t		*viewplayer;
extern boolean		automapactive;

extern fixed_t			projection;
extern fixed_t			centerxfrac, centeryfrac;


extern byte *host_basepal;

extern dstex_t		*sprites_ds;

// Cleared to 0 at frame start.
// Individual columns will be set to 1 as occluding segments are processed.
// An occluding segment is either a one-sided line, a line that has a back
// sector with equal floor and ceiling heights, a line with a back ceiling
// height lower than the fron floor height, or a line with a back floor height
// higher than the front ceiling height.
// Entire nodes are culled when their bounds does not include a 0 column.
// Individual line segments are culled when their span does not include 0 columns.
// Sprites could be checked against it, but it may not be worth it.
//char	occlusion[SCREEN_WIDTH+2];	// +2 for guard columns to avoid clamping

// this matrix is exactly what GL uses, but there will still
// be floating point differences between the GPU and CPU
float	glMVPmatrix[16];

#ifdef WIN32
double	glMVPmatrixd[16];
double	glPJPmatrixd[16];
int	glView[4];
#endif

int dsMat[16];


typedef struct {
	//GLTexture		*texture;
	int				texture;
	sector_t		*sector;
	boolean			ceiling;
} sortSectorPlane_t;

#define MAX_SECTOR_PLANES	1024
sortSectorPlane_t	sectorPlanes[MAX_SECTOR_PLANES];
int					numSectorPlanes;

typedef struct
	{
		float ytop,ybottom;
		float ul,ur,vt,vb;
		float light;
		float alpha;
		float skyymid;
		float skyyaw;
		int name;
		byte flag;
		side_t	*side;
	} GLWall;

GLWall gl_walls[2048];
int num_gl_walls = 0;

typedef struct
	{
		int cm;
		float x,y,z;
		float vt,vb;
		float ul,ur;
		float x1,y1;
		float x2,y2;
		float light;
		fixed_t scale;
		int name;
		boolean shadow;
		boolean trans;
	} GLSprite;

GLSprite gl_sprites[128];
int num_gl_sprites = 0;

// if any sector textures are the sky texture, we will draw the sky and
// ignore those sector geometries
boolean	skyIsVisible;

// used during debugging to isolate incorrect culling
int		failCount;

float	lightDistance = 96.0f/1280.0f;	// in prBoom MAP_SCALE units, increasing this makes things get dimmer faster
#define MAX_LIGHT_DROP	96

float	lightingVector[3];	// transform and scale [ x y 1 ] to get color units to subtract
static int FadedLighting( float x, float y, int sectorLightLevel ) {
	// Ramp down the lightover lightDistance world units.
	// Triangles that extend across behind the view origin and past
	// the lightDistance clamping boundary will not have completely linear fading,
	// but nobody should notice.
	
	// A proportional drop in lighting sounds like a better idea, but
	// this linear drop seems to look nicer.  It's not like Doom's
	// lighting is realistic in any case...
	float	idist = x * lightingVector[0] + y * lightingVector[1] + lightingVector[2];
	if ( idist < 0 ) {
		idist = 0;
	} else if ( idist > MAX_LIGHT_DROP ) {
		idist = MAX_LIGHT_DROP;
	}
	sectorLightLevel -= (int)idist;	

	if ( sectorLightLevel < 0 ) {
		sectorLightLevel = 0;
	}
	if ( sectorLightLevel > 255 ) {
		sectorLightLevel = 255;
	}

	return sectorLightLevel;
}

//
// IR_ProjectSprite
// Generates a vissprite for a thing if it might be visible.
//
static void IR_ProjectSprite (mobj_t* thing, int lightlevel)
{
	fixed_t   gzt;               // killough 3/27/98
	fixed_t   tx;
	fixed_t   xscale;
	int       x1;
	int       x2;
	spritedef_t   *sprdef;
	spriteframe_t *sprframe;
	int       lump;
	boolean   flip;
	int	testLow;
	int testHigh;
	dstex_t *ds;
	
	// transform the origin point
	fixed_t tr_x, tr_y;
	fixed_t fx, fy, fz;
	fixed_t gxt, gyt;
	fixed_t tz;
	int width,height,topoffset,leftoffset;
	
	fx = thing->x;
	fy = thing->y;
	fz = thing->z;
	
	tr_x = fx - viewx;
	tr_y = fy - viewy;
	
	gxt = FixedMul(tr_x,viewcos);
	gyt = -FixedMul(tr_y,viewsin);
	
	tz = gxt-gyt;
	
    // thing is behind view plane?
	if (tz < MINZ)
		return;
	
	xscale = FixedDiv(projection, tz);
	
	gxt = -FixedMul(tr_x,viewsin);
	gyt = FixedMul(tr_y,viewcos);
	tx = -(gyt+gxt);
	
	// too far off the side?
	if (D_abs(tx)>(tz<<2))
		return;
	
    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((unsigned) thing->sprite >= (unsigned)numsprites)
		I_Error ("R_ProjectSprite: Invalid sprite number %i", thing->sprite);
#endif
	
	sprdef = &sprites[thing->sprite];
	
#ifdef RANGECHECK
	if ((thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
		I_Error ("R_ProjectSprite: Invalid sprite frame %i : %i", thing->sprite,
				 thing->frame);
#endif
	
	if (!sprdef->spriteframes)
		I_Error ("R_ProjectSprite: Missing spriteframes %i : %i", thing->sprite,
				 thing->frame);
	
	sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];
	
	if (sprframe->rotate)
    {
		// choose a different rotation based on player view
		// JDC: this could be better...
		angle_t ang = R_PointToAngle(fx, fy);
		unsigned rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
		lump = sprframe->lump[rot];
		flip = (boolean) sprframe->flip[rot];
    }
	else
    {
		// use single rotation for all views
		lump = sprframe->lump[0];
		flip = (boolean) sprframe->flip[0];
    }
	ds = &sprites_ds[lump];
	
	//rpatch_t* patch = R_CachePatchNum(lump+firstspritelump);
	width = spritewidth[lump];
	height = spriteheight[lump];
	leftoffset = spriteoffset[lump];
	topoffset = spritetopoffset[lump];
		
	/* calculate edges of the shape
		* cph 2003/08/1 - fraggle points out that this offset must be flipped
		* if the sprite is flipped; e.g. FreeDoom imp is messed up by this. */
	if (flip) {
		tx -= (width - leftoffset);// << FRACBITS;
	} else {
		tx -= leftoffset;// << FRACBITS;
	}
	x1 = (centerxfrac + FixedMul(tx,xscale)) >> FRACBITS;
		
	tx += width;//<<FRACBITS;
	x2 = ((centerxfrac + FixedMul (tx,xscale) ) >> FRACBITS) - 1;
		
	gzt = fz + topoffset;// << FRACBITS);
		
	// JDC: we don't care if they never get freed,
	// so don't bother changing the zone tag status each time
	//R_UnlockPatchNum(lump+firstspritelump);
	
	// off the side?
	if (x1 > viewwidth || x2 < 0)
		return;
	
	// killough 4/9/98: clip things which are out of view due to height
	// e6y: fix of hanging decoration disappearing in Batman Doom MAP02
	// centeryfrac -> viewheightfrac
	if (fz  > viewz + FixedDiv(viewheightfrac, xscale) ||
		gzt < viewz - FixedDiv(viewheightfrac-viewheight, xscale))
		return;
	
#if 0
	// JDC: clip to the occlusion buffer
	testLow = x1 < 0 ? 0 : x1;
	testHigh = x2 >= viewwidth ? viewwidth - 1 : x2;
	if ( !memchr( occlusion+testLow, 0, testHigh - testLow ) ) {
		c_occludedSprites++;
		return;
	}
#endif
	
	// ------------ gld_AddSprite ----------
	{
		mobj_t *pSpr= thing;
		GLSprite sprite;
		float voff,hoff;
	
		sprite.scale= xscale;//FixedDiv(projectiony, tz);
		if (pSpr->frame & FF_FULLBRIGHT)
			sprite.light = 255;
		else
			sprite.light = pSpr->subsector->sector->lightlevel+(extralight<<5);
		//sprite.cm=CR_LIMIT+(int)((pSpr->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
		sprite.name=lump;//gltexture=gld_RegisterPatch(lump+firstspritelump,sprite.cm);
		//if (!sprite.gltexture)
		//	return;
		sprite.shadow = (pSpr->flags & MF_SHADOW) != 0;
		//sprite.trans  = (pSpr->flags & MF_TRANSLUCENT) != 0;
		/*if (movement_smooth)
		{
			sprite.x = (float)(-pSpr->PrevX + FixedMul (tic_vars.frac, -pSpr->x - (-pSpr->PrevX)))/MAP_SCALE;
			sprite.y = (float)(pSpr->PrevZ + FixedMul (tic_vars.frac, pSpr->z - pSpr->PrevZ))/MAP_SCALE;
			sprite.z = (float)(pSpr->PrevY + FixedMul (tic_vars.frac, pSpr->y - pSpr->PrevY))/MAP_SCALE;
		}
		else
		{*/
			sprite.x= (pSpr->x);
			sprite.y= (pSpr->y);
			sprite.z= (pSpr->z);
		//}

		sprite.vt=0.0f;
		sprite.vb=(float)ds->height/(float)ds->block_height;
		if (flip)
		{
			sprite.ul=0.0f;
			sprite.ur=(float)ds->width/(float)ds->block_width;
		}
		else
		{
			sprite.ul=(float)ds->width/(float)ds->block_width;
			sprite.ur=0.0f;
		}

		hoff=(leftoffset);
		voff=(topoffset);
		sprite.x1=(hoff-width);
		sprite.x2=(hoff);
		sprite.y1=(voff);
		sprite.y2=(voff-height);
	
		// JDC: don't let sprites poke below the ground level.
		// Software rendering Doom didn't use depth buffering, 
		// so sprites always got drawn on top of the flat they
		// were on, but in GL they tend to get a couple pixel
		// rows clipped off.
		if ( sprite.y2 < 0 ) {
			sprite.y1 -= sprite.y2;
			sprite.y2 = 0;
		}
	
		if (IR_IsOccluded(sprite.x + sprite.x1 * ir_yawc,
			sprite.y + sprite.x1 * ir_yaws,
			sprite.x + sprite.x2 * ir_yawc,
			sprite.y + sprite.x2 * ir_yaws)) {
			return;
		}
		if(num_gl_sprites < 128)
		{
			//printf("%d %f %f\n",num_gl_sprites,(float)width/65536.0f,(float)hoff/65536.0f);
			gl_sprites[num_gl_sprites++] = sprite;
		}

	}
}

PUREFUNC static int IR_PointOnSide(fixed_t x, fixed_t y, const node_t *node)
{
	// JDC: these early tests probably aren't worth it on iphone...
	if (!node->dx)
		return x <= node->x ? node->dy > 0 : node->dy < 0;
	
	if (!node->dy)
		return y <= node->y ? node->dx < 0 : node->dx > 0;
	
	x -= node->x;
	y -= node->y;
	
	// Try to quickly decide by looking at sign bits.
	if ((node->dy ^ node->dx ^ x ^ y) < 0)
		return (node->dy ^ x) < 0;  // (left is negative)

	return (int64_t)y * (int64_t)node->dx >= (int64_t)x * (int64_t)node->dy;
}

#define GLDWF_TOP 1
#define GLDWF_M1S 2
#define GLDWF_M2S 3
#define GLDWF_BOT 4
#define GLDWF_SKY 5
#define GLDWF_SKYFLIP 6

#define OU(w,seg) (((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)textures[(w).name]->width)
#define OV(w,seg) (((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)textures[(w).name]->height)
#define OV_PEG(w,seg,v_offset) (OV((w),(seg))-(((float)(v_offset)/(float)FRACUNIT)/(float)textures[(w).name]->height))

// JDC: removed the 0.001f epsilons that were presumably added
// to try to hide T-junction cracks, but now that we are drawing
// source lines instead of clipped segs, it is a non-problem.
#define LINE seg->linedef

#define CALC_Y_VALUES(w, lineheight, floor_height, ceiling_height)\
(w).ytop=MAP_COORD(ceiling_height);\
(w).ybottom=MAP_COORD(floor_height);\
lineheight=((float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT)))

#define CALC_TEX_VALUES_MIDDLE1S(w, seg, peg, linelength, lineheight)\
(w).flag=GLDWF_M1S;\
(w).ul=OU((w),(seg))+(0.0f);\
(w).ur=OU((w),(seg))+((linelength)/(float)textures[(w).name]->width);\
(peg)?\
(\
(w).vb=OV((w),(seg))+((float)textures_ds[(w).name].height/(float)textures[(w).name]->height),\
(w).vt=((w).vb-((float)(lineheight)/(float)textures[(w).name]->height))\
):(\
(w).vt=OV((w),(seg))+(0.0f),\
(w).vb=OV((w),(seg))+((float)(lineheight)/(float)textures[(w).name]->height)\
)


#define CALC_TEX_VALUES_BOTTOM(w, seg, peg, linelength, lineheight, v_offset)\
(w).flag=GLDWF_BOT;\
(w).ul=OU((w),(seg))+(0.0f);\
(w).ur=OU((w),(seg))+((linelength)/(float)textures[(w).name]->width);\
(peg)?\
(\
(w).vb=OV_PEG((w),(seg),(v_offset))+((float)textures[(w).name]->height/(float)textures[(w).name]->height),\
(w).vt=((w).vb-((float)(lineheight)/(float)textures[(w).name]->height))\
):(\
(w).vt=OV((w),(seg))+(0.0f),\
(w).vb=OV((w),(seg))+((float)(lineheight)/(float)textures[(w).name]->height)\
)


#define CALC_TEX_VALUES_TOP(w, seg, peg, linelength, lineheight)\
(w).flag=GLDWF_TOP;\
(w).ul=OU((w),(seg))+(0.0f);\
(w).ur=OU((w),(seg))+((linelength)/(float)textures[(w).name]->width);\
(peg)?\
(\
(w).vb=OV((w),(seg))+((float)textures[(w).name]->height/(float)textures[(w).name]->height),\
(w).vt=((w).vb-((float)(lineheight)/(float)textures[(w).name]->height))\
):(\
(w).vt=OV((w),(seg))+(0.0f),\
(w).vb=OV((w),(seg))+((float)(lineheight)/(float)textures[(w).name]->height)\
)

// e6y
// Sky textures with a zero index should be forced
// See third episode of requiem.wad
#define SKYTEXTURE(w)\
{\
(w).name=skytexture;\
(w).skyyaw=-2.0f*((yaw+90.0f)/90.0f);\
(w).skyymid = 200.0f/319.5f*((100.0f)/100.0f);\
(w).ul = (((yaw+90.0f)/90.0f)*256+ends[0]);\
(w).ur = (((yaw+90.0f)/90.0f)*256+ends[1]);\
(w).flag = GLDWF_SKY;\
};


#define ADDWALL(wall)\
{\
if (num_gl_walls < 2048)\
{\
gl_walls[num_gl_walls++]=*wall;\
}\
};

#define SKYWALLMAX 8192.0f

void IR_AddWall(seg_t *seg, float ends[2])
{
	GLWall wall;
	sector_t *frontsector;
	sector_t *backsector;
	float lineheight;
	int rellight = 0;
	int light;
	texture_t		*texture;
	dstex_t			*ds;

	wall.side = seg->sidedef;
	frontsector = seg->frontsector;
	
	// JDC: improve this lighting tweak
	rellight = seg->linedef->dx==0? +8 : seg->linedef->dy==0 ? -8 : 0;
	light = frontsector->lightlevel+rellight+(extralight<<5);
	wall.light = MAX(MIN((light),255),0);
	wall.alpha=1.0f;
	wall.name = -1;
	
	if (!seg->backsector) /* onesided */
	{
#ifndef SKYWALLS		
		if (frontsector->ceilingpic==skyflatnum)
		{
			wall.ytop=SKYWALLMAX;
			wall.ybottom=MAP_COORD(frontsector->ceilingheight);
			SKYTEXTURE(wall);
			ADDWALL(&wall);
		}
		if (frontsector->floorpic==skyflatnum)
		{
			wall.ytop=MAP_COORD(frontsector->floorheight);
			wall.ybottom=-SKYWALLMAX;
			SKYTEXTURE(wall);
			ADDWALL(&wall);
		}
#endif		
		wall.name=texturetranslation[seg->sidedef->midtexture];
		CALC_Y_VALUES(wall, lineheight, frontsector->floorheight, frontsector->ceilingheight);
		CALC_TEX_VALUES_MIDDLE1S(
									wall, seg, ((LINE->flags & ML_DONTPEGBOTTOM) == 0 ? 0 : 1),
									((seg->length*textures_ds[wall.name].width_scale)/(float)FRACUNIT), lineheight
									);
		ADDWALL(&wall);
	}
	else /* twosided */
	{
		int floor_height,ceiling_height;
		
		backsector=seg->backsector;
		/* toptexture */
		ceiling_height=frontsector->ceilingheight;
		floor_height=backsector->ceilingheight;
#ifndef SKYWALLS
		if (frontsector->ceilingpic==skyflatnum)
		{
			wall.ytop=SKYWALLMAX;
			if (
				// e6y
				// Fix for HOM in the starting area on Memento Mori map29 and on map30.
				// old code: (backsector->ceilingheight==backsector->floorheight) &&
				(backsector->ceilingheight==backsector->floorheight||(backsector->ceilingheight<=frontsector->floorheight)) &&
				(backsector->ceilingpic==skyflatnum)
				)
			{
				wall.ybottom=MAP_COORD(backsector->floorheight);
				SKYTEXTURE(wall);
				//--ADDWALL(&wall);
			}
			else
			{
				if ( (texturetranslation[seg->sidedef->toptexture]!=0) )
				{
					// e6y
					// It corrects some problem with sky, but I do not remember which one
					// old code: wall.ybottom=(float)frontsector->ceilingheight/MAP_SCALE;
					wall.ybottom=MAP_COORD(MAX(frontsector->ceilingheight,backsector->ceilingheight));
					
					SKYTEXTURE(wall);
					ADDWALL(&wall);
				}
				else
					if ( (backsector->ceilingheight <= frontsector->floorheight) ||
						(backsector->ceilingpic != skyflatnum) )
					{
						wall.ybottom=MAP_COORD(backsector->ceilingheight);
						SKYTEXTURE(wall);
						ADDWALL(&wall);
					}
			}
		}
#endif		
		if (floor_height<ceiling_height)
		{
			if (!((frontsector->ceilingpic==skyflatnum) && (backsector->ceilingpic==skyflatnum)))
			{
				wall.name=texturetranslation[seg->sidedef->toptexture];
				CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
				CALC_TEX_VALUES_TOP(
									wall, seg, ((LINE->flags & (ML_DONTPEGBOTTOM | ML_DONTPEGTOP))==0 ? 0 : 1),
									((seg->length*textures_ds[wall.name].width_scale)/(float)FRACUNIT), lineheight
									);
				ADDWALL(&wall);
			}
		}
		
		/* midtexture */
		wall.name=texturetranslation[seg->sidedef->midtexture];
		
		if (seg->sidedef->midtexture != 0)
		{
			if ( (LINE->flags & ML_DONTPEGBOTTOM)  != 0)
			{
				if (seg->backsector->ceilingheight<=seg->frontsector->floorheight)
					goto bottomtexture;
				floor_height=MAX(seg->frontsector->floorheight,seg->backsector->floorheight)+(seg->sidedef->rowoffset);
				ceiling_height=floor_height+(textures[wall.name]->height<<FRACBITS);
			}
			else
			{
				if (seg->backsector->ceilingheight<=seg->frontsector->floorheight)
					goto bottomtexture;
				ceiling_height=MIN(seg->frontsector->ceilingheight,seg->backsector->ceilingheight)+(seg->sidedef->rowoffset);
				floor_height=ceiling_height-(textures[wall.name]->height<<FRACBITS);
			}
			// e6y
			// The fix for wrong middle texture drawing
			// if it exceeds the boundaries of its floor and ceiling
			
			/*CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
			 CALC_TEX_VALUES_MIDDLE2S(
			 wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0,
			 segs[seg->iSegID].length, lineheight
			 );*/
			{
				fixed_t floormax, ceilingmin, linelen;
				if (seg->sidedef->bottomtexture)
					floormax=MAX(seg->frontsector->floorheight,seg->backsector->floorheight);
				else
					floormax=floor_height;
				if (seg->sidedef->toptexture)
					ceilingmin=MIN(seg->frontsector->ceilingheight,seg->backsector->ceilingheight);
				else
					ceilingmin=ceiling_height;
				linelen=abs(ceiling_height-floor_height);
				wall.ytop=MAP_COORD(MIN(ceilingmin, ceiling_height));
				wall.ybottom=MAP_COORD(MAX(floormax, floor_height));
				wall.flag=GLDWF_M2S;
				wall.ul=OU((wall),(seg))+(0.0f);
				wall.ur=OU(wall,(seg))+(seg->length / (float)textures[wall.name]->width);
				if (floormax<=floor_height)
					wall.vb=1.0f;
				else
					wall.vb=((float)(ceiling_height - floormax))/linelen;
				if (ceilingmin>=ceiling_height)
					wall.vt=0.0f;
				else
					wall.vt=((float)(ceiling_height - ceilingmin))/linelen;

			}
			
			wall.alpha=1.0f;
			ADDWALL(&wall);
		}
	bottomtexture:
		/* bottomtexture */
		ceiling_height=backsector->floorheight;
		floor_height=frontsector->floorheight;
#ifndef SKYWALLS	
		if (frontsector->floorpic==skyflatnum)
		{
			wall.ybottom=-SKYWALLMAX;
			if (
				(backsector->ceilingheight==backsector->floorheight) &&
				(backsector->floorpic==skyflatnum)
				)
			{
				wall.ytop=MAP_COORD(backsector->floorheight);
				SKYTEXTURE(wall);
				ADDWALL(&wall);
			}
			else
			{
				if ( (texturetranslation[seg->sidedef->bottomtexture]!=0) )
				{
					wall.ytop=MAP_COORD(frontsector->floorheight);
					SKYTEXTURE(wall);
					ADDWALL(&wall);
				}
				else
					if ( (backsector->floorheight >= frontsector->ceilingheight) ||
						(backsector->floorpic != skyflatnum) )
					{
						wall.ytop=MAP_COORD(backsector->floorheight);
						SKYTEXTURE(wall);
						ADDWALL(&wall);
					}
			}
		}
#endif		
		if (floor_height<ceiling_height)
		{
			wall.name = texturetranslation[seg->sidedef->bottomtexture];
			CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
			CALC_TEX_VALUES_BOTTOM(
								   wall, seg, ((LINE->flags & ML_DONTPEGBOTTOM) == 0 ? 0: 1),
								   ((seg->length*textures_ds[wall.name].width_scale)/(float)FRACUNIT), lineheight,
								   floor_height-frontsector->ceilingheight
								   );
			ADDWALL(&wall);
		}
	}
}

/*
 TransformAndClipSegment
 
 Converts a world coordinate line segment to screen space.
 Returns false if the segment is off screen.
 
 There would be some savings if all the points in a subsector
 were transformed and clip tested as a unit, instead of as discrete segments.
 */
boolean TransformAndClipSegment( float vv[2][2], float ends[2] ) {
	double	clip[2][4];
	
	float	*v0, *v1;
	float	d0, d1;
	float	nearClip;
	int		i;
	double xyz[2][3];

	v0 = vv[0];
	v1 = vv[1];
	
	// transform from model to clip space
	// because the iPhone screen hardware is portrait mode,
	// we need to look at the Y axis for the segment ends,
	// not the X axis.
	clip[0][0] = v0[0] * glMVPmatrix[0] + v0[1] * glMVPmatrix[2*4+0] + glMVPmatrix[3*4+0];
	clip[0][3] = v0[0] * glMVPmatrix[3] + v0[1] * glMVPmatrix[2*4+3] + glMVPmatrix[3*4+3];
	
	clip[1][0] = v1[0] * glMVPmatrix[0] + v1[1] * glMVPmatrix[2*4+0] + glMVPmatrix[3*4+0];
	clip[1][3] = v1[0] * glMVPmatrix[3] + v1[1] * glMVPmatrix[2*4+3] + glMVPmatrix[3*4+3];

#ifdef WIN32
	gluProject(vv[0][0],0.0,vv[0][1],
		glMVPmatrixd,glPJPmatrixd,glView,
		&xyz[0][0],&xyz[0][1],&xyz[0][2]);
	gluProject(vv[1][0],0.0,vv[1][1],
		glMVPmatrixd,glPJPmatrixd,glView,
		&xyz[1][0],&xyz[1][1],&xyz[1][2]);
#endif
	// clip to the near plane
 	nearClip = 0.01f;
	d0 = clip[0][3] - nearClip;
	d1 = clip[1][3] - nearClip;
	if ( d0 < 0 && d1 < 0 ) {
		// near clipped
		return false;
	}
	if ( d0 < 0 ) {
		float f = d0 / ( d0 - d1 );
		clip[0][0] = clip[0][0] + f * ( clip[1][0] - clip[0][0] );
		clip[0][3] = nearClip;
	} else if ( d1 < 0 ) {
		float f = d1 / ( d1 - d0 );
		clip[1][0] = clip[1][0] + f * ( clip[0][0] - clip[1][0] );
		clip[1][3] = nearClip;
	}

	if ( clip[0][0] > clip[0][3] ) {
		// entire segment is off the right side of the screen
		return false;
	}
	if ( clip[1][0] < -clip[1][3] ) {
		// entire segment is off the left side of the screen
		return false;
	}
		
	// project
	for (i = 0 ; i < 2 ; i++ ) {
		float x = viewwidth * ( ( clip[i][0] / clip[i][3] ) * 0.5 + 0.5 ); 		
		if ( x < 0 ) {
			x = 0;
		} else if ( x > viewwidth ) {
			x = viewwidth;
		}
		ends[i] = x;
	}

	// part of the segment is on screen
	return true;
}

/*
 IR_Subsector
 
 All possible culling should be performed here, but most calculations should be
 deferred until draw time, rather than storing intermediate values that are
 later referenced.
 
 Don't make this static, or the compiler inlines it in the recursive node
 function, which bloats the stack.
*/ 

void IR_Subsector(int num)
{
	int i;
	int checkMin;
	int checkMax;
	int lightlevel;
	subsector_t *sub = &subsectors[num];
	sector_t *frontsector;
	mobj_t *thing;

	c_subsectors++;
	// at this point we know that at least part of the subsector is
	// not covered in the occlusion array
	
	// if the sector that this subsector is a part of has not already had its
	// planes and sprites added, add them now.
	frontsector = sub->sector;
	lightlevel = frontsector->lightlevel+(extralight<<5);

					/*sectorPlanes[numSectorPlanes].texture = flattranslation[frontsector->floorpic];//tex;
					sectorPlanes[numSectorPlanes].ceiling = false;
					//sectorPlanes[numSectorPlanes].sector = frontsector;
					sectorPlanes[numSectorPlanes].sub = sub;
					numSectorPlanes++;
					return;*/
	// There can be several subsectors in each sector due to non-convex
	// sectors or BSP splits, but we draw the floors, ceilings and lines
	// with a single draw call for the entire thing, so ensure that they
	// are only added once per frame.
	if ( frontsector->validcount != validcount ) {
		frontsector->validcount = validcount;
		
		c_sectors++;
		/*GLFlat flat;
		flat.sectornum = frontsector->iSectorID;
		flat.light = lightlevel;
		flat.uoffs= 0;	// no support in standard doom
		flat.voffs= 0;*/
		
		if ( frontsector->floorheight < viewz ) {
			if (frontsector->floorpic == skyflatnum) {
				skyIsVisible = true;
			} else {
				// get the texture. flattranslation is maintained by doom and
				// contains the number of the current animation frame
				//GLTexture *tex = gld_RegisterFlat(flattranslation[frontsector->floorpic], true);
				//if ( tex ) {
					sectorPlanes[numSectorPlanes].texture = flattranslation[frontsector->floorpic];//tex;
					sectorPlanes[numSectorPlanes].ceiling = false;
					sectorPlanes[numSectorPlanes].sector = frontsector;
					//sectorPlanes[numSectorPlanes].sub = sub;
					numSectorPlanes++;
				//}
			}
		}
		if ( frontsector->ceilingheight > viewz ) {
			if (frontsector->ceilingpic == skyflatnum) {
				skyIsVisible = true;
			} else {
				// get the texture. flattranslation is maintained by doom and
				// contains the number of the current animation frame
				//GLTexture *tex = gld_RegisterFlat(flattranslation[frontsector->ceilingpic], true);
				//if ( tex ) {
					sectorPlanes[numSectorPlanes].texture = flattranslation[frontsector->ceilingpic];//tex;
					sectorPlanes[numSectorPlanes].ceiling = true;
					sectorPlanes[numSectorPlanes].sector = frontsector;
					//sectorPlanes[numSectorPlanes].sub = sub;
					numSectorPlanes++;
				//}
			}
		}
		
		// Add all the sprites in this sector.
		// It would be better if they were linked into all the subsectors, because
		// we could do more accurate occlusion culling.  With non-convex sectors,
		// occasionally a sprite will be added in a rear portion of the sector that
		// would have been occluded away if everything was done in BSP subsector order.
		for ( thing = frontsector->thinglist; thing; thing = thing->snext) {
			//printf("+%d ",thing->sprite);
			IR_ProjectSprite( thing, lightlevel );
		}
	}
	
	// If a segment in this subsector is not fully occluded, mark
	// the line that it is a part of as needing to be drawn.  Because
	// we are using a depth buffer, we can draw complete line segments
	// instead of just segments.
	for (i = 0 ; i < sub->numlines ; i++ ) {
		float	v[2][2];
		float	floatEnds[2];
		seg_t *seg = &segs[sub->firstline+i];
		line_t *line = seg->linedef;

		// Determine if it will completely occlude farther objects.
		// Given that changing sector heights is much less common than
		// traversing lines during every render, it would be marginally better if
		// lines had an "occluder" flag on them that was updated as sectors
		// moved, but it hardly matters.
		boolean	occluder;
		if ( seg->backsector == NULL || 
			seg->backsector->floorheight >= seg->backsector->ceilingheight ||
			seg->backsector->floorheight >= seg->frontsector->ceilingheight ||
			seg->backsector->ceilingheight <= seg->frontsector->floorheight ) {
			// this segment can't be seen past, so fill in the occlusion table
			occluder = true;
		} else {
			// If the line has already been made visible and we don't need to
			// update the occlusion buffer, we don't need to do anything else here.
			// This happens when a line is split into multiple segs, and also
			// when the line is reached from the backsector.  In the backsector
			// case, it would be back-face culled, but this test throws it out
			// without having to transform and clip the ends.
			if ( line->validcount == validcount ) {
				continue;
			}
			
			// check to see if the seg won't draw any walls at all
			
			// we won't fill in the occlusion table for this
			occluder = false;
		}
	
		//xyz[0] = -MAP_COORD(wall->side->sideSeg.v1->x);
		//xyz[1] = wall->ytop;
		//xyz[2] = MAP_COORD(wall->side->sideSeg.v1->y);
		// transform and clip the two endpoints
		v[0][0] = MAP_COORD(seg->v1->x);
		v[0][1] = MAP_COORD(seg->v1->y);
		v[1][0] = MAP_COORD(seg->v2->x);
		v[1][1] = MAP_COORD(seg->v2->y);

		if (occluder) {
			if(!IR_AddOccluder(seg->v1->x, seg->v1->y, seg->v2->x, seg->v2->y)) {
				continue;
			}
		}

#if 0
		if ( !TransformAndClipSegment( v, floatEnds ) ) {
			// the line is off to the side or facing away
			continue;
		}
	
		// Allow segs that we consider to be slightly back
		// facing to still pass through, because GPU floating
		// point calculations may not see them exactly the same.
		if ( floatEnds[0] > floatEnds[1] + (3<<15) ) {
			// back face
			continue;
		}
		// Check it against the occlusion buffer.
		// Because the perspective divide is not going to be bit-exact between
		// the CPU and GPU, we check an extra column here.  That will result
		// in an occasional line being drawn that might not need to be, but
		// it avoids missing columns.

		checkMin = floor(floatEnds[0]) - 1;
		checkMax = ceil(floatEnds[1]) + 1;

		if ( checkMin < 0 ) {
			checkMin = 0;
		}
		if ( checkMax > viewwidth ) {
			checkMax = viewwidth;
		}
		if ( !memchr( occlusion + checkMin, 0, checkMax - checkMin ) ) {
			failCount++;
			// every column it would touch is already solid, so it isn't visible
			continue;
		}
		if ( occluder ) {
			// It is important to update the occlusion array as individual
			// segs are processed to maintain pure front to back order.  If
			// the occlusion buffer was updated by complete lines, it would
			// result in some elements being incorrectly occlusion culled.
			
			// Use a consistant fill rule for the occlusion, which is only
			// referenced by the CPU, and should be water tight.
			int fillMin = ceil(floatEnds[0]);
			int fillMax = ceil(floatEnds[1]);

			if ( fillMax > fillMin ) {
				int cc = (int)(seg - segs);
				cc &= 0x000000ff;
				memset( occlusion + fillMin, 1, fillMax-fillMin );
			}
		}
#endif
		if ( line->validcount == validcount ) {
			continue;
		}
		line->validcount = validcount;

		// this line can show up on the automap now
		line->flags |= ML_MAPPED;
	
		// Adding a line may generate up to four drawn walls -- a top wall,
		// a bottom wall, a perforated middle wall, and a sky wall.
		// Use the complete, unclipped segment for the side
		IR_AddWall( &seg->sidedef->sideSeg,floatEnds );
	}
}

static const int checkcoord[12][4] = // killough -- static const
{
{3,0,2,1},
{3,0,2,0},
{3,1,2,0},
{0},
{2,0,2,1},
{0,0,0,0},
{3,1,3,0},
{0},
{2,0,3,1},
{2,1,3,1},
{2,1,3,0}
};


static boolean IR_IsBBoxCompletelyOccluded(const fixed_t *bspcoord) {
	int boxpos;
	const int *check;
	float	v[2][2];
	float	ends[2];

	//return false;
	// conservatively accept if close to the box, so
	// we don't need to worry about the near clip plane
	// in TrnasformAndClipSegment.  Mapscale is 128*fracunit
	// and nearclip is 0.1, so accepting 2 fracunits away works.
	if ( viewx > bspcoord[BOXLEFT]-2*FRACUNIT && viewx < bspcoord[BOXRIGHT] + 2*FRACUNIT
		&& viewy > bspcoord[BOXBOTTOM]-2*FRACUNIT && viewy < bspcoord[BOXTOP] + 2*FRACUNIT ) {
		return false;
	}
	
	// Find the corners of the box
	// that define the edges from current viewpoint.
	boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT ] ? 1 : 2) +
		(viewy >= bspcoord[BOXTOP ] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8);
	
	check = checkcoord[boxpos];

	v[0][0] = MAP_COORD(bspcoord[check[0]]);
	v[0][1] = MAP_COORD(bspcoord[check[1]]);

	v[1][0] = MAP_COORD(bspcoord[check[2]]);
	v[1][1] = MAP_COORD(bspcoord[check[3]]);

	return IR_IsOccluded(bspcoord[check[0]], bspcoord[check[1]], bspcoord[check[2]], bspcoord[check[3]]);
}

/*
 RenderBSPNode
 
 Renders all subsectors below a given node,
 traversing subtree recursively.
 Because this function is recursive, avoid doing work that
 would give a large stack frame.  Important that the compiler
 doesn't inline big functions.
 */
static void IR_RenderBSPNode(unsigned short bspnum ) {
	while (!(bspnum & NF_SUBSECTOR)) {
		// decision node
		const node_t *bsp = &nodes[bspnum];
		
		// Decide which side the view point is on.
		int side = IR_PointOnSide(viewx, viewy, bsp);

		ir_occlude_total++;
		if (!IR_IsBBoxCompletelyOccluded(bsp->bbox[side])) {
			IR_RenderBSPNode(bsp->children[side]);
		}
		else {
			ir_occlude_cull++;
		}
		// continue down the back space
		if (IR_IsBBoxCompletelyOccluded(bsp->bbox[side ^ 1])) {
			ir_occlude_cull++;
			return;
		}

		bspnum = bsp->children[side^1];
    }
	
	// subsector with contents
	// add all the drawable elements in the subsector
	if ( bspnum == -1 ) {
		bspnum = 0;
	}
	bspnum &= ~NF_SUBSECTOR;
	IR_Subsector( bspnum );
}

static void infinitePerspective(double fovy, double aspect, double znear)
{
	float left, right, bottom, top;	// JDC: was GLdouble
	float m[16];						// JDC: was GLdouble

	top = znear * tan(fovy * __glPi / 360.0);
	bottom = -top;
	left = bottom * aspect;
	right = top * aspect;

	//qglFrustum(left, right, bottom, top, znear, zfar);

	m[0] = (2 * znear) / (right - left);
	m[4] = 0;
	m[8] = (right + left) / (right - left);
	m[12] = 0;

	m[1] = 0;
	m[5] = (2 * znear) / (top - bottom);
	m[9] = (top + bottom) / (top - bottom);
	m[13] = 0;

	m[2] = 0;
	m[6] = 0;
	//m[10] = - (zfar + znear) / (zfar - znear);
	//m[14] = - (2 * zfar * znear) / (zfar - znear);
	m[10] = -1;
	m[14] = -2 * znear;

	m[3] = 0;
	m[7] = 0;
	m[11] = -1;
	m[15] = 0;
#ifdef WIN32
	glMultMatrixf(m);	// JDC: was glMultMatrixd
#endif
#ifdef ARM9
	{
		m4x4 mm;
		int i;
		for (i = 0; i<16; i++)
		{
			mm.m[i] = floattof32(m[i]);
		}
		glMultMatrix4x4(&mm);

	}
#endif
}
typedef struct {
	void *texture;
	void *user;
} texSort_t;

static int TexSort2( const void *a, const void *b ) {
	if ( ((texSort_t *)a)->texture != ((texSort_t *)b)->texture ) {
		if( ((texSort_t *)a)->texture == (void *)skytexture) {return -1;}
		if( ((texSort_t *)b)->texture ==  (void *)skytexture) {return 1;}
		if ( ((texSort_t *)a)->texture < ((texSort_t *)b)->texture ) {
			return -1;
		}
	} else {
		return 0;
	}
	return 1;
}
static int TexSort( const void *a, const void *b ) {
	if ( ((texSort_t *)a)->texture < ((texSort_t *)b)->texture ) {
			return -1;
	}
	return 1;
}


int sky_mask_polyid;

void IR_RenderSkyWall(GLWall *wall) {
	float st[2];
	float xyz[3];
	int dir[2][3];
	int lightInt;
	int name = wall->name;
	int i;
	s64 length;

	dir[0][0] = MAP_COORD(wall->side->sideSeg.v1->x - viewx);
	dir[0][1] = wall->ytop - MAP_COORD(viewz);
	dir[0][2] = MAP_COORD(wall->side->sideSeg.v1->y - viewy);
	
	dir[1][0] = MAP_COORD(wall->side->sideSeg.v2->x - viewx);
	dir[1][1] = wall->ybottom - MAP_COORD(viewz);
	dir[1][2] = MAP_COORD(wall->side->sideSeg.v2->y - viewy);
	for(i=0;i<2;i++) {
		length = dir[i][0]*dir[i][0] + dir[i][1]*dir[i][1] + dir[i][2]*dir[i][2];
		length = sqrt (length>>2);

		if(length == 0)
			length = 1;

		dir[i][0] = (s64)(dir[i][0]*256)/length;
		dir[i][1] = (s64)(dir[i][1]*128)/length;
		//printf("%d - %d %d\n",i,dir[i][0],dir[i][1]);
	}

	dir[0][0] = wall->ul;
	dir[1][0] = wall->ur;

}

void IR_RenderWalls(void) {
	int i,name=-1,texname,lightInt;
	float light;
	ir_vert_t *verts;// [68];
	short indexes[6] = { 0,1,2,0,2,3 };

	//glDisable(GL_DEPTH_TEST);

	//-----------------------------------------
	// draw all the walls, sky walls sorted first
	//-----------------------------------------
	// sort the walls by texture
	texSort_t	*wallSort = (texSort_t	*)alloca( sizeof( wallSort[0] ) * num_gl_walls );
	for (i=0 ; i < num_gl_walls ; i++ ) {
		GLWall *wall = &gl_walls[i];
		wallSort[i].texture = (void *)wall->name;
		wallSort[i].user = wall;
	}
	qsort( wallSort, num_gl_walls, sizeof( wallSort[0] ), TexSort2 );

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, ir_vbo);

	glVertexPointer(3, GL_FLOAT, sizeof(ir_vert_t), 0);
	glNormalPointer(GL_FLOAT, sizeof(ir_vert_t),12);
	glColorPointer(4, GL_FLOAT, sizeof(ir_vert_t), 24);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ir_vert_t), 40);

#ifdef WIN32
	glAlphaFunc(GL_GREATER, 0.5);
#endif

#if 1
#ifdef _3DS
	//printf("IR_RenderWalls: %d\n", num_gl_walls);
#endif
	ir_vert_count += num_gl_walls * 4;
	for(i=0;i<num_gl_walls;i++) {
		texSort_t *sort = &wallSort[i];
		GLWall *wall = (GLWall *)sort->user;

		verts = (ir_vert_t *)glMapBufferRange(GL_ARRAY_BUFFER, ir_vbo_base_vertex*sizeof(ir_vert_t), 4 * sizeof(ir_vert_t), GL_MAP_WRITE_BIT);

		verts[0].st[0] = wall->ul;
		verts[0].st[1] = wall->vt;
		verts[0].xyz[0] = MAP_COORD(wall->side->sideSeg.v1->x);
		verts[0].xyz[2] = wall->ytop;
		verts[0].xyz[1] = MAP_COORD(wall->side->sideSeg.v1->y);
		lightInt = (wall->name == skytexture ? 255 : FadedLighting(verts[0].xyz[0], verts[0].xyz[1], wall->light));
		light = lightInt / 255.0f;
		verts[0].c[0] = light;
		verts[0].c[1] = light;
		verts[0].c[2] = light;
		verts[0].c[3] = 1.0f;
		verts[0].n[0] = -1;
		verts[0].n[1] = -1;
		verts[0].n[2] = -1;


		verts[1].st[0] = wall->ul;
		verts[1].st[1] = wall->vb;
		verts[1].xyz[0] = MAP_COORD(wall->side->sideSeg.v1->x);
		verts[1].xyz[2] = wall->ybottom;
		verts[1].xyz[1] = MAP_COORD(wall->side->sideSeg.v1->y);
		memcpy(verts[1].c, verts[0].c, 16);
		memcpy(verts[1].n, verts[0].n, 16);

		verts[2].st[0] = wall->ur;
		verts[2].st[1] = wall->vb;
		verts[2].xyz[0] = MAP_COORD(wall->side->sideSeg.v2->x);
		verts[2].xyz[2] = wall->ybottom;
		verts[2].xyz[1] = MAP_COORD(wall->side->sideSeg.v2->y);
		lightInt = (wall->name == skytexture ? 255 : FadedLighting(verts[2].xyz[0], verts[2].xyz[1], wall->light));
		light = lightInt / 255.0f;
		verts[2].c[0] = light;
		verts[2].c[1] = light;
		verts[2].c[2] = light;
		verts[2].c[3] = 1.0f;
		memcpy(verts[2].n, verts[0].n, 16);


		verts[3].st[0] = wall->ur;
		verts[3].st[1] = wall->vt;
		verts[3].xyz[0] = MAP_COORD(wall->side->sideSeg.v2->x);
		verts[3].xyz[2] = wall->ytop;
		verts[3].xyz[1] = MAP_COORD(wall->side->sideSeg.v2->y);
		memcpy(verts[3].c, verts[2].c, 16);
		memcpy(verts[3].n, verts[0].n, 16);

		glUnmapBuffer(GL_ARRAY_BUFFER);

		if (wall->name != name) {
			if (name == skytexture) {
#ifdef _3DS
				GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
#else
				glColorMask(1, 1, 1, 1);
#endif
			}
			name = wall->name;
			if (name == skytexture) {
#ifdef _3DS
				GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_DEPTH);
#else
				glColorMask(0, 0, 0, 0);
#endif
				texname = 0;//ds_load_map_texture(name,TEXGEN_TEXCOORD);//0;//ds_load_blank_tex();
			}
			else {
				texname = ds_load_map_texture(name, 0);
			}
		}
		if (wall->flag == GLDWF_M2S) {
#ifdef _3DS
			GPU_SetAlphaTest(true, GPU_GREATER, 0x80);
#else
			glEnable(GL_ALPHA_TEST);
#endif
		}
		if (wall->flag == GLDWF_M1S) {
#ifdef _3DS
			GPU_SetAlphaTest(false, GPU_GREATER, 0x80);
#else
			glDisable(GL_ALPHA_TEST);
#endif
		}
		glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexes, ir_vbo_base_vertex);


		ir_vbo_base_vertex += 4;
	}
#endif

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

#ifdef _3DS
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
#else
	glColorMask(1, 1, 1, 1);
#endif
}

void IR_RenderSprites(player_t* player)
{
	GLSprite *sprite;
	int i,k,name;
	// get the screen space vector for sprites
	float	yaws = -sin((270.0f - yaw) * 3.141592657 / 180.0 );
	float	yawc = cos((270.0f - yaw) * 3.141592657 / 180.0  );
	angle_t ang0, ang1;
	float light[4];
	ir_vert_t *verts;// [68];
	short indexes[6] = { 0,1,2,0,2,3 };

#if 0 // def WIN32
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_ALPHA_TEST );
	glEnable( GL_BLEND );
	glAlphaFunc( GL_GREATER, 0.5 );
#endif	

#ifdef _3DS
	GPU_SetAlphaTest(true, GPU_GREATER, 0x80);
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_COLOR);
#else
	glEnable(GL_ALPHA_TEST);
	glEnable( GL_BLEND );
	glDepthMask(0);
#endif


	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, ir_vbo);

	glVertexPointer(3, GL_FLOAT, sizeof(ir_vert_t), 0);
	glNormalPointer(GL_FLOAT, sizeof(ir_vert_t), 12);
	glColorPointer(4, GL_FLOAT, sizeof(ir_vert_t), 24);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ir_vert_t), 40);

	//iprintf("\n\nnum_gl_sprites: %d\n\n",num_gl_sprites);
#ifdef _3DS
	//printf("IR_RenderSprites: %d\n", num_gl_sprites);
#endif
	ir_vert_count += num_gl_sprites * 4;
	while( 1 )
	{
		// pick out the sprites from farthest to nearest
		fixed_t max_scale=INT_MAX;
		k=-1;
		for (i=0 ; i < num_gl_sprites ; i++ ) {
			sprite = &gl_sprites[i];
			//printf("i: %d max: %d spr: %d\n",i,max_scale,sprite->scale);
			if (sprite->scale<max_scale)
			{
				max_scale=sprite->scale;
				k=i;
			}
		}
		if ( k == -1 ) {
			//printf("k == -1\n");
			break;
		}
		
		sprite = &gl_sprites[k];
		sprite->scale=INT_MAX;
		
		/*if ( sprite->gltexture != last_gltexture ) {
			c_spriteBind++;
		}
		c_spriteDraw++;*/
		
		//gld_BindPatch(sprite->gltexture,sprite->cm);
		//printf("pre: %08x\n",sprite->name);
		name = ds_load_sprite(sprite->name);
		//printf("name: %08x\n",name);
		if(sprite->shadow)
		{
			light[0] = 0.2f;
			light[1] = 0.2f;
			light[2] = 0.2f;
			light[3] = 0.2f;
#ifdef _3DS
			GPU_SetAlphaTest(true, GPU_GREATER, 0x10);
#else
			glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
			glAlphaFunc(GL_GREATER, 0.1); // don't alpha test away the blended-down version
#endif
		}
		else
		{
			light[0] = sprite->light / 255.0f;
			light[1] = sprite->light / 255.0f;
			light[2] = sprite->light / 255.0f;
			light[3] = 1.0f;
			
			// We could do the distance-lighting here, but leaving the sprites
			// brighter is a good accent in most cases.  There are a few places
			// where environmental sprites look a little wrong, but it is probably
			// better in general.
			
			if (player->fixedcolormap) {
				light[0] = 1.0f;
				light[1] = 1.0f;
				light[2] = 1.0f;
			}
		}

		verts = (ir_vert_t *)glMapBufferRange(GL_ARRAY_BUFFER, ir_vbo_base_vertex*sizeof(ir_vert_t), 4 * sizeof(ir_vert_t), GL_MAP_WRITE_BIT);

		verts[0].xyz[0] = MAP_COORD(sprite->x + sprite->x1 * yawc);
		verts[0].xyz[1] = MAP_COORD(sprite->y + sprite->x1 * yaws);
		verts[0].xyz[2] = MAP_COORD(sprite->z + sprite->y1);
		verts[0].st[0] = sprite->ul;
		verts[0].st[1] = sprite->vt;
		memcpy(verts[0].c, light, 16);

		verts[1].xyz[0] = MAP_COORD(sprite->x + sprite->x2 * yawc);
		verts[1].xyz[1] = MAP_COORD(sprite->y + sprite->x2 * yaws);
		verts[1].xyz[2] = MAP_COORD(sprite->z + sprite->y1);
		verts[1].st[0] = sprite->ur;
		verts[1].st[1] = sprite->vt;
		memcpy(verts[1].c, light, 16);

		verts[2].xyz[0] = MAP_COORD(sprite->x + sprite->x2 * yawc);
		verts[2].xyz[1] = MAP_COORD(sprite->y + sprite->x2 * yaws);
		verts[2].xyz[2] = MAP_COORD(sprite->z + sprite->y2);
		verts[2].st[0] = sprite->ur;
		verts[2].st[1] = sprite->vb;
		memcpy(verts[2].c, light, 16);

		verts[3].xyz[0] = MAP_COORD(sprite->x + sprite->x1 * yawc);
		verts[3].xyz[1] = MAP_COORD(sprite->y + sprite->x1 * yaws);
		verts[3].xyz[2] = MAP_COORD(sprite->z + sprite->y2);
		verts[3].st[0] = sprite->ul;
		verts[3].st[1] = sprite->vb;
		memcpy(verts[3].c, light, 16);

		glUnmapBuffer(GL_ARRAY_BUFFER);

		glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexes, ir_vbo_base_vertex);

		ir_vbo_base_vertex += 4;

		if(sprite->shadow)
		{
#ifdef _3DS
			GPU_SetAlphaTest(true, GPU_GREATER, 0x80);
#else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glAlphaFunc(GL_GREATER, 0.5); // don't alpha test away the blended-down version
#endif
		}
	}
#ifdef WIN32
	glDisable( GL_ALPHA_TEST );
	glDepthMask(1);
#endif
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void ds_draw_sky_part(float s, float t, float y, float y1) {
	float light = 1.0f;
	ir_vert_t *verts;// [68];
	short indexes[6] = { 0,1,2,0,2,3 };

#ifdef _3DS
	y = SCREEN_HEIGHT - y;
	y1 = SCREEN_HEIGHT - y1;
#endif

	verts = (ir_vert_t *)glMapBufferRange(GL_ARRAY_BUFFER, ir_vbo_base_vertex*sizeof(ir_vert_t), 4 * sizeof(ir_vert_t), GL_MAP_WRITE_BIT);
	//dsTexCoord2f(s, SCREEN_HEIGHT + v0); dsVertex3f(-depth, y, depth - 1);
	//dsTexCoord2f(s, v0); dsVertex3f(-depth, depth, depth - 1);
	//dsTexCoord2f(s + 256, v0); dsVertex3f(depth, depth, depth - 1);
	//dsTexCoord2f(s + 256, SCREEN_HEIGHT + v0); dsVertex3f(depth, y, depth - 1);

	verts[0].st[0] = s;
	verts[0].st[1] = t + 0.995f;
	verts[0].xyz[0] = 0;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].c[0] = light;
	verts[0].c[1] = light;
	verts[0].c[2] = light;
	verts[0].c[3] = 1.0f;
	verts[0].n[0] = -1;
	verts[0].n[1] = -1;
	verts[0].n[2] = -1;


	verts[1].st[0] = s;
	verts[1].st[1] = t;
	verts[1].xyz[0] = 0;
	verts[1].xyz[1] = y1;
	verts[1].xyz[2] = 0;
	memcpy(verts[1].c, verts[0].c, 16);
	memcpy(verts[1].n, verts[0].n, 16);

	verts[2].st[0] = s + 0.995f;
	verts[2].st[1] = t;
	verts[2].xyz[0] = SCREEN_WIDTH;
	verts[2].xyz[1] = y1;
	verts[2].xyz[2] = 0;
	memcpy(verts[2].c, verts[0].c, 16);
	memcpy(verts[2].n, verts[0].n, 16);


	verts[3].st[0] = s + 0.995f;
	verts[3].st[1] = t + 0.995f;
	verts[3].xyz[0] = SCREEN_WIDTH;
	verts[3].xyz[1] = y;
	verts[3].xyz[2] = 0;
	memcpy(verts[3].c, verts[0].c, 16);
	memcpy(verts[3].n, verts[0].n, 16);

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexes, ir_vbo_base_vertex);
	ir_vbo_base_vertex += 4;
}

void ir_push_2d();
void ir_pop_2d();
void ds_draw_sky() {
	float	s;
	float	y;
	float	dy;
	float	p, p2;
	ir_vert_t *verts;// [68];

	// Note that these texcoords would have to be corrected
	// for different screen aspect ratios or fields of view!
	s = (((270-yaw)+90.0f)/90.0f);
	y = -80.0f / 120.0f;// (1.0f - 2 * 128.0 / 200);
	p = (-pitch) / 90.0f;

	//printf("pitch: %f %f\n", pitch, p);


	ir_push_2d();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, ir_vbo);

	glVertexPointer(3, GL_FLOAT, sizeof(ir_vert_t), 0);
	glNormalPointer(GL_FLOAT, sizeof(ir_vert_t), 12);
	glColorPointer(4, GL_FLOAT, sizeof(ir_vert_t), 24);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ir_vert_t), 40);

#ifdef _3DS
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_COLOR);
#else
	glDepthMask(false);
#endif

	dy = (SCREEN_HEIGHT / 2)*200.0f / 128.0f - 1;
	y = -p*SCREEN_HEIGHT;
	if (p < 0) {
		ds_draw_sky_part(s, 0.005f, y - dy, y);
	}
	ds_draw_sky_part(s, 0.005f, y + dy, y);
	
	y += dy;
	ds_draw_sky_part(s, 0.005f, y, y + dy);

#ifdef _3DS
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
#else
	glDepthMask(true);
#endif

	ir_pop_2d();
}

void ds_print_blocks();
long long ds_time();

#ifdef WIN32
long long __ds_time = 0;
long long ds_time() {
	return ++__ds_time;
}
#endif

#ifdef _3DS
u64 __ds_time = 0;
long long ds_time() {
	return svcGetSystemTick();
}
#endif

void IR_RenderWinding(winding_t *w, float height, float light) {
	int i, count;
	float texmins0,texmins1;
	ir_vert_t *vert;// [68];
	u16 indexes[68 * 3];
	int index;
	float fade;

	while (w) {
		count = w->numpoints;
		texmins0 = w->texmin[0];
		texmins1 = w->texmin[1];

		vert = (ir_vert_t *)glMapBufferRange(GL_ARRAY_BUFFER, ir_vbo_base_vertex*sizeof(ir_vert_t), count * sizeof(ir_vert_t), GL_MAP_WRITE_BIT);

		ir_vert_count += count;

		for (i = 0; i < count; i++) {
			vert[i].n[0] = 1;
			vert[i].n[2] = 1;
			vert[i].n[1] = 1;
			vert[i].xyz[0] = MAP_COORD(w->p[i].x);
			vert[i].xyz[1] = MAP_COORD(w->p[i].y);
			vert[i].xyz[2] = height;
			fade = (float)FadedLighting(vert[i].xyz[0], vert[i].xyz[1], light)/255.0f;
			vert[i].c[0] = fade;
			vert[i].c[1] = fade;
			vert[i].c[2] = fade;
			vert[i].c[3] = 1.0f;
			vert[i].st[0] = ((-w->p[i].x >> 16) + texmins0)/64.0f;
			vert[i].st[1] = ((w->p[i].y >> 16) - texmins1)/64.0f;
		}
		index = 0;
		for (i = 2; i < count; i++) {
			indexes[index + 0] = 0;
			indexes[index + 1] = i-1;
			indexes[index + 2] = i;
			index += 3;
		}
#ifdef _3DS_
		printf("glDrawElements: %d\n", index);
#endif
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glDrawElementsBaseVertex(GL_TRIANGLES, index, GL_UNSIGNED_SHORT, indexes, ir_vbo_base_vertex);

		ir_vbo_base_vertex += count;

		w = w->next;
	}
}

void IR_RenderPlanes() {
	int i;
	int last_name = -1;

	// sort the flats by texture
	qsort(sectorPlanes, numSectorPlanes, sizeof(sectorPlanes[0]), TexSort);

	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, ir_vbo);

	glVertexPointer(3, GL_FLOAT, sizeof(ir_vert_t), 0);
	glNormalPointer(GL_FLOAT, sizeof(ir_vert_t), 12);
	glColorPointer(4, GL_FLOAT, sizeof(ir_vert_t), 24);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ir_vert_t), 40);

#ifdef _3DS
	//printf("IR_RenderPlanes: %d\n", numSectorPlanes);
#endif
	// draw them in texture order
	for (i = 0; i < numSectorPlanes; i++) {
		sortSectorPlane_t *sort = &sectorPlanes[i];
		sector_t *sector = sort->sector;
		winding_t *floor = sector->windings;
		int name;
		int light = sector->lightlevel + (extralight << 5);
		if (floor == 0)
			continue;
		if (light > 255) {
			light = 255;
		}
		if (light < 0)
			light = 0;

		if (sort->texture != last_name) {
			name = ds_load_map_flat(sort->texture);
		}

		IR_RenderWinding(floor, MAP_COORD(sort->ceiling == 0 ? sector->floorheight : sector->ceilingheight), light);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}










// vertex array for glDrawElements() and glDrawRangeElement() =================
// Notice that the sizes of these arrays become samller than the arrays for
// glDrawArrays() because glDrawElements() uses an additional index array to
// choose designated vertices with the indices. The size of vertex array is now
// 24 instead of 36, but the index array size is 36, same as the number of
// vertices required to draw a cube.
GLfloat vertices2[] = {
	0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5,   // v0,v1,v2,v3 (front)
	0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5,   // v0,v3,v4,v5 (right)
	0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5,   // v0,v5,v6,v1 (top)
	-0.5, 0.5, 0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5,   // v1,v6,v7,v2 (left)
	-0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5,   // v7,v4,v3,v2 (bottom)
	0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5 }; // v4,v7,v6,v5 (back)

																		  // normal array
GLfloat normals2[] = { 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,   // v0,v1,v2,v3 (front)
1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,   // v0,v3,v4,v5 (right)
0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,   // v0,v5,v6,v1 (top)
-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,   // v1,v6,v7,v2 (left)
0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,   // v7,v4,v3,v2 (bottom)
0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1 }; // v4,v7,v6,v5 (back)

										  // color array
GLfloat colors2[] = { 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1,   // v0,v1,v2,v3 (front)
1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1,   // v0,v3,v4,v5 (right)
1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0,   // v0,v5,v6,v1 (top)
1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0,   // v1,v6,v7,v2 (left)
0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,   // v7,v4,v3,v2 (bottom)
0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1 }; // v4,v7,v6,v5 (back)

									  // tecoord array
GLfloat texcoord2[] = {
	1,1,	0,1,	0,0,	1,0,   // v0,v1,v2,v3 (front)
	1,1,	0,1,	0,0,	1,0,   // v0,v3,v4,v5 (right)
	1,1,	1,0,	0,0,	0,1,   // v0,v5,v6,v1 (top)
	1,1,	1,0,	0,0,	0,1,   // v1,v6,v7,v2 (left)
	0,0,	1,0,	1,1,	0,1,   // v7,v4,v3,v2 (bottom)
	1,0,	0,0,	0,1,	1,1 }; // v4,v7,v6,v5 (back)

								   // index array of vertex array for glDrawElements() & glDrawRangeElement()
GLshort indices2[] = {
	0, 1, 2, 2, 3, 0,      // front
	4, 5, 6, 6, 7, 4,      // right
	8, 9, 10, 10, 11, 8,      // top
	12, 13, 14, 14, 15, 12,      // left
	16, 17, 18, 18, 19, 16,      // bottom
	20, 21, 22, 22, 23, 20 };    // back

#ifdef _3DS_

typedef struct {
	float xyz[3];
	float n[3];
	float c[4];
	float st[2];
} ir_vert_tt;

void set_shader_shader() {
	glUseProgram(0);
	u32 bufferOffsets = 0;
	u64 bufferPermutations = 0x3210;;
	u8 bufferNumAttributes = 4;
	GPU_SetAttributeBuffers(4, (u32*)osConvertVirtToPhys((void *)__ctru_linear_heap),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 3, GPU_FLOAT) | GPU_ATTRIBFMT(2, 4, GPU_FLOAT) | GPU_ATTRIBFMT(3, 2, GPU_FLOAT),
		0xFFC, 0x3210, 1, &bufferOffsets, &bufferPermutations, &bufferNumAttributes);
	GPU_SetTextureEnable(GPU_TEXUNIT0);
	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF);
}

#endif

void draw2()
{

#ifdef _3DS
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -1.0);// +0.5*sinf(angleX));
	glScalef(1.7, 1.7, 1.7);
	//glRotateX(angleX);
	//glRotateY(angleY);
	//glRotateZ(angleZ);
	//glRotatef(angleX, 0, 1, 0);

	// Rotate the cube each frame
	//angleX += M_PI / 180;
	//angleY += M_PI / 360;
	//angleZ += M_PI / 720;


	ds_load_map_flat(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// enable and specify pointers to vertex arrays
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, vertices2);
	glNormalPointer(GL_FLOAT, 0, normals2);
	glColorPointer(3, GL_FLOAT, 0, colors2);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord2);

	glDrawElements(GL_TRIANGLES, 36, GL_SHORT, indices2);
	return;

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
#endif
}
















/*
 IR_RenderPlayerView
 
 Replace the prBoom rendering code with a higher performance
 version.  Most of the fancy new features are gone, because I
 have no idea what the reight test cases would be for them.
 */
extern int heretic_prog;

#ifdef _3DS
void initFrustumMatrix(float left, float right, float bottom, float top, float near, float far)
{
	matrix_4x4 mp;
	matrix_4x4 mp2;
	matrix_4x4 m;
	float A = (right + left) / (right - left);
	float B = (top + bottom) / (top - bottom);
	float C = -(far + near) / (far - near);
	float D = (-2.0f * far * near) / (far - near);

	mp.m[0x0] = (2 * near) / (right - left);
	mp.m[0x1] = 0.0f;
	mp.m[0x2] = A;
	mp.m[0x3] = 0.0f;

	mp.m[0x4] = 0.0f;
	mp.m[0x5] = (2.0f * near) / (top - bottom);
	mp.m[0x6] = B;
	mp.m[0x7] = 0.0f;

	mp.m[0x8] = 0.0f;
	mp.m[0x9] = 0.0f;
	mp.m[0xA] = C;
	mp.m[0xB] = D;

	mp.m[0xC] = 0.0f;
	mp.m[0xD] = 0.0f;
	mp.m[0xE] = -1.0f;
	mp.m[0xF] = 0.0f;

	m4x4_identity(&mp2);
	mp2.m[0xA] = 0.5;
	mp2.m[0xB] = -0.5;

	m4x4_multiply(&m, &mp2, &mp);
	glLoadMatrixf(m.m);
	/*printf("left: %f\n", left);
	printf("right: %f\n", right);
	printf("top: %f\n", top);
	printf("bottom: %f\n", bottom);
	printf("A: %f\n", A);
	printf("B: %f\n", B);
	printf("C: %f\n", C);
	printf("D: %f\n", D);*/
}

void ApplyFrustum(float mNearClippingDistance, float mFarClippingDistance, float mFOV, float mAspectRatio, float mConvergence, float mEyeSeparation)
{
	float top, bottom, left, right;

#if 1
	float tanfov = tan(mFOV / 2);
	float fshift = (mEyeSeparation / 2.0) * mNearClippingDistance / mConvergence;

	bottom = -mNearClippingDistance*tanfov + fshift;
	top = mNearClippingDistance*tanfov + fshift;

	right = mAspectRatio * mNearClippingDistance * tanfov;
	left = -right;
#else
	top = mNearClippingDistance * tan(mFOV / 2);
	bottom = -top;

	float a = mAspectRatio * tan(mFOV / 2) * mConvergence;

	float b = a - mEyeSeparation / 2;
	float c = a + mEyeSeparation / 2;

	left = -b * mNearClippingDistance / mConvergence;
	right = c * mNearClippingDistance / mConvergence;
#endif

	// Set the Projection Matrix
	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	initFrustumMatrix(left, right, bottom, top, mNearClippingDistance, mFarClippingDistance);

	// Displace the world to right
	//gsMatrixMode(GS_MODELVIEW);
	//gsLoadIdentity();
	//glTranslatef(mEyeSeparation / 2, 0.0f, 0.0f);
}

#endif

int render_occlude = 0;

void IR_RenderPlayerView(player_t* player) {
	float trY;
	float xCamera, yCamera;
	float _yaws, _yawc;

	//int	start = 0;
	int height;
	int i, last_name = -1;
	int texname;
	long long s1, s2, s3, s4, s5;

	viewplayer = player;
	viewwidthf = viewwidth << 16;

	viewx = player->mo->x;
	viewy = player->mo->y;
	viewz = player->viewz;
	viewangle = player->mo->angle;
	extralight = player->extralight;	// gun flashes

	yaw = 270.0f - (float)(viewangle >> ANGLETOFINESHIFT)*360.0f / FINEANGLES;

	viewsin = finesine[viewangle >> ANGLETOFINESHIFT];
	viewcos = finecosine[viewangle >> ANGLETOFINESHIFT];

	/*// IR goggles
	if (player->fixedcolormap) {
		fixedcolormap = fullcolormap + player->fixedcolormap*256*sizeof(lighttable_t);
	} else {
		fixedcolormap = 0;
	}*/

	// this is used to tell if a line, sector, or sprite is going to be drawn this frame
	validcount++;
	r_framecount++;

	//gld_SetPalette(-1);

	/*if (screenblocks == 11)
		height = DS_SCREEN_HEIGHT-1;
	else if (screenblocks == 10)
		height = DS_SCREEN_HEIGHT-2;
	else
		height = (screenblocks*DS_SCREEN_HEIGHT/10) & ~7;*/

	height = viewheight;


	//glScissor(viewwindowx, DS_SCREEN_HEIGHT-(viewheight+viewwindowy), viewwidth, viewheight);
	//glEnable(GL_SCISSOR_TEST);
#ifdef WIN32
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_CULL_FACE);
#endif

#ifdef _3DS
	GPU_SetAlphaTest(false, GPU_ALWAYS, 0x00);
#else
	glDisable(GL_ALPHA_TEST);
#endif


	// Player coordinates
	xCamera = MAP_COORD(viewx);
	yCamera = MAP_COORD(viewy);
	trY = MAP_COORD(viewz);

#if 0
	viewx = 432<<16;
	viewy = 792<<16;
	xCamera = 432;
	yCamera = 792;
	trY = 97;
#endif

	yaw = (float)(viewangle >> ANGLETOFINESHIFT)*360.0f / FINEANGLES;
	inv_yaw = -90.0f + (float)(viewangle >> ANGLETOFINESHIFT)*360.0f / FINEANGLES;
	pitch = player->lookdir * 42.5 / 110.0;

	//draw2();

	glEnable(GL_TEXTURE_2D);

	// To make it easier to accurately mimic the GL model to screen transformation,
	// this is set up so that the projection transformation is also done in the
	// modelview matrix, leaving the projection matrix as an identity.  This means
	// that things done in eye space, like lighting and fog, won't work, but
	// we don't need them.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

#ifdef _3DS
	ApplyFrustum(4.0f, 8192.0f, 90.0f * M_PI / 180.0f, 240.0f / 400.0f, 500.0f, 0.0f);
	glRotateZ(90.0f);
	if (0) {
		float projm[14];
		int i;
		glGetFloatv(GL_PROJECTION_MATRIX, projm);
		printf("yaw sin: %3.6f\n", _yaws);
		printf("yaw cos: %3.6f\n", _yawc);
		for (i = 0; i < 16; i += 4) {
			printf("%2d: %+4.3f %+4.3f %+4.3f %+4.3f\n", i, projm[i], projm[i + 1], projm[i + 2], projm[i + 3]);
		}
		while (1) {
			gspWaitForVBlank();
		}
	}
	//infinitePerspective(64.0f, 400.0f / 240.0f, 0.05f);
#endif

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// make the 320x480 hardware seem like 480x320 in two different orientations	
	// and note if the occlusion segmenrs need to be reversed
	//reversedLandscape = iphoneRotateForLandscape();
	//glRotatef( -90, 0, 0, 1 );
#ifdef _WIN32
	infinitePerspective(64.0f, 400.0f / 240.0f, 4.0f);
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

	static float xp = 32;
	static float xr = 0;
	static float xy = 0;
	static float px = 0;
	static float py = 0;
	static float pz = 0;

#if _3DS
	glRotateY(-90.0f);
	glRotateX(90.0f);
	glRotateZ(180.0f);

	glRotateX(roll);
	glRotateY(pitch);
	glRotateZ(yaw);
	glTranslatef(-xCamera, -yCamera, -trY);
#else
	int zofs = 0;
	if (render_occlude) {
		zofs = 800;
		glRotateZ(90.0f);

		glRotateX(-roll);
		glRotateY(pitch);
		glRotateZ(-yaw);
		glTranslatef(0, 0, -(trY + zofs));
	} else {
		glRotateX(-90.0f);
		glRotateZ(90.0f);

		glRotateX(-roll);
		glRotateY(pitch);
		glRotateZ(-yaw);
		glTranslatef(-xCamera, -yCamera, -(trY + zofs));
	 }
#endif

	//printf("pos: %6.2f %6.2f %6.2f %3.0f %3.0f %3.0f\n", -xCamera, -trY, -yCamera, roll, pitch, yaw);

/*	xp += 1;
	if (xp > 360) xp = 0;
	
	xr += 4;
	if (xr > 360) xr = 0;
	
	xy += 8;
	if (xy > 360) xy = 0;
	
	px += 12;
	if (px > 400) px = -400;

	py += 8;
	if (py > 400) py = -400;

	pz += 4;
	if (pz > 400) pz = -400;*/

	_yaws = sin(yaw * 3.141592657 / 180.0);
	_yawc = -cos(yaw * 3.141592657 / 180.0);

	// read back the matrix so we can do exact calculations that match
	// what GL is doing.  It would probably be better to build the matricies
	// ourselves and just do a loadMatrix...
	glGetFloatv( GL_MODELVIEW_MATRIX, glMVPmatrix );
#ifdef _3DS_
	{
		printf("yaw sin: %3.6f\n", _yaws);
		printf("yaw cos: %3.6f\n", _yawc);
		int i = 0;
		for (i = 0; i < 16; i++) {
			printf("%2d: %6.6f\n", i, glMVPmatrix[i]);
		}
		while (1);
	}
#endif
	
#ifdef WIN32
	glGetDoublev( GL_MODELVIEW_MATRIX, glMVPmatrixd );
	glGetDoublev( GL_PROJECTION_MATRIX, glPJPmatrixd );
	glGetIntegerv( GL_VIEWPORT, glView );
#endif

	// setup the vector for calculating light fades, which is just a scale
	// of the forward vector
	lightingVector[0] = lightDistance * -_yawc;// glMVPmatrix[2];
	lightingVector[1] = lightDistance * _yaws;// glMVPmatrix[6];
	lightingVector[2] = lightDistance * (_yawc*xCamera - _yaws*yCamera);// glMVPmatrix[14];
	//printf("vpn: %f %f %f\n%f %f", (float)glMVPmatrix[2], (float)glMVPmatrix[6], (float)glMVPmatrix[14], _yaws, _yawc);
	//printf("vpn: %f %f %f\n", (float)lightingVector[0], (float)lightingVector[1], (float)lightingVector[2]);

	numSectorPlanes = 0;
	num_gl_walls = 0;
	num_gl_sprites = 0;

	c_occludedSprites = 0;
	c_sectors = 0;
	c_subsectors = 0;
	failCount = 0;

	// get the screen space vector for sprites
	ir_yaws = -sin((270.0f - yaw) * 3.141592657 / 180.0);
	ir_yawc = cos((270.0f - yaw) * 3.141592657 / 180.0);

	
	// Find everything we need to draw, but don't draw anything yet,
	// because we want to sort by texture to reduce GL driver overhead.
	IR_RenderBSPNode( numnodes-1 );

	ds_load_sky_texture(skytexture,0);
	ds_draw_sky();

	s1 = ds_time();
	//printf("ir cull: %6d %6d\n", ir_occlude_cull, ir_occlude_total);

	IR_RenderWalls();
	s2 = ds_time();


	IR_RenderPlanes();
	
	s3 = ds_time();

	IR_RenderSprites(player);
	s4 = ds_time();

	IR_DrawPlayerSprites();
	s5 = ds_time();

	if (ir_vert_count > ir_vert_count_max) {
		ir_vert_count_max = ir_vert_count;
	}

	//printf("ir_vert_count: %6d %6d\n", ir_vert_count, ir_vert_count_max);

	ir_vert_count = 0;

}

/*
========================
=
= R_DrawPSprite
=
========================
*/

int PSpriteSY[NUMWEAPONS] =
{
	0,				// staff
	5 *FRACUNIT,	// goldwand
	15*FRACUNIT,	// crossbow
	15*FRACUNIT,	// blaster
	15*FRACUNIT,	// skullrod
	15*FRACUNIT,	// phoenix rod
	15*FRACUNIT,	// mace
	15*FRACUNIT,	// gauntlets
	15*FRACUNIT		// beak
};

extern fixed_t		pspritescale, pspriteiscale;
extern fixed_t		pspritescalex, pspritescaley;
extern int				centerx, centery;
#define	DSBASEYCENTER			(192/2)

#ifdef _3DS
#define YPOS(_y) (SCREEN_HEIGHT - (_y))
#else
#define YPOS(_y) (_y)
#endif

void IR_DrawPSprite (pspdef_t *psp)
{
	fixed_t		tx;
	int			x1, x2;
	spritedef_t	*sprdef;
	spriteframe_t	*sprframe;
	int			lump;
	boolean		flip;
	ir_vert_t *verts;// [68];
	short indexes[6] = { 0,1,2,0,2,3 };
	float light[4];
	//vissprite_t	*vis, avis;

	int tempangle;

	int name;
	int v_texturemid,v_x1,v_x2,v_flip,v_y1,v_y2,h;
	int v_vt, v_vb, v_ul, v_ur;
	float f_vt, f_vb, f_ul, f_ur;
	dstex_t *ds;
	float scale,scaley;

//
// decide which patch to use
//
#ifdef RANGECHECK
	if ( (unsigned)psp->state->sprite >= numsprites)
		I_Error ("R_ProjectSprite: invalid sprite number %i "
		, psp->state->sprite);
#endif
	sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
	if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
		I_Error ("R_ProjectSprite: invalid sprite frame %i : %i "
		, psp->state->sprite, psp->state->frame);
#endif
	sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

	lump = sprframe->lump[0];
	flip = (boolean)sprframe->flip[0];

//
// calculate edges of the shape
//
	tx = psp->sx-160*FRACUNIT;
	//printf("tx: %x\n",tx);

	tx -= spriteoffset[lump];
	if(viewangleoffset)
	{
		tempangle = ((centerxfrac/1024)*(viewangleoffset>>ANGLETOFINESHIFT));
	}
	else
	{
		tempangle = 0;
	}
	x1 = (centerxfrac + FixedMul (tx,pspritescale)+tempangle ) >>FRACBITS;
	if (x1 > viewwidth)
		return;		// off the right side
	//tx +=  spritewidth[lump];
	//x2 = ((centerxfrac + FixedMul (tx, pspritescale)+tempangle ) >>FRACBITS) - 1;
	x2 = x1 + (FixedMul(pspritescale,spritewidth[lump])>>FRACBITS);
	if (x2 < 0)
		return;		// off the left side

	ds = &sprites_ds[lump];
	v_texturemid = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-spritetopoffset[lump]);
	//printf("v_texturemid: %f\n",(float)(v_texturemid/65536.0));
	//v_texturemid -= (8<<FRACBITS);
	if(1)//viewheight != SCREEN_HEIGHT)
	{
		v_texturemid -= PSpriteSY[players[consoleplayer].readyweapon];
	}

	v_x1 = x1 < 0 ? 0 : x1;
	v_x2 = x2 >= viewwidth ? viewwidth-1 : x2;
	scale = ((float)pspritescalex / (float)FRACUNIT);
	scaley = ((float)pspritescaley / (float)FRACUNIT);
	v_y1 = viewwindowy + centery - (int)(((float)v_texturemid / (float)FRACUNIT)*scaley);
	//v_y1=viewwindowy+centery-((v_texturemid*scale) / (float)FRACUNIT);
	h = spriteheight[lump]>>FRACBITS;
	v_y2=v_y1+(int)((float)h*scaley)+1;
	
	v_vt=0;
	v_vb=(ds->height);
	if (flip)
	{
		v_ul=(ds->width);
		v_ur=0;
	}
	else
	{
		v_ul=0;
		v_ur=(ds->width);
	}
	f_ul = (float)v_ul / (float)ds->block_width;
	f_ur = (float)v_ur / (float)ds->block_width;
	f_vb = (float)v_vb / (float)ds->block_height;
	f_vt = (float)v_vt / (float)ds->block_height;

#if 1
//
// store information in a vissprite
//
	/*vis = &avis;
	vis->mobjflags = 0;
	vis->psprite = true;
	vis->texturemid = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-spritetopoffset[lump]);
	if(viewheight == SCREENHEIGHT)
	{
		vis->texturemid -= PSpriteSY[players[consoleplayer].readyweapon];
	}
	vis->x1 = x1 < 0 ? 0 : x1;
	vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
	vis->scale = pspritescale<<detailshift;
	if (flip)
	{
		vis->xiscale = -pspriteiscale;
		vis->startfrac = spritewidth[lump]-1;
	}
	else
	{
		vis->xiscale = pspriteiscale;
		vis->startfrac = 0;
	}
	if (vis->x1 > x1)
		vis->startfrac += vis->xiscale*(vis->x1-x1);
	vis->patch = lump;

	if(viewplayer->powers[pw_invisibility] > 4*32 ||
	viewplayer->powers[pw_invisibility] & 8)
	{
		// Invisibility
		vis->colormap = spritelights[MAXLIGHTSCALE-1];
		vis->mobjflags |= MF_SHADOW;
	}
	else if(fixedcolormap)
	{
		// Fixed color
		vis->colormap = fixedcolormap;
	}
	else if(psp->state->frame & FF_FULLBRIGHT)
	{
		// Full bright
		vis->colormap = colormaps;
	}
	else
	{
		// local light
		vis->colormap = spritelights[MAXLIGHTSCALE-1];
	}
	//R_DrawVisSprite(vis, vis->x1, vis->x2);*/

	name = ds_load_sprite(lump);
	//glBindTexture(GL_TEXTURE_2D, 0);
	if (0)//sprite->shadow)
	{
		light[0] = 0.2f;
		light[1] = 0.2f;
		light[2] = 0.2f;
		light[3] = 0.2f;
#ifdef WIN32
		glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.2f, 0.2f, 0.2f, 0.33f);
		glAlphaFunc(GL_GREATER, 0.1);	// don't alpha test away the blended-down version
#endif
	}
	else
	{
		light[0] = 1.0f; //sprite->light / 255.0f;
		light[1] = 1.0f; //sprite->light / 255.0f;
		light[2] = 1.0f; //sprite->light / 255.0f;
		light[3] = 1.0f;

		// We could do the distance-lighting here, but leaving the sprites
		// brighter is a good accent in most cases.  There are a few places
		// where environmental sprites look a little wrong, but it is probably
		// better in general.

		if (fixedcolormap) {
			light[0] = 1.0f;
			light[1] = 1.0f;
			light[2] = 1.0f;
		}
	}

	verts = (ir_vert_t *)glMapBufferRange(GL_ARRAY_BUFFER, ir_vbo_base_vertex*sizeof(ir_vert_t), 4 * sizeof(ir_vert_t), GL_MAP_WRITE_BIT);

	verts[0].xyz[0] = v_x1;
	verts[0].xyz[1] = YPOS(v_y1);
	verts[0].xyz[2] = 0;
	verts[0].st[0] = f_ul;
	verts[0].st[1] = f_vt;
	memcpy(verts[0].c, light, 16);

	verts[1].xyz[0] = v_x1;
	verts[1].xyz[1] = YPOS(v_y2);
	verts[1].xyz[2] = 0;
	verts[1].st[0] = f_ul;
	verts[1].st[1] = f_vb;
	memcpy(verts[1].c, light, 16);

	verts[2].xyz[0] = v_x2;
	verts[2].xyz[1] = YPOS(v_y2);
	verts[2].xyz[2] = 0;
	verts[2].st[0] = f_ur;
	verts[2].st[1] = f_vb;
	memcpy(verts[2].c, light, 16);

	verts[3].xyz[0] = v_x2;
	verts[3].xyz[1] = YPOS(v_y1);
	verts[3].xyz[2] = 0;
	verts[3].st[0] = f_ur;
	verts[3].st[1] = f_vt;
	memcpy(verts[3].c, light, 16);

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexes, ir_vbo_base_vertex);

	ir_vbo_base_vertex += 4;

	if (0)//sprite->shadow)
	{
#ifdef WIN32
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glAlphaFunc(GL_GREATER, 0.5);
#endif
	}

#else
	// ------------ gld_AddSprite ----------
	{
	//mobj_t *pSpr= thing;
	GLSprite sprite;
	float voff,hoff;
	
	//sprite.scale= FixedDiv(projectiony, tz);
	//if (pSpr->frame & FF_FULLBRIGHT)
		sprite.light = 255;
	//else
	//	sprite.light = pSpr->subsector->sector->lightlevel+(extralight<<5);
	//sprite.cm=CR_LIMIT+(int)((pSpr->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
	sprite.name=lump;//gltexture=gld_RegisterPatch(lump+firstspritelump,sprite.cm);
	//if (!sprite.gltexture)
	//	return;
	sprite.shadow = 0;//(pSpr->flags & MF_SHADOW) != 0;
	//sprite.trans  = (pSpr->flags & MF_TRANSLUCENT) != 0;
	/*if (movement_smooth)
	{
		sprite.x = (float)(-pSpr->PrevX + FixedMul (tic_vars.frac, -pSpr->x - (-pSpr->PrevX)))/MAP_SCALE;
		sprite.y = (float)(pSpr->PrevZ + FixedMul (tic_vars.frac, pSpr->z - pSpr->PrevZ))/MAP_SCALE;
		sprite.z = (float)(pSpr->PrevY + FixedMul (tic_vars.frac, pSpr->y - pSpr->PrevY))/MAP_SCALE;
	}
	else
	{*/
		sprite.x= 0;//-(pSpr->x);
		sprite.y= 128;//(pSpr->z);
		sprite.z= 0;//(pSpr->y);
	//}

#ifdef ARM9
	sprite.vt=0;
	sprite.vb=(ds->height);
	if (flip)
	{
		sprite.ul=0;
		sprite.ur=(ds->width);
	}
	else
	{
		sprite.ul=(ds->width);
		sprite.ur=0;
	}
#else
	sprite.vt=0.0f;
	sprite.vb=(float)ds->height/(float)ds->block_height;
	if (flip)
	{
		sprite.ul=0.0f;
		sprite.ur=(float)ds->width/(float)ds->block_width;
	}
	else
	{
		sprite.ul=(float)ds->width/(float)ds->block_width;
		sprite.ur=0.0f;
	}
#endif
	//hoff=(leftoffset);
	//voff=(topoffset);
	sprite.x1=v_x1;//(hoff-width);
	sprite.x2=v_x2;//(hoff);
	sprite.y1=v_y1;//(voff);
	sprite.y2=v_y2;//(voff-height);
	
	// JDC: don't let sprites poke below the ground level.
	// Software rendering Doom didn't use depth buffering, 
	// so sprites always got drawn on top of the flat they
	// were on, but in GL they tend to get a couple pixel
	// rows clipped off.
	if ( sprite.y2 < 0 ) {
		sprite.y1 -= sprite.y2;
		sprite.y2 = 0;
	}
	
	if(num_gl_sprites < 128) 
	{
		//printf("%d %f %f\n",num_gl_sprites,(float)width/65536.0f,(float)hoff/65536.0f);
		gl_sprites[num_gl_sprites++] = sprite;
	}

	}

#endif
}

/*
========================
=
= R_DrawPlayerSprites
=
========================
*/

void IR_DrawPlayerSprites (void)
{
	int			i, lightnum;
	pspdef_t	*psp;
/*
//
// get light level
//
	lightnum = (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT)
		+extralight;
	if (lightnum < 0)
		spritelights = scalelight[0];
	else if (lightnum >= LIGHTLEVELS)
		spritelights = scalelight[LIGHTLEVELS-1];
	else
		spritelights = scalelight[lightnum];
//
// clip to screen bounds
//
*/
	ir_push_2d();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, ir_vbo);

	glVertexPointer(3, GL_FLOAT, sizeof(ir_vert_t), 0);
	glNormalPointer(GL_FLOAT, sizeof(ir_vert_t), 12);
	glColorPointer(4, GL_FLOAT, sizeof(ir_vert_t), 24);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ir_vert_t), 40);

//
// add all active psprites
//
	for (i=0, psp=viewplayer->psprites ; i<NUMPSPRITES ; i++,psp++) {
		if (psp->state) {
			IR_DrawPSprite (psp);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	ir_pop_2d();

}

#define BUFFER_OFFSET(x) ((const void*)(x))
#define BUFFER_OFFSET_ADD(x,o) ((const void*)(((u32)(x))+((u32)(o))))

#ifdef WIN32
void *GetAnyGLFuncAddress(const char *name)
{
	void *p = (void *)wglGetProcAddress(name);
	if (p == 0 ||
		(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		p = (void *)GetProcAddress(module, name);
	}

	return p;
}

void ir_init_win32() {
	glGenBuffers = GetAnyGLFuncAddress("glGenBuffers");
	glBindBuffer = GetAnyGLFuncAddress("glBindBuffer");
	glBufferData = GetAnyGLFuncAddress("glBufferData");
	glMapBuffer = GetAnyGLFuncAddress("glMapBuffer");
	glMapBufferRange = GetAnyGLFuncAddress("glMapBufferRange");
	glUnmapBuffer = GetAnyGLFuncAddress("glUnmapBuffer");

	glGenVertexArrays = GetAnyGLFuncAddress("glGenVertexArrays");
	glBindVertexArray = GetAnyGLFuncAddress("glBindVertexArray");
	glDrawElementsBaseVertex = GetAnyGLFuncAddress("glDrawElementsBaseVertex");
}
#endif

#define IR_MAX_VERTEX 32000

#ifdef WIN32
#define gpuFrameBegin()
#define gpuFrameEnd()
#endif


void ir_frame_start() {
	gpuFrameBegin();
	ir_vbo_base_vertex = 0;
	ir_occlude_total = 0;
	ir_occlude_cull = 0;
	IR_ResetOccluder();
#ifdef WIN32
	frameBegin();
	glViewport(viewwindowx, SCREEN_HEIGHT - (viewheight + viewwindowy - ((viewheight - viewheight) / 2)), viewwidth, viewheight);
	//glViewport(viewwindowx, SCREEN_HEIGHT-(viewheight+viewwindowy), viewwidth, viewheight);
#endif
}

void ir_frame_end() {
#ifdef WIN32
	frameEnd();
#endif
	gpuFrameEnd();
}

#ifdef _3DS
void initOrthoMatrix(float width, float height, float near, float far)
{
	matrix_4x4 m;
	m4x4_identity(&m);
#if 1
	m.m[0 * 4 + 0] = 0.0f;
	m.m[0 * 4 + 1] = 2.0f / (height);
	m.m[0 * 4 + 2] = 0;
	m.m[0 * 4 + 3] = -1.0f;

	m.m[1 * 4 + 0] = -2.0f / (width);
	m.m[1 * 4 + 1] = 0.0f;
	m.m[1 * 4 + 2] = 0.0f;
	m.m[1 * 4 + 3] = 1.0f;

	m.m[2 * 4 + 0] = 0;
	m.m[2 * 4 + 1] = 0;
	m.m[2 * 4 + 2] = 1.0f / (far - near);
	m.m[2 * 4 + 3] = -0.5f - 0.5f * (far + near) / (far - near);
#else
	m.m[0 * 4 + 0] = 0.0f;
	m.m[0 * 4 + 1] = 1.0f / (height * 0.5f);
	m.m[0 * 4 + 3] = -1.0f;
	m.m[1 * 4 + 0] = -1.0f / (width * 0.5f);
	m.m[1 * 4 + 1] = 0.0f;
	m.m[1 * 4 + 3] = 1.0f;
	m.m[2 * 4 + 2] = 1.0f / (far - near);
	m.m[2 * 4 + 3] = -0.5f - 0.5f * (far + near) / (far - near);
#endif
	glLoadMatrixf(m.m);
	//glRotateZ(45);
}
#endif

void ir_push_2d() {
#ifdef WIN32
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	//	glDisable (GL_ALPHA_TEST);

	glColor4f(1, 1, 1, 1);
#else
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	initOrthoMatrix(400.0f, 240.0f, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, GPU_WRITE_ALL);
#endif
}

void ir_pop_2d() {
#ifdef WIN32
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
#else
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
#endif
}

void ir_video_init();

void IR_Init() {
#ifdef WIN32
	ir_init_win32();
#endif


	// Create the vbo
	glGenBuffers(1, &ir_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ir_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ir_vert_t)*IR_MAX_VERTEX, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ir_video_init();
}

#endif
