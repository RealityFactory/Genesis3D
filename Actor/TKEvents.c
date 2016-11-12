/****************************************************************************************/
/*  TKARRAY.C																			*/
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description: Time-keyed events implementation.										*/
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
/* geTKEvents
	(Time-Keyed-Events)

	geTKEvents is a sorted array of times with an identifying descriptor.
	The descriptors are stored as strings in a separate, packed buffer.

	Error conditions are reported to errorlog
*/
#include <assert.h>
#include <stdio.h>

#include "TKEvents.h"
#include "TKArray.h"
#include "ErrorLog.h"
#include "ram.h"
#include "string.h"

typedef struct
{
	geTKEvents_TimeType EventTime;
	uint32 DataOffset;
}	EventType;

typedef struct geTKEventsIterator 
{
	geTKEvents_TimeType EndTime;
	int CurrentIndex;
}	geTKEventsIterator;

typedef struct geTKEvents
{
	geTKArray* pTimeKeys;
	uint32 DataSize;
	char* pEventData;

	geTKEventsIterator Iterator;
}	geTKEvents;



// General validity test.
// Use TKE_ASSERT_VALID to test array for reasonable data.
#ifdef _DEBUG

#define TKE_ASSERT_VALID(E) geTKEvents_Asserts(E)

// Do not call this function directly.  Use TKE_ASSERT_VALID
static void GENESISCC geTKEvents_Asserts(const geTKEvents* E)
{
	assert( (E) != NULL );
	assert( (E)->pTimeKeys != NULL );
	if(geTKArray_NumElements((E)->pTimeKeys) == 0)
	{
		assert( (E)->pEventData == NULL );
	}
	else
	{
		assert( (E)->pEventData != NULL );
	}
}

#else // !_DEBUG

#define TKE_ASSERT_VALID(E) ((void)0)

#endif // _DEBUG

geTKEvents* GENESISCC geTKEvents_Create(void)
	// Creates a new event array.
{
	geTKEvents* pEvents;

	pEvents = GE_RAM_ALLOCATE_STRUCT(geTKEvents);
	if(!pEvents)
	{
		geErrorLog_Add(ERR_TKEVENTS_CREATE_ENOMEM, NULL);
		return NULL;
	}

	pEvents->pTimeKeys = geTKArray_Create(sizeof(EventType));
	if(!pEvents->pTimeKeys)
	{
		geErrorLog_Add(ERR_TKEVENTS_CREATE_ENOMEM, NULL);
		geRam_Free(pEvents);
		return NULL;
	}

	pEvents->DataSize = 0;
	pEvents->pEventData = NULL;

	pEvents->Iterator.CurrentIndex = 0;
	pEvents->Iterator.EndTime = -99e33f;	// you could sample here I suppose...
	
	return pEvents;
}


void GENESISCC geTKEvents_Destroy(geTKEvents** ppEvents)
	// Destroys array.
{
	geTKEvents* pE;

	assert(ppEvents);
	pE = *ppEvents;
	assert(pE);

	if( pE->pEventData != NULL )
		{
			geRam_Free(pE->pEventData);
		}
	
	if (pE->pTimeKeys != NULL)
		{
			geTKArray_Destroy(&pE->pTimeKeys);
		}
	geRam_Free(*ppEvents);
	*ppEvents = NULL;
}


geBoolean GENESISCC geTKEvents_Insert(geTKEvents* pEvents, geTKEvents_TimeType tKey, const char* pEventData)
{
	int nIndex;
	uint32 DataLength;
	uint32 InitialOffset;
	int nNumElements;
	EventType* pKeyInfo;
	char* pNewData;

	TKE_ASSERT_VALID(pEvents);

	if( geTKArray_Insert(&pEvents->pTimeKeys, tKey, &nIndex) != GE_TRUE )
	{
		geErrorLog_Add(ERR_TKEVENTS_INSERT, NULL);
		return GE_FALSE;
	}
	pKeyInfo = geTKArray_Element(pEvents->pTimeKeys, nIndex);
	assert( pKeyInfo != NULL ); // I just successfully added it; it better be there.

	DataLength = strlen(pEventData) + 1;

	// Resize data to add new stuff
	pNewData = geRam_Realloc(pEvents->pEventData, pEvents->DataSize + DataLength);
	if(!pNewData)
	{
		geErrorLog_Add(ERR_TKEVENTS_INSERT_ENOMEM, NULL);
		if( geTKArray_DeleteElement(&pEvents->pTimeKeys, nIndex) == GE_FALSE)
		{
			// This object is now in an unstable state.
			assert(0);
		}
		// invalidate the iterator
		pEvents->Iterator.EndTime = -99e33f;	// you could sample here I suppose...
		return GE_FALSE;
	}
	pEvents->pEventData = pNewData;

	// Find where new data will go
	nNumElements = geTKArray_NumElements(pEvents->pTimeKeys);
	assert(nIndex < nNumElements); // sanity check
	if(nIndex == nNumElements - 1)
	{
		// We were added to the end
		InitialOffset = pEvents->DataSize;
	}
	else
	{
		EventType* pNextKeyInfo = geTKArray_Element(pEvents->pTimeKeys, nIndex + 1);
		assert( pNextKeyInfo != NULL );

		InitialOffset = pNextKeyInfo->DataOffset;
	}
	pKeyInfo->DataOffset = InitialOffset;

	// Add new data, moving only if necessary
	if(InitialOffset < pEvents->DataSize)
	{
		memmove(pEvents->pEventData + InitialOffset + DataLength,	// dest
				pEvents->pEventData + InitialOffset,				// src
				pEvents->DataSize - InitialOffset);					// count
	}
	memcpy(	pEvents->pEventData + InitialOffset,	// dest
			pEventData,								// src
			DataLength);							// count

	pEvents->DataSize += DataLength;

	// Bump all remaining offsets up
	nIndex++;
	while(nIndex < nNumElements)
	{
		pKeyInfo = geTKArray_Element(pEvents->pTimeKeys, nIndex);
		assert( pKeyInfo != NULL );
		pKeyInfo->DataOffset += DataLength;

		nIndex++;
	}

	// invalidate the iterator
	pEvents->Iterator.EndTime = -99e33f;	// you could sample here I suppose...

	return GE_TRUE;
}


geBoolean GENESISCC geTKEvents_Delete(geTKEvents* pEvents, geTKEvents_TimeType tKey)
{
	int nIndex, Count;
	geTKEvents_TimeType tFound;
	EventType* pKeyInfo;
	int DataOffset, DataSize;
	char *pNewData;

	TKE_ASSERT_VALID(pEvents);

	nIndex = geTKArray_BSearch(pEvents->pTimeKeys, tKey);

	if( nIndex < 0 )
	{	// no key wasn't found
		geErrorLog_Add(ERR_TKEVENTS_DELETE_NOT_FOUND, NULL);
		return GE_FALSE;
	}

	tFound = geTKArray_ElementTime(pEvents->pTimeKeys, nIndex);
	if(tFound < tKey - GE_TKA_TIME_TOLERANCE)
	{
		// key not found
		geErrorLog_Add(ERR_TKEVENTS_DELETE_NOT_FOUND, NULL);
		return GE_FALSE;
	}

	pKeyInfo = geTKArray_Element(pEvents->pTimeKeys, nIndex);
	DataOffset = pKeyInfo->DataOffset;
	if(nIndex < geTKArray_NumElements(pEvents->pTimeKeys) - 1)
	{
		// not the last element
		pKeyInfo = geTKArray_Element(pEvents->pTimeKeys, nIndex + 1);
		DataSize = pKeyInfo->DataOffset - DataOffset;

		memmove(pEvents->pEventData + DataOffset,				// dest
				pEvents->pEventData + DataOffset + DataSize,	// src
				pEvents->DataSize - DataOffset - DataSize);		// count
	}
	else
	{
		// It's the last element and no memory needs to be moved
		DataSize = pEvents->DataSize - DataOffset;
	}

	// Adjust data
	pEvents->DataSize -= DataSize;
	if (pEvents->DataSize == 0)
	{
		geRam_Free (pEvents->pEventData);
		pEvents->pEventData = NULL;
	}
	else
	{
		pNewData = geRam_Realloc(pEvents->pEventData, pEvents->DataSize);
		// If the reallocation failed, it doesn't really hurt.  However, it is a 
		// sign of problems ahead.
		if(pNewData)
		{
			pEvents->pEventData = pNewData;
		}
	}

	// Finally, remove this element
	geTKArray_DeleteElement(&pEvents->pTimeKeys, nIndex);

	// Adjust the offsets
	Count = geTKArray_NumElements(pEvents->pTimeKeys);
	while(nIndex < Count)
	{
		pKeyInfo = geTKArray_Element(pEvents->pTimeKeys, nIndex);
		assert( pKeyInfo != NULL );
		pKeyInfo->DataOffset -= DataSize;
		nIndex++;
	}

	// invalidate the iterator
	pEvents->Iterator.EndTime = -99e33f;	// you could sample here I suppose...

	return GE_TRUE;
}


#define TKEVENTS_ASCII_FILE_TYPE 0x56454B54 // 'TKEV'
#define TKEVENTS_FILE_VERSION 0x00F0		// Restrict to 16 bits

#define TKEVENTS_BIN_FILE_TYPE   0x42454B54 // 'TKEB'
#define TKEVENTS_TIMEKEYS_ID "TimeKeys"
#define TKEVENTS_DATASIZE_ID "DataSize"


geTKEvents* GENESISCC geTKEvents_CreateFromFile(
	geVFile* pFile)					// stream positioned at array data
	// Creates a new array from the given stream.
{
	uint32 u;
	geTKEvents* pEvents;

	assert( pFile != NULL );

	// Read the format/version flag
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL);
		return NULL;
	}

	pEvents = GE_RAM_ALLOCATE_STRUCT(geTKEvents);
	if(!pEvents)
	{
		geErrorLog_Add(ERR_TKEVENTS_CREATE_ENOMEM, NULL);
		return NULL;
	}
	pEvents->pEventData = NULL;
	pEvents->pTimeKeys  = NULL;

	if(u == TKEVENTS_ASCII_FILE_TYPE)
	{
		int		i;
		uint32	v;
		char	VersionString[32];
#define LINE_LENGTH 256
		char line[LINE_LENGTH];
		char* pTextLine = NULL;
		EventType* pEInfo;
		geTKEvents_TimeType Time;

#define ABORT_READ_AND_FREE { geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL); geTKEvents_Destroy(&pEvents); return NULL; }
#define ABORT_READ_AND_FREE_ALL				\
{											\
	geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL);	\
	geTKEvents_Destroy(&pEvents);			\
	if(pTextLine != NULL)					\
		geRam_Free(pTextLine);				\
	return NULL;							\
}

		// Read and build the version.
		if	(geVFile_GetS(pFile, VersionString, sizeof(VersionString)) == GE_FALSE)
		{
			geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL);
			return NULL;
		}
		sscanf(VersionString, "%X.%X\n", &u, &v);
		v |= (u << 8);
		// Should this structure change, then actually code multiversion support.
		if(v != TKEVENTS_FILE_VERSION)
			ABORT_READ_AND_FREE;

		if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
			ABORT_READ_AND_FREE;

		if(strnicmp(line, TKEVENTS_DATASIZE_ID, sizeof(TKEVENTS_DATASIZE_ID)-1) != 0)
			ABORT_READ_AND_FREE;

		if(sscanf(line + sizeof(TKEVENTS_DATASIZE_ID)-1, "%d", &pEvents->DataSize) != 1)
			ABORT_READ_AND_FREE;
		if (pEvents->DataSize +1> LINE_LENGTH)
			ABORT_READ_AND_FREE;

		if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
			ABORT_READ_AND_FREE;

		if(strnicmp(line, TKEVENTS_TIMEKEYS_ID, sizeof(TKEVENTS_TIMEKEYS_ID)-1) != 0)
			ABORT_READ_AND_FREE;

		if(sscanf(line + sizeof(TKEVENTS_TIMEKEYS_ID)-1, "%d", &i) != 1)
			ABORT_READ_AND_FREE;

		pEvents->pTimeKeys = geTKArray_Create(sizeof(EventType));
		if(!pEvents->pTimeKeys)
			ABORT_READ_AND_FREE;

		pEvents->pEventData = geRam_Allocate(pEvents->DataSize);
		if(!pEvents->pEventData)
			ABORT_READ_AND_FREE_ALL;

		// The strings are read in with a CR.  The max line length will then be
		// the datasize + 1.
		pTextLine = geRam_Allocate(pEvents->DataSize + 1);
		if(pTextLine == NULL)
			ABORT_READ_AND_FREE_ALL;

		while(i > 0)
		{
			char	TimeString[64];
			if	(geVFile_GetS(pFile, TimeString, sizeof(TimeString)) == GE_FALSE)
				ABORT_READ_AND_FREE_ALL;
			if(sscanf(TimeString, "%f %d\n", &Time, &v) != 2)
				ABORT_READ_AND_FREE_ALL;
			if(!geTKArray_Insert(&pEvents->pTimeKeys, Time, (int*)&u))
				ABORT_READ_AND_FREE_ALL;
			pEInfo = geTKArray_Element(pEvents->pTimeKeys, u);
			pEInfo->DataOffset = v;
			//if(!fgets(pEvents->pEventData + v, pEvents->DataSize - v, pFile))
			if(geVFile_GetS(pFile, pTextLine, pEvents->DataSize + 1) == GE_FALSE)
				ABORT_READ_AND_FREE_ALL;
			// strip the CR
			pTextLine[strlen(pTextLine) - 1] = 0;
				{  // maybe strip the rest of the CR
					int len = strlen(pTextLine)-1;
					if (pTextLine[len] == 13)  // remove trailing /r  (binary file mode)
						{
							pTextLine[len] = 0;
						}
				}
			strcpy(pEvents->pEventData + v, pTextLine);
			
			i--;
		}

		if(pTextLine != NULL)
			geRam_Free(pTextLine);
	}
	else
	{
		if(u == TKEVENTS_BIN_FILE_TYPE)
			{
				if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
					{
						geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL);
						geTKEvents_Destroy(&pEvents);
						return GE_FALSE;
					}
				if (u != TKEVENTS_FILE_VERSION)
					{
						geErrorLog_AddString(-1,"Failure to recognize TKEvents file version", NULL);
						geTKEvents_Destroy(&pEvents);
						return GE_FALSE;
					}

				if(geVFile_Read(pFile, &(pEvents->DataSize), sizeof(pEvents->DataSize)) == GE_FALSE)
					{
						geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL);
						geTKEvents_Destroy(&pEvents);
						return NULL;
					}

				pEvents->pEventData = geRam_Allocate(pEvents->DataSize);
				if(!pEvents->pEventData)
					{
						geErrorLog_Add(ERR_TKEVENTS_CREATE_ENOMEM, NULL);
						geTKEvents_Destroy(&pEvents);
						return NULL;
					}

				if(geVFile_Read(pFile, pEvents->pEventData, pEvents->DataSize) == GE_FALSE)
					{
						geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL);
						geTKEvents_Destroy(&pEvents);
						return NULL;
					}
				pEvents->pTimeKeys = geTKArray_CreateFromBinaryFile(pFile);
				if(!pEvents->pTimeKeys)
					{
						geErrorLog_Add(ERR_TKEVENTS_FILE_READ, NULL);
						geTKEvents_Destroy(&pEvents);
						return NULL;
					}
			}
	}

	return pEvents;
}


#define CHECK_FOR_WRITE(uu) if(uu <= 0) { geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL); return GE_FALSE; }

geBoolean GENESISCC geTKEvents_WriteToFile(
	const geTKEvents* pEvents,		// sorted array to write
	geVFile* pFile)					// stream positioned for writing
	// Writes the array to the given stream.
{
	uint32	u;
	int		NumElements;
	int		i;
	EventType* pEInfo;

	assert( pEvents != NULL );
	assert( pFile != NULL );

	u = TKEVENTS_ASCII_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
			return GE_FALSE;
		}

	// Write the version
	if	(geVFile_Printf(pFile,
					  " %X.%.2X\n",
					  (TKEVENTS_FILE_VERSION & 0xFF00) >> 8,
					  TKEVENTS_FILE_VERSION & 0x00FF) == GE_FALSE)
	{
		geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
		return GE_FALSE;
	}

	// Write the data size first and then combine the time array and string
	// data into one loop of human readable output.
	if	(geVFile_Printf(pFile, "%s %d\n", TKEVENTS_DATASIZE_ID, pEvents->DataSize) == GE_FALSE)
	{
		geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
		return GE_FALSE;
	}

	// Write the TimeKeys array and string data.
	NumElements = geTKArray_NumElements(pEvents->pTimeKeys);
	if	(geVFile_Printf(pFile, "%s %d\n", TKEVENTS_TIMEKEYS_ID, NumElements) == GE_FALSE)
	{
		geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
		return GE_FALSE;
	}

	for(i=0;i<NumElements;i++)
		{
			pEInfo = geTKArray_Element(pEvents->pTimeKeys, i);
			if	(geVFile_Printf(pFile, "%f %d\n", pEInfo->EventTime, pEInfo->DataOffset) == GE_FALSE)
			{
				geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
				return GE_FALSE;
			}
			if	(geVFile_Printf(pFile, "%s\n", pEvents->pEventData + pEInfo->DataOffset) == GE_FALSE)
			{
				geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
				return GE_FALSE;
			}
		}
	return GE_TRUE;
}



geBoolean GENESISCC geTKEvents_WriteToBinaryFile(
	const geTKEvents* pEvents,		// sorted array to write
	geVFile* pFile)					// stream positioned for writing
	// Writes the array to the given stream.
{
	uint32 u;
	assert( pEvents != NULL );
	assert( pFile != NULL );

	u = TKEVENTS_BIN_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
			return GE_FALSE;
		}
	u = TKEVENTS_FILE_VERSION;
	// Write the version
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
			return GE_FALSE;
		}

	if(geVFile_Write(pFile, &pEvents->DataSize, sizeof(pEvents->DataSize)) == GE_FALSE)
		{
			geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
			return GE_FALSE;
		}

	if(geVFile_Write(pFile, pEvents->pEventData, pEvents->DataSize) == GE_FALSE)
		{
			geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
			return GE_FALSE;
		}

	if (geTKArray_WriteToBinaryFile(pEvents->pTimeKeys, pFile)==GE_FALSE)
		{
			geErrorLog_Add(ERR_TKEVENTS_FILE_WRITE, NULL);
			return GE_FALSE;
		}
	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geTKEvents_GetExtents(geTKEvents *Events,
		geTKEvents_TimeType *FirstEventTime,
		geTKEvents_TimeType *LastEventTime)
{
	int Count;
	assert( Events != NULL );
	
	Count = geTKArray_NumElements(Events->pTimeKeys);
	if (Count<0)
		{
			return GE_FALSE;
		}

	*FirstEventTime = geTKArray_ElementTime(Events->pTimeKeys, 0);
	*LastEventTime  = geTKArray_ElementTime(Events->pTimeKeys, Count-1);
	return GE_TRUE;
}

void GENESISCC geTKEvents_SetupIterator(
	geTKEvents* pEvents,				// Event list to iterate
	geTKEvents_TimeType StartTime,				// Inclusive search start
	geTKEvents_TimeType EndTime)				// Non-inclusive search stop
	// For searching or querying the array for events between two times
	// times are compaired [StartTime,EndTime), '[' is inclusive, ')' is 
	// non-inclusive.  This prepares the PathGetNextEvent() function.  
{
	geTKEventsIterator* pTKEI;

	assert( pEvents != NULL );

	pTKEI = &pEvents->Iterator;

	pTKEI->EndTime = EndTime;

	// Initialize search with first index before StartTime
	pTKEI->CurrentIndex = geTKArray_BSearch(pEvents->pTimeKeys, StartTime - GE_TKA_TIME_TOLERANCE);
	while( (pTKEI->CurrentIndex > -1) && 
		(geTKArray_ElementTime(pEvents->pTimeKeys, pTKEI->CurrentIndex) >= StartTime - GE_TKA_TIME_TOLERANCE) )
	{
		pTKEI->CurrentIndex--;
	}
}


geBoolean GENESISCC geTKEvents_GetNextEvent(
	geTKEvents* pEvents,				// Event list to iterate
	geTKEvents_TimeType *pTime,				// Return time, if found
	const char **ppEventString)		// Return data, if found
	// Iterates from StartTime to EndTime as setup in geTKEvents_CreateIterator()
	// and for each event between these times [StartTime,EndTime)
	// this function will return Time and EventString returned for that event
	// and the iterator will be positioned for the next search.  When there 
	// are no more events in the range, this function will return NULL (Time
	// will be 0 and ppEventString will be empty).
{
	geTKEventsIterator* pTKEI;
	geTKArray* pTimeKeys;
	EventType* pKeyInfo;
	int Index;

	assert(pEvents);
	assert(pTime);
	assert(ppEventString);

	pTKEI = &pEvents->Iterator;

	pTimeKeys = pEvents->pTimeKeys;

	pTKEI->CurrentIndex++;
	Index = pTKEI->CurrentIndex;
	if(Index < geTKArray_NumElements(pTimeKeys))
	{
		*pTime = geTKArray_ElementTime(pTimeKeys, Index);
		if(*pTime + GE_TKA_TIME_TOLERANCE < pTKEI->EndTime)
		{
			// Looks good.  Get the string and return.
			pKeyInfo = geTKArray_Element(pTimeKeys, Index);
			*ppEventString = pEvents->pEventData + pKeyInfo->DataOffset;
			return GE_TRUE;
		}
	}

	// None found, clean up
	*pTime = 0.0f;
	*ppEventString = NULL;
	return GE_FALSE;
}
