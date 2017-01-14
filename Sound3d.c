/****************************************************************************************/
/*  Sound3D.c                                                                           */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <math.h>

#include "Trace.h"
#include "Vec3d.h"
#include "XForm3d.h"
#include "Camera.h"
#include "Sound3d.h"

// Sound
typedef struct geSound3d_Cfg
{
	geFloat			Volume;
	geFloat			Pan;
	geFloat			Frequency;
} geSound3d_Cfg;


//=====================================================================================
//	Snd3D_RoolOut
//  Snd will drop off so that it is half intensity when twice min distance
//  A 10 db reduction means half the intensity.
//  In our volume routine 0.01 represents a decible
//=====================================================================================
static void geSound3D_RollOut(geSound3d_Cfg *Cfg, float Dist, float Min, float Max)
{
	geFloat Volume;
	assert( Cfg != NULL );

	if( Dist > Max )
	{
		Volume = 0.0f;
	}
	else if( Dist < Min )
	{
		Volume = 1.0f;
	}
	else
		Volume = 1.0f - (Dist/Min - 1)*0.1f;
	Cfg->Volume = Volume;
}

//=====================================================================================
//	Snd3D_Pan
//=====================================================================================
static void geSound3D_Pan(geSound3d_Cfg *Cfg, float FaceOffset )
{
	assert(Cfg != NULL);
	Cfg->Pan = (float)sin((double)FaceOffset )*0.1f;
}

//=====================================================================================
//	Mps is the reletive velocity in  meters per second 
//=====================================================================================
static void geSound3D_Doppler(geSound3d_Cfg *Cfg, float Mps )
{
	//331 mps is the velocity of sound through air in standard conditions.
	assert(Cfg != NULL);
	Cfg->Frequency = 1.0f + (Mps/ 331.0f );
}

//=====================================================================================
//	Snd3D_3DSound
//	This is the position of the sound translated to your view
//	coordinate system
//=====================================================================================
GENESISAPI	void geSound3D_GetConfig(
		const geWorld *World, 
		const geXForm3d *MXForm, 
		const geVec3d *SndPos, 
		geFloat Min, 
		geFloat Ds,
		geFloat *Volume,
		geFloat *Pan,
		geFloat *Frequency)
{
	geVec3d			ViewPos, LocalPos, Dist;
	geSound3d_Cfg	Cfg;
	float			Magnitude;
	geVec3d			Origin = {0.0f, 0.0f, 0.0f};
	geXForm3d		CXForm;
	int32			Leaf1, Leaf2;

	assert( World     != NULL );
	assert( MXForm    != NULL );
	assert( SndPos    != NULL );
	assert( Volume    != NULL );
	assert( Pan       != NULL );
	assert( Frequency != NULL );

	
	LocalPos = MXForm->Translation;
	// Transform the sound to view space
	geCamera_ConvertWorldSpaceToCameraSpace(MXForm, &CXForm);
	geXForm3d_Transform( &CXForm, SndPos, &ViewPos);
	// FIXME: Need to check these and return TRUE or FALSE
	if( !geWorld_GetLeaf((geWorld*)World, &LocalPos, &Leaf1) )
		return;
	if( !geWorld_GetLeaf((geWorld*)World, SndPos, &Leaf2) )
		return;
	
	if (!geWorld_LeafMightSeeLeaf((geWorld*)World, Leaf1, Leaf2, 0))
	{
		Magnitude = 0.0f;
		Dist.X = 0.0f;				// Shut up compiler warning
		Cfg.Volume = 0.0f;
	}
	else
	{
		GE_Collision	Col;

		// Find the distance from the camera to the original light pos
		geVec3d_Subtract(&LocalPos, SndPos, &Dist);

		Magnitude = geVec3d_Length(&Dist);
		
		if (Trace_GEWorldCollision((geWorld*)World, NULL, NULL, &LocalPos, SndPos, GE_CONTENTS_SOLID_CLIP, GE_COLLIDE_MODELS, 0, NULL, NULL, &Col))
			Magnitude *= 1.5f;
		
		geSound3D_RollOut(&Cfg, Magnitude, Min, Min*10);
	}

	geSound3D_Pan(&Cfg, (float)atan2( (double)ViewPos.X, (double)ViewPos.Z ) );
	//Cfg->Frequency = 1.0f;
	geSound3D_Doppler(&Cfg, Ds);

	*Volume    = Cfg.Volume;
	*Pan       = Cfg.Pan;
	*Frequency = Cfg.Frequency;
}


//=====================================================================================
//	geSound3D_GetConfigIgnoreObstructions()
//=====================================================================================
GENESISAPI	void geSound3D_GetConfigIgnoreObstructions(
		const geWorld *World, 
		const geXForm3d *MXForm, 
		const geVec3d *SndPos, 
		geFloat Min, 
		geFloat Ds,
		geFloat *Volume,
		geFloat *Pan,
		geFloat *Frequency)
{
	geVec3d			ViewPos, LocalPos, Dist;
	geSound3d_Cfg	Cfg;
	float			Magnitude;
	geVec3d			Origin = {0.0f, 0.0f, 0.0f};
	geXForm3d		CXForm;
	int32			Leaf1, Leaf2;

	assert( World     != NULL );
	assert( MXForm    != NULL );
	assert( SndPos    != NULL );
	assert( Volume    != NULL );
	assert( Pan       != NULL );
	assert( Frequency != NULL );

	
	LocalPos = MXForm->Translation;
	// Transform the sound to view space
	geCamera_ConvertWorldSpaceToCameraSpace(MXForm, &CXForm);
	geXForm3d_Transform( &CXForm, SndPos, &ViewPos);
	// FIXME: Need to check these and return TRUE or FALSE
	if( !geWorld_GetLeaf((geWorld*)World, &LocalPos, &Leaf1) )
		return;
	if( !geWorld_GetLeaf((geWorld*)World, SndPos, &Leaf2) )
		return;
	
	if (!geWorld_LeafMightSeeLeaf((geWorld*)World, Leaf1, Leaf2, 0))
	{
		Magnitude = 0.0f;
		Dist.X = 0.0f;				// Shut up compiler warning
		Cfg.Volume = 0.0f;
	}
	else
	{
		// Find the distance from the camera to the original light pos
		geVec3d_Subtract(&LocalPos, SndPos, &Dist);

		Magnitude = geVec3d_Length(&Dist);
		
		geSound3D_RollOut(&Cfg, Magnitude, Min, Min*10);
	}

	geSound3D_Pan(&Cfg, (float)atan2( (double)ViewPos.X, (double)ViewPos.Z ) );
	//Cfg->Frequency = 1.0f;
	geSound3D_Doppler(&Cfg, Ds);

	*Volume    = Cfg.Volume;
	*Pan       = Cfg.Pan;
	*Frequency = Cfg.Frequency;
}
