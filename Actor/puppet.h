/****************************************************************************************/
/*  PUPPET.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Puppet interface.										.				*/
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
#ifndef GE_PUPPET_H
#define GE_PUPPET_H

#include "Motion.h"
#include "Camera.h"
#include "Body.h"
#include "Pose.h"
#include "ExtBox.h"			// geExtBox for gePuppet_RenderThroughFrustum

#include "Frustum.h"
#include "vfile.h"


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

geBoolean GENESISCC gePuppet_Render(const gePuppet *P,
					const gePose *Joints,
					geEngine *Engine, 
					geWorld *World,
					const geCamera *Camera, 
					geExtBox *Box);

int GENESISCC gePuppet_GetMaterialCount( gePuppet *P );
geBoolean GENESISCC gePuppet_GetMaterial( gePuppet *P, int MaterialIndex,
									geBitmap **Bitmap, 
									geFloat *Red, geFloat *Green, geFloat *Blue);
geBoolean GENESISCC gePuppet_SetMaterial(gePuppet *P, int MaterialIndex, geBitmap *Bitmap, 
										geFloat Red, geFloat Green, geFloat Blue);

void GENESISCC gePuppet_SetShadow(gePuppet *P, geBoolean DoShadow, geFloat Scale, 
						const geBitmap *ShadowMap,int BoneIndex);

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

