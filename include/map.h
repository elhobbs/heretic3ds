#ifndef __IMAPTYPES_H__
#define __IMAPTYPES_H__

/*
==============================================================================

					INTERNAL MAP TYPES

==============================================================================
*/

//================ used by play and refresh

typedef struct
{
	union {
		struct {
	fixed_t		x,y;
		};
	fixed_t		v[2];
	};
} vertex_t;

//struct line_s;

typedef	struct
{
	fixed_t		floorheight, ceilingheight;
	short		floorpic, ceilingpic;
	short		lightlevel;
	short		special, tag;

	struct winding_s *windings;

	int			soundtraversed;		// 0 = untraversed, 1,2 = sndlines -1
	mobj_t		*soundtarget;		// thing that made a sound (or null)
	
	int			blockbox[4];		// mapblock bounding box for height changes
	degenmobj_t	soundorg;			// for any sounds played by the sector

	int			validcount;			// if == validcount, already checked
	mobj_t		*thinglist;			// list of mobjs in sector
	void		*specialdata;		// thinker_t for reversable actions
	int			linecount;
	struct line_s	**lines;			// [linecount] size
} sector_t;


typedef enum {ST_HORIZONTAL, ST_VERTICAL, ST_POSITIVE, ST_NEGATIVE} slopetype_t;

typedef struct line_s
{
	vertex_t	*v1, *v2;
	fixed_t		dx,dy;				// v2 - v1 for side checking
	short		flags;
	short		special, tag;
	short		sidenum[2];			// sidenum[1] will be -1 if one sided
	fixed_t		bbox[4];
	slopetype_t	slopetype;			// to aid move clipping
	sector_t	*frontsector, *backsector;
	int			validcount;			// if == validcount, already checked
	void		*specialdata;		// thinker_t for reversable actions
} line_t;


typedef struct subsector_s
{
	sector_t	*sector;
	short		numlines;
	short		firstline;
} subsector_t;

typedef struct
{
	vertex_t	*v1, *v2;
	fixed_t		offset;
	angle_t		angle;
	struct side_s		*sidedef;
	line_t		*linedef;
	// figgi -- needed for glnodes
	float     length;
	sector_t	*frontsector;
	sector_t	*backsector;		// NULL for one sided lines
} seg_t;

typedef struct side_s
{
	fixed_t		textureoffset;		// add this to the calculated texture col
	fixed_t		rowoffset;			// add this to the calculated texture top
	short		toptexture, bottomtexture, midtexture;
	sector_t	*sector;

	seg_t	sideSeg;	// This segment stretches the entire length of the line,
						// even if the line was broken into multiple seg_t by
						// the bsp.
} side_t;

typedef struct
{
	fixed_t		x,y,dx,dy;			// partition line
	fixed_t		bbox[2][4];			// bounding box for each child
	unsigned short	children[2];		// if NF_SUBSECTOR its a subsector
} node_t;

// Sprites are patches with a special naming convention so they can be 
// recognized by R_InitSprites.  The sprite and frame specified by a 
// thing_t is range checked at run time.
// a sprite is a patch_t that is assumed to represent a three dimensional
// object and may have multiple rotations pre drawn.  Horizontal flipping 
// is used to save space. Some sprites will only have one picture used
// for all views.  

typedef struct
{
	boolean		rotate;		// if false use 0 for any position
	short		lump[8];	// lump to use for view angles 0-7
	byte		flip[8];	// flip (1 = flip) to use for view angles 0-7
} spriteframe_t;

typedef struct
{
	int				numframes;
	spriteframe_t	*spriteframes;
} spritedef_t;

extern	int			numvertexes;
extern	vertex_t	*vertexes;

extern	int			numsegs;
extern	seg_t		*segs;

extern	int			numsectors;
extern	sector_t	*sectors;

extern	int			numsubsectors;
extern	subsector_t	*subsectors;

extern	int			numnodes;
extern	node_t		*nodes;

extern	int			numlines;
extern	line_t		*lines;

extern	int			numsides;
extern	side_t		*sides;

extern	spritedef_t		*sprites;
extern	int				numsprites;

typedef byte	lighttable_t;		// this could be wider for >8 bit display

extern	int			skyflatnum;
extern	fixed_t		*textureheight;		// needed for texture pegging
extern	fixed_t		*spritewidth;		// needed for pre rendering (fracs)
extern	fixed_t		*spriteheight;		// needed for pre rendering (fracs)
extern	fixed_t		*spriteoffset;
extern	fixed_t		*spritetopoffset;
extern	lighttable_t	*colormaps;
extern	int		viewwidth, scaledviewwidth, viewheight;
extern	int			firstflat;
extern	int			numflats;

extern	int			*flattranslation;		// for global animation
extern	int			*texturetranslation;	// for global animation

extern	int		firstspritelump, lastspritelump, numspritelumps;

int		R_PointOnSide (fixed_t x, fixed_t y, node_t *node);
int		R_PointOnSegSide (fixed_t x, fixed_t y, seg_t *line);
angle_t R_PointToAngle (fixed_t x, fixed_t y);
angle_t R_PointToAngle2 (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
fixed_t	R_PointToDist (fixed_t x, fixed_t y);
fixed_t R_ScaleFromGlobalAngle (angle_t visangle);
subsector_t *R_PointInSubsector (fixed_t x, fixed_t y);
void R_AddPointToBox (int x, int y, fixed_t *box);


#endif