/****************************************************************************************/
/*  THandle.cpp                                                                         */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <DDraw.h>
#include <D3D.h>

#include "THandle.h"
#include "BaseType.h"
#include "D3DDrv.h"
#include "DCommon.h"
#include "D3DCache.h"
#include "D3D_Main.h"
#include "PCache.h"
#include "D3d_FX.h"

#include "TPage.h"

//#define D3D_MANAGE_TEXTURES

//#define USE_ONE_CACHE

#define MAX_TEXTURE_HANDLES					15000

#define TEXTURE_CACHE_PERCENT				0.75f
#define LMAP_CACHE_PERCENT					0.25f

#define TSTAGE_0			0
#define TSTAGE_1			1

//============================================================================================
//============================================================================================

geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];

DDMemMgr			*MemMgr;
DDMemMgr_Partition	*Partition[2];

D3DCache			*TextureCache;
D3DCache			*LMapCache;

TPage_Mgr			*TPageMgr;

DDSURFACEDESC2		CurrentSurfDesc;

THandle_MipData		SystemToVideo[MAX_LMAP_LOG_SIZE];	

geBoolean			CacheNeedsUpdate;
/*
#ifdef D3D_MANAGE_TEXTURES
	#define			NUM_LMAP_VIDEO_SURFACES		10;
	THandle_MipData		SystemToVideo[MAX_LMAP_LOG_SIZE];	
#endif
*/

//============================================================================================
//============================================================================================

//============================================================================================
//	FreeAllcaches
//============================================================================================
void FreeAllCaches(void)
{
	if (LMapCache)
		D3DCache_Destroy(LMapCache);

	if (TextureCache)
		D3DCache_Destroy(TextureCache);

	LMapCache = NULL;
	TextureCache = NULL;

	if (Partition[1])
		DDMemMgr_PartitionDestroy(Partition[1]);
	if (Partition[0])
		DDMemMgr_PartitionDestroy(Partition[0]);
	if (MemMgr)
		DDMemMgr_Destroy(MemMgr);

	
#ifdef USE_TPAGES
	if (TPageMgr)
	{
		TPage_MgrDestroy(&TPageMgr);
		TPageMgr = NULL;
	}
#endif

	Partition[1] = NULL;
	Partition[0] = NULL;
	MemMgr = NULL;
}

//============================================================================================
//	FindTextureHandle
//============================================================================================
geRDriver_THandle *FindTextureHandle(void)
{
	int32				i;
	geRDriver_THandle	*pHandle;

	pHandle = TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, pHandle++)
	{
		if (!pHandle->Active)
		{
			memset(pHandle, 0, sizeof(geRDriver_THandle));
			pHandle->Active = 1;
			return pHandle;
		}
	}

	SetLastDrvError(DRV_ERROR_GENERIC, "D3D_FindTextureHandle:  No more handles left.\n");

	return NULL;
}

//============================================================================================
//	FreeAllTextureHandles
//============================================================================================
geBoolean FreeAllTextureHandles(void)
{
	int32				i;
	geRDriver_THandle	*pHandle;

	pHandle = TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, pHandle++)
	{
		if (!pHandle->Active)
			continue;

		if (!THandle_Destroy(pHandle))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//============================================================================================
//============================================================================================
geBoolean THandle_Startup(void)
{
	// Create the main memory manager
	MemMgr = DDMemMgr_Create(AppInfo.VidMemFree);

	if (!MemMgr)
		goto ExitWithError;

	// Create partition 0
	Partition[0] = DDMemMgr_PartitionCreate(MemMgr, (uint32)((float)DDMemMgr_GetFreeMem(MemMgr)*TEXTURE_CACHE_PERCENT));

	if (!Partition[0])
		goto ExitWithError;

	// Create partition 1
	Partition[1] = DDMemMgr_PartitionCreate(MemMgr, DDMemMgr_GetFreeMem(MemMgr));

	if (!Partition[1])
		goto ExitWithError;

	// Create the texture cache from partition 0
	TextureCache = D3DCache_Create("Main Texture Cache", AppInfo.lpDD, Partition[0], AppInfo.CanDoMultiTexture);

	if (!TextureCache)
		goto ExitWithError;

#ifndef USE_ONE_CACHE
	// Create the lmap cache from partition 1
	LMapCache = D3DCache_Create("Lightmap Cache", AppInfo.lpDD, Partition[1], AppInfo.CanDoMultiTexture);

	if (!LMapCache)
		goto ExitWithError;
#endif

	// Create all the system to video surfaces (for lmaps)
	if (!CreateSystemToVideoSurfaces())
		goto ExitWithError;

	#ifdef USE_TPAGES
		TPageMgr = TPage_MgrCreate(AppInfo.lpDD, &AppInfo.ddTexFormat, 512);
		if (!TPageMgr)
			goto ExitWithError;
	#endif

	return GE_TRUE;

	ExitWithError:
	{
		THandle_Shutdown();
		return GE_FALSE;
	}
}

//============================================================================================
//============================================================================================
void THandle_Shutdown(void)
{
	FreeAllTextureHandles();
	FreeAllCaches();
	DestroySystemToVideoSurfaces();

	CacheNeedsUpdate = GE_FALSE;
}

//============================================================================================
//	Create3DTHandle
//============================================================================================
geRDriver_THandle *Create3DTHandle(geRDriver_THandle *THandle, int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	int32				Size, i;

	assert(NumMipLevels < THANDLE_MAX_MIP_LEVELS);
	assert(NumMipLevels <= 4);

	// Store width/height info
	THandle->Width = Width;
	THandle->Height = Height;
	THandle->NumMipLevels = (uint8)NumMipLevels;
	THandle->Log = (uint8)GetLog(Width, Height);
	THandle->Stride = (1<<THandle->Log);

	// Create the surfaces to hold all the mips
	THandle->MipData = (THandle_MipData*)malloc(sizeof(THandle_MipData)*NumMipLevels);
	memset(THandle->MipData, 0, sizeof(THandle_MipData)*NumMipLevels);
	
	if (!THandle->MipData)
	{
		THandle_Destroy(THandle);
		return NULL;
	}

	Size = 1<<THandle->Log;

	// Create all the surfaces for each mip level
	for (i=0; i< NumMipLevels; i++)
	{
		int32	Stage;

		if (!THandle_CreateSurfaces(&THandle->MipData[i], Size, Size, &CurrentSurfDesc, GE_FALSE, 0))
		{
			THandle_Destroy(THandle);
			return NULL;
		}

		// get a cache type for this surface since it is a 3d surface, and will need to be cached on the video card
		//THandle->MipData[i].CacheType = D3DCache_TypeCreate(TextureCache, Size, Size, NumMipLevels, &CurrentSurfDesc);
		// We can use 1 miplevel for the type, since we are createing types for each miplevel...
		if (AppInfo.CanDoMultiTexture)
			Stage = TSTAGE_0;
		else
			Stage = 0;

		THandle->MipData[i].CacheType = D3DCache_TypeCreate(TextureCache, Size, Size, 1, Stage, &CurrentSurfDesc);

		if (!THandle->MipData[i].CacheType)
		{
			THandle_Destroy(THandle);
			return NULL;
		}

		Size>>=1;
	}

	return THandle;
}

//============================================================================================
//	CreateLightmapTHandle
//============================================================================================
geRDriver_THandle *CreateLightmapTHandle(geRDriver_THandle *THandle, int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	int32				Size, Stage;

	assert(NumMipLevels < THANDLE_MAX_MIP_LEVELS);

	assert(NumMipLevels == 1);
	
	// Save some info about the lightmap
	THandle->Width = Width;
	THandle->Height = Height;
	THandle->NumMipLevels = (uint8)NumMipLevels;
	THandle->Log = (uint8)GetLog(Width, Height);
	THandle->Stride = 1<<THandle->Log;

	assert(THandle->Log < MAX_LMAP_LOG_SIZE);

	Size = 1<<THandle->Log;

	THandle->MipData = (THandle_MipData*)malloc(sizeof(THandle_MipData)*NumMipLevels);
	memset(THandle->MipData, 0, sizeof(THandle_MipData)*NumMipLevels);

	THandle->MipData[0].Flags = THANDLE_UPDATE;

#ifdef D3D_MANAGE_TEXTURES
	#ifndef USE_TPAGES
	{
		int32		Stage;

		if (AppInfo.CanDoMultiTexture)
			Stage = STAGE_1;
		else
			Stage = 0;

		if (!THandle_CreateSurfaces(&THandle->MipData[0], Size, Size, &CurrentSurfDesc, GE_FALSE, Stage))
		{
			THandle_Destroy(THandle);
			return NULL;
		}
	
		D3DSetTexture(0, THandle->MipData[0].Texture);
	}
	#endif
#endif

	if (AppInfo.CanDoMultiTexture)
		Stage = TSTAGE_1;
	else
		Stage = 0;

#ifdef USE_ONE_CACHE
	THandle->MipData[0].CacheType = D3DCache_TypeCreate(TextureCache, Size, Size, NumMipLevels, Stage, &CurrentSurfDesc);
#else
	THandle->MipData[0].CacheType = D3DCache_TypeCreate(LMapCache, Size, Size, NumMipLevels, Stage, &CurrentSurfDesc);
#endif

	if (!THandle->MipData[0].CacheType)
	{
		THandle_Destroy(THandle);
		return NULL;
	}

	return THandle;
}

//============================================================================================
//	Create2DTHandle
//============================================================================================
geRDriver_THandle *Create2DTHandle(geRDriver_THandle *THandle, int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	assert(NumMipLevels < THANDLE_MAX_MIP_LEVELS);
	assert(NumMipLevels == 1);

	// Save some info about the lightmap
	THandle->Width = Width;
	THandle->Height = Height;
	THandle->NumMipLevels = (uint8)NumMipLevels;
	THandle->Log = (uint8)GetLog(Width, Height);
	THandle->Stride = Width;

	// Create the surfaces to hold all the mips
	THandle->MipData = (THandle_MipData*)malloc(sizeof(THandle_MipData)*NumMipLevels);
	memset(THandle->MipData, 0, sizeof(THandle_MipData)*NumMipLevels);
	
	if (!THandle->MipData)
	{
		THandle_Destroy(THandle);
		return NULL;
	}
	
	if (!THandle_CreateSurfaces(&THandle->MipData[0], Width, Height, &CurrentSurfDesc, GE_TRUE, 0))
	{
		THandle_Destroy(THandle);
		return NULL;
	}

	return THandle;
}

//============================================================================================
//	SetupCurrent3dDesc
//============================================================================================
geBoolean SetupCurrent3dDesc(gePixelFormat PixelFormat)
{
	switch (PixelFormat)
	{
		case GE_PIXELFORMAT_16BIT_555_RGB:
		case GE_PIXELFORMAT_16BIT_565_RGB:
		{
			memcpy(&CurrentSurfDesc, &AppInfo.ddTexFormat, sizeof(DDSURFACEDESC2));
			break;
		}
		case GE_PIXELFORMAT_16BIT_4444_ARGB:
		{
			memcpy(&CurrentSurfDesc, &AppInfo.ddFourBitAlphaSurfFormat, sizeof(DDSURFACEDESC2));
			break;
		}
		case GE_PIXELFORMAT_16BIT_1555_ARGB:
		{
			memcpy(&CurrentSurfDesc, &AppInfo.ddOneBitAlphaSurfFormat, sizeof(DDSURFACEDESC2));
			break;
		}

		default:
		{
			SetLastDrvError(DRV_ERROR_GENERIC, "SetupCurrent3dDesc:  Invalid pixel format.\n");
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}
//============================================================================================
//	THandle_Create
//============================================================================================
geRDriver_THandle *DRIVERCC THandle_Create(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	geRDriver_THandle	*THandle;

	THandle = FindTextureHandle();

	if (!THandle)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "D3DDRV:THandle_Create:  Out of texture handles.\n");
		return NULL;
	}

	THandle->PixelFormat = *PixelFormat;

	if (PixelFormat->Flags & RDRIVER_PF_3D)
	{
		// Get the pixel format desc for this thandle
		if (!SetupCurrent3dDesc(PixelFormat->PixelFormat))
			return NULL;

		if (!Create3DTHandle(THandle, Width, Height, NumMipLevels, PixelFormat))
			return NULL;

		CacheNeedsUpdate = GE_TRUE;
	}
	else if (PixelFormat->Flags & RDRIVER_PF_LIGHTMAP)
	{
		// Get the pixel format desc for this thandle
		if (!SetupCurrent3dDesc(PixelFormat->PixelFormat))
			return NULL;

		if (!CreateLightmapTHandle(THandle, Width, Height, NumMipLevels, PixelFormat))
			return NULL;

		CacheNeedsUpdate = GE_TRUE;
	}
	else if (PixelFormat->Flags & RDRIVER_PF_2D)
	{
		// 2d surfaces are always this format for now
		memcpy(&CurrentSurfDesc, &AppInfo.ddSurfFormat, sizeof(DDSURFACEDESC2));

		if (!Create2DTHandle(THandle, Width, Height, NumMipLevels, PixelFormat))
			return NULL;
	}

	return THandle;
}

//============================================================================================
//	THandle_Destroy
//============================================================================================
geBoolean DRIVERCC THandle_Destroy(geRDriver_THandle *THandle)
{
	int32		i;

	assert(THandle);
	assert(THandle->Active);

	for (i=0; i< THandle->NumMipLevels; i++)
	{
		assert(THandle->MipData);

		if (THandle->MipData[i].CacheType)
		{
			D3DCache_TypeDestroy(THandle->MipData[i].CacheType);
			CacheNeedsUpdate = GE_TRUE;
			THandle->MipData[i].CacheType = NULL;
		}
		
		if (THandle->MipData[i].Surface)
		{
			assert(THandle->MipData[i].Texture);

			THandle_DestroySurfaces(&THandle->MipData[i]);
			THandle->MipData[i].Surface = NULL;
			THandle->MipData[i].Texture = NULL;
		}
	}

	if (THandle->MipData)
		free(THandle->MipData);

	memset(THandle, 0, sizeof(geRDriver_THandle));

	return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
geBoolean DRIVERCC THandle_Lock(geRDriver_THandle *THandle, int32 MipLevel, void **Bits)
{
    DDSURFACEDESC2		SurfDesc;
    HRESULT				Result;

    assert(!(THandle->MipData[MipLevel].Flags & THANDLE_LOCKED));

	// Lock the surface so it can be filled with the data
    memset(&SurfDesc, 0, sizeof(DDSURFACEDESC2));
    SurfDesc.dwSize = sizeof(DDSURFACEDESC2);

    Result = THandle->MipData[MipLevel].Surface->Lock(NULL, &SurfDesc, DDLOCK_WAIT, NULL);

    if (Result != DD_OK) 
	{
        return GE_FALSE;
    }

	THandle->MipData[MipLevel].Flags |= THANDLE_LOCKED;

	*Bits = (void*)SurfDesc.lpSurface;

	return GE_TRUE;
}

//=====================================================================================
//	THandle_UnLock
//=====================================================================================
geBoolean DRIVERCC THandle_UnLock(geRDriver_THandle *THandle, int32 MipLevel)
{
    HRESULT				Result;

    assert(MipLevel <= THandle->NumMipLevels);
    assert(THandle->MipData[MipLevel].Flags & THANDLE_LOCKED);

	// Unlock the surface
    Result = THandle->MipData[MipLevel].Surface->Unlock(NULL);

    if (Result != DD_OK) 
	{
        return GE_FALSE;
    }

	THandle->MipData[MipLevel].Flags |= THANDLE_UPDATE;
	THandle->MipData[MipLevel].Flags &= ~THANDLE_LOCKED;

	return GE_TRUE;
}

#ifndef NDEBUG
#define DebugIf(a, b) if (a) b
#else
#define DebugIf(a, b)
#endif

//=====================================================================================
//	THandle_GetInfo
//=====================================================================================
geBoolean DRIVERCC THandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info)
{
	DebugIf (MipLevel > THandle->Log, return GE_FALSE);

	Info->Width = THandle->Width>>MipLevel;
	Info->Height = THandle->Height>>MipLevel;
	Info->Stride = THandle->Stride>>MipLevel;

	if (THandle->PixelFormat.Flags & RDRIVER_PF_CAN_DO_COLORKEY)
	{
		Info->Flags = RDRIVER_THANDLE_HAS_COLORKEY;
		Info->ColorKey = 1;
	}
	else
	{
		Info->Flags = 0;
		Info->ColorKey = 0;
	}

	Info->PixelFormat = THandle->PixelFormat;

	return GE_TRUE;
}

//=====================================================================================
//	CreateSystemToVideoSurfaces
//	System surfaces to copy from system to video
//=====================================================================================
geBoolean CreateSystemToVideoSurfaces(void)
{
	int32				i;
	DDSURFACEDESC2		SurfDesc;

	memcpy(&SurfDesc, &AppInfo.ddTexFormat, sizeof(DDSURFACEDESC2));

	for (i=0; i<MAX_LMAP_LOG_SIZE; i++)
	{
		int32		Size;

		Size = 1<<i;
		
		if (!THandle_CreateSurfaces(&SystemToVideo[i], Size, Size, &SurfDesc, GE_FALSE, 1))
		{
			DestroySystemToVideoSurfaces();
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//=====================================================================================
//	DestroySystemToVideoSurfaces
//=====================================================================================
void DestroySystemToVideoSurfaces(void)
{
	int32				i;

	for (i=0; i<MAX_LMAP_LOG_SIZE; i++)
		THandle_DestroySurfaces(&SystemToVideo[i]);
}

//=====================================================================================
//	THandle_CreateSurfaces
//=====================================================================================
geBoolean THandle_CreateSurfaces(THandle_MipData *Surf, int32 Width, int32 Height, DDSURFACEDESC2 *SurfDesc, geBoolean ColorKey, int32 Stage)
{
	LPDIRECTDRAWSURFACE4 Surface;
	DDSURFACEDESC2		ddsd;
	HRESULT				Hr;
			
	memcpy(&ddsd, SurfDesc, sizeof(DDSURFACEDESC2));

	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;

#ifdef D3D_MANAGE_TEXTURES
	ColorKey = GE_TRUE;			// Force a colorkey on system surfaces since we are letting D3D do our cacheing...

	ddsd.ddsCaps.dwCaps = 0;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_HINTDYNAMIC;
#else
	ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
	ddsd.ddsCaps.dwCaps2 = 0;//DDSCAPS2_HINTDYNAMIC;
#endif

	ddsd.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;
	ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_HINTDYNAMIC;

	ddsd.ddsCaps.dwCaps3 = 0;
	ddsd.ddsCaps.dwCaps4 = 0;
	ddsd.dwWidth = Width;
	ddsd.dwHeight = Height;

	ddsd.dwTextureStage = Stage;
			
	Hr = AppInfo.lpDD->CreateSurface(&ddsd, &Surface, NULL);

	if(Hr != DD_OK) 
	{ 
		return FALSE;
	}

	Surf->Surface = Surface;
	
	Surf->Texture = NULL;

	Hr = Surface->QueryInterface(IID_IDirect3DTexture2, (void**)&Surf->Texture);  

	if(Hr != DD_OK) 
	{ 
		Surface->Release();
		return GE_FALSE;
	}

	if (ColorKey)
	{
		DDCOLORKEY			CKey;
		
		// Create the color key for this surface
		CKey.dwColorSpaceLowValue = 1;
		CKey.dwColorSpaceHighValue = 1;
		
		if (Surf->Surface->SetColorKey(DDCKEY_SRCBLT , &CKey) != DD_OK)
		{
			SetLastDrvError(DRV_ERROR_GENERIC, "THandle_CreateSurfaces: SetColorKey failed for texture.");
			Surf->Surface->Release();
			Surf->Surface = NULL;
			Surf->Texture->Release();
			Surf->Texture = NULL;
			return FALSE;
		}
	}
	return GE_TRUE;		// All good dude
}

//=====================================================================================
//	DestroySystemSurface
//=====================================================================================
void THandle_DestroySurfaces(THandle_MipData *Surf)
{
	if (Surf->Texture)
		Surf->Texture->Release();
	if (Surf->Surface)
		Surf->Surface->Release();

	memset(Surf, 0, sizeof (THandle_MipData));
}

//=====================================================================================
//	THandle_CheckCache
//=====================================================================================
geBoolean THandle_CheckCache(void)
{
	geRDriver_THandle	*pTHandle;
	int32				i, Stage0, Stage1;
	int32				MaxTable1[9], MaxTable2[9];

	if (!CacheNeedsUpdate)
		return GE_TRUE;

#ifndef D3D_MANAGE_TEXTURES		// If D3D is managing textures, then we don't ned to do any of this...
	D3DMain_Log("THandle_CheckCache:  Resetting texture cache...\n");

#ifdef USE_ONE_CACHE
		#pragma message ("There numbers ARE NOT DONE.  So if USE_ONE_CACHE is defined, please finish this...")
		// Texture cache & Lightmap cache (we are only using one cache, so the combine into the TextureCache)
		MaxTable1[0] = 256;			//  1x1
		MaxTable1[1] = 256;			//  2x2
		MaxTable1[2] = 256;			//  4x4
		MaxTable1[3] = 512;			//  8x8
		MaxTable1[4] = 512;			// 16x16
		MaxTable1[5] = 512;			// 32x32
		MaxTable1[6] = 512;			// 64x64
		MaxTable1[7] = 256;			//128x128
		MaxTable1[8] = 256;			//256x256
#else
	if (AppInfo.DeviceIdentifier.dwVendorId == 4634)		// 3dfx series have a limit on the number of texture handles
	{
		D3DMain_Log("   3dfx card detected, using smaller number of handles...\n");

		// Texture cache
		MaxTable1[0] = 24;			//  1x1
		MaxTable1[1] = 24;			//  2x2
		MaxTable1[2] = 24;			//  4x4
		MaxTable1[3] = 24;			//  8x8
		MaxTable1[4] = 24;			// 16x16
		MaxTable1[5] = 128;			// 32x32
		MaxTable1[6] = 128;			// 64x64
		MaxTable1[7] = 128;			//128x128
		MaxTable1[8] = 128;			//256x256

		// Lightmap cache
		MaxTable2[0] = 128;			//  1x1
		MaxTable2[1] = 128;			//  2x2
		MaxTable2[2] = 256;			//  4x4
		MaxTable2[3] = 256;			//  8x8
		MaxTable2[4] = 256;			// 16x16
		MaxTable2[5] = 128;			// 32x32
		MaxTable2[6] = 128;			// 64x64
		MaxTable2[7] = 128;			//128x128
		MaxTable2[8] = 128;			//256x256
	}
	else
	{
		D3DMain_Log("   NO 3dfx card detected, using larger number of handles...\n");

		// Texture cache
		MaxTable1[0] = 32;			//  1x1
		MaxTable1[1] = 32;			//  2x2
		MaxTable1[2] = 32;			//  4x4
		MaxTable1[3] = 32;			//  8x8
		MaxTable1[4] = 32;			// 16x16
		MaxTable1[5] = 32;			// 32x32
		MaxTable1[6] = 128;			// 64x64
		MaxTable1[7] = 128;			//128x128
		MaxTable1[8] = 128;			//256x256

		MaxTable2[0] = 128;			//  1x1
		MaxTable2[1] = 128;			//  2x2
		MaxTable2[2] = 256;			//  4x4
		MaxTable2[3] = 1024;		//  8x8
		MaxTable2[4] = 1024;		// 16x16
		MaxTable2[5] = 512;			// 32x32
		MaxTable2[6] = 256;			// 64x64
		MaxTable2[7] = 256;			//128x128
		MaxTable2[8] = 256;			//256x256
	}
#endif

	if (AppInfo.CanDoMultiTexture)
	{
		Stage0 = TSTAGE_0;
		Stage1 = TSTAGE_1;
	}
	else
	{
		Stage0 = 0;							   
		Stage1 = 0;
	}

	#ifndef USE_ONE_CACHE
		if (!D3DCache_AdjustSlots(LMapCache, MaxTable2, GE_TRUE))
		{
			D3DMain_Log("THandle_CheckCache:  D3DCache_AdjustSlots failed for LMapCache.\n");
			return GE_FALSE;
		}
	#endif

	if (!D3DCache_AdjustSlots(TextureCache, MaxTable1, GE_FALSE))
	{
		D3DMain_Log("THandle_CheckCache:  D3DCache_AdjustSlots failed for TextureCache.\n");
		return GE_FALSE;
	}

#endif

	// Make sure no THandles reference any slots, because they mave have been moved around, or gotten destroyed...
	//  (Evict all textures in the cache)
	pTHandle = TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, pTHandle++)
	{
		int32		m;

		for (m=0; m< pTHandle->NumMipLevels; m++)
		{
			pTHandle->MipData[m].Slot = NULL;
		}
	}

	CacheNeedsUpdate = GE_FALSE;

	return GE_TRUE;
}
