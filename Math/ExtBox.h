/****************************************************************************************/
/*  EXTBOX.H                                                                            */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Axial aligned bounding box (extent box) support                        */
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
#ifndef GE_EXTBOX_H
#define GE_EXTBOX_H

#include "basetype.h"
#include "vec3d.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct geExtBox
{
	geVec3d Min;
	geVec3d Max;
} geExtBox;

// Set the values in a box
void GENESISCC geExtBox_Set (  geExtBox *B,
				  geFloat X1,	  geFloat Y1,	  geFloat Z1,
				  geFloat X2,	  geFloat Y2,	  geFloat Z2 );

// Test a box for validity ( non NULL and max >= min )
geBoolean GENESISCC geExtBox_IsValid(  const geExtBox *B );

// Set box Min and Max to the passed point
void GENESISCC geExtBox_SetToPoint ( geExtBox *B, const geVec3d *Point );

// Extend a box to encompass the passed point
void GENESISCC geExtBox_ExtendToEnclose( geExtBox *B, const geVec3d *Point );

// Return result of box intersection.
// If no intersection, returns GE_FALSE and bResult is not modified.
// If intersection, returns GE_TRUE and fills bResult (if not NULL)
// with the intersected box,
// bResult may be one of b1 or b2.
// 
geBoolean GENESISCC geExtBox_Intersection ( const geExtBox *B1, const geExtBox *B2, geExtBox *Result	);

// computes union of b1 and b2 and returns in bResult.
void GENESISCC geExtBox_Union ( const geExtBox *B1, const geExtBox *B2, geExtBox *Result );

geBoolean GENESISCC geExtBox_ContainsPoint ( const geExtBox *B, const geVec3d  *Point );

void GENESISCC geExtBox_GetTranslation ( const geExtBox *B,       geVec3d *pCenter );
void GENESISCC geExtBox_SetTranslation (       geExtBox *B, const geVec3d *pCenter );
void GENESISCC geExtBox_Translate      (       geExtBox *B, geFloat DX, geFloat DY, geFloat DZ );

void GENESISCC geExtBox_GetScaling     ( const geExtBox *B,       geVec3d *pScale );
void GENESISCC geExtBox_SetScaling     (       geExtBox *B, const geVec3d *pScale );
void GENESISCC geExtBox_Scale          (       geExtBox *B, geFloat DX, geFloat DY,geFloat DZ );

//  Creates a box that encloses the entire area of a box that moves along linear path
void GENESISCC geExtBox_LinearSweep(	const geExtBox *BoxToSweep, 
						const geVec3d *StartPoint, 
						const geVec3d *EndPoint, 
						geExtBox *EnclosingBox );

// Collides a ray with box B.  The ray is directed, from Start to End.  
//   Only returns a ray hitting the outside of the box.  
//     on success, GE_TRUE is returned, and 
//       if T is non-NULL, T is returned as 0..1 where 0 is a collision at Start, and 1 is a collision at End
//       if Normal is non-NULL, Normal is the surface normal of the box where the collision occured.
geBoolean GENESISCC geExtBox_RayCollision( const geExtBox *B, const geVec3d *Start, const geVec3d *End, 
								geFloat *T, geVec3d *Normal );

#ifdef __cplusplus
	}
#endif



#endif

