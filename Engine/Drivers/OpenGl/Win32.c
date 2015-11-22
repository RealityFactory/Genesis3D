/****************************************************************************************/
/*  Win32.c                                                                             */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Win32-specific functionality for OpenGL driver                         */
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

#include <windows.h>
#include <stdio.h>		

#include "Win32.h"
#include "OglDrv.h"


HWND originalWnd		= NULL;
WNDPROC originalWndProc = NULL;
HDC	hDC					= NULL;
HGLRC hRC				= NULL;
GLboolean fullscreen	= GE_FALSE;


void WindowSetup(DRV_DriverHook *Hook)
{
	if(Hook->Width != -1 && Hook->Height != -1)
		SetWindowPos(Hook->hWnd, HWND_TOP, 0, 0, Hook->Width, Hook->Height,
		SWP_NOCOPYBITS | SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
}


void WindowCleanup()
{
	if(ClientWindow.hWnd != NULL)
	{
		if(hDC != NULL && hRC != NULL)
		{
			wglMakeCurrent(hDC, NULL);
			wglDeleteContext(hRC);
		}
					
		ReleaseDC(ClientWindow.hWnd, hDC);
	} 

	if(fullscreen)
	{
		ChangeDisplaySettings(NULL, 0);
		fullscreen = GE_FALSE;
	}
}


void FlipGLBuffers()
{
	SwapBuffers(hDC);
}


geBoolean SetFullscreen(DRV_DriverHook *Hook)
{
	GLuint		modeCount, refresh = 0;
	GLboolean	foundMatch = GE_FALSE;
	DEVMODE		devMode, bestMode;

	devMode.dmSize = sizeof(DEVMODE); 
	devMode.dmPelsWidth = Hook->Width;
	devMode.dmPelsHeight = Hook->Height;
	
	modeCount = 0;

	while(EnumDisplaySettings(NULL, modeCount++, &devMode))
	{
		if(devMode.dmBitsPerPel != COLOR_DEPTH)
		{
			continue;
		}
		
		if((GLint)devMode.dmPelsWidth == Hook->Width && 
			(GLint)devMode.dmPelsHeight == Hook->Height)
		{
			if(devMode.dmDisplayFrequency >= refresh)
			{
				bestMode = devMode;
				foundMatch = GE_TRUE;
			}
		}
	}
	

	if(foundMatch)
	{
		if(ChangeDisplaySettings(&bestMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
		{	
			fullscreen = GE_TRUE;
			return GE_TRUE;
		}
	}


	return GE_FALSE;
}


void SetGLPixelFormat(DRV_DriverHook *Hook)
{
	GLint nPixelFormat;
	static PIXELFORMATDESCRIPTOR pfd;

	memset(&pfd, 0x00, sizeof(pfd));
	
	pfd.nSize         = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion      = 1;
	pfd.dwFlags       = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.dwLayerMask   = PFD_MAIN_PLANE;
	pfd.iPixelType    = PFD_TYPE_RGBA;
	pfd.cColorBits    = COLOR_DEPTH;
	pfd.cDepthBits    = ZBUFFER_DEPTH;
	pfd.cAccumBits    = 0;
	pfd.cStencilBits  = 0;

	hDC = GetDC(Hook->hWnd);								

	if(hDC != NULL)
	{
		if(GetPixelFormat(hDC) <= 0)
		{
			nPixelFormat = ChoosePixelFormat(hDC, &pfd);	
			SetPixelFormat(hDC, nPixelFormat, &pfd);	
		}

		hRC = wglCreateContext(hDC);					 
		wglMakeCurrent(hDC, hRC);					
	}
}


GLint EnumNativeModes(DRV_ENUM_MODES_CB *Cb, void *Context)
{
	DEVMODE devMode;
    GLint modeCount, modeListCount;
	char resolution[16];
	GLint idx = 0;
	MODELIST *modeList = NULL;
	FILE *stream;
		
	COLOR_DEPTH = 16;
	ZBUFFER_DEPTH = 16;

	stream = fopen("D3D24.ini","r");
	if(stream)
	{
		fscanf(stream,"%d",&COLOR_DEPTH);
		fscanf(stream,"%d",&ZBUFFER_DEPTH);
		fclose(stream);
	}

	modeCount = 0;

	memset(&devMode, 0x00, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

	while(EnumDisplaySettings(NULL, modeCount, &devMode))
	{
		modeCount++;

		if(devMode.dmBitsPerPel != COLOR_DEPTH)
		{
			continue;
		}
		
		if(ChangeDisplaySettings(&devMode, CDS_TEST) != DISP_CHANGE_SUCCESSFUL)
		{	
			continue;
		}
	}

	modeList = (MODELIST *)malloc(modeCount * sizeof(MODELIST));
	memset(modeList, 0x00, modeCount * sizeof(MODELIST));
	modeListCount = modeCount;
	modeCount = 0;

	memset(&devMode, 0x00, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

    while(EnumDisplaySettings(NULL, modeCount, &devMode))
	{
		GLboolean found = GE_FALSE;
		GLint count;

		modeCount++;

		if(devMode.dmBitsPerPel != COLOR_DEPTH)
		{
			continue;
		}
		
		if(ChangeDisplaySettings(&devMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
		{	
			for(count = 0; count < modeListCount; count++)
			{
				if(modeList[count].width == devMode.dmPelsWidth &&
					modeList[count].height == devMode.dmPelsHeight)
				{
					found = GE_TRUE;
					break;
				}
			}

			if(!found)
			{
				sprintf(resolution, "%dx%d", devMode.dmPelsWidth, devMode.dmPelsHeight);

				if(!Cb(modeCount + 1, resolution, devMode.dmPelsWidth, devMode.dmPelsHeight, Context))
				{
					free(modeList);
					return modeCount;
				}

				for(count = 0; count < modeListCount; count++)
				{
					if(modeList[count].width == 0 && modeList[count].height == 0)
					{
						modeList[count].width = devMode.dmPelsWidth;
						modeList[count].height = devMode.dmPelsHeight;
						break;
					}
				}
			}
		}

		modeCount++;
	}

	free(modeList);

	Cb(modeCount + 1, "WindowMode", -1, -1, Context);
	
	return modeCount;
}

