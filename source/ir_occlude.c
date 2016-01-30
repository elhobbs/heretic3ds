#ifdef _3DS
#include <3ds.h>
#include <3ds/types.h>
#include "gl.h"
#include "3dmath.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include "DoomDef.h"
#include "R_local.h"
#include "map.h"

#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
extern HWND openGL;
#endif

void IR_RenderOcclude(angle_t ang0, angle_t ang1, int before);

typedef struct occlude_s {
	angle_t ang0,ang1;
	struct occlude_s *next;
} occlude_t;

#define MAX_OCCLUDERS 32000
static occlude_t ir_occluders[MAX_OCCLUDERS];
static int ir_num_occluders = 0;
static occlude_t *ir_occlude = 0;

static void ir_insert_begin(angle_t ang0, angle_t ang1) {
	//too many - bail
	if (ir_num_occluders >= MAX_OCCLUDERS) {
		return;
	}

	occlude_t *next = &ir_occluders[ir_num_occluders++];
	next->ang0 = ang0;
	next->ang1 = ang1;
	next->next = ir_occlude;
	ir_occlude = next;
}

static void ir_insert_after(occlude_t *occlude, angle_t ang0, angle_t ang1) {
	//too many - bail
	if (ir_num_occluders >= MAX_OCCLUDERS) {
		return;
	}

	//nothing before must be first
	if (occlude == 0) {
		ir_insert_begin(ang0, ang1);
		return;
	}

	occlude_t *next = &ir_occluders[ir_num_occluders++];
	next->ang0 = ang0;
	next->ang1 = ang1;
	next->next = occlude->next;
	occlude->next = next;
}

int ir_add_occluder(angle_t ang0, angle_t ang1) {
	occlude_t *occlude = ir_occlude, *next, *prev;
	int ret = 0;

	//backface
	if (ang1 - ang0 < ANG180) {
		return 0;
	}

	//crosses angle 0
	if (ang0 < ang1) {
		int ret0 = ir_add_occluder(ang0, 0);
		int ret1 = ir_add_occluder(-1, ang1);
		return ret0|ret1;
	}

	//fix order
	if (ang1 < ang0) {
		angle_t temp = ang0;
		ang0 = ang1;
		ang1 = temp;
	}

	//IR_RenderOcclude(ang0, ang1, 0);

	if (occlude) {
		//find the last occlude that starts before this one
		next = occlude->next;
		while (next) {
			if (next->ang0 > ang0) {
				break;
			}
			occlude = occlude->next;
			next = next->next;
		}
	}
	else {
		//empty list just insert
		ir_insert_begin(ang0, ang1);
		return 1;
	}
	prev = 0;
	while (occlude) {
		//start in front
		if (ang0 < occlude->ang0) {
			//end on
			if (ang1 == occlude->ang0) {
				//add to occlude
				occlude->ang0 = ang0;
				ret = 1;
				break;
			}
			//end in front
			if (ang1 < occlude->ang0) {
				//add a new occluder
				ir_insert_after(prev, ang0, ang1);
				ret = 1;
				break;
			}
			//end inside
			if (ang1 >= occlude->ang1) {
				//add to occlude
				occlude->ang0 = ang0;
				return 1;
			}
			//ends after
			//replace occlude
			occlude->ang0 = ang0;
			if (ang1 > occlude->ang1) {
				occlude->ang1 = ang1;
			}
			return 1;
		}
		//starts inside
		if (ang0 < occlude->ang1) {
			//ends inside
			if (ang1 <= occlude->ang1) {
				//completely occluded
				//do nothing
				return 0;
			}
			//must end outside
			//add to occlude
			occlude->ang1 = ang1;
			ret = 1;
			break;
		}
		prev = occlude;
		occlude = occlude->next;
	}
	//if we go past the end then we need to add
	if (prev && !occlude) {
		//starts at end
		if (prev->ang1 == ang0) {
			prev->ang1 = ang1;
			occlude = prev;
		}
		else {
			//add a new occluder
			ir_insert_after(prev, ang0, ang1);
			occlude = prev;
		}
		ret = 1;
	}
	//try to merge with next
	if (occlude && occlude->next) {
		next = occlude->next;
		while (next) {
			//if it starts after occlude end then done
			if (next->ang0 > occlude->ang1) {
				break;
			}
			//if it ends inside then remove it
			if (next->ang1 < occlude->ang1) {
				occlude->next = next->next;
				next = occlude;
			}
			//if it starts inside and ends outside then extend it
			if (next->ang0 > occlude->ang0 && next->ang1 > occlude->ang1) {
				occlude->ang1 = next->ang1;
				occlude->next = next->next;
				next = occlude->next;
				//dont advance next
				//we want to see if we can merge again
				continue;
			}

			next = next->next;
		}
	}
	return ret;
}

extern fixed_t		viewx, viewy, viewz;

int IR_AddOccluder(fixed_t x0, fixed_t y0, fixed_t x1, fixed_t y1) {
	angle_t ang0 = R_PointToAngle(x0, y0);
	angle_t ang1 = R_PointToAngle(x1, y1);

	int ret = ir_add_occluder(ang0, ang1);

	IR_RenderOcclude(ang0, ang1, ret ? 1 : 2);

	return ret;
}


int ir_is_visible(angle_t ang0, angle_t ang1) {
	occlude_t *occlude = ir_occlude, *next, *prev;

	//crosses angle 0
	if (ang0 < ang1) {
		int ret0 = ir_is_visible(ang0, 0);
		int ret1 = ir_is_visible(-1, ang1);
		return ret0 | ret1;
	}

	//fix order
	if (ang1 < ang0) {
		angle_t temp = ang0;
		ang0 = ang1;
		ang1 = temp;
	}
/**/
	if (occlude) {
		//find the last occlude that starts before this one
		next = occlude->next;
		while (next) {
			if (next->ang0 > ang0) {
				break;
			}
			occlude = occlude->next;
			next = next->next;
		}
	}
	else {
		//empty list just insert
		//ir_insert_begin(ang0, ang1);
		return 1;
	}
	prev = 0;
	while (occlude) {
		//start in front
		if (ang0 < occlude->ang0) {
			//end in front
			if (ang1 < occlude->ang0) {
				//add a new occluder
				//ir_insert_after(prev, ang0, ang1);
				return 1;
			}
			//end inside
			if (ang1 >= occlude->ang1) {
				//add to occlude
				//occlude->ang0 = ang0;
				return 1;
			}
			//ends after
			//replace occlude
			//occlude->ang0 = ang0;
			//if (ang1 > occlude->ang1) {
			//	occlude->ang1 = ang1;
			//}
			return 1;
		}
		//starts inside
		if (ang0 < occlude->ang1) {
			//ends inside
			if (ang1 <= occlude->ang1) {
				//completely occluded
				//do nothing
				return 0;
			}
			//must end outside
			//add to occlude
			//occlude->ang1 = ang1;
			return 1;
		}
		prev = occlude;
		occlude = occlude->next;
	}
	//if we go past the end then we need to add
	/*
	if (prev && !occlude) {
		//add a new occluder
		//ir_insert_after(prev, ang0, ang1);
		//occlude = prev->next;
	}
	//try to merge with next
	if (occlude && occlude->next) {
		next = occlude->next;
		while (next) {
			//if it starts after occlude end then done
			if (next->ang0 > occlude->ang1) {
				break;
			}
			//if it ends inside then remove it
			if (next->ang1 < occlude->ang1) {
				occlude->next = next->next;
			}
			next = next->next;
		}
	}*/
	return 1;
}

int IR_IsOccluded(fixed_t x0, fixed_t y0, fixed_t x1, fixed_t y1) {
	angle_t ang0 = R_PointToAngle(x0, y0);
	angle_t ang1 = R_PointToAngle(x1, y1);

	int ret  = ir_is_visible(ang0, ang1);

	//IR_RenderOcclude(ang0, ang1, ret ? 1 : 2);

	return !ret;
}

void IR_ResetOccluder() {
	ir_num_occluders = 0;
	ir_occlude = 0;
}

void ir_render_arc(angle_t ang0, angle_t ang1, float rad, float r, float g, float b, int c) {
#ifdef WIN32
	float vsin;
	float vcos;
	float x0, y0, x1, y1;
	angle_t temp = ang0;

	//fix order
	if (ang1 < ang0) {
		angle_t temp = ang0;
		ang0 = ang1;
		ang1 = temp;
	}

	vsin = finesine[ang0 >> ANGLETOFINESHIFT] / 65536.0f;
	vcos = finecosine[ang0 >> ANGLETOFINESHIFT] / 65536.0f;

	x0 = -vsin * rad;
	y0 = vcos * rad;
	x1 = 0;
	y1 = 0;
	glColor3f(r, g, b);
	glBegin(GL_LINES);
	if (c) {
		glVertex3f(x0, y0, 0);
		glVertex3f(x1, y1, 0);
	}
	while (ang0 < ang1) {
		temp += ANGLE_1;
		if (temp > ang1) {
			temp = ang1;
		}
		float vsin = finesine[temp >> ANGLETOFINESHIFT] / 65536.0f;
		float vcos = finecosine[temp >> ANGLETOFINESHIFT] / 65536.0f;
		x1 = -vsin * rad;
		y1 = vcos * rad;
		glVertex3f(x0, y0, 0);
		glVertex3f(x1, y1, 0);

		x0 = x1;
		y0 = y1;
		if (temp < ang0) {
			break;
		}
		ang0 = temp;
	}
	vsin = finesine[ang1 >> ANGLETOFINESHIFT] / 65536.0f;
	vcos = finecosine[ang1 >> ANGLETOFINESHIFT] / 65536.0f;
	x0 = -vsin * rad;
	y0 = vcos * rad;
	x1 = 0;
	y1 = 0;
	if (c) {
		glVertex3f(x0, y0, 0);
		glVertex3f(x1, y1, 0);
	}
	glEnd();
#endif
}

extern int render_occlude;
void IR_RenderOcclude(angle_t ang0, angle_t ang1, int before) {
#ifdef WIN32
	occlude_t *occlude = ir_occlude;
	float mat[16];

	if(render_occlude == 0) return;
	//crosses angle 0
	if (ang0 < ang1) {
		int i = 0;
		//int ret0 = ir_add_occluder(ang0, 0);
		//int ret1 = ir_add_occluder(-1, ang1);
		//return ret0 | ret1;
	}

	glDisable(GL_CULL_FACE);

	glMatrixMode(GL_MODELVIEW);
	glGetFloatv(GL_MODELVIEW_MATRIX, mat);

	glPushMatrix();

	switch (before) {
	case 0:
		ir_render_arc(ang0, ang1, 540, 1, 1, 0, 1);
		break;
	case 1:
		ir_render_arc(ang0, ang1, 540, 0, 1, 0, 1);
		break;
	case 2:
		ir_render_arc(ang0, ang1, 540, 1, 0, 0, 1);
		break;
	}



	while (occlude) {
		ir_render_arc(occlude->ang0, occlude->ang1, 500, 1, 0, 0, 1);
		occlude = occlude->next;
	}
	glPopMatrix();

	frameEnd();
	glFinish();

	frameBegin();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mat);
#endif

}