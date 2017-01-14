/****************************************************************************************/
/*  Frustum.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Frustum creation/clipping                                              */
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
#ifndef GE_FRUSTUM_H
#define GE_FRUSTUM_H

#include <Assert.h>
//#include <Windows.h>

#include "BaseType.h"
#include "GBSPFile.h"
#include "Vec3d.h"
#include "Camera.h"
#include "XForm3d.h"
#include "Surface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FCP						32			// Max ClipPlanes for frustum...

#define CLIP_PLANE_EPSILON  0.001f

//================================================================================
//	Structure defines
//================================================================================
typedef struct Frustum_Info
{
	int32			NumPlanes;					// Number of frustum planes to clip against
	GFX_Plane		Planes[MAX_FCP];			// Planes to clip against
					
	// Quick LUTS For BBox testing against frustum
	int32			FrustumBBoxIndexes[MAX_FCP*6];
	int32			*pFrustumBBoxIndexes[MAX_FCP];
} Frustum_Info;

//================================================================================
//	Function ProtoTypes
//================================================================================
void Frustum_SetFromCamera(Frustum_Info *Info, geCamera *Camera);
geBoolean Frustum_SetFromPoly(Frustum_Info *Info, geVec3d *Verts, int32 NumVerts, geBoolean Flip);
void Frustum_RotateToWorldSpace(Frustum_Info *In, geCamera *Camera, Frustum_Info *Out);
void Frustum_TransformToWorldSpace(const Frustum_Info *In, const geCamera *Camera, Frustum_Info *Out);

geBoolean gePlane_ClipVertsFannedUVRGB(	const geVec3d *In, const Surf_TexVert *TIn, int32 NumIn, 
										const GFX_Plane *Plane, 
										geVec3d *Out, Surf_TexVert *TOut, int32 *NumOut);

geBoolean Frustum_ClipToPlane(		GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									int32 NumVerts, int32 *OutVerts);
geBoolean Frustum_ClipToPlaneUV(	GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts);

geBoolean Frustum_ClipToPlaneUVRGB(GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts);

geBoolean Frustum_ClipToPlaneUVRGBA(GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts);

geBoolean Frustum_ClipToPlaneRGB(	GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts);

geBoolean Frustum_ClipToPlaneL(		GFX_Plane *pPlane, 
									GE_LVertex *pIn, GE_LVertex *pOut,
									int32 NumVerts, int32 *OutVerts);	// CB added

void Frustum_Project(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera);
void Frustum_ProjectRGB(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera);
void Frustum_ProjectRGBA(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera);
void Frustum_ProjectRGBNoClamp(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera);

geBoolean Frustum_PointsInFrustum(const geVec3d *Pin, const GFX_Plane *Plane, int32 NumVerts, int32 *c);

geBoolean Frustum_PointInFrustum(const Frustum_Info *Fi, const geVec3d *Point, float Radius);

geBoolean Frustum_ClipAllPlanesL(const Frustum_Info * Fi,uint32 ClipFlags,GE_LVertex *Verts, int32 *pNumVerts);


#ifdef __cplusplus
}
#endif

#endif
