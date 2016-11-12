/****************************************************************************************/
/*  PUPPET.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Puppet interface.										.				*/
/*                                                                                      */
/*  Edit History:                                                                       */
/*  03/24/2004 Wendell Buckner                                                          */
/*   BUG FIX: Rendering Transparent Polys properly (2)                                  */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                    */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved                            */
/*                                                                                      */
/****************************************************************************************/
#ifndef GE_PUPPET_H
#define GE_PUPPET_H

#include "Motion.h"
#include "Camera.h"
#include "Body.h"
#include "Pose.h"
#include "ExtBox.h"			// geExtBox for gePuppet_RenderThroughFrustum
#include "Frustum.h"
#include "VFile.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct gePuppet gePuppet;

gePuppet *GENESISCC gePuppet_Create(geVFile *TextureFS, const geBody *B, geWorld *World);

void GENESISCC gePuppet_Destroy(gePuppet **P);

geBoolean GENESISCC gePuppet_RenderThroughFrustum(const gePuppet *P, 
					const gePose *Joints, 
					const geExtBox *Box, 
					geEngine *Engine, 
					geWorld *World,
					const geCamera *Camera, 
					Frustum_Info *FInfo);

// changed QD Clipping
geBoolean GENESISCC gePuppet_Render(const gePuppet *P,
					const gePose *Joints,
					geEngine *Engine, 
					geWorld *World,
					const geCamera *Camera, 
					geExtBox *Box,
					Frustum_Info *FInfo);

// changed QD Shadows
geBoolean GENESISCC gePuppet_RenderShadowVolume(gePuppet *P,
					const gePose *Joints,
					geEngine *Engine, 
					geWorld *World,
					const geCamera *Camera,
					GFX_Plane* FPlanes,//Frustum_Info *FrustumInfo,
					geVec3d *LightPos,
					geFloat Radius,
					int LightType, 
					geVec3d* Dir, 
					geFloat Arc,
					geBoolean ZPass);

void GENESISCC gePuppet_BodyGeometryNeedsUpdate(gePuppet *P);

void GENESISCC gePuppet_SetStencilShadow(gePuppet *P, geBoolean DoStencilShadow);

// end change QD Shadows

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
geBoolean GENESISCC gePuppet_AddToGList ( const GE_TLVertex *Points, int NumPoints, const geBitmap *Bitmap, uint32 Flags, geBoolean Flush );
geBoolean GENESISCC gePuppet_IsTransparent ( const geFloat OverallAlpha, const geBitmap **Bitmap, geBoolean *IsTransparent, uint32 Count );

int GENESISCC gePuppet_GetMaterialCount( gePuppet *P );
geBoolean GENESISCC gePuppet_GetMaterial( gePuppet *P, int MaterialIndex,
									geBitmap **Bitmap, 
									geFloat *Red, geFloat *Green, geFloat *Blue);
geBoolean GENESISCC gePuppet_SetMaterial(gePuppet *P, int MaterialIndex, geBitmap *Bitmap, 
										geFloat Red, geFloat Green, geFloat Blue);

void GENESISCC gePuppet_SetShadow(gePuppet *P, geBoolean DoShadow, geFloat Scale, 
						const geBitmap *ShadowMap,int BoneIndex);

// LWM_ACTOR_RENDERING
geFloat GENESISCC gePuppet_GetAlpha( const gePuppet *P ) ;
// LWM_ACTOR_RENDERING
void GENESISCC gePuppet_SetAlpha( gePuppet *P, geFloat Alpha ) ;
//Environment mapping code...
void GENESISCC gePuppet_SetEnvironmentOptions( gePuppet *P, geEnvironmentOptions *envop );

geEnvironmentOptions GENESISCC gePuppet_GetEnvironmentOptions( gePuppet *P );

void GENESISCC gePuppet_GetStaticLightingOptions(const gePuppet *P,	geBoolean *AmbientLightFromStaticLights,	geBoolean *TestRayCollision,	int *MaxStaticLightsToUse	);	
void GENESISCC gePuppet_SetStaticLightingOptions(gePuppet *P,	geBoolean AmbientLightFromStaticLights,	geBoolean TestRayCollision,	int MaxStaticLightsToUse	);

void GENESISCC gePuppet_GetLightingOptions(const gePuppet *P,
	geBoolean *UseFillLight,
	geVec3d *FillLightNormal,
	geFloat *FillLightRed,				
	geFloat *FillLightGreen,				
	geFloat *FillLightBlue,				
	geFloat *AmbientLightRed,			
	geFloat *AmbientLightGreen,			
	geFloat *AmbientLightBlue,			
	geBoolean *UseAmbientLightFromFloor,
	int *MaximumDynamicLightsToUse,		
	int *LightReferenceBoneIndex,
	geBoolean *PerBoneLighting
	);

void GENESISCC gePuppet_SetLightingOptions(gePuppet *P,
	geBoolean UseFillLight,
	const geVec3d *FillLightNormal,
	geFloat FillLightRed,				// 0 .. 255
	geFloat FillLightGreen,				// 0 .. 255
	geFloat FillLightBlue,				// 0 .. 255
	geFloat AmbientLightRed,			// 0 .. 255
	geFloat AmbientLightGreen,			// 0 .. 255
	geFloat AmbientLightBlue,			// 0 .. 255
	geBoolean AmbientLightFromFloor,
	int MaximumDynamicLightsToUse,		// 0 for none
	int LightReferenceBoneIndex,
	int PerBoneLighting);

#ifdef __cplusplus
}
#endif


#endif

