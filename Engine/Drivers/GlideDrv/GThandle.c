/****************************************************************************************/
/*  GTHandle.c                                                                          */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <Assert.h>

#include "GTHandle.h"
#include "GCache.h"
#include "GMain.h"
#include "GlideDrv.h"


#define TEXTURE_CACHE_PERCENT			0.75f
#define LMAP_CACHE_PERCENT				0.25f

// Memory managers
GMemMgr				*MemMgr[2];

// Texture caches
GCache				*TextureCache;			// Texture cache
GCache				*LMapCache;				// Lightmap texture cache

geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];		// Contain Texture/Decal/Lightmap handles

geBoolean			TexturesChanged;
geBoolean			LMapsChanged;

//==================================================================================
//	GTHandle_Startup
//==================================================================================
geBoolean GTHandle_Startup(void)
{
	if (g_BoardInfo.NumTMU >= 2)
	{
		TMU[0] = GR_TMU0;
		TMU[1] = GR_TMU1;

		MemMgr[0] = GMemMgr_Create(GR_TMU0, grTexMinAddress(GR_TMU0), grTexMaxAddress(GR_TMU0));
		if (!MemMgr[0])
			goto ExitWithError;
		MemMgr[1] = GMemMgr_Create(GR_TMU1, grTexMinAddress(GR_TMU1), grTexMaxAddress(GR_TMU1));
		if (!MemMgr[1])
			goto ExitWithError;

	}
	else
	{
		uint32	MinAddress, MaxAddress, MidAddress;

		TMU[0] = GR_TMU0;
		TMU[1] = GR_TMU0;

		MinAddress = grTexMinAddress(GR_TMU0);
		MaxAddress = grTexMaxAddress(GR_TMU0);
		MidAddress = MinAddress + (uint32)((float)(MaxAddress - MinAddress)*TEXTURE_CACHE_PERCENT);

		MemMgr[0] = GMemMgr_Create(GR_TMU0, MinAddress, MidAddress);
		if (!MemMgr[0])
			goto ExitWithError;
		MemMgr[1] = GMemMgr_Create(GR_TMU0, MidAddress+1, MaxAddress);
		if (!MemMgr[1])
			goto ExitWithError;
	}

	TextureCache = GCache_Create("Texture Cache", MemMgr[0]);

	if (!TextureCache)
		goto ExitWithError;

	LMapCache = GCache_Create("Lightmap Cache", MemMgr[1]);

	if (!LMapCache)
		goto ExitWithError;

	return GE_TRUE;

	ExitWithError:
	{
		GTHandle_FreeAllTextureHandles();
		GTHandle_FreeAllCaches();

		return GE_FALSE;
	}
}

//==================================================================================
//	GTHandle_Shutdown
//==================================================================================
void GTHandle_Shutdown(void)
{
	// Free all the texture handles first!!!
	GTHandle_FreeAllTextureHandles();
	// Then free all the caches
	GTHandle_FreeAllCaches();

	TexturesChanged = GE_FALSE;
	LMapsChanged = GE_FALSE;
}

//==================================================================================
//==================================================================================
void GTHandle_FreeAllCaches(void)
{
	if (LMapCache)
		GCache_Destroy(LMapCache);
	if (TextureCache)
		GCache_Destroy(TextureCache);

	TextureCache = NULL;
	LMapCache = NULL;

	if (MemMgr[1])
		GMemMgr_Destroy(MemMgr[1]);
	if (MemMgr[0])
		GMemMgr_Destroy(MemMgr[0]);

	MemMgr[0] = NULL;
	MemMgr[1] = NULL;
}

//========================================================================================
//	FindTextureHandle
//========================================================================================
geRDriver_THandle *GTHandle_FindTextureHandle()
{
	int32				i;
	geRDriver_THandle	*THandle;

	THandle = TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, THandle++)
	{
		if (!THandle->Active)
		{
			assert(THandle->Data == NULL);

			memset(THandle, 0, sizeof(geRDriver_THandle));

			THandle->Active = GE_TRUE;

			return THandle;
		}
	}

	return NULL;
}

//========================================================================================
//	GTHandle_FreeTextureHandle
//========================================================================================
void GTHandle_FreeTextureHandle(geRDriver_THandle *THandle)
{
	assert(THandle);
	assert(THandle->Active == GE_TRUE);

	if (THandle->Data)
		free(THandle->Data);

	if (THandle->CacheType)
	{
		GCache_TypeDestroy(THandle->CacheType);
		TexturesChanged = GE_TRUE;
	}

	memset(THandle, 0, sizeof(geRDriver_THandle));	
}

//==================================================================================
//	GTHandle_FreeAllTextureHandles
//==================================================================================
void GTHandle_FreeAllTextureHandles(void)
{
	int32				i;
	geRDriver_THandle	*pTHandle;

	pTHandle = TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, pTHandle++)
	{
		if (!pTHandle->Active)
		{
			assert(pTHandle->Data == NULL);
			continue;
		}
		
		GTHandle_FreeTextureHandle(pTHandle);
	}
}

//========================================================================================
//	GTHandle_SetupInfo
//========================================================================================
geBoolean GTHandle_SetupInfo(GrTexInfo *Info, int32 Width, int32 Height, int32 NumMipLevels, GrTextureFormat_t Format, int32 *Size)
{
	GrLOD_t			LLod, SLod;
	GrAspectRatio_t Ratio;
	int32			SquareSize;

	SquareSize = max(Width, Height);
	SquareSize = SnapToPower2(SquareSize);

	if (!GetLod(SquareSize, &LLod))
		return GE_FALSE;
		
	if (!GetLod((SquareSize>>(NumMipLevels-1)), &SLod))
		return GE_FALSE;

#if 1
	Ratio = GR_ASPECT_1x1;
#else
	if (!GetAspectRatio(SquareSize, SquareSize, &Ratio))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_SetupInfo: Invalid aspect ratio.");
		return GE_FALSE;
	}
#endif

	*Size = SquareSize;

	Info->largeLod		= LLod;
	Info->smallLod		= SLod;
	Info->aspectRatio	= Ratio;
	Info->format		= Format;

	return GE_TRUE;
}

//============================================================================================
//	GlideFormatFromGenesisFormat
//============================================================================================
geBoolean GlideFormatFromGenesisFormat(gePixelFormat Format, GrTextureFormat_t *Out)
{
 	switch(Format)
	{
		case GE_PIXELFORMAT_16BIT_565_RGB:
		{
			*Out = GR_TEXFMT_RGB_565;
			break;
		}
		case GE_PIXELFORMAT_16BIT_4444_ARGB:
		{
			*Out = GR_TEXFMT_ARGB_4444;
			break;
		
		}
		case GE_PIXELFORMAT_16BIT_1555_ARGB:
		{
			*Out = GR_TEXFMT_ARGB_1555;
			break;
		
		}
		case GE_PIXELFORMAT_8BIT:
		{
			#ifdef ALPHA_PALETTE
				*Out = GR_TEXFMT_AP_88;
			#else
				*Out = GR_TEXFMT_P_8;
			#endif
			break;
		}
		
		default:
		{
			assert(!"GlideFormatFromGenesisFormat:  Invalid Pixel format.");
			*Out = GE_PIXELFORMAT_8BIT;	
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//========================================================================================
//	GTHandle_Create3DTexture
//	Creates a 3d texture surface
//========================================================================================
geRDriver_THandle *Create3DTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	int32				SWidth, SHeight;			// Snapped to power of 2 width/height
	GrTexInfo			Info;
	int32				DataSize;
	uint8				*Data;
	geRDriver_THandle	*THandle;
	GrTextureFormat_t	Format;

	assert(PixelFormat);
	
	Data = NULL;

	THandle = GTHandle_FindTextureHandle();

	if (!THandle)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_Create3DTexture: No more handles left.");
		goto ExitWithError;
	}

	if (Width > 256)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_Create3DTexture: Width > 256.");
		goto ExitWithError;
	}

	if (Height > 256)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_Create3DTexture: Height > 256.");
		goto ExitWithError;
	}

	SWidth = SnapToPower2(Width);
	SHeight = SnapToPower2(Height);

	if (Width != SWidth)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_Create3DTexture: Not a power of 2.");
		goto ExitWithError;
	}

	if (Height != SHeight)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_Create3DTexture: Not a power of 2.");
		goto ExitWithError;
	}
	
	if (!GlideFormatFromGenesisFormat(PixelFormat->PixelFormat, &Format))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_RegisterMiscTexture: GlideFormatFromGenesisFormat failed...");
		goto ExitWithError;
	}

	if (!GTHandle_SetupInfo(&Info, Width, Height, NumMipLevels, Format, &THandle->LogSize))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_RegisterMiscTexture: SetupInfo failed...");
		goto ExitWithError;
	}

	THandle->Log = (uint8)GetLog(THandle->LogSize);

	// Get the size of the data so we can create a block of memory for it
	DataSize = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &Info);

	// Allocate the data
	Data = (uint8*)malloc(sizeof(uint8)*DataSize);

	if (!Data)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_RegisterMiscTexture: out of memory for data.");
		goto ExitWithError;
	}

#if 0
	memset(Data, 0xff, DataSize);		// For debugging
#endif

	// Save the data pointer...
	Info.data = (void*)Data;
	
	// Save the data
	THandle->Data = Data;
	THandle->Active = GE_TRUE;
	THandle->NumMipLevels = (uint8)NumMipLevels;
	//THandle->Info = Info;

	// Save off some other goot info
	THandle->Width = Width;					// Original Width
	THandle->Height = Height;
	
	THandle->OneOverLogSize_255 = 255.0f / (float)THandle->LogSize;

	THandle->CacheType = GCache_TypeCreate(TextureCache, THandle->LogSize, THandle->LogSize, NumMipLevels, &Info);

	THandle->PixelFormat = *PixelFormat;

	TexturesChanged = GE_TRUE;

	return THandle;
		
	ExitWithError:
	{
		if (Data)
			free(Data);

		return NULL;
	}
}

//**********************************************************************************
//	Loads a lightmap texture
//**********************************************************************************
geRDriver_THandle *CreateLightmapTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{	
	geRDriver_THandle	*THandle;
	float				Size;

	THandle = GTHandle_FindTextureHandle();

	if (!THandle)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_CreateLightmapTexture:  Max texture handles.");
		return NULL;
	}

	if ( ! (PixelFormat->Flags&RDRIVER_PF_LIGHTMAP) )	// This should be the only bit set (except for Can_Do_ColorKey)
	{	
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_CreateLightmapTexture:  Invalid pixel format.");
		return NULL;
	}
	
	if (PixelFormat->PixelFormat != GE_PIXELFORMAT_16BIT_565_RGB)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_CreateLightmapTexture:  PixelFormat != GE_PIXELFORMAT_16BIT_565_RGB.");
		return NULL;
	}

	THandle->Width = Width;
	THandle->Height = Height;

	{
		GrTexInfo		Info;

		if (!GTHandle_SetupInfo(&Info, Width, Height, 1, GR_TEXFMT_RGB_565, &THandle->LogSize))
			return GE_FALSE;
		THandle->Log = (uint8)GetLog(THandle->LogSize);

		THandle->CacheType = GCache_TypeCreate(LMapCache, THandle->LogSize, THandle->LogSize, NumMipLevels, &Info);
	}

	Size = (float)(THandle->LogSize<<4);

 	THandle->OneOverLogSize_255 = 255.0f / Size;

	TexturesChanged = GE_TRUE;

	return THandle;
}

//==================================================================================
//	Create2DTexture
//==================================================================================
geRDriver_THandle *Create2DTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	geRDriver_THandle	*THandle;
	int32				Size;

	THandle = GTHandle_FindTextureHandle();

	if (!THandle)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_CreateTexture: No more handles left.");
		return NULL;
	}

	THandle->Width = Width;
	THandle->Height = Height;
	THandle->LogSize = Width;
	THandle->NumMipLevels = (uint8)NumMipLevels;

	Size = Width*Height;

	THandle->Data = (uint16*)malloc(sizeof(uint16)*Size);
	THandle->PixelFormat = *PixelFormat;

	return THandle;
}

//========================================================================================
//	GTHandle_CreatePalTexture
//========================================================================================
geRDriver_THandle *CreatePalTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	int32				DataSize;
	uint32				*Data;
	geRDriver_THandle	*THandle;
	GrTextureFormat_t	Format;

	assert(PixelFormat);
	
	Data = NULL;

	THandle = GTHandle_FindTextureHandle();

	if (!THandle)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_CreatePalTexture: No more handles left.");
		goto ExitWithError;
	}

	assert(Width == 256);
	assert(Height == 1);
	assert(NumMipLevels == 1);

	switch (PixelFormat->PixelFormat)
	{
		case THANDLE_PALETTE_FORMAT:
		{
			Format = GR_TEXFMT_P_8;
			break;
		}
		
		default:
		{
			SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_CreatePalTexture: Invalid pixel format.");
			goto ExitWithError;
		}
	}

	// Allocate the data
	DataSize = sizeof(uint32)*256;

	Data = (uint32*)malloc(DataSize);

	if (!Data)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_RegisterMiscTexture: out of memory for data.");
		goto ExitWithError;
	}

#if 0
	memset(Data, 0xff, DataSize);		// For debugging
#endif

	// Save the data
	THandle->Data = Data;
	THandle->Active = GE_TRUE;
	THandle->NumMipLevels = (uint8)NumMipLevels;

	THandle->Width = Width;					// Original Width
	THandle->Height = Height;
	THandle->LogSize = Width;
	THandle->PixelFormat = *PixelFormat;

	return THandle;
		
	ExitWithError:
	{
		if (Data)
			free(Data);

		return NULL;
	}
}

//==================================================================================
//	GTHandle_Create
//==================================================================================
geRDriver_THandle *DRIVERCC GTHandle_Create(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	if (PixelFormat->Flags & RDRIVER_PF_2D)
		return Create2DTexture(Width, Height, NumMipLevels, PixelFormat);
	else if (PixelFormat->Flags & RDRIVER_PF_3D)
		return Create3DTexture(Width, Height, NumMipLevels, PixelFormat);
	else if (PixelFormat->Flags & RDRIVER_PF_LIGHTMAP)
		return CreateLightmapTexture(Width, Height, NumMipLevels, PixelFormat);
	else if (PixelFormat->Flags & RDRIVER_PF_PALETTE)
		return CreatePalTexture(Width, Height, NumMipLevels, PixelFormat);
	else
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_CreateTexture: Invalid pixelformat.");
		return GE_FALSE;
	}
}

//==================================================================================
//	GTHandle_Destroy
//==================================================================================
geBoolean DRIVERCC GTHandle_Destroy(geRDriver_THandle *THandle)
{
	GTHandle_FreeTextureHandle(THandle);

	return GE_TRUE;
}

//==================================================================================
//	GTHandle_Lock
//==================================================================================
geBoolean DRIVERCC GTHandle_Lock(geRDriver_THandle *THandle, int32 MipLevel, void **Data)
{
	int32		i, Size;
	uint16		*pData;
	uint8		*pData8;

	assert(MipLevel <= THandle->NumMipLevels);

	if (THandle->PixelFormat.Flags & RDRIVER_PF_LIGHTMAP)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_LockTextureHandle:  Attempt to lock a lightmap texture.");
		return GE_FALSE;
	}

	THandle->Flags |= THANDLE_LOCKED;		// This texture is now locked

	Size = THandle->LogSize*THandle->LogSize;

	if (THandle->PixelFormat.PixelFormat == GE_PIXELFORMAT_8BIT)
	{
		pData8 = (uint8*)THandle->Data;

		for (i=0; i<MipLevel; i++)
		{
			pData8 += Size;
			Size >>=2;
		}
		
		*Data = pData8;
	}
	else
	{
		pData = (uint16*)THandle->Data;

		for (i=0; i<MipLevel; i++)
		{
			pData += Size;
			Size >>=2;
		}
		
		*Data = pData;
	}

	return GE_TRUE;
}

//==================================================================================
//	GTHandle_UnLock
//==================================================================================
geBoolean DRIVERCC GTHandle_UnLock(geRDriver_THandle *THandle, int32 MipLevel)
{
	#pragma message ("FIXME:  Flags needs to be per-mip!!!")
	
	//if (!(THandle->Flags & THANDLE_LOCKED))
	//	return GE_FALSE;

	THandle->Flags &= ~THANDLE_LOCKED;		// This handle is now unlocked
	THandle->Flags |= THANDLE_UPDATE;		// Mark it for updating...

	return GE_TRUE;
}

//==================================================================================
//	GThandle_SetPal
//==================================================================================
geBoolean DRIVERCC GThandle_SetPal(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle)
{
	assert(PalHandle->PixelFormat.PixelFormat == THANDLE_PALETTE_FORMAT);
	assert(PalHandle->PixelFormat.Flags & RDRIVER_PF_PALETTE);
	assert(PalHandle->Width = 256);
	assert(PalHandle->LogSize = 256);
	assert(PalHandle->Height = 1);

	THandle->PalHandle = PalHandle;

	return GE_TRUE;
}

//==================================================================================
//	GThandle_GetPal
//==================================================================================
geRDriver_THandle *DRIVERCC GThandle_GetPal(geRDriver_THandle *THandle)
{
	return THandle->PalHandle;
}

//==================================================================================
//	GTHandle_GetInfo
//==================================================================================
geBoolean DRIVERCC GTHandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info)
{
	Info->Width = THandle->Width>>MipLevel;
	Info->Height = THandle->Height>>MipLevel;
	Info->Stride = THandle->LogSize>>MipLevel;
	Info->PixelFormat = THandle->PixelFormat;

	if (THandle->PixelFormat.Flags & RDRIVER_PF_CAN_DO_COLORKEY)
	{
		// Color keys are allways on for surfaces that support it for now...
		Info->Flags = RDRIVER_THANDLE_HAS_COLORKEY;		// Color keys are allways on for surfaces that support it for now...

		if (THandle->PixelFormat.Flags & RDRIVER_PF_PALETTE)
			Info->ColorKey = (1<<16);
		else
			Info->ColorKey = 1;
	}
	else
	{
		Info->Flags = 0;			
		Info->ColorKey = 0;
	}
	
	return GE_TRUE;
}

//==================================================================================
//	GTHandle_CheckTextures
//==================================================================================
geBoolean GTHandle_CheckTextures(void)
{
	int32				i;
	geRDriver_THandle	*pTHandle;

	if (!TexturesChanged)
		return GE_TRUE;

	if (!GCache_AdjustSlots(TextureCache))
	{
		SetLastDrvError(DRV_ERROR_INVALID_REGISTER_MODE, "GTHandle_CheckTextures: GCache_AdjustSlots for textures.");
		return GE_FALSE;
	}

	if (!GCache_AdjustSlots(LMapCache))
	{
		SetLastDrvError(DRV_ERROR_INVALID_REGISTER_MODE, "GTHandle_CheckTextures: GCache_AdjustSlots for lightmaps.");
		return GE_FALSE;
	}

	// Make sure no THandles reference any slots, because they mave have been moved around, or gotten destroyed...
	//  (Evict all textures in the cache)
	pTHandle = TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, pTHandle++)
		pTHandle->Slot = NULL;

	// Reset the tedxture handle changed boolean
	TexturesChanged = GE_FALSE;

	return GE_TRUE;
}

//**********************************************************************************
//	Return the LOD of a mip-map
//**********************************************************************************
geBoolean GetLod(S32 Width, GrLOD_t *Lod)
{
	if (Width == 1)
		*Lod = GR_LOD_1;
	else if (Width == 2)
		*Lod = GR_LOD_2;
	else if (Width == 4)
		*Lod = GR_LOD_4;
	else if (Width == 8)
		*Lod = GR_LOD_8;
	else if (Width == 16)
		*Lod = GR_LOD_16;
	else if (Width == 32)
		*Lod = GR_LOD_32;
	else if (Width == 64)
		*Lod = GR_LOD_64;
	else if (Width == 128)
		*Lod = GR_LOD_128;
	else if (Width == 256)
		*Lod = GR_LOD_256;
	else
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_GetLod: Invalid texture width.");
		return GE_FALSE;
	}
	
	return GE_TRUE;
}

//**********************************************************************************
//	Returns the aspect ratio of a texture mip-map
//**********************************************************************************
geBoolean GetAspectRatio(int32 Width, int32 Height, GrAspectRatio_t *Aspect)
{
	int Aspect2;

	Aspect2 = ( 8 * Width ) / Height;

	switch( Aspect2)
	{
		case 64:
			*Aspect = GR_ASPECT_8x1;
			break;
		case 32:
			*Aspect = GR_ASPECT_4x1;
			break;
		case 16:
			*Aspect = GR_ASPECT_2x1;
			break;
		case 8:
			*Aspect = GR_ASPECT_1x1;
			break;
		case 4:
			*Aspect = GR_ASPECT_1x2;
			break;
		case 2:
			*Aspect = GR_ASPECT_1x4;
			break;
		case 1:
			*Aspect = GR_ASPECT_1x8;
			break;

		default:
			return GE_FALSE;
    }
	
	return GE_TRUE;
}

//**********************************************************************************
// Returns the log of a number (number must be a power of 2)
//**********************************************************************************
uint32 GetLog(uint32 P2)
{
	U32		p = 0;
	S32		i = 0;
	
	for (i = P2; i > 0; i>>=1)
		p++;

	return (p-1);
}

//**********************************************************************************
//	Snaps a number to a power of 2
//**********************************************************************************
int32 SnapToPower2(int32 Width)
{
int RetWidth;
	// CB : I couldn't stand that big branch of if's !
	for ( RetWidth = 1; RetWidth < Width; RetWidth <<= 1 ) ;
	assert( RetWidth <= 256 );
return RetWidth;
}


