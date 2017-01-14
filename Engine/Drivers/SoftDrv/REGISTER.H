/****************************************************************************************/
/*  register.h                                                                          */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  Texture Handle code header file                                       */
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
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef REGISTER_H
#define REGISTER_H

#include <Windows.h>

#include "ddraw.h"

#include "DCommon.h"

#define	MAX_TEXTURE_HANDLES		15000

// THandle flags
#define THANDLE_UPDATE		(1<<0)		// Force a thandle to be uploaded to the card
#define	THANDLE_TRANS		(1<<2)		// Texture has transparency
#define THANDLE_LOCKED		(1<<3)		// THandle is currently locked (invalid for rendering etc)

typedef struct geRDriver_THandle
{
	int32						Active, Width, Height, MipLevels;
	geRDriver_PixelFormat	PixelFormat;
	uint16						*BitPtr[16];//8 or 16
	geRDriver_THandle		*PalHandle;
	geRDriver_THandle		*AlphaHandle;

	uint32						Flags;
} geRDriver_THandle;

extern	geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];

geBoolean			DRIVERCC	DrvResetAll(void);
geRDriver_THandle	*DRIVERCC	CreateTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geBoolean			DRIVERCC	DestroyTexture(geRDriver_THandle *THandle);

geBoolean			DRIVERCC	LockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel, void **Data);
geBoolean			DRIVERCC	UnLockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel);
geBoolean			DRIVERCC	THandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info);

geBoolean			DRIVERCC	SetPalette(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle);
geRDriver_THandle	*DRIVERCC	GetPalette(geRDriver_THandle *THandle);
geBoolean			DRIVERCC	SetAlpha(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle);
geRDriver_THandle	*DRIVERCC	GetAlpha(geRDriver_THandle *THandle);

#endif