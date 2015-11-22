/****************************************************************************************/
/*  THandle.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: THandle manager for D3DDrv                                             */
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

#include "BaseType.h"
#include "DCommon.h"
#include "D3DCache.h"

#include "TPage.h"

//============================================================================================
//============================================================================================
#define THANDLE_MAX_MIP_LEVELS		255
//#define	MAX_LMAP_LOG_SIZE			8			// Max lightmap size in pixels will be 128x128
//#define	MAX_LMAP_LOG_SIZE			7			// Max lightmap size in pixels will be 64x64
#define	MAX_LMAP_LOG_SIZE			6			// Max lightmap size in pixels will be 32x32

typedef struct
{
	LPDIRECTDRAWSURFACE4	Surface;			// The DD surface
	D3DCache_Type			*CacheType;
	D3DCache_Slot			*Slot;
	
	LPDIRECT3DTEXTURE2		Texture;			// The texture interface to the surface

	uint8					Flags;
} THandle_MipData;

// THandle flags
#define THANDLE_LOCKED					(1<<0)
#define THANDLE_UPDATE					(1<<1)

typedef struct geRDriver_THandle
{
	uint8					Active;
	int32					Width;
	int32					Height;
	int32					Stride;
	uint8					NumMipLevels;
	uint8					Log;

	THandle_MipData			*MipData;				// A mipdata per miplevel

	geRDriver_PixelFormat	PixelFormat;

#ifdef USE_TPAGES
	TPage_Block				*Block;
#endif

} geRDriver_THandle;

extern geRDriver_THandle	TextureHandles[];

extern D3DCache				*TextureCache;
extern D3DCache				*LMapCache;

extern TPage_Mgr			*TPageMgr;

extern THandle_MipData		SystemToVideo[];

extern geBoolean CacheNeedsUpdate;

//============================================================================================
//============================================================================================
void FreeAllCaches(void);
geRDriver_THandle *FindTextureHandle(void);
geBoolean FreeAllTextureHandles(void);
geBoolean THandle_Startup(void);
void THandle_Shutdown(void);
geRDriver_THandle *Create3DTHandle(geRDriver_THandle *THandle, int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geRDriver_THandle *CreateLightmapTHandle(geRDriver_THandle *THandle, int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geRDriver_THandle *Create2DTHandle(geRDriver_THandle *THandle, int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geRDriver_THandle *DRIVERCC THandle_Create(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
geBoolean DRIVERCC THandle_Destroy(geRDriver_THandle *THandle);
geBoolean DRIVERCC THandle_Lock(geRDriver_THandle *THandle, int32 MipLevel, void **Bits);
geBoolean DRIVERCC THandle_UnLock(geRDriver_THandle *THandle, int32 MipLevel);
geBoolean DRIVERCC THandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info);
geBoolean CreateSystemToVideoSurfaces(void);
void DestroySystemToVideoSurfaces(void);
geBoolean THandle_CreateSurfaces(THandle_MipData *MipData, int32 Width, int32 Height, DDSURFACEDESC2 *SurfDesc, geBoolean ColorKey, int32 Stage);
void THandle_DestroySurfaces(THandle_MipData *MipData);
geBoolean THandle_CheckCache(void);

#endif
