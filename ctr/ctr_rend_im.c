#include "ctr_rend.h"
#include <stdlib.h>

void glBegin(GLenum a) {
	DBGPRINT("glBegin %d\n", a);
}

void glVertex3fv(GLfloat *a) {
	DBGPRINT("glVertex3fv\n");
}

void glVertex2f(GLfloat x, GLfloat y) {
	DBGPRINT("glVertex2f\n");
}

void glTexCoord2f(GLfloat s, GLfloat t) {
	DBGPRINT("glVertex2f\n");
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	DBGPRINT("glTexCoord2f\n");
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	DBGPRINT("glColor3f\n");
}


void glEnd() {
	DBGPRINT("glEnd\n");
}
