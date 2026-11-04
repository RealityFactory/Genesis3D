/****************************************************************************************/
/*  GLX.h                                                                               */
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

#ifndef __GLX_H__
#define __GLX_H__

#include <GL/gl.h>
#include <X11/Xlib.h>
#include "DCommon.h"

typedef struct
{
    Display * display;
    Window win;
} glxWnd;

struct
{
	GLuint width;
	GLuint height;
	GLuint refresh;
} typedef MODELIST;


void WindowSetup(DRV_DriverHook *Hook);
void SetGLPixelFormat(DRV_DriverHook *Hook);
void WindowCleanup();
void FlipGLBuffers();
geBoolean SetFullscreen(DRV_DriverHook *Hook);
GLint EnumNativeModes(DRV_ENUM_MODES_CB *Cb, void *Context);

#endif
