#ifndef __P_MAP_H__
#define __P_MAP_H__

// ***** P_MAP *****

extern boolean floatok;				// if true, move would be ok if
extern fixed_t tmfloorz, tmceilingz;	// within tmfloorz - tmceilingz

extern line_t *ceilingline;
boolean P_TestMobjLocation(mobj_t *mobj);
boolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y);
mobj_t *P_CheckOnmobj(mobj_t *thing);
void P_FakeZMovement(mobj_t *mo);
boolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y);
boolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y);
void P_SlideMove(mobj_t *mo);
boolean P_CheckSight(mobj_t *t1, mobj_t *t2);
void P_UseLines(player_t *player);

boolean P_ChangeSector (sector_t *sector, boolean crunch);

extern	mobj_t		*linetarget;			// who got hit (or NULL)
fixed_t P_AimLineAttack (mobj_t *t1, angle_t angle, fixed_t distance);

void P_LineAttack (mobj_t *t1, angle_t angle, fixed_t distance, fixed_t slope, int damage);

void P_RadiusAttack (mobj_t *spot, mobj_t *source, int damage);

#endif