/****************************************************************************************/
/*  BOX.H                                                                               */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Box is a 3D Oriented Bounding Box                                      */
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

#if !defined (GE_BOX_H)
#define GE_BOX_H

#include "Vec3d.h"
#include "XForm3d.h"

typedef struct geBox
{
	// all member variables are **PRIVATE**
	// the Box's scales along the Box's local frame axes

	float xScale, yScale, zScale;

	// the Box's local frame origin lies at (0, 0, 0) in local space
	//
	// these are the scaled Box axes in the global frame
	 
	geVec3d GlobalFrameAxes[3];

	// the transformation that takes the Box's axes from local space
	// to global space, and its inverse

	geXForm3d Transform, TransformInv;

}geBox;

/////////////////////////////////////////////////////////////////////////////
// call this to set up a Box for the first time or when the Box's
// local frame axes scale(s) change
void geBox_Set(geBox* Box, float xScale, float yScale, float zScale, const geXForm3d* Transform);


// call this to set the Box's transformation matrix (does not change the
// scales of the Box's local frame axes)
void geBox_SetXForm(geBox* Box, const geXForm3d* Transform);


// returns GE_TRUE if the boxes overlap, GE_FALSE otherwise
geBoolean geBox_DetectCollisionBetween(const geBox* Box1, const geBox* Box2);

#endif
