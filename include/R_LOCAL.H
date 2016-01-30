// R_local.h

#ifndef __R_LOCAL__
#define __R_LOCAL__

#define	ANGLETOSKYSHIFT		22		// sky map is 256*128*4 maps

#define	BASEYCENTER			100

#define MAXWIDTH			1120
#define	MAXHEIGHT			832

//#define DS_SCREEN_WIDTH 1024
//#define DS_SCREEN_HEIGHT 768
//#define DS_SCREEN_WIDTH (256*3/2)
//#define DS_SCREEN_HEIGHT (192*3/2)
#ifdef WIN32
#define SCREEN_WIDTH (1024)
#define SCREEN_HEIGHT (768)
#else
#define SCREEN_WIDTH (400)
#define SCREEN_HEIGHT (240)
#endif


#define	PI					3.141592657

#define	CENTERY				(SCREENHEIGHT/2)

#define	MINZ			(FRACUNIT*4)

#define	FIELDOFVIEW		2048	// fineangles in the SCREENWIDTH wide window

//
// lighting constants
//
#define	LIGHTLEVELS			16
#define	LIGHTSEGSHIFT		4
#define	MAXLIGHTSCALE		48
#define	LIGHTSCALESHIFT		12
#define	MAXLIGHTZ			128
#define	LIGHTZSHIFT			20
#define	NUMCOLORMAPS		32		// number of diminishing
#define	INVERSECOLORMAP		32

void R_PrecacheLevel (void);

#endif		// __R_LOCAL__

