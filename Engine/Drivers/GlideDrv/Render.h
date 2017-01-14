/****************************************************************************************/
/*  Render.h                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Code to render polys in glide                                          */
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
#ifndef RENDER_H
#define RENDER_H

#include <Windows.h>

#include "BaseType.h"
#include "Glide.h"
#include "DCommon.h"
#include "GCAche.h"

#ifdef __cplusplus
extern "C" {
#endif

enum 
{  
	RENDER_UNKNOWN_MODE,
	RENDER_MISC_TEX_POLY_MODE, 
	RENDER_MISC_GOURAD_POLY_MODE, 
	RENDER_LINES_POLY_MODE, 
	RENDER_WORLD_TRANSPARENT_POLY_MODE,
	RENDER_WORLD_POLY_MODE_NO_LIGHTMAP,
	RENDER_WORLD_POLY_MODE,
	RENDER_LIGHTMAP_POLY_MODE,
	RENDER_LIGHTMAP_FOG_POLY_MODE,
	RENDER_DECAL_MODE,
};

extern uint32				PolyMode;
extern DRV_CacheInfo		CacheInfo;

void TextureSource(GrChipID_t Tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo  *info );
void Render_SetHardwareMode(int32 NewMode, uint32 NewFlags);
geBoolean DRIVERCC Render_GouraudPoly(DRV_TLVertex *Pnts, int32 NumPoints, uint32 Flags);
geBoolean DRIVERCC Render_LinesPoly(DRV_TLVertex *Pnts, int32 NumPoints);
geBoolean DRIVERCC Render_WorldPoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags);
void RenderLightmapPoly(GrVertex *vrtx, int32 NumPoints, DRV_LInfo *LInfo, geBoolean Dynamic, uint32 Flags);
void DownloadLightmap(DRV_LInfo *LInfo, int32 Wh, GCache_Slot *Slot, int32 LMapNum);
geBoolean DRIVERCC Render_MiscTexturePoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, uint32 Flags);
void SetupTexture(geRDriver_THandle *THandle);
GCache_Slot *SetupLMapTexture(geRDriver_THandle *THandle, DRV_LInfo *LInfo, geBoolean Dynamic, int32 LMapNum);
geBoolean DRIVERCC Render_DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y);
geBoolean DRIVERCC BeginScene(geBoolean Clear, geBoolean ClearZ, RECT *WorldRect);
geBoolean DRIVERCC EndScene(void);
geBoolean DRIVERCC BeginWorld(void);
geBoolean DRIVERCC EndWorld(void);
geBoolean DRIVERCC BeginMeshes(void);
geBoolean DRIVERCC EndMeshes(void);
geBoolean DRIVERCC BeginModels(void);
geBoolean DRIVERCC EndModels(void);

//============================================================================================
//============================================================================================

#ifdef __cplusplus
}
#endif

#endif