/****************************************************************************************/
/*  TKARRAY.H																			*/
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description: Time-keyed events interface.											*/
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
#ifndef GE_TKEVENTS_H
#define GE_TKEVENTS_H
/* TKEvents
	(Time-Keyed-Events)
	This module is designed primarily to support motion.c

	geTKEvents is a sorted array of times with an identifying descriptor.
	The descriptors are stored as strings in a separate, packed buffer.

	Error conditions are reported to errorlog
*/

#include "basetype.h"
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geTKEvents geTKEvents;
typedef geFloat geTKEvents_TimeType;

geTKEvents* GENESISCC geTKEvents_Create(void);
	// Creates a new event array.

void GENESISCC geTKEvents_Destroy(geTKEvents** pEvents);
	// Destroys array.

geBoolean GENESISCC geTKEvents_Insert(geTKEvents* pEvents, geTKEvents_TimeType tKey, const char* pEventData);
	// Inserts the new key and corresponding data.

geBoolean GENESISCC geTKEvents_Delete(geTKEvents* pEvents, geTKEvents_TimeType tKey);
	// Deletes the key 

geTKEvents* GENESISCC geTKEvents_CreateFromFile(
	geVFile* pFile);					// stream positioned at array data
	// Creates a new array from the given stream.

geBoolean GENESISCC geTKEvents_WriteToFile(
	const geTKEvents* pEvents,		// sorted array to write
	geVFile* pFile);					// stream positioned for writing
	// Writes the array to the given stream.

geBoolean GENESISCC geTKEvents_WriteToBinaryFile(
	const geTKEvents* pEvents,		// sorted array to write (in binary format)
	geVFile* pFile);					// stream positioned for writing
	// Writes the array to the given stream.
//---------------------------------------------------------------------------
// Event Iteration

void GENESISCC geTKEvents_SetupIterator(
	geTKEvents* pEvents,				// Event list to iterate
	geTKEvents_TimeType StartTime,				// Inclusive search start
	geTKEvents_TimeType EndTime);				// Non-inclusive search stop
	// For searching or querying the array for events between two times
	// times are compaired [StartTime,EndTime), '[' is inclusive, ')' is 
	// non-inclusive.  This prepares the PathGetNextEvent() function.

geBoolean GENESISCC geTKEvents_GetNextEvent(
	geTKEvents* pEvents,				// Event list to iterate
	geTKEvents_TimeType *pTime,				// Return time, if found
	const char **ppEventString);	// Return data, if found
	// Iterates from StartTime to EndTime as setup in geTKEvents_CreateIterator()
	// and for each event between these times [StartTime,EndTime)
	// this function will return Time and EventString returned for that event
	// and the iterator will be positioned for the next search.  When there 
	// are no more events in the range, this function will return GE_FALSE (Time
	// will be 0 and ppEventString will be empty).

GENESISAPI geBoolean GENESISCC geTKEvents_GetExtents(
		geTKEvents *Events,
		geTKEvents_TimeType *FirstEventTime,	// time of first event
		geTKEvents_TimeType *LastEventTime);	// time of last event

#ifdef __cplusplus
}
#endif



#endif // __TKEVENTS_H__