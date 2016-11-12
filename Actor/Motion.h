/****************************************************************************************/
/*  MOTION.H	                                                                        */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Motion interface.					                                    */
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
#ifndef GE_MOTION_H
#define GE_MOTION_H

/*	motion

	This object is a list of named Path objects

*/

#include <stdio.h>
#include "basetype.h"
#include "path.h"
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

// GENESIS_PUBLIC_APIS
typedef struct geMotion geMotion;

GENESISAPI geMotion *GENESISCC geMotion_Create(geBoolean ManageNames);

GENESISAPI void GENESISCC geMotion_Destroy(geMotion **PM);

// GENESIS_PRIVATE_APIS

GENESISAPI geBoolean GENESISCC geMotion_IsValid(const geMotion *M);

	// AddPath adds a reference of P to the motion M.  Ownership is shared - The caller must destroy P.
GENESISAPI geBoolean GENESISCC geMotion_AddPath(geMotion *M, gePath *P,const char *Name,int *Index);

GENESISAPI geBoolean GENESISCC geMotion_HasNames(const geMotion *M);
GENESISAPI int32 GENESISCC geMotion_GetNameChecksum(const geMotion *M);

GENESISAPI geBoolean GENESISCC geMotion_RemoveNames(geMotion *M);

GENESISAPI void GENESISCC geMotion_SampleChannels(const geMotion *M, int PathIndex, geFloat Time, geQuaternion *Rotation, geVec3d *Translation);
GENESISAPI geBoolean GENESISCC geMotion_SampleChannelsNamed(const geMotion *M, const char *PathName, geFloat Time, geQuaternion *Rotation, geVec3d *Translation);

GENESISAPI void GENESISCC geMotion_Sample(const geMotion *M, int PathIndex, geFloat Time, geXForm3d *Transform);
GENESISAPI geBoolean GENESISCC geMotion_SampleNamed(const geMotion *M, const char *PathName, geFloat Time, geXForm3d *Transform);

	// the returned Paths from _Get functions should not be destroyed.  
	// if ownership is desired, call gePath_CreateRef() to create another owner. 
	// an 'owner' has access to the object regardless of the number of other owners, and 
	// an owner must call the object's destroy method to relinquish ownership
GENESISAPI gePath *GENESISCC geMotion_GetPathNamed(const geMotion *M,const char *Name);
GENESISAPI const char *GENESISCC geMotion_GetNameOfPath(const geMotion *M, int Index);

// GENESIS_PUBLIC_APIS
GENESISAPI gePath *GENESISCC geMotion_GetPath(const geMotion *M,int Index);
GENESISAPI int GENESISCC geMotion_GetPathCount(const geMotion *M);


GENESISAPI geBoolean GENESISCC geMotion_SetName(geMotion *M, const char * Name);
GENESISAPI const char *GENESISCC geMotion_GetName(const geMotion *M);

// GENESIS_PRIVATE_APIS

	// support for compound motions.  A motion can either have sub-motions, or be single motion.
	// these functions support motions that have sub-motions.
GENESISAPI int GENESISCC geMotion_GetSubMotionCount(const geMotion*M);

	// the returned motions from these _Get functions should not be destroyed.  
	// if ownership is desired, call geMotion_CreateRef() to create another owner. 
	// an 'owner' has access to the object regardless of the number of other owners, and 
	// an owner must call the object's destroy method to relinquish ownership
GENESISAPI geMotion *GENESISCC geMotion_GetSubMotion(const geMotion *M,int Index);
GENESISAPI geMotion *GENESISCC geMotion_GetSubMotionNamed(const geMotion *M,const char *Name);
GENESISAPI geBoolean GENESISCC geMotion_AddSubMotion(
								geMotion *ParentMotion,
								geFloat TimeScale,			// Scale factor for this submotion
								geFloat TimeOffset,			// Time in parent motion when submotion should start
								geMotion *SubMotion,
								geFloat StartTime,			// Blend start time (relative to submotion)
								geFloat StartMagnitude,		// Blend start magnitude (0..1)
								geFloat EndTime,			// Blend ending time (relative to submotion)
								geFloat EndMagnitude,		// Blend ending magnitude (0..1)
								const geXForm3d *Transform,	// Base transform to apply to this submotion
								int *Index);				// returned motion index

GENESISAPI geMotion *GENESISCC  geMotion_RemoveSubMotion(geMotion *ParentMotion, int SubMotionIndex);

// Get/Set submotion time offset.  The time offset is the offset into the 
// compound (parent) motion at which the submotion should start.
GENESISAPI geFloat   GENESISCC  geMotion_GetTimeOffset( const geMotion *M,int SubMotionIndex );
GENESISAPI geBoolean  GENESISCC geMotion_SetTimeOffset( geMotion *M,int SubMotionIndex,geFloat TimeOffset );

// Get/Set submotion time scale.  Time scaling is applied to the submotion after the TimeOffset
// is applied.  The formula is:  (CurrentTime - TimeOffset) * TimeScale
GENESISAPI geFloat   GENESISCC  geMotion_GetTimeScale( const geMotion *M,int SubMotionIndex );
GENESISAPI geBoolean  GENESISCC geMotion_SetTimeScale( geMotion *M,int SubMotionIndex,geFloat TimeScale );

// Get blending amount for a particular submotion.  The Time parameter is parent-relative.
GENESISAPI geFloat    GENESISCC geMotion_GetBlendAmount( const geMotion *M, int SubMotionIndex, geFloat Time);

// Get/Set blending path.  The keyframe times in the blend path are relative to the submotion.
GENESISAPI gePath    *GENESISCC geMotion_GetBlendPath( const geMotion *M,int SubMotionIndex );
GENESISAPI geBoolean  GENESISCC geMotion_SetBlendPath( geMotion *M,int SubMotionIndex, gePath *Blend );

GENESISAPI const geXForm3d *GENESISCC geMotion_GetBaseTransform( const geMotion *M,int SubMotionIndex );
GENESISAPI geBoolean  GENESISCC geMotion_SetBaseTransform( geMotion *M,int SubMotionIndex, geXForm3d *BaseTransform );
GENESISAPI geBoolean  GENESISCC geMotion_GetTransform(const geMotion *M, geFloat Time, geXForm3d *Transform);
// GENESIS_PUBLIC_APIS

	// gets time of first key and time of last key (as if motion did not loop)
	// if there are no paths in the motion: returns GE_FALSE and times are not set
	// otherwise returns GE_TRUE
	//
	// For a compound motion, GetTimeExtents will return the extents of the scaled submotions.
	// For a single motion, no scaling is applied.
GENESISAPI geBoolean GENESISCC geMotion_GetTimeExtents(const geMotion *M,geFloat *StartTime,geFloat *EndTime);

// Only one event is allowed per time key.

GENESISAPI geBoolean GENESISCC geMotion_InsertEvent(geMotion *M, geFloat tKey, const char* String);
	// Inserts the new event and corresponding string.

GENESISAPI geBoolean GENESISCC geMotion_DeleteEvent(geMotion *M, geFloat tKey);
	// Deletes the event

GENESISAPI void GENESISCC geMotion_SetupEventIterator(
	geMotion *M,
	geFloat StartTime,				// Inclusive search start
	geFloat EndTime);				// Non-inclusive search stop
	// For searching or querying the array for events between two times
	// times are compaired [StartTime,EndTime), '[' is inclusive, ')' is 
	// non-inclusive.  This prepares the geMotion_GetNextEvent() function.

GENESISAPI geBoolean GENESISCC geMotion_GetNextEvent(
	geMotion *M,						// Event list to iterate
	geFloat *pTime,				// Return time, if found
	const char **ppEventString);	// Return data, if found
	// Iterates from StartTime to EndTime as setup in geMotion_SetupEventIterator()
	// and for each event between these times [StartTime,EndTime)
	// this function will return Time and EventString returned for that event
	// and the iterator will be positioned for the next search.  When there 
	// are no more events in the range, this function will return GE_FALSE (Time
	// will be 0 and ppEventString will be empty).

GENESISAPI geBoolean GENESISCC geMotion_GetEventExtents(const geMotion *M,
			geFloat *FirstEventTime,
			geFloat *LastEventTime);
	// returns the time associated with the first and last events 
	// returns GE_FALSE if there are no events (and Times are not set)


// GENESIS_PRIVATE_APIS
GENESISAPI geMotion *GENESISCC geMotion_CreateFromFile(geVFile *f);
GENESISAPI geBoolean GENESISCC geMotion_WriteToFile(const geMotion *M, geVFile *f);
GENESISAPI geBoolean GENESISCC geMotion_WriteToBinaryFile(const geMotion *M,geVFile *pFile);

#ifdef __cplusplus
}
#endif


#endif
