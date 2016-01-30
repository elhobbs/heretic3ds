/*
========================
=
= IN_lude.c
=
========================
*/

#include "DoomDef.h"
#include "soundst.h"
#include "R_LOCAL.H"

typedef enum
{
	SINGLE,
	COOPERATIVE,
	DEATHMATCH
} gametype_t;

// Public functions

void IN_Start(void);
void IN_Ticker(void);
void IN_Drawer(void);

boolean intermission;

// Private functions

void IN_WaitStop(void);
void IN_Stop(void);
void IN_LoadPics(void);
void IN_UnloadPics(void);
void IN_CheckForSkip(void);
void IN_InitStats(void);
void IN_InitDeathmatchStats(void);
void IN_InitNetgameStats(void);
void IN_DrawOldLevel(void);
void IN_DrawYAH(void);
void IN_DrawStatBack(void);
void IN_DrawSingleStats(void);
void IN_DrawCoopStats(void);
void IN_DrawDMStats(void);
void IN_DrawNumber(int val, int x, int y, int digits);
void IN_DrawTime(int x, int y, int h, int m, int s);
void IN_DrTextB(char *text, int x, int y);

static boolean skipintermission;
static int interstate = 0;
static int intertime = -1;
static int oldintertime = 0;
static gametype_t gametype;

static int cnt;

static int time;
static int hours;
static int minutes;
static int seconds;

static int slaughterboy; // in DM, the player with the most kills

static int killPercent[MAXPLAYERS];
static int bonusPercent[MAXPLAYERS];
static int secretPercent[MAXPLAYERS];

static patch_t *patchINTERPIC;
static patch_t *patchBEENTHERE;
static patch_t *patchGOINGTHERE;
static patch_t *FontBNumbers[10];
static patch_t *FontBNegative;
static patch_t *FontBSlash;
static patch_t *FontBPercent;

static int patchINTERPIC_num;
static int patchBEENTHERE_num;
static int patchGOINGTHERE_num;
static int FontBNumbers_num[10];
static int FontBNegative_num;
static int FontBSlash_num;
static int FontBPercent_num;

static int FontBLump;
static int FontBLumpBase;
static int patchFaceOkayBase;
static int patchFaceDeadBase;

static signed int totalFrags[MAXPLAYERS];
static fixed_t dSlideX[MAXPLAYERS];
static fixed_t dSlideY[MAXPLAYERS];

static char *KillersText[] = { "K", "I", "L", "L", "E", "R", "S" };

extern char *LevelNames[];

typedef struct
{
	int x;
	int y;
} yahpt_t;

static yahpt_t YAHspot[3][9] =
{
	{
		{ 172, 78 },
		{ 86, 90 },
		{ 73, 66 },
		{ 159, 95 },
		{ 148, 126 },
		{ 132, 54 },
		{ 131, 74 },
		{ 208, 138 },
		{ 52, 101 }
	},
	{
		{ 218, 57 },
		{ 137, 81 },
		{ 155, 124 },
		{ 171, 68 },
		{ 250, 86 },
		{ 136, 98 },
		{ 203, 90 },
		{ 220, 140 },
		{ 279, 106 }
	},
	{
		{ 86, 99 },
		{ 124, 103 },
		{ 154, 79 },
		{ 202, 83 },
		{ 178, 59 },
		{ 142, 58 },
		{ 219, 66 },
		{ 247, 57 },
		{ 107, 80 }
	}
};

//========================================================================
//
// IN_Start
//
//========================================================================

extern void AM_Stop (void);

void IN_Start(void)
{
	extern byte *host_basepal;
	//I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
	I_SetPalette(host_basepal);
	IN_LoadPics();
	IN_InitStats();
	intermission = true;
	interstate = -1;
	skipintermission = false;
	intertime = 0;
	oldintertime = 0;
	AM_Stop();
	S_StartSong(mus_intr, true);
}

//========================================================================
//
// IN_WaitStop
//
//========================================================================

void IN_WaitStop(void)
{
	if(!--cnt)
	{
		IN_Stop();
		G_WorldDone();
	}
}

//========================================================================
//
// IN_Stop
//
//========================================================================

void IN_Stop(void)
{
	intermission = false;
	IN_UnloadPics();
	SB_state = -1;
	BorderNeedRefresh = true;
}

//========================================================================
//
// IN_InitStats
//
//      Initializes the stats for single player mode
//========================================================================

void IN_InitStats(void)
{
	int i;
	int j;
	signed int slaughterfrags;
	int posnum;
	int slaughtercount;
	int playercount;


	if(!netgame)
	{
		gametype = SINGLE;
		time = leveltime/35;
		hours = time/3600;
		time -= hours*3600;
		minutes = time/60;
		time -= minutes*60;
		seconds = time;
	}
	else if(netgame && !deathmatch)
	{
		gametype = COOPERATIVE;
		memset(killPercent, 0, MAXPLAYERS*sizeof(int));
		memset(bonusPercent, 0, MAXPLAYERS*sizeof(int));
		memset(secretPercent, 0, MAXPLAYERS*sizeof(int));
		for(i=0; i<MAXPLAYERS; i++)
		{
			if(playeringame[i])
			{
				if(totalkills)
				{
					killPercent[i] = players[i].killcount*100/totalkills;
				}
				if(totalitems)
				{
					bonusPercent[i] = players[i].itemcount*100/totalitems;
				}
				if(totalsecret)
				{
					secretPercent[i] = players[i].secretcount*100/totalsecret;
				}
			}
		}
	}
	else
	{
		gametype = DEATHMATCH;
		slaughterboy = 0;
		slaughterfrags = -9999;
		posnum = 0;
		playercount = 0;
		slaughtercount = 0;
		for(i=0; i<MAXPLAYERS; i++)
		{
			totalFrags[i] = 0;
			if(playeringame[i])
			{
				playercount++;
				for(j=0; j<MAXPLAYERS; j++)
				{
					if(playeringame[j])
					{
						totalFrags[i] += players[i].frags[j];
					}
				}
				dSlideX[i] = (43*posnum*FRACUNIT)/20;
				dSlideY[i] = (36*posnum*FRACUNIT)/20;
				posnum++;
			}
			if(totalFrags[i] > slaughterfrags)
			{
				slaughterboy = 1<<i;
				slaughterfrags = totalFrags[i];
				slaughtercount = 1;
			}
			else if(totalFrags[i] == slaughterfrags)
			{
				slaughterboy |= 1<<i;
				slaughtercount++;
			}
		}
		if(playercount == slaughtercount)
		{ // don't do the slaughter stuff if everyone is equal
			slaughterboy = 0;
		}
	}
}

//========================================================================
//
// IN_LoadPics
//
//========================================================================

void IN_LoadPics(void)
{
	int i;

	switch(gameepisode)
	{
		case 1:
			patchINTERPIC_num = W_GetNumForName("MAPE1");
			patchINTERPIC = W_CacheLumpNum(patchINTERPIC_num, PU_STATIC);
			break;
		case 2:
			patchINTERPIC_num = W_GetNumForName("MAPE2");
			patchINTERPIC = W_CacheLumpNum(patchINTERPIC_num, PU_STATIC);
			break;
		case 3:
			patchINTERPIC_num = W_GetNumForName("MAPE3");
			patchINTERPIC = W_CacheLumpNum(patchINTERPIC_num, PU_STATIC);
			break;
		default:
			break;
	}
	patchBEENTHERE_num = W_GetNumForName("IN_X");
	patchGOINGTHERE_num = W_GetNumForName("IN_YAH");
	patchBEENTHERE = W_CacheLumpName("IN_X", PU_STATIC);
	patchGOINGTHERE = W_CacheLumpName("IN_YAH", PU_STATIC);
	FontBLumpBase = W_GetNumForName("FONTB16");
	for(i=0; i<10; i++)
	{
		FontBNumbers_num[i] = FontBLumpBase + i;
		FontBNumbers[i] = W_CacheLumpNum(FontBLumpBase + i, PU_STATIC);
	}
	FontBLump = W_GetNumForName("FONTB_S") + 1;
	FontBNegative_num = W_GetNumForName("FONTB13");
	FontBNegative = W_CacheLumpName("FONTB13", PU_STATIC);

	FontBSlash = W_CacheLumpName("FONTB15", PU_STATIC);
	FontBPercent = W_CacheLumpName("FONTB05", PU_STATIC);
	FontBSlash_num = W_GetNumForName("FONTB15");
	FontBPercent_num = W_GetNumForName("FONTB05");
	patchFaceOkayBase = W_GetNumForName("FACEA0");
	patchFaceDeadBase = W_GetNumForName("FACEB0");
}

//========================================================================
//
// IN_UnloadPics
//
//========================================================================

void IN_UnloadPics(void)
{
	int i;

	if(patchINTERPIC)
	{
		Z_ChangeTag(patchINTERPIC, PU_CACHE);
	}
	Z_ChangeTag(patchBEENTHERE, PU_CACHE);
	Z_ChangeTag(patchGOINGTHERE, PU_CACHE);
	for(i=0; i<10; i++)
	{
		Z_ChangeTag(FontBNumbers[i], PU_CACHE);
	}
	Z_ChangeTag(FontBNegative, PU_CACHE);
	Z_ChangeTag(FontBSlash, PU_CACHE);
	Z_ChangeTag(FontBPercent, PU_CACHE);
}

//========================================================================
//
// IN_Ticker
//
//========================================================================

void IN_Ticker(void)
{
	if(!intermission)
	{
		return;
	}
	if(interstate == 3)
	{
		IN_WaitStop();
		return;
	}
	IN_CheckForSkip();
	intertime++;
	if(oldintertime < intertime)
	{
		interstate++;
		if(gameepisode > 3 && interstate >= 1)
		{ // Extended Wad levels:  skip directly to the next level
			interstate = 3;
		}
		switch(interstate)
		{
			case 0:
				oldintertime = intertime+300;
				if(gameepisode > 3)
				{
					oldintertime = intertime+1200;
				}
				break;
			case 1:
				oldintertime = intertime+200;
				break;
			case 2:
				oldintertime = MAXINT;
				break;
			case 3:
				cnt = 10;
				break;
			default:
				break;
		}
	}
	if(skipintermission)
	{
		if(interstate == 0 && intertime < 150)
		{
			intertime = 150;
			skipintermission = false;
			return;
		}
		else if(interstate < 2 && gameepisode < 4)
		{
			interstate = 2;
			skipintermission = false;
			S_StartSound(NULL, sfx_dorcls);
			return;
		}
		interstate = 3;
		cnt = 10;
		skipintermission = false;
		S_StartSound(NULL, sfx_dorcls);
	}
}

//========================================================================
//
// IN_CheckForSkip
//
//      Check to see if any player hit a key
//========================================================================

void IN_CheckForSkip(void)
{
	int   i;
	player_t  *player;

	for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
	{
		if (playeringame[i])
		{
			if (player->cmd.buttons&BT_ATTACK)
			{
				if (!player->attackdown)
				{
					skipintermission = 1;
				}
				player->attackdown = true;
			}
			else
			{
				player->attackdown = false;
			}
			if (player->cmd.buttons&BT_USE)
			{
				if (!player->usedown)
				{
					skipintermission = 1;
				}
				player->usedown = true;
			}
			else
			{
				player->usedown = false;
			}
		}
	}
}

void ir_draw_patch_num(int x, int y, int lump, int tag);

//========================================================================
//
// IN_Drawer
//
//========================================================================

void IN_Drawer(void)
{
	extern int ir_adust_position;
	static int oldinterstate;
	int last = ir_adust_position;

	if(!intermission)
	{
		return;
	}
	if(interstate == 3)
	{
		return;
	}
	UpdateState |= I_FULLSCRN;
	if(oldinterstate != 2 && interstate == 2)
	{
		S_StartSound(NULL, sfx_pstop);
	}
	oldinterstate = interstate;

	ir_adust_position = 1; //center

	switch(interstate)
	{
		case 0: // draw stats
			ir_adust_position = 4; //top left
			IN_DrawStatBack();
			ir_adust_position = 1; //center
			switch(gametype)
			{
				case SINGLE:
					IN_DrawSingleStats();
					break;
				case COOPERATIVE:
					IN_DrawCoopStats();
					break;
				case DEATHMATCH:
					IN_DrawDMStats();
					break;
			}
			break;
		case 1: // leaving old level
			if(gameepisode < 4)
			{
				//V_DrawPatch(0, 0, patchINTERPIC);
				ir_draw_patch_num(0, 0, patchINTERPIC_num, PU_STATIC);
				IN_DrawOldLevel();
			}
			break;
		case 2: // going to the next level
			if(gameepisode < 4)
			{
				//V_DrawPatch(0, 0, patchINTERPIC);
				ir_draw_patch_num(0, 0, patchINTERPIC_num, PU_STATIC);
				IN_DrawYAH();
			}
			break;
		case 3: // waiting before going to the next level
			if(gameepisode < 4)
			{
				//V_DrawPatch(0, 0, patchINTERPIC);
				ir_draw_patch_num(0, 0, patchINTERPIC_num, PU_STATIC);
			}
			break;
		default:
			I_Error("IN_lude:  Intermission state out of range.\n");
			break;
	}
	ir_adust_position = last; //reset back to last
}

//========================================================================
//
// IN_DrawStatBack
//
//========================================================================

typedef struct {
	int id;
	short leftoffset, topoffset;
	short width, height;
	int rotated;
	float st[2][2];
} ir_patch_t;

void ir_draw_patch(int x, int y, ir_patch_t *ir_patch);

void IN_DrawStatBack(void)
{
#if 1
	ir_patch_t ir_patch;
	extern int		firstflat;
	int ds_load_map_flat(int name);

	memset(&ir_patch, 0, sizeof(ir_patch));

	ir_patch.id = ds_load_map_flat(W_GetNumForName("FLOOR16") - firstflat);
	ir_patch.width = SCREEN_WIDTH;
	ir_patch.height = SCREEN_HEIGHT;
	ir_patch.st[1][0] = (float)SCREEN_WIDTH / 64.0f;
	ir_patch.st[1][1] = (float)SCREEN_HEIGHT / 64.0f;

	ir_draw_patch(0, 0, &ir_patch);

#else
	int x;
	int y;

	byte *src;
	byte *dest;

	src = W_CacheLumpName ("FLOOR16", PU_CACHE);
	dest = screen;

	for (y=0 ; y<SCREENHEIGHT ; y++)
	{
		for (x=0 ; x<SCREENWIDTH/64 ; x++)
		{
			memcpy (dest, src+((y&63)<<6), 64);
			dest += 64;
		}
		if (SCREENWIDTH&63)
		{
			memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
			dest += (SCREENWIDTH&63);
		}
	}
#endif
}

//========================================================================
//
// IN_DrawOldLevel
//
//========================================================================

void IN_DrawOldLevel(void)
{
	int i;
	int x;

	x = 160-MN_TextBWidth(LevelNames[(gameepisode-1)*9+prevmap-1]+7)/2;
	IN_DrTextB(LevelNames[(gameepisode-1)*9+prevmap-1]+7, x, 3);
	x = 160-MN_TextAWidth("FINISHED")/2;
	MN_DrTextA("FINISHED", x, 25);

	if(prevmap == 9)
	{
		for(i=0; i<gamemap-1; i++)
		{
			//V_DrawPatch(YAHspot[gameepisode-1][i].x, YAHspot[gameepisode-1][i].y, patchBEENTHERE);
			ir_draw_patch_num(YAHspot[gameepisode - 1][i].x, YAHspot[gameepisode - 1][i].y, patchBEENTHERE_num, PU_CACHE);
		}
		if(!(intertime&16))
		{
			//V_DrawPatch(YAHspot[gameepisode - 1][8].x, YAHspot[gameepisode - 1][8].y, patchBEENTHERE);
			ir_draw_patch_num(YAHspot[gameepisode - 1][8].x, YAHspot[gameepisode - 1][8].y, patchBEENTHERE_num, PU_CACHE);
		}
	}
	else
	{
		for(i=0; i<prevmap-1; i++)
		{
			//V_DrawPatch(YAHspot[gameepisode - 1][i].x, YAHspot[gameepisode - 1][i].y, patchBEENTHERE);
			ir_draw_patch_num(YAHspot[gameepisode - 1][i].x, YAHspot[gameepisode - 1][i].y, patchBEENTHERE_num, PU_CACHE);
		}
		if(players[consoleplayer].didsecret)
		{
			//V_DrawPatch(YAHspot[gameepisode - 1][8].x, YAHspot[gameepisode - 1][8].y, patchBEENTHERE);
			ir_draw_patch_num(YAHspot[gameepisode - 1][8].x, YAHspot[gameepisode - 1][8].y, patchBEENTHERE_num, PU_CACHE);
		}
		if(!(intertime&16))
		{
			//V_DrawPatch(YAHspot[gameepisode - 1][prevmap - 1].x, YAHspot[gameepisode - 1][prevmap - 1].y, patchBEENTHERE);
			ir_draw_patch_num(YAHspot[gameepisode - 1][prevmap - 1].x, YAHspot[gameepisode - 1][prevmap - 1].y, patchBEENTHERE_num, PU_CACHE);
		}
	}
}

//========================================================================
//
// IN_DrawYAH
//
//========================================================================

void IN_DrawYAH(void)
{
	int i;
	int x;

	x = 160-MN_TextAWidth("NOW ENTERING:")/2;
	MN_DrTextA("NOW ENTERING:", x, 10);
	x = 160-MN_TextBWidth(LevelNames[(gameepisode-1)*9+gamemap-1]+7)/2;
	IN_DrTextB(LevelNames[(gameepisode-1)*9+gamemap-1]+7, x, 20);

	if(prevmap == 9)
	{
		prevmap = gamemap-1;
	}
	for(i=0; i<prevmap; i++)
	{
		//V_DrawPatch(YAHspot[gameepisode - 1][i].x, YAHspot[gameepisode - 1][i].y, patchBEENTHERE);
		ir_draw_patch_num(YAHspot[gameepisode - 1][i].x, YAHspot[gameepisode - 1][i].y, patchBEENTHERE_num, PU_CACHE);
	}
	if(players[consoleplayer].didsecret)
	{
		//V_DrawPatch(YAHspot[gameepisode - 1][8].x, YAHspot[gameepisode - 1][8].y, patchBEENTHERE);
		ir_draw_patch_num(YAHspot[gameepisode - 1][8].x, YAHspot[gameepisode - 1][8].y, patchBEENTHERE_num, PU_CACHE);
	}
	if(!(intertime&16) || interstate == 3)
	{ // draw the destination 'X'
		//V_DrawPatch(YAHspot[gameepisode - 1][gamemap - 1].x, YAHspot[gameepisode - 1][gamemap - 1].y, patchGOINGTHERE);
		ir_draw_patch_num(YAHspot[gameepisode - 1][gamemap - 1].x, YAHspot[gameepisode - 1][gamemap - 1].y, patchGOINGTHERE_num, PU_CACHE);
	}
}

//========================================================================
//
// IN_DrawSingleStats
//
//========================================================================

void IN_DrawSingleStats(void)
{
	int x;
	static int sounds;

	IN_DrTextB("KILLS", 50, 65);
	IN_DrTextB("ITEMS", 50, 90);
	IN_DrTextB("SECRETS", 50, 115);

	x = 160-MN_TextBWidth(LevelNames[(gameepisode-1)*9+prevmap-1]+7)/2;
	IN_DrTextB(LevelNames[(gameepisode-1)*9+prevmap-1]+7, x, 3);
	x = 160-MN_TextAWidth("FINISHED")/2;
	MN_DrTextA("FINISHED", x, 25);

	if(intertime < 30)
	{
		sounds = 0;
		return;
	}
	if(sounds < 1 && intertime >= 30)
	{
		S_StartSound(NULL, sfx_dorcls);
		sounds++;
	}
	IN_DrawNumber(players[consoleplayer].killcount, 200, 65, 3);
	//V_DrawShadowedPatch(237, 65, FontBSlash);
	ir_draw_shadow_patch_num(237, 65, FontBSlash_num, PU_CACHE);
	IN_DrawNumber(totalkills, 248, 65, 3);
	if(intertime < 60)
	{
		return;
	}
	if(sounds < 2 && intertime >= 60)
	{
		S_StartSound(NULL, sfx_dorcls);
		sounds++;
	}
	IN_DrawNumber(players[consoleplayer].itemcount, 200, 90, 3);
	//V_DrawShadowedPatch(237, 90, FontBSlash);
	ir_draw_shadow_patch_num(237, 90, FontBSlash_num, PU_CACHE);
	IN_DrawNumber(totalitems, 248, 90, 3);
	if(intertime < 90)
	{
		return;
	}
	if(sounds < 3 && intertime >= 90)
	{
		S_StartSound(NULL, sfx_dorcls);
		sounds++;
	}
	IN_DrawNumber(players[consoleplayer].secretcount, 200, 115, 3);
	//V_DrawShadowedPatch(237, 115, FontBSlash);
	ir_draw_shadow_patch_num(237, 115, FontBSlash_num, PU_CACHE);
	IN_DrawNumber(totalsecret, 248, 115, 3);
	if(intertime < 150)
	{
		return;
	}
	if(sounds < 4 && intertime >= 150)
	{
		S_StartSound(NULL, sfx_dorcls);
		sounds++;
	}

	if(!ExtendedWAD || gameepisode < 4)
	{
		IN_DrTextB("TIME", 85, 160);
		IN_DrawTime(155, 160, hours, minutes, seconds);
	}
	else
	{
		x = 160-MN_TextAWidth("NOW ENTERING:")/2;
		MN_DrTextA("NOW ENTERING:", x, 160);
		x = 160-MN_TextBWidth(LevelNames[(gameepisode-1)*9+gamemap-1]+7)/2;
		IN_DrTextB(LevelNames[(gameepisode-1)*9+gamemap-1]+7, x, 170);
		skipintermission = false;
	}
}

//========================================================================
//
// IN_DrawCoopStats
//
//========================================================================

void IN_DrawCoopStats(void)
{
	int i;
	int x;
	int ypos;

	static int sounds;

	IN_DrTextB("KILLS", 95, 35);
	IN_DrTextB("BONUS", 155, 35);
	IN_DrTextB("SECRET", 232, 35);
	x = 160-MN_TextBWidth(LevelNames[(gameepisode-1)*9+prevmap-1]+7)/2;
	IN_DrTextB(LevelNames[(gameepisode-1)*9+prevmap-1]+7, x, 3);
	x = 160-MN_TextAWidth("FINISHED")/2;
	MN_DrTextA("FINISHED", x, 25);

	ypos = 50;
	for(i=0; i<MAXPLAYERS; i++)
	{
		if(playeringame[i])
		{
			V_DrawShadowedPatch(25, ypos, W_CacheLumpNum(patchFaceOkayBase+i, PU_CACHE));
			if(intertime < 40)
			{
				sounds = 0;
				ypos += 37;
				continue;
			}
			else if(intertime >= 40 && sounds < 1)
			{
				S_StartSound(NULL, sfx_dorcls);
				sounds++;
			}
			IN_DrawNumber(killPercent[i], 85, ypos+10, 3);
			V_DrawShadowedPatch(121, ypos+10, FontBPercent);
			IN_DrawNumber(bonusPercent[i], 160, ypos+10, 3);
			V_DrawShadowedPatch(196, ypos+10, FontBPercent);
			IN_DrawNumber(secretPercent[i], 237, ypos+10, 3);
			V_DrawShadowedPatch(273, ypos+10, FontBPercent);
			ypos += 37;
		}
	}
}

//========================================================================
//
// IN_DrawDMStats
//
//========================================================================

void IN_DrawDMStats(void)
{
	int i;
	int j;
	int ypos;
	int xpos;
	int kpos;
	int x;

	static int sounds;

	xpos = 90;
	ypos = 55;

	IN_DrTextB("TOTAL", 265, 30);
	MN_DrTextA("VICTIMS", 140, 8);
	for(i=0; i<7; i++)
	{
		MN_DrTextA(KillersText[i], 10, 80+9*i);
	}
	if(intertime < 20)
	{
		for(i=0; i<MAXPLAYERS; i++)
		{
			if(playeringame[i])
			{
				V_DrawShadowedPatch(40, ((ypos<<FRACBITS)+dSlideY[i]*intertime)
					>>FRACBITS, W_CacheLumpNum(patchFaceOkayBase+i, PU_CACHE));
				V_DrawShadowedPatch(((xpos<<FRACBITS)+dSlideX[i]*intertime)
					>>FRACBITS, 18, W_CacheLumpNum(patchFaceDeadBase+i, PU_CACHE));
			}
		}
		sounds = 0;
		return;
	}
	if(intertime >= 20 && sounds < 1)
	{
		S_StartSound(NULL, sfx_dorcls);
		sounds++;
	}
	if(intertime >= 100 && slaughterboy && sounds < 2)
	{
		S_StartSound(NULL, sfx_wpnup);
		sounds++;
	}
	for(i=0; i<MAXPLAYERS; i++)
	{
		if(playeringame[i])
		{
			if(intertime < 100 || i == consoleplayer)
			{
				//V_DrawShadowedPatch(40, ypos, W_CacheLumpNum(patchFaceOkayBase + i, PU_CACHE));
				//V_DrawShadowedPatch(xpos, 18, W_CacheLumpNum(patchFaceDeadBase + i, PU_CACHE));
				ir_draw_shadow_patch_num(40, ypos, patchFaceOkayBase + i, PU_CACHE);
				ir_draw_shadow_patch_num(xpos, 18, patchFaceDeadBase + i, PU_CACHE);
			}
			else
			{
				//V_DrawFuzzPatch(40, ypos, W_CacheLumpNum(patchFaceOkayBase + i, PU_CACHE));
				//V_DrawFuzzPatch(xpos, 18, W_CacheLumpNum(patchFaceDeadBase + i, PU_CACHE));
				ir_draw_fuzz_patch_num(40, ypos, patchFaceOkayBase + i, PU_CACHE);
				ir_draw_fuzz_patch_num(xpos, 18, patchFaceDeadBase + i, PU_CACHE);
			}
			kpos = 86;
			for(j=0; j<MAXPLAYERS; j++)
			{
				if(playeringame[j])
				{
					IN_DrawNumber(players[i].frags[j], kpos, ypos+10, 3);
					kpos += 43;
				}
	      }
			if(slaughterboy&(1<<i))
			{
				if(!(intertime&16))
				{
					IN_DrawNumber(totalFrags[i], 263, ypos+10, 3);
				}
			}
			else
			{
				IN_DrawNumber(totalFrags[i], 263, ypos+10, 3);
			}
			ypos += 36;
			xpos += 43;
		}
	}
}

//========================================================================
//
// IN_DrawTime
//
//========================================================================

void IN_DrawTime(int x, int y, int h, int m, int s)
{
	if(h)
	{
		IN_DrawNumber(h, x, y, 2);
		IN_DrTextB(":", x+26, y);
	}
	x += 34;
	if(m || h)
	{
		IN_DrawNumber(m, x, y, 2);
	}
	x += 34;
	if(s)
	{
		IN_DrTextB(":", x-8, y);
		IN_DrawNumber(s, x, y, 2);
	}
}

//========================================================================
//
// IN_DrawNumber
//
//========================================================================

void IN_DrawNumber(int val, int x, int y, int digits)
{
	patch_t *patch;
	int xpos;
	int oldval;
	int realdigits;
	boolean neg;

	oldval = val;
	xpos = x;
	neg = false;
	realdigits = 1;

	if(val < 0)
	{ //...this should reflect negative frags
		val = -val;
		neg = true;
		if(val > 99)
		{
			val = 99;
		}
	}
	if(val > 9)
	{
		realdigits++;
		if(digits < realdigits)
		{
			realdigits = digits;
			val = 9;
		}
	}
	if(val > 99)
	{
		realdigits++;
		if(digits < realdigits)
		{
			realdigits = digits;
			val = 99;
		}
	}
	if(val > 999)
	{
		realdigits++;
		if(digits < realdigits)
		{
			realdigits = digits;
			val = 999;
		}
	}
	if(digits == 4)
	{
		patch = FontBNumbers[val/1000];
		//V_DrawShadowedPatch(xpos + 6 - patch->width / 2 - 12, y, patch);
		ir_draw_shadow_patch_num(xpos + 6 - patch->width / 2 - 12, y, FontBNumbers_num[val / 1000], PU_CACHE);
	}
	if(digits > 2)
	{
		if(realdigits > 2)
		{
			patch = FontBNumbers[val/100];
			//V_DrawShadowedPatch(xpos + 6 - patch->width / 2, y, patch);
			ir_draw_shadow_patch_num(xpos + 6 - patch->width / 2, y, FontBNumbers_num[val / 100], PU_CACHE);
		}
		xpos += 12;
	}
	val = val%100;
	if(digits > 1)
	{
		if(val > 9)
		{
			patch = FontBNumbers[val/10];
			//V_DrawShadowedPatch(xpos + 6 - patch->width / 2, y, patch);
			ir_draw_shadow_patch_num(xpos + 6 - patch->width / 2, y, FontBNumbers_num[val / 10]);
		}
		else if(digits == 2 || oldval > 99)
		{
			//V_DrawShadowedPatch(xpos, y, FontBNumbers[0]);
			ir_draw_shadow_patch_num(xpos, y, FontBNumbers_num[0], PU_CACHE);
		}
		xpos += 12;
	}
	val = val%10;
	patch = FontBNumbers[val];
	//V_DrawShadowedPatch(xpos + 6 - patch->width / 2, y, patch);
	ir_draw_shadow_patch_num(xpos + 6 - patch->width / 2, y, FontBNumbers_num[val], PU_CACHE);
	if(neg)
	{
		patch = FontBNegative;
		//V_DrawShadowedPatch(xpos + 6 - patch->width / 2 - 12 * (realdigits), y, patch);
		ir_draw_shadow_patch_num(xpos + 6 - patch->width / 2 - 12 * (realdigits), y, FontBNegative_num, PU_CACHE);
	}
}

//========================================================================
//
// IN_DrTextB
//
//========================================================================

void IN_DrTextB(char *text, int x, int y)
{
	char c;
	patch_t *p;

	while((c = *text++) != 0)
	{
		if(c < 33)
		{
			x += 8;
		}
		else
		{
			p = W_CacheLumpNum(FontBLump+c-33, PU_CACHE);
			//V_DrawShadowedPatch(x, y, p);
			ir_draw_shadow_patch_num(x, y, FontBLump + c - 33, PU_CACHE);
			x += p->width-1;
		}
	}
}
