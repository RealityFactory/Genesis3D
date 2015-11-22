/****************************************************************************************/
/*  D3DCache.h                                                                          */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef D3DCache_H
#define D3DCache_H

#include <Windows.h>
#include <DDraw.h>
#include <D3D.H>

#include "BaseType.h"
#include "DDMemMgr.h"			

#define	D3DCACHE_MAX_NAME			256

typedef struct D3DCache				D3DCache;
typedef struct D3DCache_Type		D3DCache_Type;
typedef struct D3DCache_Slot		D3DCache_Slot;

D3DCache	*D3DCache_Create(const char *Name, LPDIRECTDRAW4 lpDD, DDMemMgr_Partition *Partition, geBoolean UseStages);
void		D3DCache_Destroy(D3DCache *Cache);
geBoolean	D3DCache_IsValid(D3DCache *Cache);
geBoolean	D3DCache_EvictAllSurfaces(D3DCache *Cache);
D3DCache_Type *D3DCache_FindCacheType(D3DCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, int32 Stage, const DDSURFACEDESC2 *ddsd);
D3DCache_Type *D3DCache_InsertCacheType(D3DCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, int32 Stage, const DDSURFACEDESC2 *ddsd);
D3DCache_Type *D3DCache_TypeCreate(D3DCache *Cache, int32 Width, int32 Height, int32 NumMipLevels, int32 Stage, const DDSURFACEDESC2 *ddsd);
void		D3DCache_TypeDestroy(D3DCache_Type *CacheType);
geBoolean	D3DCache_TypeIsValid(D3DCache_Type *Type);
geBoolean	D3DCache_FreeAllSlots(D3DCache *Cache);
geBoolean	D3DCache_AdjustSlots(D3DCache *Cache, const int32 *MaxTable, geBoolean UsePartition);
geBoolean	D3DCache_SlotIsValid(D3DCache_Slot *Slot);
int32		D3DCache_SetupSlot(D3DCache *Cache, D3DCache_Slot *Slot, int32 Width, int32 Height, const DDSURFACEDESC2 *SurfDesc, geBoolean UseStage, int32 Stage);
D3DCache_Slot *D3DCache_TypeFindSlot(D3DCache_Type *CacheType);
void		D3DCache_SlotSetUserData(D3DCache_Slot *Slot, void *UserData);
void		*D3DCache_SlotGetUserData(D3DCache_Slot *Slot);
void		D3DCache_SlotSetLRU(D3DCache_Slot *Slot, uint32 LRU);
uint32		D3DCache_SlotGetLRU(D3DCache_Slot *Slot);
LPDIRECT3DTEXTURE2 D3DCache_SlotGetTexture(D3DCache_Slot *Slot);
LPDIRECTDRAWSURFACE4 D3DCache_SlotGetSurface(D3DCache_Slot *Slot);

uint32 Log2(uint32 P2);
int32 SnapToPower2(int32 Width);
int32 GetLog(int32 Width, int32 Height);

#endif

