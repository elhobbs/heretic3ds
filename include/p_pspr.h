#ifndef __P_PSPR_H__
#define __P_PSPR_H__

// ***** P_PSPR *****

#define USE_GWND_AMMO_1 1
#define USE_GWND_AMMO_2 1
#define USE_CBOW_AMMO_1 1
#define USE_CBOW_AMMO_2 1
#define USE_BLSR_AMMO_1 1
#define USE_BLSR_AMMO_2 5
#define USE_SKRD_AMMO_1 1
#define USE_SKRD_AMMO_2 5
#define USE_PHRD_AMMO_1 1
#define USE_PHRD_AMMO_2 1
#define USE_MACE_AMMO_1 1
#define USE_MACE_AMMO_2 5

void P_OpenWeapons(void);
void P_CloseWeapons(void);
void P_AddMaceSpot(mapthing_t *mthing);
void P_RepositionMace(mobj_t *mo);
void P_SetPsprite(player_t *player, int position, statenum_t stnum);
void P_SetupPsprites(player_t *curplayer);
void P_MovePsprites(player_t *curplayer);
void P_DropWeapon(player_t *player);
void P_ActivateBeak(player_t *player);
void P_PostChickenWeapon(player_t *player, weapontype_t weapon);
void P_UpdateBeak(player_t *player, pspdef_t *psp);

#endif