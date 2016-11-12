/****************************************************************************************/
/*  PCache.h                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D poly cache                                                         */
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
#ifndef PCACHE_H
#define PCACHE_H

extern DRV_CacheInfo						CacheInfo;

BOOL PCache_InsertWorldPoly(DRV_TLVertex *Verts, int32 NumVerts, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags);
BOOL PCache_FlushWorldPolys(void);

BOOL PCache_FlushMiscPolys(void);
BOOL PCache_InsertMiscPoly(DRV_TLVertex *Verts, int32 NumVerts, geRDriver_THandle *THandle, uint32 Flags);

BOOL PCache_Reset(void);

#endif
