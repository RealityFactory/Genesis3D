/****************************************************************************************/
/*  Sound3D.h                                                                           */
/*                                                                                      */
/*  Author: Brian Adelberg                                                              */
/*  Description: 3D Sound code                                                          */
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
#ifndef GE_SOUND3D_H
#define GE_SOUND3D_H

#include "BaseType.h"
#include "Sound.h"

#ifdef __cplusplus
extern "C" {
#endif

// GENESIS_PUBLIC_APIS

GENESISAPI	void geSound3D_GetConfig(
			const geWorld *World, 
			const geXForm3d *CameraTransform, 
			const geVec3d *SoundPos, 
			geFloat Min, 
			geFloat Ds,
			geFloat *Volume,
			geFloat *Pan,
			geFloat *Frequency);

GENESISAPI	void geSound3D_GetConfigIgnoreObstructions(
		const geWorld *World, 
		const geXForm3d *MXForm, 
		const geVec3d *SndPos, 
		geFloat Min, 
		geFloat Ds,
		geFloat *Volume,
		geFloat *Pan,
		geFloat *Frequency);

// GENESIS_PRIVATE_APIS

#ifdef __cplusplus
}
#endif

#endif
