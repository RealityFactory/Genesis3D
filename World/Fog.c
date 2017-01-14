/****************************************************************************************/
/*  Fog.c                                                                               */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <Assert.h>

#include "Fog.h"

//================================================================================
//	geFog_Create
//================================================================================
GENESISAPI geFog *geFog_Create(SET_ATTR_CB *SetAttrCB)
{
	geFog	*Fog;

	Fog = GE_RAM_ALLOCATE_STRUCT(geFog);

	if (!Fog)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return NULL;
	}

	memset(Fog, 0, sizeof(geFog));

	if (SetAttrCB)
		Fog->SetAttrCB = SetAttrCB;
	
	return Fog;
}

//================================================================================
//	geFog_Destroy
//================================================================================
GENESISAPI void geFog_Destroy(geFog *Fog)
{
	assert(Fog);

	geRam_Free(Fog);
}

//================================================================================
//	geFog_SetAttributes
//================================================================================
GENESISAPI geBoolean geFog_SetAttributes(	geFog			*Fog, 
											const geVec3d	*Pos, 
											GE_RGBA			*Color,
											float			LightBrightness, 
											float			VolumeBrightness, 
											float			VolumeRadius)
{
	Fog->Pos = *Pos;
	Fog->Color.r = Color->r*(1.0f/255.0f)*(1<<8);
	Fog->Color.g = Color->g*(1.0f/255.0f)*(1<<8);
	Fog->Color.b = Color->b*(1.0f/255.0f)*(1<<8);
	Fog->LightBrightness = LightBrightness;
	Fog->VolumeBrightness = VolumeBrightness;
	Fog->VolumeRadius = VolumeRadius;
	Fog->VolumeRadiusSquared = VolumeRadius*VolumeRadius;
	Fog->VolumeRadius2 = VolumeRadius*2;

	// Now that all is set, call the CB if it exist...
	if (Fog->SetAttrCB)
	{
		if (!Fog->SetAttrCB(Fog))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//================================================================================
//	geFog_SetUserData
//================================================================================
GENESISAPI geBoolean geFog_SetUserData(geFog *Fog, void *UserData)
{
	assert(Fog);

	Fog->UserData = UserData;

	return GE_TRUE;
}

//================================================================================
//	geFog_GetUserData
//================================================================================
GENESISAPI void *geFog_GetUserData(geFog *Fog)
{
	assert(Fog);

	return Fog->UserData;
}

