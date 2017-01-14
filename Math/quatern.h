/****************************************************************************************/
/*  QUATERN.H                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description: Quaternion mathematical system interface                               */
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
#ifndef GE_QUATERNION_H
#define GE_QUATERNION_H

/***************************************************************************

	The quatern module contains basic support for a quaternion object.

	quaternions are an extension of complex numbers that allows an
	expression for rotation that can be easily interpolated.  geQuaternion_s are also 
	more numericaly stable for repeated rotations than matrices.

	
	A quaternion is a 4 element 'vector'  [w,x,y,z] where:

	q = w + xi + yj + zk
	i*i = -1
	j*j = -1
	k*k = -1
	i*j = -j*i = k
	j*k = -k*j = i
	k*i = -i*k = j
	q' (conjugate) = w - xi - yj - zk
	||q|| (magnitude) = sqrt(q*q') = sqrt(w*w + x*x + y*y + z*z)
	unit quaternion ||q|| == 1; this implies  q' == qinverse 
	quaternions are associative (q1*q2)*q3 == q1*(q2*q3)
	quaternions are not commutative  q1*q2 != q2*q1
	qinverse (inverse (1/q) ) = q'/(q*q')
	
	q can be expressed by w + xi + yj + zk or [w,x,y,z] 
	or as in this implementation (s,v) where s=w, and v=[x,y,z]

	quaternions can represent a rotation.  The rotation is an angle t, around a 
	unit vector u.   q=(s,v);  s= cos(t/2);   v= u*sin(t/2).

	quaternions can apply the rotation to a point.  let the point be p [px,py,pz],
	and let P be a quaternion(0,p).  Protated = q*P*qinverse 
	( Protated = q*P*q' if q is a unit quaternion)

	concatenation rotations is similar to matrix concatenation.  given two rotations
	q1 and q2,  to rotate by q1, then q2:  let qc = (q2*q1), then the combined 
	rotation is given by qc*P*qcinverse (= qc*P*qc' if q is a unit quaternion)

	multiplication: 
	q1 = w1 + x1i + y1j + z1k
	q2 = w2 + x2i + y2j + z2k
	q1*q2 = q3 =
			(w1*w2 - x1*x2 - y1*y2 - z1*z2)     {w3}
	        (w1*x2 + x1*w2 + y1*z2 - z1*y2)i	{x3}
			(w1*y2 - x1*z2 + y1*w2 + z1*x2)j    {y3}
			(w1*z2 + x1*y2 + y1*x2 + z1*w2)k	{z3}

	also, 
	q1 = (s1,v1) = [s1,(x1,y1,z1)]
	q2 = (s2,v2) = [s2,(x2,y2,z2)]
	q1*q2 = q3	=	(s1*s2 - dot_product(v1,v2),			{s3}
					(s1*v2 + s2*v1 + cross_product(v1,v2))	{v3}


	interpolation - it is possible (and sometimes reasonable) to interpolate between
	two quaternions by interpolating each component.  This does not quarantee a 
	resulting unit quaternion, and will result in an animation with non-linear 
	rotational velocity.

	spherical interpolation: (slerp) treat the quaternions as vectors 
	find the angle between them (w = arccos(q1 dot q2) ).
	given 0<=t<=1,  q(t) = q1*(sin((1-t)*w)/sin(w) + q2 * sin(t*w)/sin(w).
	since q == -q, care must be taken to rotate the proper way.  

	this implementation uses the notation quaternion q = (quatS,quatV) 
	  where quatS is a scalar, and quatV is a 3 element vector.

********************************************/

#include "basetype.h"
#include "xform3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	geFloat	W;
	geFloat   X,Y,Z;
	//geVec3d   QuatV;
} geQuaternion;


#define	QUATERNION_PI	(GE_PI)

geBoolean GENESISCC geQuaternion_IsValid( const geQuaternion *Q );
	// return GE_TRUE if Q is non null and for has no NAN's in its components

void GENESISCC geQuaternion_Set( geQuaternion *Q, geFloat W, geFloat X, geFloat Y, geFloat Z);
	// set quaternion components.  Doesn't normalize
void GENESISCC geQuaternion_SetVec3d( geQuaternion *Q, geFloat W, const geVec3d *V);
	// set quaternion components.  Doesn't normalize
GENESISAPI void GENESISCC geQuaternion_SetFromAxisAngle(geQuaternion *Q, const geVec3d *Axis, geFloat Theta);
	// set a quaternion from an axis and a rotation around the axis
geBoolean GENESISCC geQuaternion_GetAxisAngle(const geQuaternion *Q, geVec3d *Axis, geFloat *Theta);
	// gets an axis and angle of rotation around the axis from a quaternion
	// returns GE_TRUE if there is an axis.  
	// returns GE_FALSE if there is no axis (and Axis is set to 0,0,0, and Theta is 0)

void GENESISCC geQuaternion_Get( const geQuaternion *Q, 
					geFloat *W, geFloat *X, geFloat *Y, geFloat *Z);
	// get quaternion components into W,X,Y,Z
void GENESISCC geQuaternion_GetVec3d( const geQuaternion *Q, geFloat *W, geVec3d *V);
	// get quaternion components into W and V

void GENESISCC geQuaternion_FromMatrix(
	const geXForm3d		*RotationMatrix,
	      geQuaternion	*QDest);
	// takes upper 3 by 3 portion of matrix (rotation sub matrix) 
	// and generates a quaternion

GENESISAPI void GENESISCC geQuaternion_ToMatrix(
	const geQuaternion	*Q, 
		  geXForm3d		*RotationMatrixDest);
	// takes a unit quaternion and makes RotationMatrixDest an equivelant rotation xform.
	// (any translation in RotationMatrixDest will be list)

void GENESISCC geQuaternion_Slerp(
	const geQuaternion		*Q0, 
	const geQuaternion		*Q1, 
	geFloat					T,		
	geQuaternion			*QT);
	// spherical interpolation between q0 and q1.   0<=t<=1 
	// resulting quaternion is 'between' q0 and q1
	// with t==0 being all q0, and t==1 being all q1.
	// returns a quaternion with a positive W - always takes shortest route
	// through the positive W domain.

void GENESISCC geQuaternion_SlerpNotShortest(
	const geQuaternion		*Q0, 
	const geQuaternion		*Q1, 
	geFloat					T,		
	geQuaternion			*QT);
	// spherical interpolation between q0 and q1.   0<=t<=1 
	// resulting quaternion is 'between' q0 and q1
	// with t==0 being all q0, and t==1 being all q1.


void GENESISCC geQuaternion_Multiply(
	const geQuaternion	*Q1, 
	const geQuaternion	*Q2, 
	geQuaternion			*QProduct);
	// multiplies q1 * q2, and places the result in q.
	// no failure. 	renormalization not automatic

void GENESISCC geQuaternion_Rotate(
	const geQuaternion	*Q, 
	const geVec3d       *V, 
	geVec3d				*VRotated);
	// Rotates V by the quaternion Q, places the result in VRotated.

geBoolean GENESISCC geQuaternion_IsUnit(const geQuaternion *Q);
	// returns GE_TRUE if q is a unit quaternion.  GE_FALSE otherwise.

GENESISAPI geFloat GENESISCC geQuaternion_Normalize(geQuaternion *Q);
	// normalizes q to be a unit quaternion.  returns original magnitude of q

GENESISAPI void GENESISCC geQuaternion_Copy(const geQuaternion *QSrc, geQuaternion *QDst);
	// copies quaternion QSrc into QDst

void GENESISCC geQuaternion_SetNoRotation(geQuaternion *Q);
	// sets Q to be a quaternion with no rotation (like an identity matrix)

void GENESISCC geQuaternion_Ln(
	const geQuaternion *Q, 
	geQuaternion *LnQ);
	// ln(Q) for unit quaternion only!

void GENESISCC geQuaternion_Exp(
	const geQuaternion *Q,
	geQuaternion *ExpQ);
	// exp(Q) for pure quaternion only!  (zero scalar part (W))

void GENESISCC geQuaternion_Scale(
	const geQuaternion *Q,
	geFloat Scale,
	geQuaternion *QScaled);
	// Q = Q * Scale  (result is not generally a unit quaternion!)

void GENESISCC geQuaternion_Add(
	const geQuaternion *Q1,
	const geQuaternion *Q2,
	geQuaternion *QSum);
	// QSum = Q1 + Q2  (result is not generally a unit quaternion!)

void GENESISCC geQuaternion_Subtract(
	const geQuaternion *Q1, 
	const geQuaternion *Q2, 
	geQuaternion *QSum);
	// QSum = Q1 - Q2  (result is not generally a unit quaternion!)

void GENESISCC geQuaternion_Inverse(const geQuaternion *Q, geQuaternion *QInv);
	// sets QInv to the inverse of Q.  

geFloat GENESISCC geQuaternion_Magnitude(const geQuaternion *Q);
	// returns Magnitude of Q.  

geBoolean GENESISCC geQuaternion_Compare( geQuaternion *Q1, geQuaternion *Q2, geFloat Tolerance );
	// return GE_TRUE if quaternions differ elementwise by less than Tolerance.


#ifndef NDEBUG
void GENESISCC geQuaternion_SetMaximalAssertionMode( geBoolean Enable );
#endif

#ifdef __cplusplus
}
#endif


#endif // GE_QUATERNION_H
