/****************************************************************************************/
/*  GTHandle.h                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: THandle manager for glide                                              */
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
#ifndef THANDLE_H
#define THANDLE_H

#include <Windows.h>

#include "DCommon.h"
#include "BaseType.h"
#include "GCache.h"
#include "Glide.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	MAX_TEXTURE_HANDLES		20000

// THandle flags
#define THANDLE_UPDATE		(1<<0)		// Force a thandle to be uploaded to the card
#define THANDLE_LOCKED		(1<<1)		// THandle is currently locked (invalid for rendering etc)

#define THANDLE_PALETTE_FORMAT	(GE_PIXELFORMAT_32BIT_XRGB)

//============================================================================================
//============================================================================================
typedef struct
{
	int32					RefCount;
	int32					Width;
	int32					Height;
	int32					LogSize;
	uint8					NumMipLevels;
	geRDriver_PixelFormat	PixelFormat;

	uint8					Log;
	float					OneOverLogSize_255;
} THandle_Info;

typedef struct geRDriver_THandle
{
	uint8					Active;
	struct geRDriver_THandle	*PalHandle;
	int32					Width;				// Original width/height
	int32					Height;
	int32					LogSize;			// Square width/height in cache
	uint8					NumMipLevels;
	float					OneOverLogSize_255;
	uint8					Log;
	geRDriver_PixelFormat	PixelFormat;
	//GrTexInfo				Info;
	//uint16				InfoIndex;			// Use this ASAP!!!

	void					*Data;				// Actual data bits of LogSize*LogSize

	GCache_Type				*CacheType;
	GCache_Slot				*Slot;				// Current slot this handle is being textured with

	uint8					Flags;			
} geRDriver_THandle;

extern geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];

// Memory managers
extern GMemMgr				*MemMgr[2];

// Texture caches
extern GCache				*TextureCache;			// Texture cache
extern GCache				*LMapCache;				// Lightmap texture cache

extern geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];		// Contain Texture/Decal/Lightmap handles

extern geBoolean			TexturesChanged;
extern geBoolean			LMapsChanged;

//============================================================================================
//============================================================================================
geBoolean GTHandle_Startup(void);
void GTHandle_Shutdown(void);
void GTHandle_FreeAllCaches(void);
geRDriver_THandle *GTHandle_FindTextureHandle();
void GTHandle_FreeTextureHandle(geRDriver_THandle *THandle);
void GTHandle_FreeAllTextureHandles(void);
geBoolean GTHandle_SetupInfo(GrTexInfo *Info, int32 Width, int32 Height, int32 NumMipLevels, GrTextureFormat_t Format, int32 *Size);
geBoolean GlideFormatFromGenesisFormat(gePixelFormat Format, GrTextureFormat_t *Out);
geRDriver_THandle *Create3DTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geRDriver_THandle *CreateLightmapTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geRDriver_THandle *Create2DTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geRDriver_THandle *DRIVERCC GTHandle_Create(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geBoolean DRIVERCC GTHandle_Destroy(geRDriver_THandle *THandle);
geBoolean DRIVERCC GTHandle_Lock(geRDriver_THandle *THandle, int32 MipLevel, void **Data);
geBoolean DRIVERCC GTHandle_UnLock(geRDriver_THandle *THandle, int32 MipLevel);
geBoolean DRIVERCC GThandle_SetPal(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle);
geRDriver_THandle *DRIVERCC GThandle_GetPal(geRDriver_THandle *THandle);
geBoolean DRIVERCC GTHandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info);
geBoolean GTHandle_CheckTextures(void);
geBoolean GetLod(S32 Width, GrLOD_t *Lod);
geBoolean GetAspectRatio(int32 Width, int32 Height, GrAspectRatio_t *Aspect);
uint32 GetLog(uint32 P2);
int32 SnapToPower2(int32 Width);

#ifdef __cplusplus
}
#endif

#endif