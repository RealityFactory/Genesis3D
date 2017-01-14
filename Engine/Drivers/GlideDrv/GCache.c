/****************************************************************************************/
/*  GCache.c                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Texture cache manager for glide                                        */
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

#include "GCache.h"
#include "GMemMGr.h"
#include "DCommon.h"
#include "Glide.h"

#define							GCACHE_WRITELOG
//#define							DYNAMIC_CACHE


#define	GCACHE_MAX_CACHE_TYPES		128

//========================================================================================================
//========================================================================================================
typedef struct GCache_Type
{
	GrTexInfo	Info;
	int32		Log;
	int32		Width;						// Width/Height
	int32		Height;
	uint8		NumMipLevels;
	uint32		Size;

	float		OneOverWidth_255;
	float		OneOverHeight_255;

	GCache_Slot	*Slots;						// Cache slots for this Cache type
	int32		RefCount;					// How many slots are allocated 
	int32		UsedSlots;					// How many slots were used

#ifdef DYNAMIC_CACHE
	struct GCache_Type	*PrevNext[2];		// NextPrev cache type in list
#endif

} GCache_Type;

typedef struct GCache
{
	char		Name[GCACHE_MAX_NAME];

	GMemMgr		*MemMgr;							// Memory manager

	GCache_Type	CacheTypes[GCACHE_MAX_CACHE_TYPES];	// CacheTypes

	uint32		LastMemAddr;

	struct GCache	*SelfCheck;
} GCache;

typedef struct GCache_Slot
{

	GCache_Type	*Type;

	uint32		MemAddr;					// Points to the tex mem where mem is
	uint32		LRU;						// Current LRU for cache slot

	void		*UserData;

	struct GCache_Slot	*SelfCheck;

} GCache_Slot;

//========================================================================================================
//========================================================================================================

//========================================================================================================
//	GCache_Create
//========================================================================================================
GCache *GCache_Create(const char *Name, GMemMgr *MemMgr)
{
	GCache		*Cache;

	assert(Name);
	assert(strlen(Name) < GCACHE_MAX_NAME);
	assert(MemMgr);

	Cache = malloc(sizeof(GCache));

	if (!Cache)
		return NULL;

	memset(Cache, 0, sizeof(GCache));

	strcpy(Cache->Name, Name);

	Cache->MemMgr = MemMgr;

	Cache->LastMemAddr = 0xffffffff;

	Cache->SelfCheck = Cache;

	if (!GCache_Reset(Cache))
		goto ExitWithError;

	return Cache;

	ExitWithError:
	{
		if (Cache)
			free(Cache);

		return NULL;
	}
}

//========================================================================================================
//	GCache_Destroy
//========================================================================================================
void GCache_Destroy(GCache *Cache)
{
	assert(Cache);

	free(Cache);
}

//========================================================================================================
//========================================================================================================
geBoolean GCache_Reset(GCache *Cache)
{
	int32				i;
	GCache_Type			*pCacheType;

	GCache_FreeAllSlots(Cache);

	for (pCacheType = Cache->CacheTypes, i=0; i<GCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		memset(pCacheType, 0, sizeof(GCache_Type));

		#ifdef DYNAMIC_CACHE
		// Set prev/next pointers in array
			if (i > 0)
				pCacheType->PrevNext[0] = (pCacheType-1);
			if (i < GCACHE_MAX_CACHE_TYPES-1)
				pCacheType->PrevNext[1] = (pCacheType+1);
		#endif
	}
	
	return GE_TRUE;
}

//========================================================================================================
//========================================================================================================
GCache_Type *GCache_FindCacheTypeByInfo(GCache *Cache, const GrTexInfo *Info)
{
	int32		i;
	GCache_Type	*pCacheType;

	pCacheType = Cache->CacheTypes;

	for (i=0; i<GCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		//uint32	Size;

		if (pCacheType->RefCount == 0)			// Nobody is using this slot yet
			continue;
		
		if (Info->smallLod != pCacheType->Info.smallLod)
			continue;
		if (Info->largeLod != pCacheType->Info.largeLod)
			continue;
		if (Info->aspectRatio != pCacheType->Info.aspectRatio)
			continue;

		if (Info->format != pCacheType->Info.format)
			continue;

		#if 0
		// Check to see if the size required to create the surface matches
		Size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, (GrTexInfo*)Info);

		//if (Size != pCacheType->Size)
			continue;
		#endif

		return pCacheType;						// Found a match
	}

	return NULL;								// Cache Type not found!!!
}

//========================================================================================================
//========================================================================================================
GCache_Type *GCache_InsertCacheTypeByInfo(GCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, const GrTexInfo *Info)
{
	int32		i;
	GCache_Type	*pCacheType;

	assert(NumMipLevels <= 255);
	assert(NumMipLevels >= 0);

	pCacheType = Cache->CacheTypes;

	for (i=0; i<GCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		if (pCacheType->RefCount == 0)		// Nobody is using this slot yet
		{
			assert(pCacheType->Slots == NULL);
			assert(pCacheType->UsedSlots == 0);
			break;
		}
	}

	if (i == GCACHE_MAX_CACHE_TYPES)			// No types left
		return NULL;

	pCacheType->Info = *Info;
	pCacheType->Info.data = NULL;
	pCacheType->Width = Width;
	pCacheType->Height = Height;
	pCacheType->NumMipLevels = (uint8)NumMipLevels;
	pCacheType->Size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, (GrTexInfo*)Info);

	// Found one
	pCacheType->RefCount++;

	return pCacheType;
}

//========================================================================================================
//	GCache_UpdateSlot
//========================================================================================================
geBoolean GCache_UpdateSlot(GCache *Cache, GCache_Slot *Slot, GrTexInfo *Info)
{
	assert(GCache_SlotIsValid(Slot));

	grTexDownloadMipMap(GMemMgr_GetTmu(Cache->MemMgr), Slot->MemAddr, GR_MIPMAPLEVELMASK_BOTH, Info);

	return GE_TRUE;
}

//========================================================================================================
//	GCache_SetTexture
//========================================================================================================
geBoolean GCache_SetTexture(GCache *Cache, GCache_Slot *Slot)
{
	assert(GCache_SlotIsValid(Slot));

	if (Cache->LastMemAddr == Slot->MemAddr)
		return GE_TRUE;

	grTexSource(GMemMgr_GetTmu(Cache->MemMgr), Slot->MemAddr, GR_MIPMAPLEVELMASK_BOTH, &Slot->Type->Info);

	Cache->LastMemAddr = Slot->MemAddr;

	Slot->LRU++;

	return GE_TRUE;
}

//========================================================================================================
//========================================================================================================
GCache_Type *GCache_TypeCreate(GCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, const GrTexInfo *Info)
{
	GCache_Type		*CacheType;

	CacheType = GCache_FindCacheTypeByInfo(Cache, Info);

	if (CacheType)
	{
		CacheType->RefCount++;
		return CacheType;
	}

	// Could not find one allready in the list, so add a new one...
	return GCache_InsertCacheTypeByInfo(Cache, Width, Height, NumMipLevels, Info);
}

//========================================================================================================
//	GCache_TypeDestroy
//========================================================================================================
void GCache_TypeDestroy(GCache_Type *CacheType)
{
	assert(CacheType->RefCount > 0);

	CacheType->RefCount--;

	if (CacheType->RefCount == 0)
	{
		if (CacheType->Slots)
		{
			assert(CacheType->UsedSlots > 0);
			free(CacheType->Slots);
		}
		else
		{
			assert(CacheType->UsedSlots == 0);
		}
		
		CacheType->UsedSlots = 0;
		CacheType->Slots = NULL;
	}
}

//========================================================================================================
//	GCache_FreeAllSlots
//========================================================================================================
geBoolean GCache_FreeAllSlots(GCache *Cache)
{
	int32		i;
	GCache_Type	*pCacheType;

	pCacheType = Cache->CacheTypes;

	for (i=0; i< GCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		assert(pCacheType->RefCount >= 0);

		if (pCacheType->RefCount == 0)
		{
			assert(pCacheType->Slots == NULL);
			assert(pCacheType->UsedSlots == 0);
			continue;
		}

		if (pCacheType->Slots)
		{
			assert(pCacheType->UsedSlots > 0);
			free(pCacheType->Slots);
		}
		else
			assert(pCacheType->UsedSlots == 0);

		pCacheType->Slots = NULL;
		pCacheType->UsedSlots = 0;
	}

	return GE_TRUE;
}

//========================================================================================================
//	GCache_WriteToFile
//========================================================================================================
geBoolean GCache_WriteToFile(GCache *Cache, const char *FileName, geBoolean Append)
{
	int32		i;
	GCache_Type	*pCacheType;
	int32		TotalRef, TotalUsed;
	SYSTEMTIME	Time;
	FILE		*f;

	if (Append)
		f = fopen(FileName, "a+t");
	else
		f = fopen(FileName, "w");

	if (!f)
		return GE_FALSE;

	pCacheType = Cache->CacheTypes;

	TotalRef = TotalUsed = 0;

	GetSystemTime(&Time);

	fprintf(f, "=======================================================\n");
	fprintf(f, "Date: %i/%i/%i, Time: %i:%i\n", Time.wMonth, Time.wDay, Time.wYear, Time.wHour, Time.wMinute);
	fprintf(f, "Cache Name: %s\n", Cache->Name);
	fprintf(f, "Total Memory for Cache: %6i\n", GMemMgr_GetTotalMemory(Cache->MemMgr));
	fprintf(f, "Free Memory for Cache: %6i\n", GMemMgr_GetFreeMemory(Cache->MemMgr));
	fprintf(f, "--- Slots ---\n");

	for (i=0; i< GCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		if (!pCacheType->RefCount)
			continue;

		fprintf(f, "Width: %3i, Height %3i, Mips: %2i, Ref: %4i, Used: %4i\n",
			pCacheType->Width, pCacheType->Height, pCacheType->NumMipLevels, pCacheType->RefCount, pCacheType->UsedSlots);

		TotalRef += pCacheType->RefCount;
		TotalUsed += pCacheType->UsedSlots;
	}

	fprintf(f, "Total Ref: %4i, Total Used: %4i\n", TotalRef, TotalUsed);

	fclose(f);

	return GE_TRUE;
}

static	geBoolean	AppendHack = GE_FALSE;

//========================================================================================================
//	GCache_AdjustSlots
//========================================================================================================
geBoolean GCache_AdjustSlots(GCache *Cache)
{
	GCache_Type		*pCacheType;
	int32			i;

	GCache_FreeAllSlots(Cache);

	GMemMgr_Reset(Cache->MemMgr);			// Reset the caches memory manager

	while(1)
	{
		GCache_Slot	*LastSlot;

		LastSlot = NULL;

		pCacheType = Cache->CacheTypes;

		for (i=0; i< GCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
		{
			GCache_Slot		*pSlot;
			uint32			Size;

			if (pCacheType->RefCount <= 0)
			{
				assert(pCacheType->Slots == NULL);
				assert(pCacheType->UsedSlots == 0);
				continue;
			}

			if (pCacheType->UsedSlots >= pCacheType->RefCount)
				continue;					// This is all we need for this slot...

			if (!pCacheType->Slots)			// If no slots have been allocated, allocate them now...
			{
				assert(pCacheType->UsedSlots == 0);

				pCacheType->Slots = malloc(sizeof(GCache_Type)*pCacheType->RefCount);

				if (!pCacheType->Slots)
					return GE_FALSE;

				memset(pCacheType->Slots, 0, sizeof(GCache_Type)*pCacheType->RefCount);
			}

			Size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &pCacheType->Info);

			if (GMemMgr_GetFreeMemory(Cache->MemMgr) < Size)
				break;		// Took all available ram from memmgr.  No choice, but to stop

			pSlot = &pCacheType->Slots[pCacheType->UsedSlots++];
			pSlot->SelfCheck = pSlot;

			pSlot->Type = pCacheType;

			if (!GMemMgr_AllocMem(Cache->MemMgr, Size, &pSlot->MemAddr))
			{
				pCacheType->UsedSlots--;		// Cancel last operation
				break;
			}

			LastSlot = pSlot;
		}

		if (!LastSlot)		// Nothing was allocated on that pass, so assume we are out of memory
			break;
	}

	pCacheType = Cache->CacheTypes;
	
	// Go through one last time, and make sure all got allocated
	for (i=0; i< GCACHE_MAX_CACHE_TYPES; i++, pCacheType++)
	{
		if (pCacheType->RefCount <= 0)
		{
			assert(pCacheType->Slots == NULL);
			assert(pCacheType->UsedSlots == 0);
			continue;
		}

		if (pCacheType->UsedSlots <= 0)		// Oops...
			return GE_FALSE;

		assert(pCacheType->Slots != NULL);
	}

#ifdef GCACHE_WRITELOG
	GCache_WriteToFile(Cache, "GCache.Log", AppendHack);

	AppendHack = GE_TRUE;
#endif

	return GE_TRUE;
}

//========================================================================================================
//	GCache_SlotGetMemAddress
//========================================================================================================
uint32 GCache_SlotGetMemAddress(GCache_Slot *Slot)
{
	assert(GCache_SlotIsValid(Slot));

	return Slot->MemAddr;
}

//========================================================================================================
//	GCache_SlotIsValid
//========================================================================================================
geBoolean GCache_SlotIsValid(GCache_Slot *Slot)
{
	if (!Slot)
		return GE_FALSE;

	if (Slot->SelfCheck != Slot)
		return GE_FALSE;

	return GE_TRUE;
}

#ifdef DYNAMIC_CACHE

#define		GROW_DIR_LEFT		0
#define		GROW_DIR_RIGHT		1

#define		MAX_STACK			64

typedef struct
{
	GCache_Type			*Type;
	int32				UsedSlots;
} GCache_TypeStack;

//========================================================================================================
//	GCache_TypeGrowDir
//========================================================================================================
GCache_Slot *GCache_TypeGrowDir(GCache_Type *CacheType, GCache_Type *Current, int32 Dir)
{
	uint32				Size;
	int32				UsedSlots;
	GCache_Type			*Original;
	GCache_TypeStack	Stack[MAX_STACK], *pStack;

	assert(CacheType);
	assert(Current);

	Original = Current;

	Size = Current->Size;
	UsedSlots = Current->UsedSlots;

	pStack = Stack;

	// Do a test run, to see if we can make a new slot
	while (CacheType->Size > Size)
	{
		// CacheType does not have enough memory, need to take more

		if (pStack >= &Stack[MAX_STACK])
			return NULL;		// No more stack space, oh well...

		if (UsedSlots <= 1)		// Keep at least one slot used
		{
			pStack->Type = Current;
			pStack->UsedSlots = UsedSlots;
			pStack++;

			Current = Current->PrevNext[Dir];

			if (!Current)
				return NULL;		// Can't do it

			UsedSlots = Current->UsedSlots;
			continue;
		}

		UsedSlots--;
		Size += Current->Size;
		
		pStack->Type = Current;
		pStack->UsedSlots = UsedSlots;
		pStack++;
	}

	// Go back up the stack, fixing up slots in the types we are gonna grow into...
	while (pStack-- > Stack)
	{
		pStack->Type->UsedSlots = pStack->UsedSlots;
	}
	
	return &Original->Slots[Original->UsedSlots++];
}

//========================================================================================================
//	GCache_TypeGrow
//	Trys to grow to the left or to the right by stealing one of it's neigbors
//========================================================================================================
GCache_Slot *GCache_TypeGrow(GCache_Type *CacheType)
{
	GCache_Type		**PrevNext;
	GCache_Slot		*Slot;
	int32			Dir;

	PrevNext = CacheType->PrevNext;

	assert(PrevNext[0] || PrevNext[1]);		// At least one must be initialized

	if (!PrevNext[0])					
		return GCache_TypeGrowDir(CacheType, PrevNext[1], GROW_DIR_RIGHT);
	if (!PrevNext[1])
		return GCache_TypeGrowDir(CacheType, PrevNext[0], GROW_DIR_LEFT);

	// Pick the best side first (the one with the biggest texture size)
	if (PrevNext[GROW_DIR_LEFT]->Size > PrevNext[GROW_DIR_RIGHT]->Size)
		Dir = GROW_DIR_LEFT;
	else
		Dir = GROW_DIR_RIGHT;
		
	Slot = GCache_TypeGrowDir(CacheType, PrevNext[Dir], Dir);

	if (!Slot)		// Try opposite direction
		Slot = GCache_TypeGrowDir(CacheType, PrevNext[!Dir], !Dir);

	return Slot;
}

#endif

//========================================================================================================
//	GCache_TypeFindSlot
//========================================================================================================
GCache_Slot	*GCache_TypeFindSlot(GCache_Type *CacheType)
{
	GCache_Slot		*pBestSlot, *pSlot;
	uint32			BestLRU;
	int32			i;

	assert(CacheType->Slots);

#ifdef DYNAMIC_CACHE
	// First, try to steal a slot from one of our neigbors...
	// Try the biggest side first 
	if (pBestSlot = GCache_TypeGrow(CacheType))
		goto GotIt;
#endif

	pSlot = CacheType->Slots;
	pBestSlot = pSlot;
	BestLRU = pBestSlot->LRU;

	for (i=0; i< CacheType->UsedSlots; i++, pSlot++)
	{
		if (pSlot->LRU < BestLRU)
		{
			pBestSlot = pSlot;
			BestLRU = pSlot->LRU;
		}
	}

#ifdef DYNAMIC_CACHE
	GotIt:
#endif

	pBestSlot->LRU = 0;
	pBestSlot->UserData = NULL;

	return pBestSlot;
}

//========================================================================================================
//	GCache_SlotSetUserData
//========================================================================================================
void GCache_SlotSetUserData(GCache_Slot *Slot, void *UserData)
{
	assert(GCache_SlotIsValid(Slot));

	Slot->UserData = UserData;
}

//========================================================================================================
//	GCache_SlotGetUserData
//========================================================================================================
void *GCache_SlotGetUserData(GCache_Slot *Slot)
{
	assert(GCache_SlotIsValid(Slot));

	return Slot->UserData;
}

//========================================================================================================
//	GCache_SlotGetInfo
//========================================================================================================
GrTexInfo *GCache_SlotGetInfo(GCache_Slot *Slot)
{
	assert(GCache_SlotIsValid(Slot));

	return &Slot->Type->Info;
}

//========================================================================================================
//	GCache_SlotSetLRU
//========================================================================================================
void GCache_SlotSetLRU(GCache_Slot *Slot, uint32 LRU)
{
	assert(GCache_SlotIsValid(Slot));

	Slot->LRU = LRU;
}
