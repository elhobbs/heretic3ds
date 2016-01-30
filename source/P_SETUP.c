
// P_main.c

#include "DoomDef.h"
#include "map.h"
#include "p_spec.h"
#include "p_tick.h"
#include "p_mobj.h"
#include "p_pspr.h"
#include "p_enemy.h"
#include "p_user.h"
#include "p_inter.h"
#include "p_map.h"
#include "p_maputl.h"
#include "p_local.h"
#include "soundst.h"
#include "geometry.h"
#include <stdlib.h>

void	P_SpawnMapThing (mapthing_t *mthing);

int			numvertexes;
vertex_t	*vertexes;

int			numsegs;
seg_t		*segs;

int			numsectors;
sector_t	*sectors;

int			numsubsectors;
subsector_t	*subsectors;

int			numnodes;
node_t		*nodes;

int			numlines;
line_t		*lines;

int			numsides;
side_t		*sides;

short		*blockmaplump;			// offsets in blockmap are from here
short		*blockmap;
int			bmapwidth, bmapheight;	// in mapblocks
fixed_t		bmaporgx, bmaporgy;		// origin of block map
mobj_t		**blocklinks;			// for thing chains

byte		*rejectmatrix;			// for fast sight rejection

mapthing_t	deathmatchstarts[10], *deathmatch_p;
mapthing_t	playerstarts[MAXPLAYERS];

/*
=================
=
= P_LoadVertexes
=
=================
*/

void P_LoadVertexes (int lump)
{
	byte		*data;
	int			i;
	mapvertex_t	*ml;
	vertex_t	*li;
	
	numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);
	vertexes = Z_Malloc (numvertexes*sizeof(vertex_t),PU_LEVEL,0);	
	data = W_CacheLumpNum (lump,PU_STATIC);
	
	ml = (mapvertex_t *)data;
	li = vertexes;
	for (i=0 ; i<numvertexes ; i++, li++, ml++)
	{
		li->x = SHORT(ml->x)<<FRACBITS;
		li->y = SHORT(ml->y)<<FRACBITS;
	}
	
	Z_Free (data);
}

static float GetDistance(int dx, int dy)
{
  float fx = (float)(dx)/FRACUNIT, fy = (float)(dy)/FRACUNIT;
  return (float)sqrt(fx*fx + fy*fy);
}


/*
=================
=
= P_LoadSegs
=
=================
*/

void P_LoadSegs (int lump)
{
	byte		*data;
	int			i;
	mapseg_t	*ml;
	seg_t		*li;
	line_t	*ldef;
	int			linedef, side;
	
	numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
	segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);	
	memset (segs, 0, numsegs*sizeof(seg_t));
	data = W_CacheLumpNum (lump,PU_STATIC);
	
	ml = (mapseg_t *)data;
	li = segs;
	for (i=0 ; i<numsegs ; i++, li++, ml++)
	{
		li->v1 = &vertexes[SHORT(ml->v1)];
		li->v2 = &vertexes[SHORT(ml->v2)];
		li->length  = GetDistance(li->v2->x - li->v1->x, li->v2->y - li->v1->y);
					
		li->angle = (SHORT(ml->angle))<<16;
		li->offset = (SHORT(ml->offset))<<16;
		linedef = SHORT(ml->linedef);
		ldef = &lines[linedef];
		li->linedef = ldef;
		side = SHORT(ml->side);
		li->sidedef = &sides[ldef->sidenum[side]];
		li->frontsector = sides[ldef->sidenum[side]].sector;
		if (ldef-> flags & ML_TWOSIDED)
			li->backsector = sides[ldef->sidenum[side^1]].sector;
		else
			li->backsector = 0;
	}
	
	Z_Free (data);
}


/*
=================
=
= P_LoadSubsectors
=
=================
*/

void P_LoadSubsectors (int lump)
{
	byte			*data;
	int				i;
	mapsubsector_t	*ms;
	subsector_t		*ss;
	
	numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
	subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);	
	data = W_CacheLumpNum (lump,PU_STATIC);
	
	ms = (mapsubsector_t *)data;
	memset (subsectors,0, numsubsectors*sizeof(subsector_t));
	ss = subsectors;
	for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
	{
		ss->numlines = SHORT(ms->numsegs);
		ss->firstline = SHORT(ms->firstseg);
	}
	
	Z_Free (data);
}


/*
=================
=
= P_LoadSectors
=
=================
*/

void P_LoadSectors (int lump)
{
	byte			*data;
	int				i;
	mapsector_t		*ms;
	sector_t		*ss;
	
	numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
	sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);	
	memset (sectors, 0, numsectors*sizeof(sector_t));
	data = W_CacheLumpNum (lump,PU_STATIC);
	
	ms = (mapsector_t *)data;
	ss = sectors;
	for (i=0 ; i<numsectors ; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
		ss->floorpic = R_FlatNumForName(ms->floorpic);
		ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);
		ss->thinglist = NULL;
	}
	
	Z_Free (data);
}


/*
=================
=
= P_LoadNodes
=
=================
*/

void P_LoadNodes (int lump)
{
	byte		*data;
	int			i,j,k;
	mapnode_t	*mn;
	node_t		*no;
	
	numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
	nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);	
	data = W_CacheLumpNum (lump,PU_STATIC);
	
	mn = (mapnode_t *)data;
	no = nodes;
	for (i=0 ; i<numnodes ; i++, no++, mn++)
	{
		no->x = SHORT(mn->x)<<FRACBITS;
		no->y = SHORT(mn->y)<<FRACBITS;
		no->dx = SHORT(mn->dx)<<FRACBITS;
		no->dy = SHORT(mn->dy)<<FRACBITS;
		for (j=0 ; j<2 ; j++)
		{
			no->children[j] = SHORT(mn->children[j]);
			for (k=0 ; k<4 ; k++)
				no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
		}
	}
	
	Z_Free (data);
}



/*
=================
=
= P_LoadThings
=
=================
*/

void P_LoadThings (int lump)
{
	byte			*data;
	int				i;
	mapthing_t		*mt;
	int				numthings;
	
	data = W_CacheLumpNum (lump,PU_STATIC);
	numthings = W_LumpLength (lump) / sizeof(mapthing_t);
	
	mt = (mapthing_t *)data;
	for (i=0 ; i<numthings ; i++, mt++)
	{
		mt->x = SHORT(mt->x);
		mt->y = SHORT(mt->y);
		mt->angle = SHORT(mt->angle);
		mt->type = SHORT(mt->type);
		mt->options = SHORT(mt->options);
		P_SpawnMapThing (mt);
	}
	
	Z_Free (data);
}



/*
=================
=
= P_LoadLineDefs
=
= Also counts secret lines for intermissions
=================
*/

void P_LoadLineDefs (int lump)
{
	byte			*data;
	int				i;
	maplinedef_t	*mld;
	line_t			*ld;
	vertex_t		*v1, *v2;
	
	numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
	lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);	
	memset (lines, 0, numlines*sizeof(line_t));
	data = W_CacheLumpNum (lump,PU_STATIC);
	
	mld = (maplinedef_t *)data;
	ld = lines;
	for (i=0 ; i<numlines ; i++, mld++, ld++)
	{
		ld->flags = SHORT(mld->flags);
		ld->special = SHORT(mld->special);
		ld->tag = SHORT(mld->tag);
		v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
		v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
		ld->dx = v2->x - v1->x;
		ld->dy = v2->y - v1->y;
		if (!ld->dx)
			ld->slopetype = ST_VERTICAL;
		else if (!ld->dy)
			ld->slopetype = ST_HORIZONTAL;
		else
		{
			if (FixedDiv (ld->dy , ld->dx) > 0)
				ld->slopetype = ST_POSITIVE;
			else
				ld->slopetype = ST_NEGATIVE;
		}
		
		if (v1->x < v2->x)
		{
			ld->bbox[BOXLEFT] = v1->x;
			ld->bbox[BOXRIGHT] = v2->x;
		}
		else
		{
			ld->bbox[BOXLEFT] = v2->x;
			ld->bbox[BOXRIGHT] = v1->x;
		}
		if (v1->y < v2->y)
		{
			ld->bbox[BOXBOTTOM] = v1->y;
			ld->bbox[BOXTOP] = v2->y;
		}
		else
		{
			ld->bbox[BOXBOTTOM] = v2->y;
			ld->bbox[BOXTOP] = v1->y;
		}
		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);
		if (ld->sidenum[0] != -1)
			ld->frontsector = sides[ld->sidenum[0]].sector;
		else
			ld->frontsector = 0;
		if (ld->sidenum[1] != -1)
			ld->backsector = sides[ld->sidenum[1]].sector;
		else
			ld->backsector = 0;
	}
	
	Z_Free (data);
}


/*
=================
=
= P_LoadSideDefs
=
=================
*/

void P_LoadSideDefs (int lump)
{
	byte			*data;
	int				i;
	mapsidedef_t	*msd;
	side_t			*sd;
	
	numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
	sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);	
	memset (sides, 0, numsides*sizeof(side_t));
	data = W_CacheLumpNum (lump,PU_STATIC);
	
	msd = (mapsidedef_t *)data;
	sd = sides;
	for (i=0 ; i<numsides ; i++, msd++, sd++)
	{
		sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
		sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
		sd->toptexture = R_TextureNumForName(msd->toptexture);
		sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
		sd->midtexture = R_TextureNumForName(msd->midtexture);
		sd->sector = &sectors[SHORT(msd->sector)];
	}
	
	Z_Free (data);
}



/*
=================
=
= P_LoadBlockMap
=
=================
*/

void P_LoadBlockMap (int lump)
{
	int		i, count;
	
	blockmaplump = W_CacheLumpNum (lump,PU_LEVEL);
	blockmap = blockmaplump+4;
	count = W_LumpLength (lump)/2;
	for (i=0 ; i<count ; i++)
		blockmaplump[i] = SHORT(blockmaplump[i]);
		
	bmaporgx = blockmaplump[0]<<FRACBITS;
	bmaporgy = blockmaplump[1]<<FRACBITS;
	bmapwidth = blockmaplump[2];
	bmapheight = blockmaplump[3];
	
// clear out mobj chains
	count = sizeof(*blocklinks)* bmapwidth*bmapheight;
	blocklinks = Z_Malloc (count,PU_LEVEL, 0);
	memset (blocklinks, 0, count);
}




/*
=================
=
= P_GroupLines
=
= Builds sector line lists and subsector sector numbers
= Finds block bounding boxes for sectors
=================
*/

void P_GroupLines (void)
{
	line_t		**linebuffer;
	int			i, j, total;
	line_t		*li;
	sector_t	*sector;
	subsector_t	*ss;
	seg_t		*seg;
	fixed_t		bbox[4];
	int			block;
	
// look up sector number for each subsector
	ss = subsectors;
	for (i=0 ; i<numsubsectors ; i++, ss++)
	{
		seg = &segs[ss->firstline];
		ss->sector = seg->sidedef->sector;
	}

// count number of lines in each sector
	li = lines;
	total = 0;
	for (i=0 ; i<numlines ; i++, li++)
	{
		total++;
		li->frontsector->linecount++;
		if (li->backsector && li->backsector != li->frontsector)
		{
			li->backsector->linecount++;
			total++;
		}
	}
	
// build line tables for each sector	
	linebuffer = Z_Malloc (total*4, PU_LEVEL, 0);
	sector = sectors;
	for (i=0 ; i<numsectors ; i++, sector++)
	{
		M_ClearBox (bbox);
		sector->lines = linebuffer;
		li = lines;
		for (j=0 ; j<numlines ; j++, li++)
		{
			if (li->frontsector == sector || li->backsector == sector)
			{
				*linebuffer++ = li;
				M_AddToBox (bbox, li->v1->x, li->v1->y);
				M_AddToBox (bbox, li->v2->x, li->v2->y);
			}
		}
		if (linebuffer - sector->lines != sector->linecount)
			I_Error ("P_GroupLines: miscounted");
			
		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
		sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;
		
		// adjust bounding box to map blocks
		block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= bmapheight ? bmapheight-1 : block;
		sector->blockbox[BOXTOP]=block;

		block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXBOTTOM]=block;

		block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= bmapwidth ? bmapwidth-1 : block;
		sector->blockbox[BOXRIGHT]=block;

		block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXLEFT]=block;
	}
	
}

//=============================================================================

void BuildSideSegs() {
	int i;
	// JDC: since we are drawing with a depth buffer, we can draw complete line
	// sides instead of the cut up seg_t used by the software renderer.  This saves
	// about 20% of the wall primitives, and also makes the drawn geometry "water tight",
	// without the occasional pixel cracks that would occur at T-junctions between cut
	// walls and uncut sector geometry.

	// We can do more processing here to make the processing during drawing faster
	// if we need more speed.
	for ( i = 0 ; i < numsegs ; i++ ) {
		float dx,dy;
		seg_t *seg = &segs[i];
		seg_t *sideSeg = &seg->sidedef->sideSeg;
		*sideSeg = *seg;
		// this might be either the front or back of the line
		if ( seg->frontsector == seg->linedef->frontsector ) {
			sideSeg->v1 = seg->linedef->v1;
			sideSeg->v2 = seg->linedef->v2;
		} else {
			//assert( seg->frontsector == seg->linedef->backsector );
			sideSeg->v1 = seg->linedef->v2;
			sideSeg->v2 = seg->linedef->v1;
		}
		// the total length is needed for textur coordinate generation
		dx = (float)( sideSeg->v2->x - sideSeg->v1->x ) / FRACUNIT;
		dy = (float)( sideSeg->v2->y - sideSeg->v1->y ) / FRACUNIT;
		sideSeg->length  = sqrt( dx * dx + dy * dy );
		sideSeg->offset = 0;	// full-side segs will always have 0 offset
	}
	
	// check that every sidedef had a sideSeg created
	//for ( int i = 0 ; i < numsides ; i++ ) {
	//	assert( sides[i].sideSeg.sidedef == &sides[i] );
	//}
}

//
// killough 10/98
//
// Remove slime trails.
//
// Slime trails are inherent to Doom's coordinate system -- i.e. there is
// nothing that a node builder can do to prevent slime trails ALL of the time,
// because it's a product of the integer coodinate system, and just because
// two lines pass through exact integer coordinates, doesn't necessarily mean
// that they will intersect at integer coordinates. Thus we must allow for
// fractional coordinates if we are to be able to split segs with node lines,
// as a node builder must do when creating a BSP tree.
//
// A wad file does not allow fractional coordinates, so node builders are out
// of luck except that they can try to limit the number of splits (they might
// also be able to detect the degree of roundoff error and try to avoid splits
// with a high degree of roundoff error). But we can use fractional coordinates
// here, inside the engine. It's like the difference between square inches and
// square miles, in terms of granularity.
//
// For each vertex of every seg, check to see whether it's also a vertex of
// the linedef associated with the seg (i.e, it's an endpoint). If it's not
// an endpoint, and it wasn't already moved, move the vertex towards the
// linedef by projecting it using the law of cosines. Formula:
//
//      2        2                         2        2
//    dx  x0 + dy  x1 + dx dy (y0 - y1)  dy  y0 + dx  y1 + dx dy (x0 - x1)
//   {---------------------------------, ---------------------------------}
//                  2     2                            2     2
//                dx  + dy                           dx  + dy
//
// (x0,y0) is the vertex being moved, and (x1,y1)-(x1+dx,y1+dy) is the
// reference linedef.
//
// Segs corresponding to orthogonal linedefs (exactly vertical or horizontal
// linedefs), which comprise at least half of all linedefs in most wads, don't
// need to be considered, because they almost never contribute to slime trails
// (because then any roundoff error is parallel to the linedef, which doesn't
// cause slime). Skipping simple orthogonal lines lets the code finish quicker.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
// Firelines (TM) is a Rezistered Trademark of MBF Productions
//

static void P_RemoveSlimeTrails(void)         // killough 10/98
{
  byte *hit = calloc(1, numvertexes);         // Hitlist for vertices
  int i;
  for (i=0; i<numsegs; i++)                   // Go through each seg
  {
    const line_t *l;

    //if (segs[i].miniseg == true)        //figgi -- skip minisegs
    //  return;

    l = segs[i].linedef;            // The parent linedef
    if (l->dx && l->dy)                     // We can ignore orthogonal lines
    {
    vertex_t *v = segs[i].v1;
    do
      if (!hit[v - vertexes])           // If we haven't processed vertex
        {
    hit[v - vertexes] = 1;        // Mark this vertex as processed
    if (v != l->v1 && v != l->v2) // Exclude endpoints of linedefs
      { // Project the vertex back onto the parent linedef
        int64_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
        int64_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
        int64_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
        int64_t s = dx2 + dy2;
        int x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;
        v->x = (int)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
        v->y = (int)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
      }
        }  // Obsfucated C contest entry:   :)
    while ((v != segs[i].v2) && (v = segs[i].v2));
  }
    }
  free(hit);
}

/*
=================
=
= P_SetupLevel
=
=================
*/

void P_SetupLevel (int episode, int map, int playermask, skill_t skill)
{
	int i;
	int parm;
	char	lumpname[9];
	int		lumpnum;
	mobj_t	*mobj;
	
	totalkills = totalitems = totalsecret = 0;
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		players[i].killcount = players[i].secretcount 
		= players[i].itemcount = 0;
	}
	players[consoleplayer].viewz = 1; // will be set by player think
	
	S_Start ();			// make sure all sounds are stopped before Z_FreeTags
	
	Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);
	
	printf("P_InitThinkers\n");
	P_InitThinkers();
	
//
// look for a regular (development) map first
//
	lumpname[0] = 'E';
	lumpname[1] = '0' + episode;
	lumpname[2] = 'M';
	lumpname[3] = '0' + map;
	lumpname[4] = 0;
	leveltime = 0;
	
	lumpnum = W_GetNumForName (lumpname);
	
// note: most of this ordering is important	
	printf("ML_BLOCKMAP\n");
	P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
	P_LoadVertexes (lumpnum+ML_VERTEXES);
	P_LoadSectors (lumpnum+ML_SECTORS);
	P_LoadSideDefs (lumpnum+ML_SIDEDEFS);

	printf("ML_LINEDEFS\n");
	P_LoadLineDefs (lumpnum+ML_LINEDEFS);
	printf("P_LoadSubsectors\n");
	P_LoadSubsectors (lumpnum+ML_SSECTORS);
	printf("P_LoadNodes\n");
	P_LoadNodes (lumpnum+ML_NODES);
	printf("P_LoadSegs\n");
	P_LoadSegs (lumpnum+ML_SEGS);
	
	printf("ML_REJECT\n");
	rejectmatrix = W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
	printf("P_GroupLines\n");
	P_GroupLines ();

	printf("P_RemoveSlimeTrails\n");
	P_RemoveSlimeTrails();    // killough 10/98: remove slime trails from wad
	BuildSideSegs();

	bodyqueslot = 0;
	deathmatch_p = deathmatchstarts;
	P_InitAmbientSound();
	P_InitMonsters();
	P_OpenWeapons();
	P_LoadThings(lumpnum+ML_THINGS);
	P_CloseWeapons();

	printf("TimerGame\n");
	//
// if deathmatch, randomly spawn the active players
//
	TimerGame = 0;
	if(deathmatch)
	{
		for (i=0 ; i<MAXPLAYERS ; i++)
		{
			if (playeringame[i])
			{	// must give a player spot before deathmatchspawn
				mobj = P_SpawnMobj (playerstarts[i].x<<16,
				playerstarts[i].y<<16,0, MT_PLAYER);
				players[i].mo = mobj;
				G_DeathMatchSpawnPlayer (i);
				P_RemoveMobj (mobj);
			}
		}
		parm = M_CheckParm("-timer");
		if(parm && parm < myargc-1)
		{
			TimerGame = atoi(myargv[parm+1])*35*60;
		}
	}

// set up world state
	printf("P_SpawnSpecials\n");
	P_SpawnSpecials ();
	
// build subsector connect matrix
//	P_ConnectSubsectors ();

// preload graphics
	printf("precache: %d\n", precache);
	if (precache)
		R_PrecacheLevel ();

	printf("ds_gen_floors\n");
	ds_gen_floors();
//printf ("free memory: 0x%x\n", Z_FreeMemory());

}


/*
=================
=
= P_Init
=
=================
*/

void P_Init (void)
{	
	P_InitSwitchList();
	P_InitPicAnims();
	P_InitTerrainTypes();
	P_InitLava();
	R_InitSprites(sprnames);
}
