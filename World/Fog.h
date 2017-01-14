/****************************************************************************************/
/*  Fog.h                                                                               */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Fog module                                                             */
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
#ifndef GE_FOG_H
#define GE_FOG_H

#include <Assert.h>

#include "Vec3d.h"
#include "BaseType.h"
#include "GeTypes.h"
#include "Ram.h"
#include "Errorlog.h"

#ifdef __cplusplus
extern "C" {
#endif

//=====================================================================================
//	Defines / Structure defines
//=====================================================================================
typedef struct		geFog		geFog;

typedef geBoolean	SET_ATTR_CB(geFog *Fog);

typedef struct geFog
{
	geVec3d			Pos;
	GE_RGBA			Color;
	float			LightBrightness;
	float			VolumeBrightness;
	float			VolumeRadius;
	float			VolumeRadius2;				// *2
	float			VolumeRadiusSquared;		// Radius squared

	void			*UserData;	

	SET_ATTR_CB		*SetAttrCB;					// CB for when geFog_SetAttributes is called

	struct geFog	*Next;
	struct geFog	*Prev;
} geFog;

//=====================================================================================
//	Function ProtoTypes
//=====================================================================================
GENESISAPI		geFog *geFog_Create(SET_ATTR_CB *SetAttrCB);

GENESISAPI		void geFog_Destroy(geFog *Fog);

GENESISAPI		geBoolean geFog_SetAttributes(	geFog			*Fog, 
												const geVec3d	*Pos, 
												GE_RGBA			*Color,
												float			LightBrightness, 
												float			VolumeBrightness, 
												float			VolumeRadius);

GENESISAPI		geBoolean geFog_SetUserData(geFog *Fog, void *UserData);
GENESISAPI		void *geFog_GetUserData(geFog *Fog);

#ifdef __cplusplus
}
#endif

#endif
