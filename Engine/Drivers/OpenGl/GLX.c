/****************************************************************************************/
/*  GLX.c                                                                               */
/*                                                                                      */
/*  Author: Daniel Queteschiner                                                         */
/*  Description: GLX-specific functionality for OpenGL driver                           */
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*                                                                                      */
/****************************************************************************************/

#include <stdio.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h> // install libxrandr-dev

#include "GLX.h"
#include "OglDrv.h"

static GLXContext hRC;

GLboolean Fullscreen	= GE_FALSE;

void WindowSetup(DRV_DriverHook *Hook)
{
	// if not windowed, set window pos to (0, 0), remove decorations/borders
	if(Hook->Width != -1 && Hook->Height != -1)
	{
		Display *dpy = ((glxWnd*)(Hook->hWnd))->display;
		Window win = ((glxWnd*)(Hook->hWnd))->win;
		XWindowChanges wc;
		wc.border_width = 0;
		wc.x = 0;
		wc.y = 0;
		XConfigureWindow(dpy, win, CWX | CWY | CWBorderWidth, &wc);
		return;
	}
}


void SwitchFullScreen(HWND hWnd, geBoolean fullscreen)
{
	fprintf(stderr,"OglDrv: GLX: SwitchFullScreen\n");
	if (fullscreen == Fullscreen)
		return;

	if (fullscreen)
	{
		Display *dpy = ((glxWnd*)(hWnd))->display;
		Window win = ((glxWnd*)(hWnd))->win;

		XSetWindowAttributes swa;
		swa.border_pixmap = None;
		swa.border_pixel = 0;
		swa.override_redirect = True;
		XChangeWindowAttributes(dpy, win, CWBorderPixmap | CWBorderPixel | CWOverrideRedirect, &swa);
		XWindowChanges wc;
		wc.border_width = 0;
		wc.x = 0;
		wc.y = 0;
		XConfigureWindow(dpy, win, CWX | CWY | CWBorderWidth, &wc);
	}
	else
	{
		Display *dpy = ((glxWnd*)(hWnd))->display;
		Window win = ((glxWnd*)(hWnd))->win;
		XSetWindowAttributes swa;
		swa.border_pixmap = CopyFromParent;
		swa.override_redirect = False;
		XChangeWindowAttributes(dpy, win, CWBorderPixmap | CWOverrideRedirect, &swa);
	}
	Fullscreen = fullscreen;
}


void WindowCleanup()
{
	if(ClientWindow.hWnd != NULL)
	{
		glxWnd * GLXWnd = (glxWnd*)ClientWindow.hWnd;
		{
			glXMakeCurrent(GLXWnd->display, None, NULL);
			glXDestroyContext(GLXWnd->display, hRC);
			hRC = 0;
		}

		if (GLXWnd->win)
		{
			if(Fullscreen)
			{
				SwitchFullScreen(ClientWindow.hWnd, GE_FALSE);
				Fullscreen = GE_FALSE;
			}
		}
	}
}


void FlipGLBuffers()
{
	glxWnd * GLXWnd = (glxWnd*)ClientWindow.hWnd;
	glXSwapBuffers(GLXWnd->display, GLXWnd->win);
}


geBoolean SetFullscreen(DRV_DriverHook *Hook)
{
	fprintf(stderr,"OglDrv: SetFullscreen\n");
	SwitchFullScreen(Hook->hWnd, GE_TRUE);
	return GE_TRUE;
}


void SetGLPixelFormat(DRV_DriverHook *Hook)
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, ZBUFFER_DEPTH, GLX_DOUBLEBUFFER, None };
	glxWnd *GLXWnd = (glxWnd*)(Hook->hWnd);
	Display *dpy = GLXWnd->display;
	Window win = GLXWnd->win;

	XVisualInfo *vis = glXChooseVisual(dpy, 0, att);

	if(vis != NULL)
	{
		hRC = glXCreateContext(dpy, vis, NULL, True);
		if(!hRC)
		{
			return;
		}

		glXMakeCurrent(dpy, win, hRC);
	}
}


GLint EnumNativeModes(DRV_ENUM_MODES_CB *Cb, void *Context)
{
	GLint modeCount = 0;
	char resolution[16];
	FILE *stream;
	int dummy;

	COLOR_DEPTH = 24;
	ZBUFFER_DEPTH = 16;

	stream = fopen("D3D24.ini","r");
	if(stream)
	{
		(void)fscanf(stream,"%d",&COLOR_DEPTH);
		(void)fscanf(stream,"%d",&ZBUFFER_DEPTH);
		fclose(stream);
	}

	modeCount = 0;

	Display *dpy = XOpenDisplay(NULL);
	if (XQueryExtension(dpy, "RANDR", &dummy, &dummy, &dummy))
	{
		XRRScreenConfiguration *screenConfig;
		screenConfig = XRRGetScreenInfo(dpy, DefaultRootWindow(dpy));

		if (screenConfig)
		{
			XRRScreenSize *screenSizes;
			int nSizes = 0;
			int sizeID;

			screenSizes = XRRConfigSizes(screenConfig, &nSizes);

			for(sizeID = 0; sizeID < nSizes; sizeID++)
			{
				sprintf(resolution, "%dx%d", screenSizes[sizeID].width, screenSizes[sizeID].height);

				if(!Cb(modeCount + 1, resolution, screenSizes[sizeID].width, screenSizes[sizeID].height, Context))
				{
					XRRFreeScreenConfigInfo(screenConfig);
					return modeCount;
				}
				modeCount++;
			}
			XRRFreeScreenConfigInfo(screenConfig);
		}
	}
	else
	{
		int width = DisplayWidth(dpy, DefaultScreen(dpy));
		int height = DisplayHeight(dpy, DefaultScreen(dpy));

		sprintf(resolution, "%dx%d", width, height);

		if(!Cb(modeCount + 1, resolution, width, height, Context))
		{
			return modeCount;
		}
		modeCount++;
	}

	XCloseDisplay(dpy);

	Cb(modeCount + 1, "WindowMode", -1, -1, Context);

	return modeCount;
}

