#ifndef __P_TICK_H__
#define __P_TICK_H__
// ***** P_TICK *****

extern thinker_t thinkercap; // both the head and tail of the thinker list
extern int TimerGame; // tic countdown for deathmatch

void P_InitThinkers(void);
void P_AddThinker(thinker_t *thinker);
void P_RemoveThinker(thinker_t *thinker);

#endif