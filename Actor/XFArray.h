/****************************************************************************************/
/*  XFARRAY.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Array of transforms interface.											*/
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
#ifndef GE_XFARRAY_H
#define GE_XFARRAY_H

/* This is a simple object to formalize an array of transforms (geXForm3d)

   Unfortunately, it's not a very safe object.

   This object exports data (allows external access to one of it's data members)
   This is dangerous - no checking can be done on the use of that data, and no
   checking can be done on array boundry conditions.  This is on purpose.
   
   ...In the name of optimal access to the array.
*/

#include "xform3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geXFArray geXFArray;

	// Create the object.  Creates an array of Size elements.  
	// All elements are initialized to the identity transform
geXFArray *GENESISCC geXFArray_Create(int Size);

	// Destroy the object.  Don't use the pointer returned by _GetElements
	// after destroying the ojbect!
void GENESISCC geXFArray_Destroy( geXFArray **XFA );

	// Get a pointer to the array.  For external iteration.  The size of the 
	// array is returned in Size.  Valid array indicies are (0..Size-1)
geXForm3d *GENESISCC geXFArray_GetElements(const geXFArray *XFA, int *Size);

	// Sets every transform in the array to the given transform.
void GENESISCC geXFArray_SetAll(geXFArray *XFA, const geXForm3d *Matrix);

#ifdef __cplusplus
}
#endif


#endif
