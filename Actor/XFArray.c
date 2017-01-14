/****************************************************************************************/
/*  XFARRAY.C																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Array of transforms implementation.									*/
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
#include <assert.h>
#include "XFArray.h"
#include "Ram.h"
#include "ErrorLog.h"


typedef struct geXFArray
{
	int		 TransformCount;
	geXForm3d *TransformArray;
} geXFArray;

geXFArray *GENESISCC geXFArray_Create(int Size)
{
	geXFArray *XFA;

	assert( Size > 0 );

	XFA = GE_RAM_ALLOCATE_STRUCT( geXFArray );
	if (XFA == NULL)
		{
			geErrorLog_Add( ERR_XFARRAY_ENOMEM , NULL);
			return NULL;
		}
	XFA->TransformArray = GE_RAM_ALLOCATE_ARRAY(geXForm3d,Size);
	if (XFA->TransformArray == NULL)
		{
			geErrorLog_Add( ERR_XFARRAY_ENOMEM , NULL);
			geRam_Free( XFA );
			return NULL;
		}
	XFA->TransformCount = Size;
	{
		geXForm3d X;
		geXForm3d_SetIdentity(&X);
		geXFArray_SetAll(XFA,&X);
	}
	return XFA;
}

void GENESISCC geXFArray_Destroy( geXFArray **XFA )
{
	assert( XFA != NULL );
	assert( *XFA != NULL );
	assert( (*XFA)->TransformCount > 0 );
	assert( (*XFA)->TransformArray != NULL );
	
	(*XFA)->TransformCount = -1;
	geRam_Free( (*XFA)->TransformArray);
	(*XFA)->TransformArray = NULL;
	geRam_Free( (*XFA) );
	(*XFA) = NULL;
}

geXForm3d *GENESISCC geXFArray_GetElements(const geXFArray *XFA, int *Size)
{
	assert( XFA != NULL );
	assert( Size != NULL );
	assert( XFA->TransformCount > 0 );
	assert( XFA->TransformArray != NULL );

	*Size = XFA->TransformCount;
	return XFA->TransformArray;
}

void GENESISCC geXFArray_SetAll(geXFArray *XFA, const geXForm3d *Matrix)
{
	assert( XFA != NULL );
	assert( Matrix != NULL );
	assert( XFA->TransformCount > 0 );
	assert( XFA->TransformArray != NULL );
	{
		int i;
		geXForm3d *X;
		for (i=0,X=XFA->TransformArray; i<XFA->TransformCount; i++,X++)
			{
				*X = *Matrix;
			}
	}
}
