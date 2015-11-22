/****************************************************************************************/
/*  D3DCache.cpp                                                                        */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D cache manager                                                      */
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
#include <Stdio.h>
#include <Assert.h>
#include <DDraw.h>
#include <D3D.h>

#include "D3DCache.h"

// Cache types are just the different types of textures
// Texutres may vary from width/height/num miplevels/etc...
// Cache types is just a way to combine them to similar types...
#define	D3DCACHE_MAX_CACHE_TYPES		128		

//========================================================================================================
//========================================================================================================
typedef struct D3DCache_Type
{
	int32					Log;
	int32					Width;						// Width/Height
	int32					Height;
	int32					NumMipLevels;
	int32					Stage;
	int32					RefCount;					// How many references to this cache_type

	DDSURFACEDESC2			ddsd;						// DD surface description

	D3DCache_Slot			*Slots;						// Cache slots for this Cache type
	int32					NumUsedSlots;				// Number of slots being used

	D3DCache_Type			*SelfCheck;
	D3DCache				*Cache;
} D3DCache_Type;

typedef struct D3DCache
{
	struct D3DCache			*SelfCheck;

	char					Name[D3DCACHE_MAX_NAME];

	LPDIRECTDRAW4			lpDD;						// DD object for the cache manager

	DDMemMgr_Partition		*Partition;

	geBoolean				UseStages;

	D3DCache_Type			CacheTypes[D3DCACHE_MAX_CACHE_TYPES];	// CacheTypes
} D3DCache;

typedef struct D3DCache_Slot
{
	struct D3DCache_Slot	*SelfCheck;

	D3DCache_Type			*CacheType;

	LPDIRECTDRAWSURFACE4	Surface;						// The DD surface for this slot
	LPDIRECT3DTEXTURE2		Texture;						// The texture interface to the surface

	uint32					LRU;							// Current LRU for cache slot
						
	void					*UserData;

} D3DCache_Slot;

//========================================================================================================
//========================================================================================================

//========================================================================================================
//	D3DCache_Create
//========================================================================================================
D3DCache *D3DCache_Create(const char *Name, LPDIRECTDRAW4 lpDD, DDMemMgr_Partition *Partition, geBoolean UseStages)
{
	D3DCache		*Cache;

	assert(strlen(Name) < D3DCACHE_MAX_NAME);

	Cache = (D3DCache*)malloc(sizeof(D3DCache));

	if (!Cache)
		return NULL;

	memset(Cache, 0, sizeof(D3DCache));

	Cache->lpDD = lpDD;

	Cache->Partition = Partition;

	Cache->UseStages = UseStages;

	Cache->SelfCheck = Cache;

	strcpy(Cache->Name, Name);

	return Cache;
}

//========================================================================================================
//	D3DCache_Destroy
//========================================================================================================
void D3DCache_Destroy(D3DCache *Cache)
{
	assert(Cache);

	D3DCache_FreeAllSlots(Cache);

	free(Cache);
}

//========================================================================================================
//	D3DCache_IsValid
//========================================================================================================
geBoolean D3DCache_IsValid(D3DCache *Cache)
{
	if (!Cache)
		return GE_FALSE;

	if (Cache->SelfCheck != Cache)
		return GE_FALSE;

	return GE_TRUE;
}

//========================================================================================================
//========================================================================================================
geBoolean D3DCache_EvictAllSurfaces(D3DCache *Cache)
{
	int32			i;
	D3DCache_Type	*pCacheType;

	assert(D3DCache_IsValid(Cache));

	for (pCacheType = Cache->CacheTypes, i=0; i< D3DCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		D3DCache_Slot		*pSlot;
		int32				s;

		if (!pCacheType->RefCount)
			continue;

		assert(D3DCache_TypeIsValid(pCacheType));

		for (pSlot = pCacheType->Slots, s=0; s<pCacheType->NumUsedSlots; s++, pSlot++)
		{
			D3DCache_SlotSetUserData(pSlot, NULL);
		}
	}
	
	return GE_TRUE;
}

//========================================================================================================
//========================================================================================================
D3DCache_Type *D3DCache_FindCacheType(D3DCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, int32 Stage, const DDSURFACEDESC2 *ddsd)
{
	int32			i;
	D3DCache_Type	*pCacheType;

	assert(D3DCache_IsValid(Cache));

	pCacheType = Cache->CacheTypes;

	for (i=0; i<D3DCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		if (pCacheType->RefCount == 0)			// Nobody is using this slot yet
			continue;

		assert(D3DCache_TypeIsValid(pCacheType));

		if (pCacheType->Width != Width)
			continue;
		if (pCacheType->Height != Height)
			continue;

		if (pCacheType->NumMipLevels != NumMipLevels)
			continue;

		if (pCacheType->Stage != Stage)
			continue;

		if (memcmp(&pCacheType->ddsd, ddsd, sizeof(DDSURFACEDESC2)))
			continue;
		
		return pCacheType;						// Found a match
	}

	return NULL;								// Cache Type not found!!!
}

//========================================================================================================
//========================================================================================================
D3DCache_Type *D3DCache_InsertCacheType(D3DCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, int32 Stage, const DDSURFACEDESC2 *ddsd)
{
	int32			i;
	D3DCache_Type	*pCacheType;

	assert(D3DCache_IsValid(Cache));

	pCacheType = Cache->CacheTypes;

	for (i=0; i<D3DCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		if (pCacheType->RefCount == 0)			// Nobody is using this slot yet
			break;
	}

	if (i == D3DCACHE_MAX_CACHE_TYPES)			// No types left
		return NULL;

	assert(pCacheType->Slots == NULL);
	assert(pCacheType->NumUsedSlots == 0);

	pCacheType->Width = Width;
	pCacheType->Height = Height;
	pCacheType->NumMipLevels = NumMipLevels;
	pCacheType->Stage = Stage;
	pCacheType->ddsd = *ddsd;

	pCacheType->SelfCheck = pCacheType;
	pCacheType->Cache = Cache;

	pCacheType->Log = GetLog(Width, Height);

	// Found one
	pCacheType->RefCount++;

	return pCacheType;
}

//========================================================================================================
//========================================================================================================
D3DCache_Type *D3DCache_TypeCreate(D3DCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, int32 Stage, const DDSURFACEDESC2 *ddsd)
{
	D3DCache_Type		*CacheType;

	assert(D3DCache_IsValid(Cache));

	CacheType = D3DCache_FindCacheType(Cache, Width, Height, NumMipLevels, Stage, ddsd);

	if (CacheType)
	{
		CacheType->RefCount++;
		return CacheType;
	}

	// Could not find one allready in the list, so add a new one...
	return D3DCache_InsertCacheType(Cache, Width, Height, NumMipLevels, Stage, ddsd);
}

//========================================================================================================
//	D3DCache_TypeDestroy
//========================================================================================================
void D3DCache_TypeDestroy(D3DCache_Type *CacheType)
{
	assert(CacheType->RefCount > 0);
	assert(D3DCache_TypeIsValid(CacheType));

	CacheType->RefCount--;

	if (CacheType->RefCount == 0)
	{
		if (CacheType->Slots)
		{
			D3DCache_Slot	*pSlot;
			int32			k;

			// Go through each slot, and free all the surfaces on them
			for (pSlot = CacheType->Slots, k=0; k< CacheType->NumUsedSlots; k++, pSlot++)
			{
				assert(D3DCache_SlotIsValid(pSlot));
				assert(pSlot->Surface);
				assert(pSlot->Texture);

				if (pSlot->Texture)
					pSlot->Texture->Release();
				if (pSlot->Surface)
					pSlot->Surface->Release();
			}

			free(CacheType->Slots);
			CacheType->Slots = NULL;
			CacheType->NumUsedSlots = 0;
		}
	}
}

//========================================================================================================
//	D3DCache_TypeIsValid
//========================================================================================================
geBoolean D3DCache_TypeIsValid(D3DCache_Type *Type)
{
	if (!Type)
		return GE_FALSE;

	if (Type->SelfCheck != Type)
		return GE_FALSE;

	if (!D3DCache_IsValid(Type->Cache))
		return GE_FALSE;

	return GE_TRUE;
}

//========================================================================================================
//	D3DCache_FreeAllSlots
//========================================================================================================
geBoolean D3DCache_FreeAllSlots(D3DCache *Cache)
{
	int32			i;
	D3DCache_Type	*pCacheType;

	assert(D3DCache_IsValid(Cache));

	pCacheType = Cache->CacheTypes;

	for (i=0; i< D3DCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		D3DCache_Slot	*pSlot;
		int32			k;

		assert(pCacheType->RefCount >= 0);

		if (pCacheType->RefCount == 0)
		{
			assert(pCacheType->Slots == NULL);
			continue;
		}

		assert(D3DCache_TypeIsValid(pCacheType));

		// Go through each slot, and free all the surfaces on them
		for (pSlot = pCacheType->Slots, k=0; k< pCacheType->NumUsedSlots; k++, pSlot++)
		{
			assert(D3DCache_SlotIsValid(pSlot));
			assert(pSlot->Surface);
			assert(pSlot->Texture);

			if (pSlot->Texture)
				pSlot->Texture->Release();
			if (pSlot->Surface)
				pSlot->Surface->Release();
		}

		if (pCacheType->Slots)
			free(pCacheType->Slots);

		pCacheType->Slots = NULL;
		pCacheType->NumUsedSlots = 0;
	}

	DDMemMgr_PartitionReset(Cache->Partition);			// Reset the caches memory manager

	return GE_TRUE;
}

//========================================================================================================
//	D3DCache_WriteToFile
//========================================================================================================
geBoolean D3DCache_WriteToFile(D3DCache *Cache, const char *FileName, geBoolean Append)
{
	int32			i;
	D3DCache_Type	*pCacheType;
	int32			TotalRef, TotalUsed;
	SYSTEMTIME		Time;
	FILE			*f;

	if (Append)
		f = fopen(FileName, "a+t");
	else
		f = fopen(FileName, "w");

	if (!f)
		return GE_FALSE;

	GetSystemTime(&Time);

	fprintf(f, "=======================================================\n");
	fprintf(f, "Date: %i/%i/%i, Time: %i:%i\n", Time.wMonth, Time.wDay, Time.wYear, Time.wHour, Time.wMinute);
	fprintf(f, "Cache Name: %s\n", Cache->Name);
	fprintf(f, "Total Mem: %5i\n", DDMemMgr_PartitionGetTotalMem(Cache->Partition));
	fprintf(f, "Free Mem: %5i\n", DDMemMgr_PartitionGetFreeMem(Cache->Partition));
	fprintf(f, " --- Slots ---\n");

	TotalRef = TotalUsed = 0;

	pCacheType = Cache->CacheTypes;

	for (i=0; i< D3DCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		if (!pCacheType->RefCount)
			continue;

		fprintf(f, "Width: %3i, Height %3i, Mips: %2i, Stage: %2i, Ref: %4i, Used: %4i\n",
			pCacheType->Width, pCacheType->Height, pCacheType->NumMipLevels, pCacheType->Stage, pCacheType->RefCount, pCacheType->NumUsedSlots);

		TotalRef += pCacheType->RefCount;
		TotalUsed += pCacheType->NumUsedSlots;
	}

	fprintf(f, "Total Ref: %4i, Total Used: %4i\n", TotalRef, TotalUsed);

	fclose(f);

	return GE_TRUE;
}

static geBoolean		AppendHack = GE_FALSE;

// HACK!!!!
void				D3DMain_Log(LPSTR Str, ... );
//========================================================================================================
//	D3DCache_AdjustSlots
//========================================================================================================
geBoolean D3DCache_AdjustSlots(D3DCache *Cache, const int32 *MaxTable, geBoolean UsePartition)
{
	D3DCache_Type	*pCacheType;
	int32			i, Total, NumPasses;

	assert(D3DCache_IsValid(Cache));

	D3DCache_FreeAllSlots(Cache);							// Just get rid of everything for now...
	DDMemMgr_PartitionReset(Cache->Partition);				// Reset the caches memory manager

	Total = 0;
	NumPasses = 0;

	while(1)
	{
		D3DCache_Slot		*LastSlot;

		LastSlot = NULL;

		pCacheType = Cache->CacheTypes;

		for (i=0; i< D3DCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
		{
			D3DCache_Slot	*pSlot;
			uint32			Size, Width, Height, Result;

			if (pCacheType->RefCount <= 0)
			{
				assert(pCacheType->Slots == NULL);
				continue;
			}

			if (pCacheType->NumUsedSlots >= pCacheType->RefCount)
				continue;					// This is all we need for this slot...

			if (pCacheType->NumUsedSlots >= MaxTable[pCacheType->Log])
				continue;

			if (!pCacheType->Slots)			// If no slots have been allocated, allocate them now...
			{
				pCacheType->Slots = (D3DCache_Slot*)malloc(sizeof(D3DCache_Type)*pCacheType->RefCount);
				memset(pCacheType->Slots, 0, sizeof(D3DCache_Type)*pCacheType->RefCount);
			}

			Width = pCacheType->Width;
			Height = pCacheType->Height;

			Size = Width*Height*(pCacheType->ddsd.ddpfPixelFormat.dwRGBBitCount>>3);  // (BitCount/8)
		
			if (UsePartition)
			{
				if (!DDMemMgr_PartitionAllocMem(Cache->Partition, Size))
				{
					LastSlot = NULL;	// Make a complete stop
					break;				// No more memory in the partition, stop now...
				}
			}

			pSlot = &pCacheType->Slots[pCacheType->NumUsedSlots];
			pSlot->SelfCheck = pSlot;

			pSlot->CacheType = pCacheType;

			// Allocate surfaces now
			Result = D3DCache_SetupSlot(Cache, pSlot, Width, Height, &pCacheType->ddsd, Cache->UseStages, pCacheType->Stage);

			if (!Result)
			{
				memset(pSlot, 0, sizeof(D3DCache_Slot));
				break;
			}
			else if (Result == -1)
			{
				D3DMain_Log("D3DCache_AdjustSlots:  D3DCache_SetupSlot failed.\n");
				return GE_FALSE;
			}

			pCacheType->NumUsedSlots++;
			Total++;

			LastSlot = pSlot;
		}

		NumPasses++;

		if (!LastSlot)		// Nothing was allocated on that pass, so assume we are out of memory
			break;
	}

	pCacheType = Cache->CacheTypes;
	
	// Go through one last time, and make sure all got allocated
	for (i=0; i< D3DCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		if (pCacheType->RefCount <= 0)
		{
			assert(pCacheType->Slots == NULL);
			continue;
		}

		if (pCacheType->NumUsedSlots <= 0)		
		{
			D3DMain_Log("D3DCache_AdjustSlots:  Out of ram creating surfaces for cache.\n");
			D3DMain_Log("D3DCache_AdjustSlots:  Pick a display mode with a smaller resolution.\n");
			return GE_FALSE;			// Not all slots with refs got a texture
		}

		assert(pCacheType->Slots != NULL);
	}

	D3DCache_WriteToFile(Cache, "D3DCache.Log", AppendHack);
	AppendHack = GE_TRUE;

	return GE_TRUE;
}

//========================================================================================================
//	D3DCache_SlotIsValid
//========================================================================================================
geBoolean D3DCache_SlotIsValid(D3DCache_Slot *Slot)
{
	if (!Slot)
		return GE_FALSE;

	if (Slot->SelfCheck != Slot)
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	D3DCache_SetupSlot
//
//	Returns -1 on failure
//	Returns 0 on out of memory
//	Returns 1 on success
//=====================================================================================
int32 D3DCache_SetupSlot(D3DCache *Cache, D3DCache_Slot *Slot, int32 Width, int32 Height, const DDSURFACEDESC2 *SurfDesc, geBoolean UseStage, int32 Stage)
{
	LPDIRECTDRAWSURFACE4 Surface;
	DDSURFACEDESC2		ddsd;
	HRESULT				Hr;
			
	assert(D3DCache_IsValid(Cache));
	assert(D3DCache_SlotIsValid(Slot));

	memcpy(&ddsd, SurfDesc, sizeof(DDSURFACEDESC2));

	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

	if (UseStage)
		ddsd.dwFlags |= DDSD_TEXTURESTAGE;

	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_HINTDYNAMIC;
	ddsd.ddsCaps.dwCaps3 = 0;
	ddsd.ddsCaps.dwCaps4 = 0;
	ddsd.dwHeight = Width;
	ddsd.dwWidth = Height;
	
	ddsd.dwTextureStage = Stage;
	
	Hr = Cache->lpDD->CreateSurface(&ddsd, &Surface, NULL);

	if(Hr != DD_OK) 
	{ 
		if (Hr == DDERR_OUTOFVIDEOMEMORY)
		{
			return 0;
		}

		return -1;
	}

	Slot->Surface = Surface;

	// Set the color key
#if 0
	{
		DDCOLORKEY			CKey;
		
		// Create the color key for this surface
		CKey.dwColorSpaceLowValue = 1;
		CKey.dwColorSpaceHighValue = 1;
		
		if (Slot->Surface->SetColorKey(DDCKEY_SRCBLT , &CKey) != DD_OK)
		{
			Slot->Surface->Release();
			Slot->Surface = NULL;
			return -1;
		}
	}
 #endif

	Hr = Surface->QueryInterface(IID_IDirect3DTexture2, (void**)&Slot->Texture);  

	if(Hr != DD_OK) 
	{ 
		Surface->Release();
		return -1;
	}

	return 1;		// All good dude
}


//========================================================================================================
//	D3DCache_TypeFindSlot
//========================================================================================================
D3DCache_Slot *D3DCache_TypeFindSlot(D3DCache_Type *CacheType)
{
	D3DCache_Slot	*pBestSlot, *pSlot;
	uint32			BestLRU;
	int32			i;

	assert(D3DCache_TypeIsValid(CacheType));

	assert(CacheType->Slots);

	pSlot = CacheType->Slots;
	pBestSlot = pSlot;
	BestLRU = pBestSlot->LRU;

	for (i=0; i< CacheType->NumUsedSlots; i++, pSlot++)
	{
		assert(D3DCache_SlotIsValid(pSlot));

		if (pSlot->LRU < BestLRU)
		{
			pBestSlot = pSlot;
			BestLRU = pSlot->LRU;
		}
	}

	pBestSlot->LRU = 0;
	pBestSlot->UserData = NULL;

	return pBestSlot;
}

//========================================================================================================
//	D3DCache_SlotSetUserData
//========================================================================================================
void D3DCache_SlotSetUserData(D3DCache_Slot *Slot, void *UserData)
{
	assert(D3DCache_SlotIsValid(Slot));

	Slot->UserData = UserData;
}

//========================================================================================================
//	D3DCache_SlotGetUserData
//========================================================================================================
void *D3DCache_SlotGetUserData(D3DCache_Slot *Slot)
{
	assert(D3DCache_SlotIsValid(Slot));

	return Slot->UserData;
}

//========================================================================================================
//	D3DCache_SlotSetLRU
//========================================================================================================
void D3DCache_SlotSetLRU(D3DCache_Slot *Slot, uint32 LRU)
{
	assert(D3DCache_SlotIsValid(Slot));

	Slot->LRU = LRU;
}

//========================================================================================================
//	D3DCache_SlotGetLRU
//========================================================================================================
uint32 D3DCache_SlotGetLRU(D3DCache_Slot *Slot)
{
	assert(D3DCache_SlotIsValid(Slot));

	return Slot->LRU;
}

LPDIRECT3DTEXTURE2 D3DCache_SlotGetTexture(D3DCache_Slot *Slot)
{
	assert(D3DCache_SlotIsValid(Slot));

	return Slot->Texture;
}

LPDIRECTDRAWSURFACE4 D3DCache_SlotGetSurface(D3DCache_Slot *Slot)
{
	assert(D3DCache_SlotIsValid(Slot));

	return Slot->Surface;
}

//=====================================================================================
//	Log2
//	Return the log of a size
//=====================================================================================
uint32 Log2(uint32 P2)
{
	uint32		p = 0;
	int32		i = 0;
	
	for (i = P2; i > 0; i>>=1)
		p++;

	return (p-1);
}

//=====================================================================================
//	SnapToPower2
//	Snaps a number to a power of 2
//=====================================================================================
int32 SnapToPower2(int32 Width)
{
#if 1
	if (Width > 0 && Width <= 1) Width = 1;
	else if (Width > 1 && Width <= 2) Width = 2;
	else if (Width > 2 && Width <= 4) Width = 4;
	else if (Width > 4 && Width <= 8) Width = 8;
	else if (Width > 8 && Width <= 16) Width =16;
	else if (Width > 16 && Width <= 32) Width = 32;
	else if (Width > 32 && Width <= 64) Width = 64;
	else if (Width > 64 && Width <= 128) Width = 128;
	else if (Width > 128 && Width <= 256) Width = 256;
	else 
		return -1;
#else
	
	if (Width > 1 && Width <= 8) Width = 8;
	else if (Width > 8 && Width <= 16) Width =16;
	else if (Width > 16 && Width <= 32) Width = 32;
	else if (Width > 32 && Width <= 64) Width = 64;
	else if (Width > 64 && Width <= 128) Width = 128;
	else if (Width > 128 && Width <= 256) Width = 256;
	else 
		return -1;
#endif

	return Width;
}

//=====================================================================================
//	Return the max log of a (power of 2) width and height
//=====================================================================================
int32 GetLog(int32 Width, int32 Height)
{
	int32	LWidth = SnapToPower2(max(Width, Height));
	
	return Log2(LWidth);
}

