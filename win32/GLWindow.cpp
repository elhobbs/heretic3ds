// GLWindow.cpp: implementation of the CGLWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "GLWindow.h"
lpMTexFUNC		glMTexCoord2fSGIS;
lpSelTexFUNC	glSelectTextureSGIS;
lpSetVsyncFUNC	wglSwapIntervalEXT;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGLWindow::CGLWindow()
{

	m_bpp = m_stencildepth = m_alphadepth = 0;
	m_x = m_y = m_width = m_height = 0;
	m_hglrc = NULL;
	m_hdc = NULL;
	m_hwnd  = NULL;
	gl_mtexable = true;

}

int CGLWindow::Link(HWND hwnd)
{
	if(m_hglrc)
	{
		wglMakeCurrent(m_hdc,NULL);
		wglDeleteContext(m_hglrc);
	}

	if(m_hwnd && m_hdc)
		ReleaseDC(m_hwnd,m_hdc);

	m_hwnd = hwnd;

	return TRUE;
}

CGLWindow::~CGLWindow()
{

}

int CGLWindow::ShutDown()
{
	if(m_hglrc)
	{
		glFinish();
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(m_hglrc);
	}

	if(m_hwnd && m_hdc)
		ReleaseDC(m_hwnd,m_hdc);

	return TRUE;;
}

void CGLWindow::Push2D()
{
    glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, m_width, 0, m_height);
	
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void CGLWindow::Pop2D()
{
	glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

int CGLWindow::Size(int x,int y,int width,int height)
{
	if(m_hglrc && m_hdc)
	{
		float aspect;
		if(wglGetCurrentContext() != m_hglrc)
			wglMakeCurrent(m_hdc,m_hglrc);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity(); 
		glViewport(0,0,width,height);

		//glMatrixMode(GL_PROJECTION);
		//glLoadIdentity();
		//aspect = (float)width/(float)height;
		//glFrustum(-5.0*aspect,5.0*aspect,-5.0,5.0,5.0,10000.0);
	}
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;

	return FALSE;
}

int CGLWindow::SetFormat(int bpp,int zdepth,int alphadepth,int stencildepth)
{
	int pixelformat;
	static DEVMODE gdevmode;

/*		gdevmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		gdevmode.dmBitsPerPel = 32;
		gdevmode.dmPelsWidth = 1024;
		gdevmode.dmPelsHeight = 768;
		gdevmode.dmSize = sizeof (gdevmode);

		if (ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			MessageBox(m_hwnd,"ChangeDisplaySettings","Error",MB_OK);
			//Sys_Error ("Couldn't set fullscreen DIB mode");
*/    
	PIXELFORMATDESCRIPTOR pfd = 
        {
         sizeof(PIXELFORMATDESCRIPTOR),  // size of this pfd
                1,                       // version number
                PFD_DRAW_TO_WINDOW |     // support window
                PFD_SUPPORT_OPENGL |     // support OpenGL
                PFD_DOUBLEBUFFER,        // double buffered
                PFD_TYPE_RGBA,//PFD_TYPE_COLORINDEX,           // RGBA type
                bpp,                      // 24-bit color depth
                0, 0, 0, 0, 0, 0,        // color bits ignored
                alphadepth,                       // no alpha buffer
                0,                       // shift bit ignored
                0,                       // no accumulation buffer
                0, 0, 0, 0,              // accum bits ignored
                zdepth,                      // 32-bit z-buffer      
                stencildepth,                       // no stencil buffer
                0,                       // no auxiliary buffer
                PFD_MAIN_PLANE,          // main layer
                0,                       // reserved
                0, 0, 0                  // layer masks ignored
        };

	m_hdc = GetDC(m_hwnd);
	if(m_hdc == NULL)
		return FALSE;

	pixelformat = ChoosePixelFormat(m_hdc,&pfd);
	if(pixelformat == 0)
		return FALSE;
	
	if(SetPixelFormat(m_hdc,pixelformat,&pfd) == 0)
		return FALSE;

	m_hglrc = wglCreateContext(m_hdc);
	if(m_hglrc == NULL)
		return FALSE;

	if(wglMakeCurrent(m_hdc,m_hglrc) == 0)
		return FALSE;

	wglSwapIntervalEXT = (lpSetVsyncFUNC)wglGetProcAddress("wglSwapIntervalEXT");

	if(wglSwapIntervalEXT) {
		wglSwapIntervalEXT(1);
	}

	if(zdepth)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_TEXTURE_2D);
		//glDisable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_ALPHA_TEST);
		/*glCullFace(GL_BACK);
		glFrontFace(GL_CW);
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glAlphaFunc(GL_GREATER,0.0f);*/
		glClearColor(0.0f,0.0f,0.0f,0.0f);
	}
	m_bpp = bpp;
	m_zdepth = zdepth;
	m_alphadepth = alphadepth;
	m_stencildepth = stencildepth;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); 
	glFrustum(-5.0,5.0,-5.0,5.0,5.0,10000.0);

	return TRUE;
}

int CGLWindow::SwapBuffers()
{
	return 	::SwapBuffers(m_hdc);
}

void CGLWindow::GetPos(RECT *rc)
{
	if(rc == NULL)
		return;

	rc->left = m_x;
	rc->top = m_y;
	rc->bottom = m_y + m_height;
	rc->right = m_x + m_width;
}