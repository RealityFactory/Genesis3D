/****************************************************************************************/
/*  VKFRAME.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Vector keyframe interface.												*/
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
/* VKFrame (Vector-Keyframe)
	This module handles interpolation for keyframes that contain a vector (a geVec3d)
	This is intended to support Path.c
	geTKArray supplies general support for a time-keyed array, and this supplements
	that support to include the two specific time-keyed arrays:
	  An array of geVec3d interpolated linearly
	  An array of geVec3d interpolated with hermite blending
	These are phycially separated and have different base structures because:
		linear blending requires less data.
		future blending might require more data.
	The two types of lists are created with different creation calls,
	interpolated with different calls, but insertion and queries share a call.

	Hermite interpolation requires additional computation after changes are
	made to the keyframe list.  Call geVKFrame_HermiteRecompute() to update the
	calculations.
*/
#ifndef GE_VKFRAME_H
#define GE_VKFRAME_H

#include "TKArray.h"
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	VKFRAME_LINEAR,
	VKFRAME_HERMITE,
	VKFRAME_HERMITE_ZERO_DERIV,
} geVKFrame_InterpolationType;


geTKArray *GENESISCC geVKFrame_LinearCreate(void);
	// creates a frame list for linear interpolation

geTKArray *GENESISCC geVKFrame_HermiteCreate(void);
	// creates a frame list for hermite interpolation


geBoolean GENESISCC geVKFrame_Insert(
	geTKArray **KeyList,			// keyframe list to insert into
	geTKArray_TimeType Time,		// time of new keyframe
	const geVec3d *V,				// vector at new keyframe
	int *Index);					// indx of new key
	// inserts a new keyframe with the given time and vector into the list.

void GENESISCC geVKFrame_Query(
	const geTKArray *KeyList,		// keyframe list
	int Index,						// index of frame to return
	geTKArray_TimeType *Time,		// time of the frame is returned
	geVec3d *V);						// vector from the frame is returned
	// returns the vector and the time at keyframe[index] 

void GENESISCC geVKFrame_Modify(
	geTKArray *KeyList,				// keyframe list
	int Index,						// index of frame to change
	const geVec3d *V);				// vector for the key
	// chganes the vector at keyframe[index] 

void GENESISCC geVKFrame_LinearInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result);			// put the result in here (geVec3d)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates linearly

void GENESISCC geVKFrame_HermiteInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result);			// put the result in here (geVec3d)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates using 'hermite' blending


void GENESISCC geVKFrame_HermiteRecompute(
	int Looped,					// if keylist has the first key connected to last key
	geBoolean ZeroDerivative,	// if each key should have a zero derivatives (good for 2 point S curves)
	geTKArray *KeyList);		// list of keys to recompute hermite values for
	// rebuild precomputed data for keyframe list.


geBoolean GENESISCC geVKFrame_LinearRead(geVFile* pFile, void* geVKFrame);
geBoolean GENESISCC geVKFrame_HermiteRead(geVFile* pFile, void* geVKFrame);

geBoolean GENESISCC geVKFrame_WriteToFile(geVFile *pFile, void *geVKFrame, 
								geVKFrame_InterpolationType InterpolationType,int Looping);
geTKArray *GENESISCC geVKFrame_CreateFromFile(geVFile *pFile, geVKFrame_InterpolationType *InterpolationType, int *Looping);
geBoolean GENESISCC geVKFrame_WriteToBinaryFile(geVFile *pFile, void *geVKFrame, 
								geVKFrame_InterpolationType InterpolationType, int Looping);
geTKArray *GENESISCC geVKFrame_CreateFromBinaryFile(geVFile *pFile, geVKFrame_InterpolationType *InterpolationType, int *Looping);

#ifdef __cplusplus
}
#endif

#endif