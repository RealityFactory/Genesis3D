/****************************************************************************************/
/*  TKARRAY.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Time-keyed array interface.											*/
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
#ifndef GE_TKARRAY_H
#define GE_TKARRAY_H
/* TKArray
	(Time-Keyed-Array)
	This module is designed primarily to support path.c

	The idea is that there are these packed arrays of elements,
	sorted by a geTKArray_TimeType key.  The key is assumed to be the 
	first field in each element.

	the TKArray functions operate on this very specific array type.

	Error conditions are reported to errorlog
*/

#include "basetype.h"
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef geFloat geTKArray_TimeType;

#define GE_TKA_TIME_TOLERANCE (0.00001f)

typedef struct geTKArray geTKArray;

geTKArray *GENESISCC geTKArray_Create(int ElementSize);
	// creates new array with given attributes

geTKArray *GENESISCC geTKArray_CreateEmpty(int ElementSize,int ElementCount);
	// creates new array with given element size and given count of uninitialized members

geTKArray* GENESISCC geTKArray_CreateFromBinaryFile(
	geVFile* pFile);					// stream positioned at array data
	// Creates a new array from the given stream.

geBoolean GENESISCC geTKArray_WriteToBinaryFile(
	const geTKArray* Array,			// sorted array to write
	geVFile* pFile);					// stream positioned for writing
	// Writes the array to the given stream.


int GENESISCC geTKArray_BSearch(
	const geTKArray *Array,			// sorted array to search
	geTKArray_TimeType Key);		// searching for this time
	// Searches for key in the Array. (assumes array is sorted) 
	// if key is found (within +-tolerance), the index to that element is returned.
	// if key is not found, the index to the key just smaller than the 
	// given key is returned.  (-1 if the key is smaller than the first element)
	// search is only accurate to 2*TKA_TIME_TOLERANCE.  
	// if multiple keys exist within 2*TKA_TIME_TOLERANCE, this will find an arbitrary one of them.

geBoolean GENESISCC geTKArray_Insert(
	geTKArray **Array,
	geTKArray_TimeType Key,			// time to insert
	int *Index);					// new element index
	// inserts a new element into Array.
	// sets only the key for the new element - the rest is junk
	// returns TRUE if the insertion was successful.
	// returns FALSE if the insertion failed. 
	// if Array is empty (no elements, NULL pointer) it is allocated and filled 
	// with the one Key element
	// Index is the index of the new element 

geBoolean GENESISCC geTKArray_DeleteElement(
	geTKArray **Array,
	int N);							// element to delete
	// deletes an element from Array.
	// returns TRUE if the deletion was successful. 
	// returns FALSE if the deletion failed. (key not found or realloc failed)

void GENESISCC geTKArray_Destroy(
	geTKArray **Array);	
	// destroys array

void *GENESISCC geTKArray_Element(
	const geTKArray *Array,
	int N);
	// returns a pointer to the Nth element of the array.

int GENESISCC geTKArray_NumElements(
	const geTKArray *Array);
	// returns the number of elements in the array

geTKArray_TimeType GENESISCC geTKArray_ElementTime(
	const geTKArray *Array, 
	int N);
	// returns the Time associated with the Nth element of the array

int GENESISCC geTKArray_ElementSize(
	const geTKArray *A);
	// returns the size of each element in the array

geBoolean GENESISCC geTKArray_SamplesAreTimeLinear(const geTKArray *Array,geFloat Tolerance);
	// returns true if the samples are linear in time within a tolerance

#ifdef __cplusplus
}
#endif



#endif
