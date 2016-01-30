#ifndef __P_USER_H__
#define __P_USER_H__

// ***** P_USER *****

void P_PlayerThink(player_t *player);
void P_Thrust(player_t *player, angle_t angle, fixed_t move);
void P_PlayerRemoveArtifact(player_t *player, int slot);
void P_PlayerUseArtifact(player_t *player, artitype_t arti);
boolean P_UseArtifact(player_t *player, artitype_t arti);
int P_GetPlayerNum(player_t *player);

#endif