/****************************************************************************************/
/*  Camera.h                                                                            */
/*                                                                                      */
/*  Author: John Pollard/Charles Bloom                                                  */
/*  Description: Creation/Transformation/projection code for a camera                   */
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
#ifndef GE_CAMERA_H
#define GE_CAMERA_H

#include "BaseType.h"
#include "Vec3d.h"
#include "XForm3d.h"
#include "GETypes.h"

#ifdef __cplusplus
extern "C" {
#endif

//================================================================================
//	Structure defines
//================================================================================
typedef struct geCamera geCamera;


//================================================================================
//	Function ProtoTypes
//================================================================================
GENESISAPI geCamera *GENESISCC geCamera_Create(geFloat Fov, const geRect *Rect);
GENESISAPI void GENESISCC geCamera_Destroy(geCamera **pCamera);
GENESISAPI void GENESISCC geCamera_SetZScale(geCamera *Camera, geFloat ZScale);
GENESISAPI geFloat GENESISCC geCamera_GetZScale(const geCamera *Camera);
GENESISAPI void GENESISCC geCamera_SetFarClipPlane(geCamera *Camera, geBoolean Enable, geFloat ZFar);
GENESISAPI void GENESISCC geCamera_GetFarClipPlane(const geCamera *Camera, geBoolean *Enable, geFloat *ZFar);
GENESISAPI void GENESISCC geCamera_GetClippingRect(const geCamera *Camera, geRect *Rect);
void GENESISCC geCamera_GetWidthHeight(const geCamera *Camera,geFloat *Width,geFloat *Height);
float GENESISCC geCamera_GetScale(const geCamera *Camera);
GENESISAPI void GENESISCC geCamera_SetAttributes(geCamera *Camera, geFloat Fov, const geRect *Rect);
void geCamera_FillDriverInfo(geCamera *Camera);
GENESISAPI void GENESISCC geCamera_ScreenPointToWorld (	const geCamera	*Camera,
														int32			 ScreenX,
														int32			 ScreenY,
														geVec3d			*Vector);
GENESISAPI void GENESISCC geCamera_Project(const geCamera *Camera, 
								const geVec3d *PointInCameraSpace, 
								geVec3d *ProjectedPoint);
GENESISAPI void GENESISCC geCamera_ProjectZ(const geCamera *Camera, 
								const geVec3d *PointInCameraSpace, 
								geVec3d *ProjectedPoint);
void GENESISCC geCamera_ProjectAndClamp(const geCamera *Camera, 
										const geVec3d *PointInCameraSpace, 
										geVec3d *ProjectedPoint);
void GENESISCC geCamera_GetViewAngleXSinCos( const geCamera *Camera, geFloat *SinAngle, geFloat *CosAngle );
void GENESISCC geCamera_GetViewAngleYSinCos( const geCamera *Camera, geFloat *SinAngle, geFloat *CosAngle );
GENESISAPI void GENESISCC geCamera_Transform(const geCamera *Camera, 
						const geVec3d *WorldSpacePoint, 
						      geVec3d *CameraSpacePoint);
GENESISAPI void GENESISCC geCamera_TransformArray(const geCamera *Camera, 
						const geVec3d *WorldSpacePointPtr, 
						      geVec3d *CameraSpacePointPtr, int count);
GENESISAPI void GENESISCC geCamera_TransformAndProjectArray(const geCamera *Camera, 
						const geVec3d *WorldSpacePointPtr, 
						      geVec3d *ProjectedSpacePointPtr, int count);
GENESISAPI void GENESISCC geCamera_TransformAndProjectLArray(const geCamera *Camera, 
						const GE_LVertex *WorldSpacePointPtr, 
						      GE_TLVertex *ProjectedSpacePointPtr, int count);
GENESISAPI void GENESISCC geCamera_TransformAndProject(const geCamera *Camera,
								const geVec3d *Point, 
								geVec3d *ProjectedPoint);
GENESISAPI void GENESISCC geCamera_TransformAndProjectL(const geCamera *Camera,
								const GE_LVertex *Point, 
								GE_TLVertex *ProjectedPoint);

GENESISAPI geBoolean GENESISCC geCamera_SetWorldSpaceXForm(geCamera *Camera, const geXForm3d *XForm);
GENESISAPI geBoolean GENESISCC geCamera_SetWorldSpaceVisXForm(geCamera *Camera, const geXForm3d *XForm);
GENESISAPI const geXForm3d * GENESISCC geCamera_GetWorldSpaceXForm( const geCamera *Camera);
const geXForm3d * GENESISCC geCamera_GetCameraSpaceXForm( const geCamera *Camera);
GENESISAPI const geXForm3d * GENESISCC geCamera_GetCameraSpaceVisXForm( const geCamera *Camera);
GENESISAPI const geXForm3d * GENESISCC geCamera_GetWorldSpaceVisXForm( const geCamera *Camera);
const geVec3d *GENESISCC geCamera_GetPov(const geCamera *Camera);
const geVec3d *GENESISCC geCamera_GetVisPov(const geCamera *Camera);
GENESISAPI geBoolean GENESISCC geCamera_ConvertWorldSpaceToCameraSpace(const geXForm3d *WXForm, geXForm3d *CXForm);

#ifdef __cplusplus
}
#endif

#endif
