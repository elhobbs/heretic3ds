#ifndef __P_SETUP_H__
#define __P_SETUP_H__

// ***** P_SETUP *****

extern byte *rejectmatrix;				// for fast sight rejection
extern short *blockmaplump;				// offsets in blockmap are from here
extern short *blockmap;
extern int bmapwidth, bmapheight;		// in mapblocks
extern fixed_t bmaporgx, bmaporgy;		// origin of block map
extern mobj_t **blocklinks;				// for thing chains

#endif