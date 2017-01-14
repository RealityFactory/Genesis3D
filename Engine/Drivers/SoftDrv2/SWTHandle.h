/****************************************************************************************/
/*  SWTHandle.H                                                                         */
/*                                                                                      */
/*  Author: Mike Sandige, John Pollard                                                  */
/*  Description:  Manager for texture construction and available texture formats for    */
/*                the software driver                                                   */
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
#ifndef SWTHANDLE_H
#define SWTHANDLE_H

#include "DCommon.h"
#include "PixelFormat.h"

// THandle flags
#define THANDLE_UPDATE		(1<<0)		// Force a thandle to be uploaded to the card
#define	THANDLE_TRANS		(1<<2)		// Texture has transparency
#define THANDLE_LOCKED		(1<<3)		// THandle is currently locked (invalid for rendering etc)

typedef struct geRDriver_THandle
{
	int32					Active, Width, Height, MipLevels;
	geRDriver_PixelFormat	PixelFormat;
	uint16					*BitPtr[16];//8 or 16
	geRDriver_THandle		*PalHandle;
	geRDriver_THandle		*AlphaHandle;

	uint32					Flags;
} geRDriver_THandle;

geBoolean			DRIVERCC	SWTHandle_EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context);
geBoolean						SWTHandle_FreeAllTextureHandles(void);
geRDriver_THandle	*DRIVERCC	SWTHandle_CreateTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geBoolean			DRIVERCC	SWTHandle_DestroyTexture(geRDriver_THandle *THandle);

geBoolean			DRIVERCC	SWTHandle_LockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel, void **Data);
geBoolean			DRIVERCC	SWTHandle_UnLockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel);
geBoolean			DRIVERCC	SWTHandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info);

geBoolean			DRIVERCC	SWTHandle_SetPalette(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle);
geRDriver_THandle	*DRIVERCC	SWTHandle_GetPalette(geRDriver_THandle *THandle);
geBoolean			DRIVERCC	SWTHandle_SetAlpha(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle);
geRDriver_THandle	*DRIVERCC	SWTHandle_GetAlpha(geRDriver_THandle *THandle);

#endif