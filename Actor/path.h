/****************************************************************************************/
/*  PATH.H																				*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Time-indexed keyframe creation, maintenance, and sampling.				*/
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
#ifndef GE_PATH_H
#define GE_PATH_H

#include "basetype.h"
#include "xform3d.h"
#include "quatern.h"
#include "vfile.h"

#ifdef __cplusplus
	extern "C" {
#endif


// GENESIS_PUBLIC_APIS
typedef struct _gePath gePath;

#define GE_PATH_ROTATION_CHANNEL    1
#define GE_PATH_TRANSLATION_CHANNEL 2

#define GE_PATH_ALL_CHANNELS (GE_PATH_ROTATION_CHANNEL | GE_PATH_TRANSLATION_CHANNEL)

#ifndef GE_PATH_ENUMS
	#define GE_PATH_ENUMS
	typedef enum 
	{
		GE_PATH_INTERPOLATE_LINEAR  = 0,	// linear blend for translation or rotation channel
		GE_PATH_INTERPOLATE_HERMITE,		// hermite cubic spline for translation channel
		GE_PATH_INTERPOLATE_SLERP,			// spherical-linear blend for rotation channel
		GE_PATH_INTERPOLATE_SQUAD,			// higher order blend for rotation channel 'G1' continuity
		//GE_PATH_INTEROPLATE_TRIPOD,		 // not supported yet.
		GE_PATH_INTERPOLATE_HERMITE_ZERO_DERIV = 7	// hermite cubic with zero derivative at keyframes ('easing' curve)
	}gePath_Interpolator;
#endif

GENESISAPI void GENESISCC gePath_CreateRef( gePath *P );

GENESISAPI gePath *GENESISCC gePath_Create(
	gePath_Interpolator TranslationInterpolation,	// type of interpolation for translation channel
	gePath_Interpolator RotationInterpolation,	// type of interpolation for rotation channel
	geBoolean Looped);				// True if end of path is connected to head
	// creates new gePath
	//  A looping path should have the same first & last point.  The path
	//  generator will choose arbitrarily between these points for a 
	//  sample exactly at the end of the loop.

GENESISAPI gePath *GENESISCC gePath_CreateCopy( const gePath *P );
	
GENESISAPI void GENESISCC gePath_Destroy(gePath **PP);		
	// destroys path *PP

//------------------ time based keyframe operations
GENESISAPI geBoolean GENESISCC gePath_InsertKeyframe(
	gePath *P, 
	int ChannelMask, 
	geFloat Time, 
	const geXForm3d *Matrix); 
	// inserts a keyframe at a specific time.
	
GENESISAPI geBoolean GENESISCC gePath_DeleteKeyframe(
	gePath *P,
	int Index,
	int ChannelMask); 
	// deletes the nth keyframe

GENESISAPI geBoolean GENESISCC gePath_GetTimeExtents(
	const gePath *P,
	geFloat *StartTime, 
	geFloat *EndTime);
	// gets the time for the first and last keys in the path (ignoring looping)
	// if there are no keys, return GE_FALSE and times are not set.
	// returns GE_TRUE if there are keys.

//----------------- index based keyframe operations
GENESISAPI void GENESISCC gePath_GetKeyframe(
	const gePath *P, 
	int Index,				// gets keyframe[index]
	int Channel,			// for this channel
	geFloat *Time,			// returns the time of the keyframe
	geXForm3d *Matrix);		// returns the matrix of the keyframe
	// retrieves keyframe[index], and it's time

GENESISAPI int GENESISCC gePath_GetKeyframeCount(const gePath *P,int Channel);
	// retrieves count of keyframes for a specific channel

GENESISAPI int GENESISCC gePath_GetKeyframeIndex(const gePath *P, int Channel, geFloat Time);
	// retrieves the index of the keyframe at a specific time for a specific channel

//----------------- sampling a path  (time based)
GENESISAPI void GENESISCC gePath_Sample(const gePath *P, geFloat Time,geXForm3d *Matrix);
	// returns a transform matrix sampled at 'Time'.
	// p is not const because information is cached in p for next sample

// GENESIS_PRIVATE_APIS
void GENESISCC gePath_SampleChannels(
	const gePath *P, 
	geFloat Time, 
	geQuaternion *Rotation, 
	geVec3d *Translation);
	// returns a rotation and a translation for the path at 'Time'
	// p is not const because information is cached in p for next sample

GENESISAPI geBoolean GENESISCC gePath_OffsetTimes(gePath *P, 
	int StartingIndex, int ChannelMask, geFloat TimeOffset );
		// slides all samples in path starting with StartingIndex down by TimeOffset

GENESISAPI geBoolean GENESISCC gePath_ModifyKeyframe(
	gePath *P,
	int Index,
	int ChannelMask,
	const geXForm3d *Matrix);
	

// GENESIS_PUBLIC_APIS

//------------------ saving/loading a path
GENESISAPI gePath* GENESISCC gePath_CreateFromFile(geVFile *F);
	// loads a file  (binary or ascii)

GENESISAPI geBoolean GENESISCC gePath_WriteToFile(const gePath *P, geVFile *F);
	// dumps formatted ascii to the file.  

GENESISAPI geBoolean GENESISCC gePath_WriteToBinaryFile(const gePath *P, geVFile *F);
	// dumps a minimal binary image for fastest reading



#ifdef __cplusplus
	}
#endif


#endif
