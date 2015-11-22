/****************************************************************************************/
/*  Render.h                                                                            */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Polygon rasterization functions for OpenGL driver                      */
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
/*                                                                                      */
/****************************************************************************************/

#ifndef RENDER_H
#define RENDER_H

#include "DCommon.h"

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
	RENDER_WORLD_POLY_MODE,
	RENDER_LIGHTMAP_POLY_MODE,
	RENDER_LIGHTMAP_FOG_POLY_MODE,
	RENDER_DECAL_MODE,
};

extern uint32				PolyMode;
extern DRV_CacheInfo		CacheInfo;
extern GLint decalTexObj;

void Render_SetHardwareMode(int32 NewMode, uint32 NewFlags);
geBoolean DRIVERCC Render_GouraudPoly(DRV_TLVertex *Pnts, int32 NumPoints, uint32 Flags);
geBoolean DRIVERCC Render_WorldPoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags);
geBoolean DRIVERCC Render_MiscTexturePoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, uint32 Flags);
// changed QD Shadows
geBoolean DRIVERCC Render_StencilPoly(DRV_XYZVertex *Pnts, int32 NumPoints, uint32 Flags);
geBoolean DRIVERCC DrawShadowPoly(geFloat r, geFloat g, geFloat b, geFloat a);
// end change
geBoolean DRIVERCC DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y);
// changed QD Shadows
//geBoolean DRIVERCC BeginScene(geBoolean Clear, geBoolean ClearZ, RECT *WorldRect);
geBoolean DRIVERCC BeginScene(geBoolean Clear, geBoolean ClearZ, geBoolean ClearStencil, RECT *WorldRect);
// end change
geBoolean DRIVERCC EndScene(void);
geBoolean DRIVERCC BeginWorld(void);
geBoolean DRIVERCC EndWorld(void);
geBoolean DRIVERCC BeginMeshes(void);
geBoolean DRIVERCC EndMeshes(void);
geBoolean DRIVERCC BeginModels(void);
geBoolean DRIVERCC EndModels(void);
// changed QD Shadows
geBoolean DRIVERCC BeginShadowVolumes(void);
geBoolean DRIVERCC EndShadowVolumes(void);
// end change

#ifdef __cplusplus
}
#endif

#endif