#ifndef __P_ENEMY_H__
#define __P_ENEMY_H__

// ***** P_ENEMY *****

void P_NoiseAlert (mobj_t *target, mobj_t *emmiter);
void P_InitMonsters(void);
void P_AddBossSpot(fixed_t x, fixed_t y, angle_t angle);
void P_Massacre(void);
void P_DSparilTeleport(mobj_t *actor);

#endif