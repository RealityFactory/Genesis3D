/****************************************************************************************/
/*  TKARRAY.C																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Time-keyed array implementation.										*/
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
/*TKArray
	(Time-Keyed-Array)
	This module is designed primarily to support path.c

	The idea is that there are these packed arrays of elements,
	sorted by a geTKArray_TimeType key.  The key is assumed to be the 
	first field in each element.

	the geTKArray functions operate on this very specific array type.

	Error conditions are reported to errorlog
*/
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "TKArray.h"
#include "ErrorLog.h"
#include "ram.h"

typedef struct geTKArray
{
	int32 NumElements;		// number of elements in use
	int32 ElementSize;		// size of each element
	char Elements[1];		// array elements.  This list will be expanded by changing
							// the allocated size of the entire geTKArray object
}	geTKArray;

typedef struct 
{
	int32 NumElements;		// number of elements in use
	int32 ElementSize;		// size of each element
} geTKArray_BinaryFileHeader;


#define TK_MAX_ARRAY_LENGTH (0x7FFFFFFF)  // NumElements is (signed) 32 bit int


#define TK_ARRAYSIZE (offsetof(geTKArray, Elements))	// gets rid of the extra element char in the def.

// General validity test.
// Use TK_ASSERT_VALID to test array for reasonable data.
#ifdef _DEBUG

#define TK_ASSERT_VALID(A) geTKArray_Asserts(A)

// Do not call this function directly.  Use TK_ASSERT_VALID
static void GENESISCC geTKArray_Asserts(const geTKArray* A)
{
	assert( (A) != NULL );
	assert( ((A)->NumElements == 0) ||
			(((A)->NumElements > 0) && ((A)->Elements != NULL)) );
	assert( (A)->NumElements >= 0 );
	assert( (A)->NumElements <= TK_MAX_ARRAY_LENGTH );
	assert( (A)->ElementSize > 0 );
}

#else // !_DEBUG

#define TK_ASSERT_VALID(A) ((void)0)

#endif // _DEBUG


geTKArray *GENESISCC geTKArray_Create(				
	int ElementSize)				// element size
	// Creates new array with given attributes.  The first field of the element
	// is assumed to be the geTKArray_TimeType key.
{
	geTKArray *A;

	// first item in each element must be the time key
	assert( ElementSize >= sizeof(geTKArray_TimeType) );

	A = geRam_Allocate(TK_ARRAYSIZE);
	if ( A == NULL)
	{
		geErrorLog_Add(ERR_TKARRAY_CREATE, NULL);
		return NULL;
	}

	A->ElementSize = ElementSize;
	A->NumElements = 0;

	TK_ASSERT_VALID(A);

	return A;	
}

geTKArray *GENESISCC geTKArray_CreateEmpty(				
	int ElementSize,int ElementCount)				// element size
	// Creates new array with given size and count.  The first field of the element
	// is assumed to be the geTKArray_TimeType key.
{
	geTKArray *A;
	int32 size = TK_ARRAYSIZE + ElementCount * ElementSize;
	A = (geTKArray*)geRam_Allocate(size);
	if( A == NULL )
	{
		geErrorLog_AddString(-1,"Failure to allocate empty TKArray", NULL);
		return NULL;
	}
	A->ElementSize = ElementSize;
	A->NumElements = ElementCount;

	TK_ASSERT_VALID(A);

	return A;	
}

geTKArray* GENESISCC geTKArray_CreateFromBinaryFile(
	geVFile* pFile)					// stream positioned at array data
	// Creates a new array from the given stream.
{
	int32 size;
	geTKArray* A;
	geTKArray_BinaryFileHeader Header;

	if (geVFile_Read(pFile, &Header, sizeof(geTKArray_BinaryFileHeader)) == GE_FALSE)
	{
		geErrorLog_AddString(-1,"Failure to binary read TKArray header", NULL);
		return NULL;
	}

	size = TK_ARRAYSIZE + Header.NumElements * Header.ElementSize;
	A = (geTKArray*)geRam_Allocate(size);
	if( A == NULL )
	{
		geErrorLog_AddString(-1,"Failure to allocate TKArray during binary read", NULL);
		return NULL;
	}


	if(geVFile_Read(pFile, A->Elements, size - sizeof(geTKArray_BinaryFileHeader)) == GE_FALSE)
		{
			geRam_Free(A);
			geErrorLog_AddString(-1,"Failure to binary read TKArray body", NULL);
			return NULL;
		}

	A->NumElements = Header.NumElements;
	A->ElementSize = Header.ElementSize;

	return A;
}


geBoolean GENESISCC geTKArray_SamplesAreTimeLinear(const geTKArray *Array,geFloat Tolerance)
{
	int i;

	geTKArray_TimeType Delta,Nth,LastNth,NthDelta;
			
	if (Array->NumElements < 2)
		return GE_TRUE;

	LastNth = geTKArray_ElementTime(Array, 0);
	Nth     = geTKArray_ElementTime(Array, 1);
	Delta   =  Nth - LastNth;
	LastNth = Nth;
	
	for (i=2; i< Array->NumElements; i++)
		{
			Nth = geTKArray_ElementTime(Array, i);
			NthDelta = (Nth-LastNth)-Delta;
			if (NthDelta<0.0f) NthDelta = -NthDelta;
			if (NthDelta>Tolerance)
				{
					return GE_FALSE;
				}
			LastNth = Nth;
		}
	return GE_TRUE;
}

geBoolean GENESISCC geTKArray_WriteToBinaryFile(
	const geTKArray* Array,			// sorted array to write
	geVFile* pFile)					// stream positioned for writing
	// Writes the array to the given stream.
{
	int size;
	
	size = TK_ARRAYSIZE + Array->NumElements * Array->ElementSize;
	if(geVFile_Write(pFile, Array, size) == GE_FALSE)
	{
		geErrorLog_AddString(-1,"Failure to write binary TKArray data", NULL);
		return GE_FALSE;
	}
	return GE_TRUE;
}

void GENESISCC geTKArray_Destroy(geTKArray **PA)
	// destroys array
{
	assert( PA  != NULL );
	TK_ASSERT_VALID(*PA);

	geRam_Free(*PA);
	*PA = NULL;
}


int GENESISCC geTKArray_BSearch(
	const geTKArray *A,				// sorted array to search
	geTKArray_TimeType Key)			// searching for this key
	// Searches for key in the Array.   A is assumed to be sorted
	// if key is found (within +-tolarance), the index to that element is returned.
	// if key is not found, the index to the key just smaller than the 
	// given key is returned.  (-1 if the key is smaller than the first element)
{
	int low,hi,mid;
	int ElementSize;
	const char *Array;
	geTKArray_TimeType test;

	TK_ASSERT_VALID(A);
	
	low = 0;
	hi = A->NumElements - 1;
	Array = A->Elements;
	ElementSize = A->ElementSize;
	
	while ( low<=hi )
		{
			mid = (low+hi)/2;
			test = *(geTKArray_TimeType *)(Array + mid*ElementSize);
			if ( Key > test )
				{
					low = mid+1;
				}
			else
				{
					if ( Key < test )
						{
							hi = mid-1;
						}
					else
						{
							return mid;
						}
				}
		}
	return hi;
}


geBoolean GENESISCC geTKArray_Insert(
	geTKArray **PtrA,				// sorted array to insert into
	geTKArray_TimeType Key,			// key to insert
	int *Index)						// new element index
	// inserts a new element into Array.
	// sets only the key for the new element - the rest is junk
	// returns GE_TRUE if the insertion was successful.
	// returns GE_FALSE if the insertion failed. 
	// if Array is empty (no elements, NULL pointer) it is allocated and filled 
	// with the one Key element
	// Index is the index of the new element 
{
	int n;
	geTKArray *ChangedA;
	geTKArray *A;
	geTKArray_TimeType Found;

	assert( PtrA );
	A = *PtrA;
	TK_ASSERT_VALID(A);

	n = geTKArray_BSearch(A,Key);
	// n is the element just prior to the location of the new element

	if(Index)
		*Index = n+1;

	if (n >= 0)
	{
		Found =  *(geTKArray_TimeType *)(A->Elements + (n * (A->ElementSize)) );
		// Found <= Key  (within +-GE_TKA_TIME_TOLERANCE)
		if (Found > Key - GE_TKA_TIME_TOLERANCE)
		{	// if Found==Key, bail.  Can't have two identical keys.
			geErrorLog_Add(ERR_TKARRAY_INSERT_IDENTICAL, NULL);
			return GE_FALSE;
		}
	}

	if (A->NumElements >= TK_MAX_ARRAY_LENGTH)
	{
		geErrorLog_Add(ERR_TKARRAY_TOO_BIG, NULL);
		return GE_FALSE;
	}

	ChangedA = (geTKArray *)geRam_Realloc(A, 
				TK_ARRAYSIZE + (A->NumElements + 1) * A->ElementSize);

	if ( ChangedA == NULL )
	{	
		geErrorLog_Add(ERR_TKARRAY_INSERT_ENOMEM, NULL);
		return GE_FALSE;
	}
	A = ChangedA;

	// advance n to new element's position
	n++;
	
	// move elements as necessary
	if(n < A->NumElements)
	{
		memmove( A->Elements + (n + 1) * A->ElementSize,	// dest
				 A->Elements + n * A->ElementSize,			// src
				 (A->NumElements - n) * A->ElementSize);	// count
	}

	*(geTKArray_TimeType *)((A->Elements) + ((n) * (A->ElementSize)) ) = Key;
	A->NumElements++;
	*PtrA = A;

	return GE_TRUE;
}


geBoolean GENESISCC geTKArray_DeleteElement(
	geTKArray **PtrA,				// sorted array to delete from
	int N)							// element to delete
	// deletes an element from Array.
	// returns GE_TRUE if the deletion was successful. 
	// returns GE_FALSE if the deletion failed. (key not found or realloc failed)
{
	geTKArray *A;
	geTKArray *ChangedA;
	
	assert( PtrA != NULL);
	A = *PtrA;
	TK_ASSERT_VALID(A);
	assert(N >= 0);
	assert(N < A->NumElements);
	
	memmove( (A->Elements) + (N) * (A->ElementSize),  //dest
			 (A->Elements) + (N+1) * (A->ElementSize),  //src
			 ((A->NumElements) - (N+1))* (A->ElementSize) );

	A->NumElements--;
	ChangedA = (geTKArray *)geRam_Realloc(A, 
				TK_ARRAYSIZE + A->NumElements * A->ElementSize);
	if ( ChangedA != NULL ) 
	{	
		// if realloc fails to shrink block. no real error.
		A = ChangedA;
	}

	*PtrA = A;

	return GE_TRUE;
}


void *GENESISCC geTKArray_Element(const geTKArray *A, int N)
	// returns the Nth element 
{
	TK_ASSERT_VALID(A);
	assert(N >= 0);
	assert(N < A->NumElements);

	return (void *)( (A->Elements) + (N * (A->ElementSize)) );
}


geTKArray_TimeType GENESISCC geTKArray_ElementTime(const geTKArray *A, int N)
	// returns the time key for the Nth element 
{
	TK_ASSERT_VALID(A);
	assert(N >= 0);
	assert(N < A->NumElements);
	
	return *(geTKArray_TimeType *)((A->Elements) + (N * (A->ElementSize)) );
}


int GENESISCC geTKArray_NumElements(const geTKArray *A)
	// returns the number of elements in the array
{
	TK_ASSERT_VALID(A);
	return A->NumElements;
}


int GENESISCC geTKArray_ElementSize(const geTKArray *A)
	// returns the size of each element in the array
{
	TK_ASSERT_VALID(A);
	return A->ElementSize;
}
