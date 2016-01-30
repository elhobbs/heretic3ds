#ifndef __P_MAPUTL_H__
#define __P_MAPUTL_H__

// ***** P_MAPUTL *****

typedef struct
{
	fixed_t x, y, dx, dy;
} divline_t;

typedef struct
{
	fixed_t		frac;		// along trace line
	boolean		isaline;
	union {
		mobj_t	*thing;
		line_t	*line;
	}			d;
} intercept_t;

#define	MAXINTERCEPTS	128
extern	intercept_t		intercepts[MAXINTERCEPTS], *intercept_p;
typedef boolean (*traverser_t) (intercept_t *in);


fixed_t P_AproxDistance (fixed_t dx, fixed_t dy);
int 	P_PointOnLineSide (fixed_t x, fixed_t y, line_t *line);
int 	P_PointOnDivlineSide (fixed_t x, fixed_t y, divline_t *line);
void 	P_MakeDivline (line_t *li, divline_t *dl);
fixed_t P_InterceptVector (divline_t *v2, divline_t *v1);
int 	P_BoxOnLineSide (fixed_t *tmbox, line_t *ld);

extern	fixed_t opentop, openbottom, openrange;
extern	fixed_t	lowfloor;
void 	P_LineOpening (line_t *linedef);

boolean P_BlockLinesIterator (int x, int y, boolean(*func)(line_t*) );
boolean P_BlockThingsIterator (int x, int y, boolean(*func)(mobj_t*) );

#define PT_ADDLINES		1
#define	PT_ADDTHINGS	2
#define	PT_EARLYOUT		4

extern	divline_t 	trace;
boolean P_PathTraverse (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
	int flags, boolean (*trav) (intercept_t *));

void 	P_UnsetThingPosition (mobj_t *thing);
void	P_SetThingPosition (mobj_t *thing);

extern	int				validcount;

#endif