#ifndef __P_MOBJ_H__
#define __P_MOBJ_H__

// ***** P_MOBJ *****

#define FLOOR_SOLID 0
#define FLOOR_WATER 1
#define FLOOR_LAVA 2
#define FLOOR_SLUDGE 3

#define ONFLOORZ MININT
#define ONCEILINGZ MAXINT
#define FLOATRANDZ (MAXINT-1)

extern mobjtype_t PuffType;
extern mobj_t *MissileMobj;

mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);
void P_RemoveMobj(mobj_t *th);
boolean	P_SetMobjState(mobj_t *mobj, statenum_t state);
boolean	P_SetMobjStateNF(mobj_t *mobj, statenum_t state);
void P_ThrustMobj(mobj_t *mo, angle_t angle, fixed_t move);
int P_FaceMobj(mobj_t *source, mobj_t *target, angle_t *delta);
boolean P_SeekerMissile(mobj_t *actor, angle_t thresh, angle_t turnMax);
void P_MobjThinker(mobj_t *mobj);
void P_BlasterMobjThinker(mobj_t *mobj);
void P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z);
void P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage);
void P_BloodSplatter(fixed_t x, fixed_t y, fixed_t z, mobj_t *originator);
void P_RipperBlood(mobj_t *mo);
int P_GetThingFloorType(mobj_t *thing);
int P_HitFloor(mobj_t *thing);
boolean P_CheckMissileSpawn(mobj_t *missile);
mobj_t *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type);
mobj_t *P_SpawnMissileAngle(mobj_t *source, mobjtype_t type,
	angle_t angle, fixed_t momz);
mobj_t *P_SpawnPlayerMissile(mobj_t *source, mobjtype_t type);
mobj_t *P_SPMAngle(mobj_t *source, mobjtype_t type, angle_t angle);

#endif