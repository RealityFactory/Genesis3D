/****************************************************************************************/
/*  XFORM3D.H                                                                           */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: 3D transform interface                                                 */
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
#ifndef GE_XFORM_H
#define GE_XFORM_H


#include "Vec3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{	
	geFloat AX,AY,AZ;	// e[0][0],e[0][1],e[0][2]
	geFloat BX,BY,BZ;	// e[1][0],e[1][1],e[1][2]
	geFloat CX,CY,CZ;	// e[2][0],e[2][1],e[2][2]
	geVec3d Translation;  // e[0][3],e[1][3],e[2][3]
	//	  0,0,0,1		// e[3][0],e[3][1],e[3][2]
} geXForm3d;

/*   this is essentially a 'standard' 4x4 transform matrix,
     with the bottom row always 0,0,0,1

	| AX, AY, AZ, Translation.X |  
	| BX, BY, BZ, Translation.Y |  
	| CX, CY, CZ, Translation.Z |  
	|  0,  0,  0,      1        |  
*/

//  all geXForm3d_Set* functions return a right-handed transform.

#define GEXFORM3D_MINIMUM_SCALE (0.00001f)


GENESISAPI void GENESISCC geXForm3d_Copy(
	const geXForm3d *Src, 
	geXForm3d *Dst);
	// copies Src to Dst.  

GENESISAPI geBoolean GENESISCC geXForm3d_IsValid(const geXForm3d *M);
	// returns GE_TRUE if M is 'valid'  
	// 'valid' means that M is non NULL, and there are no NAN's in the matrix.

GENESISAPI geBoolean GENESISCC geXForm3d_IsOrthonormal(const geXForm3d *M);
	// returns GE_TRUE if M is orthonormal 
	// (if the rows and columns are all normalized (transform has no scaling or shearing)
	// and is orthogonal (row1 cross row2 = row3 & col1 cross col2 = col3)
	// * does not check for right-handed convention *

GENESISAPI geBoolean GENESISCC geXForm3d_IsOrthogonal(const geXForm3d *M);
	// returns GE_TRUE if M is orthogonal
	// (row1 cross row2 = row3 & col1 cross col2 = col3)
	// * does not check for right-handed convention *

GENESISAPI void GENESISCC geXForm3d_Orthonormalize(geXForm3d *M);
	// essentially removes scaling (or other distortions) from 
	// an orthogonal (or nearly orthogonal) matrix 
	// returns a right-handed matrix


GENESISAPI void GENESISCC geXForm3d_SetIdentity(geXForm3d *M);			
	// sets M to an identity matrix (clears it)
	
GENESISAPI void GENESISCC geXForm3d_SetXRotation(geXForm3d *M,geFloat RadianAngle);
	// sets up a transform that rotates RadianAngle about X axis
	// all existing contents of M are replaced
	
GENESISAPI void GENESISCC geXForm3d_SetYRotation(geXForm3d *M,geFloat RadianAngle);
	// sets up a transform that rotates RadianAngle about Y axis
	// all existing contents of M are replaced

GENESISAPI void GENESISCC geXForm3d_SetZRotation(geXForm3d *M,geFloat RadianAngle);
	// sets up a transform that rotates RadianAngle about Z axis
	// all existing contents of M are replaced

GENESISAPI void GENESISCC geXForm3d_SetTranslation(geXForm3d *M,geFloat x, geFloat y, geFloat z);
	// sets up a transform that translates x,y,z
	// all existing contents of M are replaced

GENESISAPI void GENESISCC geXForm3d_SetScaling(geXForm3d *M,geFloat x, geFloat y, geFloat z);
	// sets up a transform that scales by x,y,z
	// all existing contents of M are replaced

GENESISAPI void GENESISCC geXForm3d_RotateX(geXForm3d *M,geFloat RadianAngle);  
	// Rotates M by RadianAngle about X axis   
	// applies the rotation to the existing contents of M

GENESISAPI void GENESISCC geXForm3d_RotateY(geXForm3d *M,geFloat RadianAngle);
	// Rotates M by RadianAngle about Y axis
	// applies the rotation to the existing contents of M

GENESISAPI void GENESISCC geXForm3d_RotateZ(geXForm3d *M,geFloat RadianAngle);
	// Rotates M by RadianAngle about Z axis
	// applies the rotation to the existing contents of M

GENESISAPI void GENESISCC geXForm3d_Translate(geXForm3d *M,geFloat x, geFloat y, geFloat z);	
	// Translates M by x,y,z
	// applies the translation to the existing contents of M

GENESISAPI void GENESISCC geXForm3d_Scale(geXForm3d *M,geFloat x, geFloat y, geFloat z);		
	// Scales M by x,y,z
	// applies the scale to the existing contents of M

GENESISAPI void GENESISCC geXForm3d_Multiply(
	const geXForm3d *M1, 
	const geXForm3d *M2, 
	geXForm3d *MProduct);
	// MProduct = matrix multiply of M1*M2
	// Concatenates the transformation in the M2 matrix onto the transformation in M1

GENESISAPI void GENESISCC geXForm3d_Transform(
	const geXForm3d *M,
	const geVec3d *V, 
	geVec3d *Result);
	// Result is Matrix M * Vector V:  V Tranformed by M

GENESISAPI void GENESISCC geXForm3d_TransformArray(	const geXForm3d *XForm, 
								const geVec3d *Source, 
								geVec3d *Dest, 
								int32 Count);

GENESISAPI void GENESISCC geXForm3d_Rotate(
	const geXForm3d *M,
	const geVec3d *V, 
	geVec3d *Result);
	// Result is Matrix M * Vector V:  V Rotated by M (no translation)


/***
*
	"Left,Up,In" are just the basis vectors in the new coordinate space.
	You can get them by multiplying the unit bases into the transforms.
*
******/

GENESISAPI void GENESISCC geXForm3d_GetLeft(const geXForm3d *M, geVec3d *Left);
	// Gets a vector that is 'left' in the frame of reference of M (facing -Z)

GENESISAPI void GENESISCC geXForm3d_GetUp(const geXForm3d *M,    geVec3d *Up);
	// Gets a vector that is 'up' in the frame of reference of M (facing -Z)

GENESISAPI void GENESISCC geXForm3d_GetIn(const geXForm3d *M,  geVec3d *In);
	// Gets a vector that is 'in' in the frame of reference of M (facing -Z)

GENESISAPI void GENESISCC geXForm3d_GetTranspose(const geXForm3d *M, geXForm3d *MTranspose);
	// Gets the Transpose transform of M   (M^T) 
	// Transpose of a matrix is the switch of the rows and columns
	// The transpose is usefull because it is rapidly computed and is equal to the inverse 
	// transform for orthonormal transforms    [inverse is (M')  where M*M' = Identity ]

GENESISAPI void GENESISCC geXForm3d_TransposeTransform(
	const geXForm3d *M, 
	const geVec3d *V, 
	geVec3d *Result);
	// applies the transpose transform of M to V.  Result = (M^T) * V

/*****
*
	the Euler angles are subsequent rotations :
		by Angles->Z around the Z axis
		then by Angles->Y around the Y axis, in the newly rotate coordinates
		then by Angles->X around the X axis
*
******/	

GENESISAPI void GENESISCC geXForm3d_GetEulerAngles(const geXForm3d *M, geVec3d *Angles);
	// Finds Euler angles from M and puts them into Angles
	
GENESISAPI void GENESISCC geXForm3d_SetEulerAngles(geXForm3d *M, const geVec3d *Angles);
	// Applies Euler angles to build M

GENESISAPI void GENESISCC geXForm3d_SetFromLeftUpIn(
	geXForm3d *M,
	const geVec3d *Left, 
	const geVec3d *Up, 
	const geVec3d *In);
	// Builds an geXForm3d from orthonormal Left, Up and In vectors

GENESISAPI void GENESISCC geXForm3d_Mirror(
	const		geXForm3d *Source, 
	const		geVec3d *PlaneNormal, 
	float		PlaneDist, 
	geXForm3d	*Dest);
	// Mirrors a XForm3d about a plane


//--------------

#ifndef NDEBUG
	GENESISAPI 	void GENESISCC geXForm3d_SetMaximalAssertionMode( geBoolean Enable );
#else
	#define geXForm3d_SetMaximalAssertionMode(Enable)
#endif

#ifdef __cplusplus
}
#endif

#endif

