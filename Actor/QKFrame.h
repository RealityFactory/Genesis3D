/****************************************************************************************/
/*  QKFRAME.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Quaternion keyframe interface.											*/
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
/* geQKFrame   (geQuaternion - Keyframe)
	This module handles interpolation for keyframes that contain a quaternion
	This is intended to support Path.c
	geTKArray supplies general support for a time-keyed array, and this supplements
	that support to include the specific time-keyed arrays:
	  An array of geQuaternion interpolated linearly
	  An array of geQuaternion with spherical linear interpolation (SLERP)
	  An array of geQuaternion with spherical quadrangle 
		interpolation (SQUAD) as defined by:
	    Advanced Animation and Rendering Techniques by Alan Watt and Mark Watt

	These are phycially separated and have different base structures because
	the different interpolation techniques requre different additional data.
	
	The two lists are created with different creation calls,
	interpolated with different calls, but insertion and queries share a call.
	
	Quadrangle interpolation requires additional computation after changes are
	made to the keyframe list.  Call geQKFrame_SquadRecompute() to update the
	calculations.
*/
#ifndef GE_QKFRAME_H
#define GE_QKFRAME_H


#include "TKArray.h"
#include "Quatern.h"
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	QKFRAME_LINEAR,
	QKFRAME_SLERP,
	QKFRAME_SQUAD
} geQKFrame_InterpolationType;


geTKArray *GENESISCC geQKFrame_LinearCreate(void);
	// creates a frame list for linear interpolation

geTKArray *GENESISCC geQKFrame_SlerpCreate();
	// creates a frame list for spherical linear interpolation	

geTKArray *GENESISCC geQKFrame_SquadCreate();
	// creates a frame list for spherical linear interpolation	


geBoolean GENESISCC geQKFrame_Insert(
	geTKArray **KeyList,			// keyframe list to insert into
	geTKArray_TimeType Time,		// time of new keyframe
	const geQuaternion *Q,			// quaternion at new keyframe
	int *Index);					// index of new frame
	// inserts a new keyframe with the given time and vector into the list.

void GENESISCC geQKFrame_Query(
	const geTKArray *KeyList,		// keyframe list
	int Index,						// index of frame to return
	geTKArray_TimeType *Time,		// time of the frame is returned
	geQuaternion *V);					// vector from the frame is returned
	// returns the vector and the time at keyframe[index] 

void GENESISCC geQKFrame_Modify(
	geTKArray *KeyList,				// keyframe list
	int Index,						// index of frame to change
	const geQuaternion *Q);			// vector for the new key
	// modifies a vector at keyframe[index]

void GENESISCC geQKFrame_LinearInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result);			// put the result in here (geQuaternion)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates linearly
	
void GENESISCC geQKFrame_SlerpInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result);			// put the result in here (geQuaternion)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates using spherical linear blending

void GENESISCC geQKFrame_SquadInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result);			// put the result in here (geQuaternion)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates using spherical quadratic blending

void GENESISCC geQKFrame_SquadRecompute(
	int Looped,				// if keylist has the first key connected to last key
	geTKArray *KeyList);
	// rebuild precomputed data for keyframe list.

void GENESISCC geQKFrame_SlerpRecompute(
	geTKArray *KeyList);		// list of keys to recompute hermite values for
	// rebuild precomputed data for keyframe list.


geBoolean GENESISCC geQKFrame_LinearRead(geVFile* pFile, void* geQKFrame);
geBoolean GENESISCC geQKFrame_SlerpRead(geVFile* pFile, void* geQKFrame);
geBoolean GENESISCC geQKFrame_SquadRead(geVFile* pFile, void* geQKFrame);

geBoolean GENESISCC geQKFrame_WriteToFile(geVFile *pFile, void *geQKFrame, 
								geQKFrame_InterpolationType InterpolationType, int Looping);
geTKArray *GENESISCC geQKFrame_CreateFromFile(geVFile *pFile, geQKFrame_InterpolationType *InterpolationType, int *Looping);
geTKArray *GENESISCC geQKFrame_CreateFromBinaryFile(geVFile *pFile, int *InterpolationType, int *Looping);
geBoolean GENESISCC geQKFrame_WriteToBinaryFile(geVFile *pFile, geTKArray *KeyList, 
		geQKFrame_InterpolationType InterpolationType, int Looping);



#ifdef __cplusplus
}
#endif


#endif