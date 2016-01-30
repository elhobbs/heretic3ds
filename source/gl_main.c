#if 0
#include <math.h>
#include <stdlib.h>
#include "DoomDef.h"
#include "R_local.h"
#include "map.h"
#ifdef WIN32
#include <gl\gl.h>
extern HWND openGL;
#define TEXGEN_TEXCOORD 0
#endif
#include "geometry.h"
#include "ds_textures.h"

int					r_framecount = 0;

extern fixed_t		viewx, viewy, viewz;
extern angle_t		viewangle;
extern fixed_t		viewcos, viewsin;
extern player_t		*viewplayer;
extern boolean		automapactive;

extern byte *host_basepal;

extern float ds_texture_width;
extern float ds_texture_height;

float DS_FLOAT_VAL = 65536.0f;

void aDS_VERTEX3V16(int x, int y, int z);
void glRotateZ(float x);
void glRotateX(float x);
void glRotateY(float x);

int ds_quads,ds_triangles;
void dsVertex3fv(float *v);

void DS_VERTEX3FV(float *p) {
#ifdef WIN32
	float x,y,z;

	x = p[0];
	y = p[1];
	z = p[2];
	//aDS_VERTEX3V16(x,y,z);
	glVertex3f(x,y,z);
	//printf("p: %f %f %f\n",x,y,z);
#endif

#ifdef ARM9
	dsVertex3fv(p);
#endif
}

void R_AddLineGL (seg_t*	line)
   {
    int			x1;
    int			x2;
    angle_t		angle1;
    angle_t		angle2;
    angle_t		span;
    angle_t		tspan;
    sector_t*	frontsector;
    sector_t*	backsector;
	float midbottom,midtop; //used to mark midtexture 
	float s[2],t[2],len,s_off,t_off,w,h;
	float p1[3],p2[3],t_coord[2][2];
	float bottom,top;
	//texture_t *texture;
	float light = 1.0f;
	int				name;
	texture_t		*texture;
	dstex_t			*ds;
    
    //curline = line;
	//return;

    // OPTIMIZE: quickly reject orthogonal back sides.
    angle1 = R_PointToAngle (line->v1->x, line->v1->y);
    angle2 = R_PointToAngle (line->v2->x, line->v2->y);
    
    // Clip to view edges.
    // OPTIMIZE: make constant out of 2*clipangle (FIELDOFVIEW).
    span = angle1 - angle2;
    
    // Back side? I.e. backface culling?
    if (span >= ANG180)
       {
        //WriteDebug("Back face culled...\n");
        return;		
       }
/*
    // Global angle needed by segcalc.
    rw_angle1 = angle1;
    angle1 -= viewangle;
    angle2 -= viewangle;
	
    tspan = angle1 + clipangle;
    if (tspan > 2*clipangle)
       {
        tspan -= 2*clipangle;

        // Totally off the left edge?
        if (tspan >= span)
           {
            //WriteDebug("Off the left edge...\n");
            return;
           }
	
	    angle1 = clipangle;
       }
    tspan = clipangle - angle2;
    if (tspan > 2*clipangle)
       {
        tspan -= 2*clipangle;

        // Totally off the left edge?
        if (tspan >= span)
           {
            //WriteDebug("Off the left edge...\n");
            return;	
           }
        angle2 = (clipangle * -1);
       }
    
    // The seg is in the view range,
    // but not necessarily visible.
    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    x1 = viewangletox[angle1];
    x2 = viewangletox[angle2];

    // Does not cross a pixel?
    if (x1 == x2)
       {
        //WriteDebug("No pixel spanned...\n");
        return;				
       }
*/	
    backsector = line->backsector;
    frontsector = line->frontsector;
	light = (frontsector->lightlevel)/256.0f;
	s[0] = (line->v2->x - line->v1->x)/DS_FLOAT_VAL;
	s[1] = (line->v2->y - line->v1->y)/DS_FLOAT_VAL;
	len = s[0]*s[0] + s[1]*s[1];
	if(len)
		len = sqrt(len);
	else
		len = 1.0f;

	s[0] /= len;
	s[1] /= len;

	s_off = ((line->sidedef->textureoffset + line->offset)/DS_FLOAT_VAL);	
	t_off = line->sidedef->rowoffset/DS_FLOAT_VAL;

    // Single sided line?
    if (!backsector)
       {
        //WriteDebug("Wall...\n");
        goto clipsolid;		
       }

    // Closed door.
    if (backsector->ceilingheight <= frontsector->floorheight || backsector->floorheight >= frontsector->ceilingheight)
       {
        //WriteDebug("Closed door...\n");
        goto clippass;		
        //goto clipsolid;		
       }

    // Window.
    if (backsector->ceilingheight != frontsector->ceilingheight || backsector->floorheight != frontsector->floorheight)
       {
        //WriteDebug("Window (portal)...\n");
        goto clippass;	
       }
		
    // Reject empty lines used for triggers and special events.
    // Identical floor and ceiling on both sides,
    // identical light levels on both sides,
    // and no middle texture.
    if (backsector->ceilingpic == frontsector->ceilingpic && backsector->floorpic == frontsector->floorpic &&
        backsector->lightlevel == frontsector->lightlevel && line->sidedef->midtexture == 0)
       {
        //WriteDebug("Trip line...\n");
		//goto clipsolid;
        return;
       }
				
    clippass:

	   bottom = midbottom = frontsector->floorheight/DS_FLOAT_VAL;
	   top = midtop = frontsector->ceilingheight/DS_FLOAT_VAL;
	   
	   p1[0] = line->v1->x/DS_FLOAT_VAL;
	   p1[1] = line->v1->y/DS_FLOAT_VAL;
	   p2[0] = line->v2->x/DS_FLOAT_VAL;
	   p2[1] = line->v2->y/DS_FLOAT_VAL;

	   //draw bottom
	   if(frontsector->floorheight < backsector->floorheight &&
		  line->sidedef->bottomtexture)
	   {
		midbottom = backsector->floorheight/DS_FLOAT_VAL;
		
		name = line->sidedef->bottomtexture;
		ds = &textures_ds[name];
		texture = textures[texturetranslation[name]];
		w = texture->width;
		h = texture->height;
		name = ds_load_map_texture(name,TEXGEN_TEXCOORD);

		t_coord[0][0] = (p1[0]*s[0] + p1[1]*s[1])/w;
	    t_coord[1][0] = (p2[0]*s[0] + p2[1]*s[1] + s_off)/w - t_coord[0][0];
		t_coord[0][0] = s_off/w;
		if (line->linedef->flags & ML_DONTPEGBOTTOM)
		{
			t_coord[1][1] = (-bottom)/h;
			t_coord[0][1] = (-midbottom + t_off)/h - t_coord[1][1];
			t_coord[1][1] = t_off/h;
		}
		else
		{
			t_coord[0][1] = (-midbottom)/h;
			t_coord[1][1] = (-bottom + t_off)/h - t_coord[0][1];
			t_coord[0][1] = t_off/h;
		}

		ds_quads++;
#ifdef WIN32
		glColor4f( light, light, light, 1.0f );
		//glColor4f( 0.0f, 0.0f, 1.0f, 1.0f );
		glBegin(GL_POLYGON);
		  
		  p1[2] = bottom;
		  glTexCoord2f(t_coord[0][0],t_coord[1][1]);
		  DS_VERTEX3FV(p1);
		  
		  p1[2] = midbottom;
		  glTexCoord2f(t_coord[0][0],t_coord[0][1]);
		  DS_VERTEX3FV(p1);
		  
		  p2[2] = midbottom;
		  glTexCoord2f(t_coord[1][0],t_coord[0][1]);
		  glVertex3fv(p2);
		  
		  p2[2] = bottom;
		  glTexCoord2f(t_coord[1][0],t_coord[1][1]);
		  DS_VERTEX3FV(p2);

		 glEnd();
#endif
	   }
	   //draw top
	   if(frontsector->ceilingheight > backsector->ceilingheight &&
		  line->sidedef->toptexture)
	   {
        // hack to allow height changes in outdoor areas
        if (frontsector->ceilingpic == skyflatnum && backsector->ceilingpic == skyflatnum)
			midtop = frontsector->ceilingheight/DS_FLOAT_VAL;
		else
			midtop = backsector->ceilingheight/DS_FLOAT_VAL;
		
		name = line->sidedef->toptexture;
		ds = &textures_ds[name];
		texture = textures[texturetranslation[name]];
		w = texture->width;
		h = texture->height;
		name = ds_load_map_texture(name,TEXGEN_TEXCOORD);

		t_coord[0][0] = (p1[0]*s[0] + p1[1]*s[1])/w;
	    t_coord[1][0] = (p2[0]*s[0] + p2[1]*s[1] + s_off)/w - t_coord[0][0];
		t_coord[0][0] = s_off/w;
		if (line->linedef->flags & ML_DONTPEGTOP)
		{
			t_coord[0][1] = (-top)/h;
			t_coord[1][1] = (-midtop + t_off)/h - t_coord[0][1];
			t_coord[0][1] = t_off/h;
		}
		else
		{
			t_coord[1][1] = (-midtop)/h;
			t_coord[0][1] = (-top)/h - t_coord[1][1];
			t_coord[1][1] = t_off/h;
		}

		ds_quads++;
#ifdef WIN32
		glColor4f( light, light, light, 1.0f );
		//glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
		glBegin(GL_POLYGON);
		  p1[2] = midtop;
		  glTexCoord2f(t_coord[0][0],t_coord[1][1]);
		  DS_VERTEX3FV(p1);
		  
		  p1[2] = top;
		  glTexCoord2f(t_coord[0][0],t_coord[0][1]);
		  DS_VERTEX3FV(p1);
		  
		  p2[2] = top;
		  glTexCoord2f(t_coord[1][0],t_coord[0][1]);
		  DS_VERTEX3FV(p2);
		  
		  p2[2] = midtop;
		  glTexCoord2f(t_coord[1][0],t_coord[1][1]);
		  DS_VERTEX3FV(p2);
		glEnd();
#endif
	   }

		midtop = backsector->ceilingheight/DS_FLOAT_VAL;
	   //draw middle
	   if(midbottom < midtop &&
		  line->sidedef->midtexture)
	   {
		
		name = line->sidedef->midtexture;
		ds = &textures_ds[name];
		texture = textures[texturetranslation[name]];
		w = texture->width;
		h = texture->height;
		name = ds_load_map_texture(name,TEXGEN_TEXCOORD);
	    t_coord[0][0] = (p1[0]*s[0] + p1[1]*s[1])/w;
	    t_coord[1][0] = (p2[0]*s[0] + p2[1]*s[1] + s_off)/w - t_coord[0][0];
		t_coord[0][0] = s_off/w;
		if (line->linedef->flags & ML_DONTPEGBOTTOM)
		{
			t_coord[0][1] = (-midtop + t_off)/h;
			t_coord[1][1] = (-midbottom + t_off)/h;
		}
		else
		{
			t_coord[0][1] = (-midtop)/h;
			t_coord[1][1] = (-midbottom + t_off)/h - t_coord[0][1];
			t_coord[0][1] = t_off/h;
		}

		ds_quads++;
#ifdef WIN32
		glColor4f( light, light, light, 1.0f );
		//glColor4f( 0.0f, 1.0f, 0.0f, 1.0f );
		glBegin(GL_POLYGON);
		  p1[2] = midbottom;
		  glTexCoord2f(t_coord[0][0],t_coord[1][1]);
		  DS_VERTEX3FV(p1);
		  
		  p1[2] = midtop;
		  glTexCoord2f(t_coord[0][0],t_coord[0][1]);
		  DS_VERTEX3FV(p1);
		  
		  p2[2] = midtop;
		  glTexCoord2f(t_coord[1][0],t_coord[0][1]);
		  DS_VERTEX3FV(p2);
		  
		  p2[2] = midbottom;
		  glTexCoord2f(t_coord[1][0],t_coord[1][1]);
		  DS_VERTEX3FV(p2);
		glEnd();
#endif
	   }
       //R_ClipPassWallSegmentGL (x1, x2-1);	
       return;
		
    clipsolid:
	   midbottom = frontsector->floorheight/DS_FLOAT_VAL;
	   midtop = frontsector->ceilingheight/DS_FLOAT_VAL;
	   
	   p1[0] = line->v1->x/DS_FLOAT_VAL;
	   p1[1] = line->v1->y/DS_FLOAT_VAL;
	   p2[0] = line->v2->x/DS_FLOAT_VAL;
	   p2[1] = line->v2->y/DS_FLOAT_VAL;

		name = line->sidedef->midtexture;
		ds = &textures_ds[name];
		texture = textures[texturetranslation[name]];
		w = texture->width;
		h = texture->height;
		name = ds_load_map_texture(name,TEXGEN_TEXCOORD);
		
	    t_coord[0][0] = (p1[0]*s[0] + p1[1]*s[1])/w;
	    t_coord[1][0] = (p2[0]*s[0] + p2[1]*s[1] + s_off)/w - t_coord[0][0];
		t_coord[0][0] = s_off/w;
		if (line->linedef->flags & ML_DONTPEGBOTTOM)
		{
			t_coord[1][1] = (-midbottom)/h;
			t_coord[0][1] = (-midtop + t_off)/h - t_coord[1][1];
			t_coord[1][1] = t_off/h;
		}
		else
		{
			t_coord[0][1] = (-midtop)/h;
			t_coord[1][1] = (-midbottom + t_off)/h - t_coord[0][1];
			t_coord[0][1] = t_off/h;
		}
		
		ds_quads++;
#ifdef WIN32
		glColor4f( light, light, light, 1.0f );
		//glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		glBegin(GL_POLYGON);
		  p1[2] = midbottom;
		  glTexCoord2f(t_coord[0][0],t_coord[1][1]);
		  DS_VERTEX3FV(p1);
		  
		  p1[2] = midtop;
		  glTexCoord2f(t_coord[0][0],t_coord[0][1]);
		  DS_VERTEX3FV(p1);
		  
		  p2[2] = midtop;
		  glTexCoord2f(t_coord[1][0],t_coord[0][1]);
		  DS_VERTEX3FV(p2);
		  
		  p2[2] = midbottom;
		  glTexCoord2f(t_coord[1][0],t_coord[1][1]);
		  DS_VERTEX3FV(p2);
		glEnd();
#endif
       //R_ClipSolidWallSegmentGL (x1, x2-1);
       return;
   }

void R_SubsectorGL (int num)
{
    int			i,count,name;
    seg_t*		line;
    subsector_t*	sub;
	sector_t* frontsector;
	int sec,l;
	float dist;
	winding_t *floor;
	float light,p[3];
	/*vec_t dist;
	float light,ceiling;
	winding_t *temp;
	float ss[3],st[3],ss_start[3],st_start[3],ss_stop[3],st_stop[3];
	float ss_blen,st_blen,ss_elen,st_elen;
	vec3_t p1,p2;*/

	
#ifdef RANGECHECK
    if (num >= numsubsectors)
	    I_Error ("R_Subsector: ss %i with numss = %i", num, numsubsectors);
#endif

    //sscount++;
    sub = &subsectors[num];
    frontsector = sub->sector;
    count = sub->numlines;
    line = &segs[sub->firstline];
	sec = frontsector - sectors;

	//floor = floors[num];
	dist = frontsector->floorheight/DS_FLOAT_VAL;
	//ceiling = ceilings[sec];
	i = count;
    while (i--)
	{
        R_AddLineGL (line);
        // hack to allow height changes in outdoor areas
        /*if((frontsector->ceilingpic == skyflatnum && line->backsector) || // && line->backsector->ceilingpic == skyflatnum)
          (frontsector->ceilingpic != skyflatnum && line->backsector && line->backsector->ceilingpic == skyflatnum))
			if(ceilings[line->backsector - sectors] > ceiling)
				ceiling = ceilings[line->backsector - sectors];*/
        line++;
	}

	floor = floors_ds[num];
	if(floor == 0)
		return;

	/*if((ceiling > (frontsector->ceilingheight/65536.0f)))
	{
		if(frontsector->ceilingpic == skyflatnum)
		{
		}
	}*/

	light = (frontsector->lightlevel)/256.0f;
	dist = frontsector->floorheight/65536.0f;

	switch(sub->sector->special)
	{
	case 4:
	case 5:
	case 7:
	case 16:
		//glBindTexture(GL_TEXTURE_2D,first_flat + frontsector->floorpic);
		//glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		//EmitWaterPolys(floor->next,dist);
		//break;
	default:
		count = floor->numpoints;
		ds_triangles += ((count-2)*2);
		//glBindTexture(GL_TEXTURE_2D,0);
		name = ds_load_map_flat(flattranslation[frontsector->floorpic]);
		l = ((((unsigned)frontsector)>>2)&0xff);
		//l = ((((unsigned)num))&0xff);
#ifdef WIN32
		//glBegin(GL_LINE_LOOP);
		glBegin(GL_POLYGON);
		glColor4f( light, light, light, 1.0f );
		//glColor4f( host_basepal[l*3]/255.0f, host_basepal[l*3+1]/255.0f, host_basepal[l*3+2]/255.0f, 1.0f );
		//glIndexi( num & 1 ? 2 : 9);
		while(count--)
		{
			p[0] = floor->p[count].x/DS_FLOAT_VAL;
			p[1] = floor->p[count].y/DS_FLOAT_VAL;
			p[2] = dist;
			glTexCoord2f(p[0]/64.0f,p[1]/64.0f);
			DS_VERTEX3FV(p);
		}	
		glEnd();
#endif
		
		dist = frontsector->ceilingheight/65536.0f;
		count = floor->numpoints;
		//glBindTexture(GL_TEXTURE_2D,0);
		name = ds_load_map_flat(flattranslation[frontsector->ceilingpic]);
		l = ((((unsigned)frontsector)>>2)&0xff);
		//l = ((((unsigned)num))&0xff);
		//glBegin(GL_LINE_LOOP);
#ifdef WIN32
		glBegin(GL_POLYGON);
		glColor4f( light, light, light, 1.0f );
		//glColor4f( host_basepal[l*3]/255.0f, host_basepal[l*3+1]/255.0f, host_basepal[l*3+2]/255.0f, 1.0f );
		//glIndexi( num & 1 ? 2 : 9);
		while(count--)
		{
			p[0] = floor->p[count].x/DS_FLOAT_VAL;
			p[1] = floor->p[count].y/DS_FLOAT_VAL;
			p[2] = dist;
			glTexCoord2f(p[0]/64.0f,p[1]/64.0f);
			DS_VERTEX3FV(p);
		}	
		glEnd();
#endif
	}
}
void R_RenderBSPNodeGL (int bspnum)
   {
    node_t*	bsp;
    int		side;

    // Found a subsector?
    if (bspnum & NF_SUBSECTOR)
       {
        if (bspnum == -1)			
            R_SubsectorGL (0);
        else
            R_SubsectorGL (bspnum&(~NF_SUBSECTOR));
        return;
       }
		
    bsp = &nodes[bspnum];
    
    // Decide which side the view point is on.
    side = R_PointOnSide(viewx, viewy, bsp);

    // Recursively divide front space.
    R_RenderBSPNodeGL (bsp->children[side^1]); 

    // Possibly divide back space.
    //if (R_CheckBBox (bsp->bbox[side^1]))	
	R_RenderBSPNodeGL (bsp->children[side]);
}

void R_RenderBSPGL()
{
    if (gamestate == GS_LEVEL && !automapactive && gametic)
       {
        //WriteDebug("R_RenderPlayerView...\n");
        //R_RenderPlayerView (&players[displayplayer]);
			R_RenderBSPNodeGL (numnodes-1);
		//GL_DrawText(100,100,"!HELLO WORLD!");
       }
}


void glTranslate3f32(int x,int y, int z);
int frameBegin();
float vx,vy,vz;

extern int draw_gen_floors;
extern int draw_gen_sub;
void R_SetupFrameGL() {
	float aspect,rx,ry,rz,x,y,z,fAngle;
	int width,height;
#ifdef WIN32
	RECT rc;
	GetWindowRect(openGL,&rc);
	aspect = (float)(rc.right-rc.left)/(float)(rc.bottom-rc.top);
	width = rc.right-rc.left;
	height = rc.bottom-rc.top;

#else
	aspect = (float)DS_SCREEN_WIDTH/(float)DS_SCREEN_HEIGHT;
	width = DS_SCREEN_WIDTH - 1;
	height = DS_SCREEN_HEIGHT - 1;
#endif

#ifdef WIN32
	frameBegin();
#endif

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef WIN32
	//glFrustum(-1.0*aspect,1.0*aspect,-1.0,1.0,1.0,8192.0);
	glFrustum(-1.0*aspect,1.0*aspect,-1.0,1.0,0.005,40.0);
#endif

#ifdef ARM9
	//gluPerspective(68.3, (float)1024 / (float)768, 0.005, 40.0);
	glFrustum(-1.0*aspect,1.0*aspect,-1.0,1.0,0.005,40.0);
#endif
    glViewport (0, 0, width,height);


	// Set the current matrix to be the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if(draw_gen_floors || draw_gen_sub)
	{
		x = MAP_COORD(-300*(1<<16));//128.0f;//viewx/DS_FLOAT_VAL;
		y = MAP_COORD(300*(1<<16));//-944.0f;//viewy/DS_FLOAT_VAL;
		z = MAP_COORD(10*(1<<16));//viewz/DS_FLOAT_VAL + 2000; 
		fAngle = 90.0f;//(float)360.0*(viewangle>>ANGLETOFINESHIFT)/8192.0f;
		//fPitch = (float)360.0*(viewpitch>>ANGLETOFINESHIFT)/8192.0f;

		glRotateX(-90);//f( -90.0f, 1.0f, 0.0f, 0.0f );
		glRotateX(90);//f( -90.0f, 1.0f, 0.0f, 0.0f );
		glRotatef( 90-fAngle, 0.0f, 0.0f, 1.0f );
	}
	else
	{
		x = viewx/DS_FLOAT_VAL;
		y = viewy/DS_FLOAT_VAL;
		z = viewz/DS_FLOAT_VAL; 
		fAngle = (float)360.0*(viewangle>>ANGLETOFINESHIFT)/8192.0f;
		//fPitch = (float)360.0*(viewpitch>>ANGLETOFINESHIFT)/8192.0f;

		glRotateX(-90);//f( -90.0f, 1.0f, 0.0f, 0.0f );
		//glRotateX(90);//f( -90.0f, 1.0f, 0.0f, 0.0f );
		glRotateZ( 90-fAngle);//, 0.0f, 0.0f, 1.0f );
	}
    //glRotatef( fPitch, sinf, -cosf, 0.0f );
#ifdef ARM9
	glTranslate3f32(-(int)(x),-(int)(y),-(int)(z));
#else
	glTranslatef(-x,-y,-z);
#endif
}

#if 0
void R_SetupFrameGL2() {
	RECT rc;
	float aspect,rx,ry,rz,x,y,z;

	frameBegin();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GetWindowRect(openGL,&rc);
	aspect = (float)(rc.right-rc.left)/(float)(rc.bottom-rc.top);
	glFrustum(-1.0*aspect,1.0*aspect,-1.0,1.0,1.0,8192.0);
    glViewport (0, 0, rc.right-rc.left, rc.bottom-rc.top);

#ifdef NDS
	glColor3b(255,255,255);
	glMaterialf(GL_AMBIENT, RGB15(24,24,24));
	glMaterialf(GL_DIFFUSE, RGB15(24,24,24));
	glMaterialf(GL_SPECULAR, RGB15(0,0,0));
	glMaterialf(GL_EMISSION, RGB15(0,0,0));

	//MYgluPerspective (r_refdef.fov_y,  vid.aspect,  0.005,  40.0*0.3);
#endif
	// Set the current matrix to be the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
#ifdef NDS
	glLight(0, RGB15(31,31,31) , floattov10(r_vright[0]), floattov10(r_vright[1]), floattov10(r_vright[2]));
#endif


	glRotateX(-90);
	//glRotateZ(90);//r_angle[2]);
#ifdef NDS
	glStoreMatrix(3);
#endif
	glRotateX(90);//r_angle[1]);
	//glRotateY(-r_refdef.viewangles[0]);//r_angle[0]);
	rz = 90.0f-((float)360.0f*(viewangle>>ANGLETOFINESHIFT)/8192.0f);
	glRotateZ(rz);//r_angle[2]);
#define floattodsv16(n)       ((int)((n) * (1 << 2)))
	x = -(viewx/DS_FLOAT_VAL);
	y = -(viewy/DS_FLOAT_VAL);
	z = -(viewz/DS_FLOAT_VAL + 1000);

	printf("f: %f (%f %f %f)\n",rz,x,y,z);
	glTranslate3f32(x,y,z);
#ifdef NDS
	glStoreMatrix(4);
#endif

}
#endif

void R_EndFrameGL (void)
{

#ifdef ARM9
		glFlush(2);
#endif

#ifdef WIN32
int frameEnd();
	frameEnd();
#endif
		//Con_DPrintf("draw: %3d cull: %3d all: %3d\n",r_surf_draw,r_surf_cull,r_surf_draw+r_surf_cull);
		//Con_DPrintf("surf: %4d alias: %4d all: %4d\n",r_surf_tri,r_alias_tri,r_surf_tri+r_alias_tri);
}
void draw_floors();
void draw_lines();
void draw_gen_subs();
void R_RenderView() {
	ds_quads = ds_triangles = 0;
	R_SetupFrameGL();
#ifdef DEBUG_FLOORS
	if(draw_gen_sub)
	{
#ifdef WIN32
		glDisable(GL_DEPTH_TEST);
#endif

		draw_gen_subs();

#ifdef WIN32
		glEnable(GL_DEPTH_TEST);
#endif
	}
	else if(draw_gen_floors)
	{
		draw_floors();
		draw_lines();
	}
	else
#endif
	{
		R_RenderBSPGL();
	}
	R_EndFrameGL();
	r_framecount++;
	printf("q: %d t: %d - all: %d\n",ds_quads,ds_triangles,ds_quads*2+ds_triangles);
}

void draw_sub(int num,float dist)
{
	subsector_t *sub;
	seg_t *line;
	int i,count;
	float p0[3],p1[3];

	if(num == -1)
		return;

	sub  = &subsectors[num];
    line = &segs[sub->firstline];
	count = sub->numlines;
	i = 0;

	do
	{
		p0[0] = line->v1->x/65536.0f;
		p0[1] = line->v1->y/65536.0f;
		p0[2] = dist;
		p1[0] = line->v2->x/65536.0f;
		p1[1] = line->v2->y/65536.0f;
		p1[2] = dist;
#ifdef WIN32
		glBegin(GL_LINES);

		glColor4f(1.0f,0.0f,0.0f,1.0f);
		glVertex3fv(p0);
		glColor4f(1.0f,1.0f,1.0f,1.0f);
		glVertex3fv(p1);

		glEnd();
#endif

		i++;
		line++;
	}
	while(i < count);
}
int draw_windings = 1;

void draw_winding(winding_t *floor)
{
	int i,count = floor->numpoints;
	unsigned l;
	byte c;
	float p[3];
	float p1[3];
	float p2[3];
#ifdef WIN32
	glBindTexture(GL_TEXTURE_2D,0);
#endif

#if 1
	if(draw_windings)
	{
		if(floor->sub == -1)
		{
			l = ((((unsigned)(floor))>>2)&0xff);
		}
		else 
		{
			//l = ((((unsigned)(subsectors[floor->sub].sector))>>2)&0xff);
			l = ((((unsigned)floor->sub))&0xff);
		}
		//glBegin(GL_LINE_LOOP);
		glBegin(GL_TRIANGLES);
		//glColor3b( host_basepal[l*3], host_basepal[l*3+1], host_basepal[l*3+2]);
		c = (l & 0x3f) + 0xC0;
#ifdef WIN32
		glColor3ub( c,c,c);
#endif

#ifdef ARM9
		c >>= 3;
		DS_COLOR(RGB15(c,c,c));
#endif
			p[0] = MAP_COORD(floor->p[0].x);
			p[1] = MAP_COORD(floor->p[0].y);
			p[2] = MAP_COORD2(floor->dist);
			p1[0] = MAP_COORD(floor->p[1].x);
			p1[1] = MAP_COORD(floor->p[1].y);
			p1[2] = MAP_COORD2(floor->dist);
		for(i=2;i<count;i++)
		{
			p2[0] = MAP_COORD(floor->p[i].x);
			p2[1] = MAP_COORD(floor->p[i].y);
			p2[2] = MAP_COORD2(floor->dist);
#ifdef ARM9
			dsVertex3fv(p);
			dsVertex3fv(p1);
			dsVertex3fv(p2);
#else
			glVertex3fv(p);
			glVertex3fv(p1);
			glVertex3fv(p2);
#endif

			memcpy(p1,p2,sizeof(p1));
		}	
		glEnd();

	}
#endif
	draw_sub(floor->sub,floor->dist + 5.0f);
}


void draw_line(vertex_t *a,vertex_t *b,float dist)
{
	float p0[3],p1[3];
		p0[0] = a->x/DS_FLOAT_VAL;
		p0[1] = a->y/DS_FLOAT_VAL;
		p0[2] = dist;
		p1[0] = b->x/DS_FLOAT_VAL;
		p1[1] = b->y/DS_FLOAT_VAL;
		p1[2] = dist;
#ifdef WIN32
		glBegin(GL_LINES);
		glColor4f( 1.0f,1.0f,0.0f,1.0f );
		DS_VERTEX3FV(p0);
		glColor4f( 0.0f,0.0f,1.0f,1.0f );
		DS_VERTEX3FV(p1);
		glEnd();
#endif
}
#endif
