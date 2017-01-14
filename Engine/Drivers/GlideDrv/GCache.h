/****************************************************************************************/
/*  GCache.h                                                                            */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef GCACHE_H
#define GCACHE_H

#include <Windows.h>
#include "Glide.h"

#include "BaseType.h"
#include "GMemMGr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GCACHE_MAX_NAME			256

typedef struct GCache			GCache;
typedef struct GCache_Type		GCache_Type;
typedef struct GCache_Slot		GCache_Slot;


GCache		*GCache_Create(const char *Name, GMemMgr *MemMgr);
void		GCache_Destroy(GCache *Cache);
geBoolean	GCache_Reset(GCache *Cache);
GCache_Type *GCache_FindCacheTypeByInfo(GCache *Cache, const GrTexInfo *Info);
GCache_Type *GCache_InsertCacheTypeByInfo(GCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, const GrTexInfo *Info);
geBoolean	GCache_UpdateSlot(GCache *Cache, GCache_Slot *Slot, GrTexInfo *Info);
geBoolean	GCache_SetTexture(GCache *Cache, GCache_Slot *Slot);
GCache_Type *GCache_TypeCreate(GCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, const GrTexInfo *Info);
void		GCache_TypeDestroy(GCache_Type *CacheType);
geBoolean	GCache_FreeAllSlots(GCache *Cache);
geBoolean	GCache_AdjustSlots(GCache *Cache);
uint32		GCache_SlotGetMemAddress(GCache_Slot *Slot);
uint32		GCache_SlotGetMemAddress(GCache_Slot *Slot);
geBoolean	GCache_SlotIsValid(GCache_Slot *Slot);
GCache_Slot	*GCache_TypeFindSlot(GCache_Type *CacheType);
void		GCache_SlotSetUserData(GCache_Slot *Slot, void *UserData);
void		*GCache_SlotGetUserData(GCache_Slot *Slot);
GrTexInfo	*GCache_SlotGetInfo(GCache_Slot *Slot);
void		GCache_SlotSetLRU(GCache_Slot *Slot, uint32 LRU);

#ifdef __cplusplus
}
#endif

#endif