/****************************************************************************************/
/*  THandle.h                                                                           */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Texture handle manager for OpenGL driver                               */
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

#ifndef THANDLE_H
#define THANDLE_H

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#include "DCommon.h"
#include "OglMisc.h"

#define	MAX_TEXTURE_HANDLES			20000
#define THANDLE_MAX_MIP_LEVELS		16

// THandle flags
#define THANDLE_UPDATE		(1<<0)		// Force a thandle to be uploaded to the card
#define	THANDLE_TRANS		(1<<2)		// Texture has transparency
#define THANDLE_LOCKED		(1<<3)		// THandle is currently locked (invalid for rendering etc)
#define THANDLE_UPDATE_LM	(1<<4)		// THandle is a lightmap that needs updating

typedef struct geRDriver_THandle
{
	GLboolean				Active;
	GLint					Width, Height, MipLevels;
	GLint					PaddedWidth, PaddedHeight;
	geRDriver_PixelFormat	PixelFormat;
	GLuint					Flags;
	GLuint					TextureID;
	GLubyte					*Data[THANDLE_MAX_MIP_LEVELS];
	GLfloat					InvScale;
} geRDriver_THandle;

extern	geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];

geBoolean						FreeAllTextureHandles(void);
geBoolean			DRIVERCC	DrvResetAll(void);
geRDriver_THandle	*DRIVERCC	THandle_Create(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geBoolean			DRIVERCC	THandle_Destroy(geRDriver_THandle *THandle);
geBoolean			DRIVERCC	THandle_Lock(geRDriver_THandle *THandle, int32 MipLevel, void **Data);
geBoolean			DRIVERCC	THandle_UnLock(geRDriver_THandle *THandle, int32 MipLevel);
geBoolean			DRIVERCC	THandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info);
void							THandle_Update(geRDriver_THandle *THandle);
void							THandle_DownloadLightmap(DRV_LInfo *LInfo);

int32 GetLog(int32 Width, int32 Height);
uint32 Log2(uint32 P2);
S32 SnapToPower2(S32 Width);

geBoolean THandle_Startup(void);

#endif
