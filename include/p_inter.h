#ifndef __P_INTER_H__
#define __P_INTER_H__

// ***** P_INTER *****

extern int maxammo[NUMAMMO];
extern int clipammo[NUMAMMO];

void P_SetMessage(player_t *player, char *message, boolean ultmsg);
void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher);
void P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source,
	int damage);
boolean P_GiveAmmo(player_t *player, ammotype_t ammo, int count);
boolean P_GiveArtifact(player_t *player, artitype_t arti, mobj_t *mo);
boolean P_GiveBody(player_t *player, int num);
boolean P_GivePower(player_t *player, powertype_t power);
boolean P_ChickenMorphPlayer(player_t *player);

#endif