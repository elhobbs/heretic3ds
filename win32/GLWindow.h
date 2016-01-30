// GLWindow.h: interface for the CGLWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLWINDOW_H__DC33B7BB_1160_11D3_A72D_00A0C909DB8A__INCLUDED_)
#define AFX_GLWINDOW_H__DC33B7BB_1160_11D3_A72D_00A0C909DB8A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>

#define    TEXTURE0_SGIS				0x835E
#define    TEXTURE1_SGIS				0x835F
typedef void (APIENTRY *lpMTexFUNC) (GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *lpSelTexFUNC) (GLenum);
typedef void (APIENTRY *lpSetVsyncFUNC) (GLenum);
extern lpMTexFUNC		glMTexCoord2fSGIS;
extern lpSelTexFUNC	glSelectTextureSGIS;
extern lpSetVsyncFUNC	wglSwapIntervalEXT;

class CGLWindow  
{
public:
	CGLWindow();
	virtual ~CGLWindow();

	int				SetFormat(int bpp,int zdepth,int alphadepth,int stencildepth);
	int				Size(int x,int y, int width, int height);
	int				ShutDown();
	int				SwapBuffers();
	int				Link(HWND hwnd);
	void			GetPos(RECT *rc);
	void			Push2D();
	void			Pop2D();
	bool			gl_mtexable;

private:

	HWND		m_hwnd;
	HDC			m_hdc;
	HGLRC		m_hglrc;

	//window size
	int			m_x;
	int			m_y;
	int			m_width;
	int			m_height;

	//pixel format
	int			m_bpp;
	int			m_zdepth;
	int			m_stencildepth;
	int			m_alphadepth;

};

#endif // !defined(AFX_GLWINDOW_H__DC33B7BB_1160_11D3_A72D_00A0C909DB8A__INCLUDED_)
