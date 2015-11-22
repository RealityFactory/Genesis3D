/****************************************************************************************/
/*  OglDrv.h                                                                            */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Exposed interface for OpenGL version of Genesis Driver                 */
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

#ifndef OGLDRV_H
#define OGLDRV_H

#include "DCommon.h"
#include "glext.h"

// Here are some useful values you can change to meet your needs.
// These should, in the future, really be handled at run-time as opposed to compile-time.
// The defaults work well with TNT2 based cards.
#define COLOR_DEPTH			16		// Bits per pixel to use for OpenGL Window/Context
#define ZBUFFER_DEPTH		16		// Depth of the ZBuffer to use in OpenGL.   

#define USE_LIGHTMAPS					// Render lightmaps
#define USE_LINEAR_INTERPOLATION		// Comment out to use nearest neighbor interpolation
//#define TRILINEAR_INTERPOLATION		// Comment out to use bilinear interpolation


extern		DRV_Driver	OGLDRV;
extern		DRV_Window	ClientWindow;

extern		PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern      PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;
extern		GLboolean multitexture;

extern		maxTextureSize;

geBoolean DRIVERCC SetFogEnable(geBoolean Enable, float r, float g, float b, float Start, float End);
geBoolean DRIVERCC DrvInit(DRV_DriverHook *Hook);
geBoolean DRIVERCC DrvShutdown(void);
geBoolean DRIVERCC DrvUpdateWindow(void);
geBoolean DRIVERCC DrvSetActive(geBoolean Active);
geBoolean DRIVERCC SetGamma(float Gamma);
geBoolean DRIVERCC GetGamma(float *Gamma);
geBoolean DRIVERCC ScreenShot(const char *Name);
geBoolean DRIVERCC EnumSubDrivers(DRV_ENUM_DRV_CB *Cb, void *Context);
geBoolean DRIVERCC EnumModes(int32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, void *Context);
geBoolean DRIVERCC EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context);
void SetLastDrvError(int32 Error, char *ErrorStr);

#endif