/****************************************************************************************/
/*  QUATERN.C                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description: Quaternion mathematical system implementation                          */
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
#include <math.h>
#include "basetype.h"
#include "quatern.h"


#ifndef NDEBUG
	static geQuaternion_MaximalAssertionMode = GE_TRUE;
	#define geQuaternion_Assert if (geQuaternion_MaximalAssertionMode) assert

	void GENESISCC geQuaternion_SetMaximalAssertionMode( geBoolean Enable )
	{
		assert( (Enable == GE_TRUE) || (Enable == GE_FALSE) );
		geQuaternion_MaximalAssertionMode = Enable;
	}
#else
	#define geQuaternion_Assert assert
#endif

#define UNIT_TOLERANCE 0.001  
	// Quaternion magnitude must be closer than this tolerance to 1.0 to be 
	// considered a unit quaternion

#define QZERO_TOLERANCE 0.00001 
	// quaternion magnitude must be farther from this tolerance to 0.0 to be 
	// normalized

#define TRACE_QZERO_TOLERANCE 0.1
	// trace of matrix must be greater than this to be used for converting a matrix
	// to a quaternion.

#define AA_QZERO_TOLERANCE 0.0001
	

geBoolean GENESISCC geQuaternion_IsValid(const geQuaternion *Q)
{
	if (Q == NULL)
		return GE_FALSE;
	if ((Q->W * Q->W) < 0.0f)
		return GE_FALSE;
	if ((Q->X * Q->X) < 0.0f)
		return GE_FALSE;
	if ((Q->Y * Q->Y) < 0.0f)
		return GE_FALSE;
	if ((Q->Z * Q->Z) < 0.0f)
		return GE_FALSE;
	return GE_TRUE;
}

void GENESISCC geQuaternion_Set( 
	geQuaternion *Q, geFloat W, geFloat X, geFloat Y, geFloat Z)
{
	assert( Q != NULL );

	Q->W = W;
	Q->X = X;
	Q->Y = Y;
	Q->Z = Z;
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
}

void GENESISCC geQuaternion_SetVec3d(
	geQuaternion *Q, geFloat W, const geVec3d *V)
{
	assert( Q != NULL );
	assert( geVec3d_IsValid(V) != GE_FALSE );

	Q->W = W;
	Q->X = V->X;
	Q->Y = V->Y;
	Q->Z = V->Z;
}	

void GENESISCC geQuaternion_Get( 
	const geQuaternion *Q, 
	geFloat *W, 
	geFloat *X, 
	geFloat *Y, 
	geFloat *Z)
	// get quaternion components into W,X,Y,Z
{
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( W != NULL );
	assert( X != NULL );
	assert( Y != NULL );
	assert( Z != NULL );

	*W = Q->W;
	*X = Q->X;
	*Y = Q->Y;
	*Z = Q->Z;
}

GENESISAPI void GENESISCC geQuaternion_SetFromAxisAngle(geQuaternion *Q, const geVec3d *Axis, geFloat Theta)
	// set a quaternion from an axis and a rotation around the axis
{
	geFloat sinTheta;
	assert( Q != NULL);
	assert( geVec3d_IsValid(Axis) != GE_FALSE);
	assert( (Theta * Theta) >= 0.0f );
	assert( ( fabs(geVec3d_Length(Axis)-1.0f) < AA_QZERO_TOLERANCE) );
	
	Theta = Theta * (geFloat)0.5f;
	Q->W     = (geFloat) cos(Theta);
	sinTheta = (geFloat) sin(Theta);
	Q->X = sinTheta * Axis->X;
	Q->Y = sinTheta * Axis->Y;
	Q->Z = sinTheta * Axis->Z;

	geQuaternion_Assert( geQuaternion_IsUnit(Q) == GE_TRUE );
}


geBoolean GENESISCC geQuaternion_GetAxisAngle(const geQuaternion *Q, geVec3d *Axis, geFloat *Theta)
{	
	float OneOverSinTheta;
	float HalfTheta;
	assert( Q != NULL );
	assert( Axis != NULL );
	assert( Theta != NULL );
	geQuaternion_Assert( geQuaternion_IsUnit(Q) != GE_FALSE );
	
	HalfTheta  = (geFloat)acos( Q->W );
	if (HalfTheta>QZERO_TOLERANCE)
		{
			OneOverSinTheta = 1.0f / (geFloat)sin( HalfTheta );
			Axis->X = OneOverSinTheta * Q->X;
			Axis->Y = OneOverSinTheta * Q->Y;
			Axis->Z = OneOverSinTheta * Q->Z;
			*Theta = 2.0f * HalfTheta;
			geQuaternion_Assert( geVec3d_IsValid(Axis) != GE_FALSE );
			geQuaternion_Assert( (*Theta * *Theta) >= 0.0f);
			return GE_TRUE;
		}
	else
		{
			Axis->X = Axis->Y = Axis->Z = 0.0f;
			*Theta = 0.0f;
			return GE_FALSE;
		}
}


void GENESISCC geQuaternion_GetVec3d( 
	const geQuaternion *Q, 
	geFloat *W, 
	geVec3d *V)
	// get quaternion components into W and V
{
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( W != NULL );
	assert( V != NULL );
	
	*W   = Q->W;
	V->X = Q->X;
	V->Y = Q->Y;
	V->Z = Q->Z;
}


void GENESISCC geQuaternion_FromMatrix(
	const geXForm3d		*M,
	      geQuaternion	*Q)
	// takes upper 3 by 3 portion of matrix (rotation sub matrix) 
	// and generates a quaternion
{
	geFloat trace,s;

	assert( M != NULL );
	assert( Q != NULL );
	geQuaternion_Assert( geXForm3d_IsOrthonormal(M)==GE_TRUE );

	trace = M->AX + M->BY + M->CZ;
	if (trace > 0.0f)
		{
			s = (geFloat)sqrt(trace + 1.0f);
			Q->W = s * 0.5f;
			s = 0.5f / s;

			Q->X = (M->CY - M->BZ) * s;
			Q->Y = (M->AZ - M->CX) * s;
			Q->Z = (M->BX - M->AY) * s;
		}
	else
		{
			int biggest;
			enum {A,E,I};
			if (M->AX > M->BY)
				{
					if (M->CZ > M->AX)
						biggest = I;	
					else
						biggest = A;
				}
			else
				{
					if (M->CZ > M->AX)
						biggest = I;
					else
						biggest = E;
				}

			// in the unusual case the original trace fails to produce a good sqrt, try others...
			switch (biggest)
				{
				case A:
					s = (geFloat)sqrt( M->AX - (M->BY + M->CZ) + 1.0);
					if (s > TRACE_QZERO_TOLERANCE)
						{
							Q->X = s * 0.5f;
							s = 0.5f / s;
							Q->W = (M->CY - M->BZ) * s;
							Q->Y = (M->AY + M->BX) * s;
							Q->Z = (M->AZ + M->CX) * s;
							break;
						}
							// I
							s = (geFloat)sqrt( M->CZ - (M->AX + M->BY) + 1.0);
							if (s > TRACE_QZERO_TOLERANCE)
								{
									Q->Z = s * 0.5f;
									s = 0.5f / s;
									Q->W = (M->BX - M->AY) * s;
									Q->X = (M->CX + M->AZ) * s;
									Q->Y = (M->CY + M->BZ) * s;
									break;
								}
							// E
							s = (geFloat)sqrt( M->BY - (M->CZ + M->AX) + 1.0);
							if (s > TRACE_QZERO_TOLERANCE)
								{
									Q->Y = s * 0.5f;
									s = 0.5f / s;
									Q->W = (M->AZ - M->CX) * s;
									Q->Z = (M->BZ + M->CY) * s;
									Q->X = (M->BX + M->AY) * s;
									break;
								}
							break;
				case E:
					s = (geFloat)sqrt( M->BY - (M->CZ + M->AX) + 1.0);
					if (s > TRACE_QZERO_TOLERANCE)
						{
							Q->Y = s * 0.5f;
							s = 0.5f / s;
							Q->W = (M->AZ - M->CX) * s;
							Q->Z = (M->BZ + M->CY) * s;
							Q->X = (M->BX + M->AY) * s;
							break;
						}
							// I
							s = (geFloat)sqrt( M->CZ - (M->AX + M->BY) + 1.0);
							if (s > TRACE_QZERO_TOLERANCE)
								{
									Q->Z = s * 0.5f;
									s = 0.5f / s;
									Q->W = (M->BX - M->AY) * s;
									Q->X = (M->CX + M->AZ) * s;
									Q->Y = (M->CY + M->BZ) * s;
									break;
								}
							// A
							s = (geFloat)sqrt( M->AX - (M->BY + M->CZ) + 1.0);
							if (s > TRACE_QZERO_TOLERANCE)
								{
									Q->X = s * 0.5f;
									s = 0.5f / s;
									Q->W = (M->CY - M->BZ) * s;
									Q->Y = (M->AY + M->BX) * s;
									Q->Z = (M->AZ + M->CX) * s;
									break;
								}
					break;
				case I:
					s = (geFloat)sqrt( M->CZ - (M->AX + M->BY) + 1.0);
					if (s > TRACE_QZERO_TOLERANCE)
						{
							Q->Z = s * 0.5f;
							s = 0.5f / s;
							Q->W = (M->BX - M->AY) * s;
							Q->X = (M->CX + M->AZ) * s;
							Q->Y = (M->CY + M->BZ) * s;
							break;
						}
							// A
							s = (geFloat)sqrt( M->AX - (M->BY + M->CZ) + 1.0);
							if (s > TRACE_QZERO_TOLERANCE)
								{
									Q->X = s * 0.5f;
									s = 0.5f / s;
									Q->W = (M->CY - M->BZ) * s;
									Q->Y = (M->AY + M->BX) * s;
									Q->Z = (M->AZ + M->CX) * s;
									break;
								}
							// E
							s = (geFloat)sqrt( M->BY - (M->CZ + M->AX) + 1.0);
							if (s > TRACE_QZERO_TOLERANCE)
								{
									Q->Y = s * 0.5f;
									s = 0.5f / s;
									Q->W = (M->AZ - M->CX) * s;
									Q->Z = (M->BZ + M->CY) * s;
									Q->X = (M->BX + M->AY) * s;
									break;
								}
					break;
				default:
					assert(0);
				}
		}
	geQuaternion_Assert( geQuaternion_IsUnit(Q) == GE_TRUE );
}

GENESISAPI void GENESISCC geQuaternion_ToMatrix(
	const geQuaternion	*Q, 
		  geXForm3d		*M)
	// takes a unit quaternion and fills out an equivelant rotation
	// portion of a xform
{
	geFloat X2,Y2,Z2;		//2*QX, 2*QY, 2*QZ
	geFloat XX2,YY2,ZZ2;	//2*QX*QX, 2*QY*QY, 2*QZ*QZ
	geFloat XY2,XZ2,XW2;	//2*QX*QY, 2*QX*QZ, 2*QX*QW
	geFloat YZ2,YW2,ZW2;	// ...

	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( M != NULL );
	geQuaternion_Assert( geQuaternion_IsUnit(Q) == GE_TRUE );
	
	
	X2  = 2.0f * Q->X;
	XX2 = X2   * Q->X;
	XY2 = X2   * Q->Y;
	XZ2 = X2   * Q->Z;
	XW2 = X2   * Q->W;

	Y2  = 2.0f * Q->Y;
	YY2 = Y2   * Q->Y;
	YZ2 = Y2   * Q->Z;
	YW2 = Y2   * Q->W;
	
	Z2  = 2.0f * Q->Z;
	ZZ2 = Z2   * Q->Z;
	ZW2 = Z2   * Q->W;
	
	M->AX = 1.0f - YY2 - ZZ2;
	M->AY = XY2  - ZW2;
	M->AZ = XZ2  + YW2;

	M->BX = XY2  + ZW2;
	M->BY = 1.0f - XX2 - ZZ2;
	M->BZ = YZ2  - XW2;

	M->CX = XZ2  - YW2;
	M->CY = YZ2  + XW2;
	M->CZ = 1.0f - XX2 - YY2;

	M->Translation.X = M->Translation.Y = M->Translation.Z = 0.0f;

	geQuaternion_Assert( geXForm3d_IsOrthonormal(M)==GE_TRUE );

}


#define EPSILON (0.00001)


void GENESISCC geQuaternion_Slerp(
	const geQuaternion		*Q0, 
	const geQuaternion		*Q1, 
	geFloat					T,		
	geQuaternion			*QT)
	// spherical interpolation between q0 and q1.   0<=t<=1 
	// resulting quaternion is 'between' q0 and q1
	// with t==0 being all q0, and t==1 being all q1.
{
	geFloat omega,cosom,sinom,Scale0,Scale1;
	geQuaternion QL;
	assert( Q0 != NULL );
	assert( Q1 != NULL );
	assert( QT  != NULL );
	assert( ( 0 <= T ) && ( T <= 1.0f ) );
	geQuaternion_Assert( geQuaternion_IsUnit(Q0) == GE_TRUE );
	geQuaternion_Assert( geQuaternion_IsUnit(Q1) == GE_TRUE );

	cosom =		(Q0->W * Q1->W) + (Q0->X * Q1->X) 
			  + (Q0->Y * Q1->Y) + (Q0->Z * Q1->Z);

	if (cosom < 0)
		{
			cosom = -cosom;
			QL.X = -Q1->X;
			QL.Y = -Q1->Y;
			QL.Z = -Q1->Z;
			QL.W = -Q1->W;
		}
	else
		{
			QL = *Q1;
		}
			

	if ( (1.0f - cosom) > EPSILON )
		{
			omega  = (geFloat) acos( cosom );
			sinom  = (geFloat) sin( omega );
			Scale0 = (geFloat) sin( (1.0f-T) * omega) / sinom;
			Scale1 = (geFloat) sin( T*omega) / sinom;
		}
	else
		{
			// has numerical difficulties around cosom == 0
			// in this case degenerate to linear interpolation
		
			Scale0 = 1.0f - T;
			Scale1 = T;
		}


	QT-> X = Scale0 * Q0->X + Scale1 * QL.X;
	QT-> Y = Scale0 * Q0->Y + Scale1 * QL.Y;
	QT-> Z = Scale0 * Q0->Z + Scale1 * QL.Z;
	QT-> W = Scale0 * Q0->W + Scale1 * QL.W;
	geQuaternion_Assert( geQuaternion_IsUnit(QT) == GE_TRUE );
}




void GENESISCC geQuaternion_SlerpNotShortest(
	const geQuaternion		*Q0, 
	const geQuaternion		*Q1, 
	geFloat					T,		
	geQuaternion			*QT)
	// spherical interpolation between q0 and q1.   0<=t<=1 
	// resulting quaternion is 'between' q0 and q1
	// with t==0 being all q0, and t==1 being all q1.
{
	geFloat omega,cosom,sinom,Scale0,Scale1;
	assert( Q0 != NULL );
	assert( Q1 != NULL );
	assert( QT  != NULL );
	assert( ( 0 <= T ) && ( T <= 1.0f ) );
	geQuaternion_Assert( geQuaternion_IsUnit(Q0) == GE_TRUE );
	geQuaternion_Assert( geQuaternion_IsUnit(Q1) == GE_TRUE );

	cosom =		(Q0->W * Q1->W) + (Q0->X * Q1->X) 
			  + (Q0->Y * Q1->Y) + (Q0->Z * Q1->Z);
	if ( (1.0f + cosom) > EPSILON )
		{
			if ( (1.0f - cosom) > EPSILON )
				{
					omega  = (geFloat) acos( cosom );
					sinom  = (geFloat) sin( omega );
					// has numerical difficulties around cosom == nPI/2
					// in this case everything is up for grabs... 
					//  ...degenerate to linear interpolation
					if (sinom < EPSILON)
						{
							Scale0 = 1.0f - T;
							Scale1 = T;	
						}
					else
						{
							Scale0 = (geFloat) sin( (1.0f-T) * omega) / sinom;
							Scale1 = (geFloat) sin( T*omega) / sinom;
						}
				}
			else
				{
					// has numerical difficulties around cosom == 0
					// in this case degenerate to linear interpolation
				
					Scale0 = 1.0f - T;
					Scale1 = T;
				}
			QT-> X = Scale0 * Q0->X + Scale1 * Q1->X;
			QT-> Y = Scale0 * Q0->Y + Scale1 * Q1->Y;
			QT-> Z = Scale0 * Q0->Z + Scale1 * Q1->Z;
			QT-> W = Scale0 * Q0->W + Scale1 * Q1->W;
			//#pragma message (" ack:!!!!!!")
			//geQuaternionNormalize(QT); 
			geQuaternion_Assert( geQuaternion_IsUnit(QT));
		}
	else
		{
			QT->X = -Q0->Y; 
			QT->Y =  Q0->X;
			QT->Z = -Q0->W;
			QT->W =  Q0->Z;
			Scale0 = (geFloat) sin( (1.0f - T) * (QUATERNION_PI*0.5) );
			Scale1 = (geFloat) sin( T * (QUATERNION_PI*0.5) );
			QT-> X = Scale0 * Q0->X + Scale1 * QT->X;
			QT-> Y = Scale0 * Q0->Y + Scale1 * QT->Y;
			QT-> Z = Scale0 * Q0->Z + Scale1 * QT->Z;
			QT-> W = Scale0 * Q0->W + Scale1 * QT->W;
			geQuaternion_Assert( geQuaternion_IsUnit(QT));
		}
}

void GENESISCC geQuaternion_Multiply(
	const geQuaternion	*Q1, 
	const geQuaternion	*Q2, 
	geQuaternion		*Q)
	// multiplies q1 * q2, and places the result in q.
	// no failure. 	renormalization not automatic

{
	geQuaternion Q1L,Q2L;
	assert( geQuaternion_IsValid(Q1) != GE_FALSE );
	assert( geQuaternion_IsValid(Q2) != GE_FALSE );
	assert( Q  != NULL );
	Q1L = *Q1;
	Q2L = *Q2;

	Q->W  =	(  (Q1L.W*Q2L.W) - (Q1L.X*Q2L.X) 
			 - (Q1L.Y*Q2L.Y) - (Q1L.Z*Q2L.Z) );

	Q->X  =	(  (Q1L.W*Q2L.X) + (Q1L.X*Q2L.W) 
			 + (Q1L.Y*Q2L.Z) - (Q1L.Z*Q2L.Y) );

	Q->Y  =	(  (Q1L.W*Q2L.Y) - (Q1L.X*Q2L.Z) 
			 + (Q1L.Y*Q2L.W) + (Q1L.Z*Q2L.X) );

	Q->Z  = (  (Q1L.W*Q2L.Z) + (Q1L.X*Q2L.Y) 
			 - (Q1L.Y*Q2L.X) + (Q1L.Z*Q2L.W) );
	geQuaternion_Assert( geQuaternion_IsValid(Q) != GE_FALSE );

}


void GENESISCC geQuaternion_Rotate(
	const geQuaternion	*Q, 
	const geVec3d         *V, 
	geVec3d				*VRotated)
	// Rotates V by the quaternion Q, places the result in VRotated.
{
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( geVec3d_IsValid(V)  != GE_FALSE );
	assert( VRotated  != NULL );

	geQuaternion_Assert( geQuaternion_IsUnit(Q) == GE_TRUE );

	{
		geQuaternion Qinv,QV,QRotated, QT;
		geFloat zero;
		geQuaternion_SetVec3d(&QV ,0.0f,V);
		geQuaternion_Inverse (Q,&Qinv);
		geQuaternion_Multiply(Q,&QV,&QT);
		geQuaternion_Multiply(&QT,&Qinv,&QRotated);
		geQuaternion_GetVec3d(&QRotated,&zero,VRotated);
	}
}



geBoolean GENESISCC geQuaternion_IsUnit(const geQuaternion *Q)
	// returns GE_TRUE if Q is a unit geQuaternion.  GE_FALSE otherwise.
{
	geFloat magnitude;
	assert( Q != NULL );

	magnitude  =   (Q->W * Q->W) + (Q->X * Q->X) 
					  + (Q->Y * Q->Y) + (Q->Z * Q->Z);

	if (( magnitude < 1.0+UNIT_TOLERANCE ) && ( magnitude > 1.0-UNIT_TOLERANCE ))
		return GE_TRUE;
	return GE_FALSE;
}

geFloat GENESISCC geQuaternion_Magnitude(const geQuaternion *Q)
	// returns Magnitude of Q.  
{

	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	return   (Q->W * Q->W) + (Q->X * Q->X)  + (Q->Y * Q->Y) + (Q->Z * Q->Z);
}


GENESISAPI geFloat GENESISCC geQuaternion_Normalize(geQuaternion *Q)
	// normalizes Q to be a unit geQuaternion
{
	geFloat magnitude,one_over_magnitude;
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	
	magnitude =   (geFloat) sqrt ((Q->W * Q->W) + (Q->X * Q->X) 
							  + (Q->Y * Q->Y) + (Q->Z * Q->Z));

	if (( magnitude < QZERO_TOLERANCE ) && ( magnitude > -QZERO_TOLERANCE ))
		{
			return 0.0f;
		}

	one_over_magnitude = 1.0f / magnitude;

	Q->W *= one_over_magnitude;
	Q->X *= one_over_magnitude;
	Q->Y *= one_over_magnitude;
	Q->Z *= one_over_magnitude;
	return magnitude;
}


GENESISAPI void GENESISCC geQuaternion_Copy(const geQuaternion *QSrc, geQuaternion *QDst)
	// copies quaternion QSrc into QDst
{
	assert( geQuaternion_IsValid(QSrc) != GE_FALSE );
	assert( QDst != NULL );
	*QDst = *QSrc;
}

void GENESISCC geQuaternion_Inverse(const geQuaternion *Q, geQuaternion *QInv)
	// sets QInv to the inverse of Q.  
{
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( QInv != NULL );

	QInv->W =  Q->W;
	QInv->X = -Q->X;
	QInv->Y = -Q->Y;
	QInv->Z = -Q->Z;
}


void GENESISCC geQuaternion_Add(
	const geQuaternion *Q1, 
	const geQuaternion *Q2, 
	geQuaternion *QSum)
	// QSum = Q1 + Q2  (result is not generally a unit quaternion!)
{
	assert( geQuaternion_IsValid(Q1) != GE_FALSE );
	assert( geQuaternion_IsValid(Q2) != GE_FALSE );
	assert( QSum != NULL );
	QSum->W = Q1->W + Q2->W;
	QSum->X = Q1->X + Q2->X;
	QSum->Y = Q1->Y + Q2->Y;
	QSum->Z = Q1->Z + Q2->Z;
}

void GENESISCC geQuaternion_Subtract(
	const geQuaternion *Q1, 
	const geQuaternion *Q2, 
	geQuaternion *QSum)
	// QSum = Q1 - Q2  (result is not generally a unit quaternion!)
{
	assert( geQuaternion_IsValid(Q1) != GE_FALSE );
	assert( geQuaternion_IsValid(Q2) != GE_FALSE );
	assert( QSum != NULL );
	QSum->W = Q1->W - Q2->W;
	QSum->X = Q1->X - Q2->X;
	QSum->Y = Q1->Y - Q2->Y;
	QSum->Z = Q1->Z - Q2->Z;
}


#define ZERO_EPSILON (0.0001f)
static int32 geQuaternion_XFormTable[]={1768710981,560296816};

void GENESISCC geQuaternion_Ln(
	const geQuaternion *Q, 
	geQuaternion *LnQ)
	// ln(Q) for unit quaternion only!
{
	geFloat Theta;
	geQuaternion QL;
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( LnQ != NULL );
	geQuaternion_Assert( geQuaternion_IsUnit(Q) == GE_TRUE );
	
	if (Q->W < 0.0f)
		{
			QL.W = -Q->W;
			QL.X = -Q->X;
			QL.Y = -Q->Y;
			QL.Z = -Q->Z;
		}
	else
		{
			QL = *Q;
		}
	Theta    = (geFloat)  acos( QL.W  );
	 //  0 < Theta < pi
	if (Theta< ZERO_EPSILON)
		{
			// lim(t->0) of t/sin(t) = 1, so:
			LnQ->W = 0.0f;
			LnQ->X = QL.X;
			LnQ->Y = QL.Y;
			LnQ->Z = QL.Z;
		}
	else
		{
			geFloat Theta_Over_sin_Theta =  Theta / (geFloat) sin ( Theta );
			LnQ->W = 0.0f;
			LnQ->X = Theta_Over_sin_Theta * QL.X;
			LnQ->Y = Theta_Over_sin_Theta * QL.Y;
			LnQ->Z = Theta_Over_sin_Theta * QL.Z;
		}
}
	
void GENESISCC geQuaternion_Exp(
	const geQuaternion *Q,
	geQuaternion *ExpQ)
	// exp(Q) for pure quaternion only!  (zero scalar part (W))
{
	geFloat Theta;
	geFloat sin_Theta_over_Theta;

	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( ExpQ != NULL);
	assert( Q->W == 0.0 );	//check a range?

	Theta = (geFloat) sqrt(Q->X*Q->X  +  Q->Y*Q->Y  +  Q->Z*Q->Z);
	if (Theta > ZERO_EPSILON)
		{
			sin_Theta_over_Theta = (geFloat) sin(Theta) / Theta;
		}
	else
		{
			sin_Theta_over_Theta = (geFloat) 1.0f;
		}

	ExpQ->W   = (geFloat) cos(Theta);
	ExpQ->X   = sin_Theta_over_Theta * Q->X;
	ExpQ->Y   = sin_Theta_over_Theta * Q->Y;
	ExpQ->Z   = sin_Theta_over_Theta * Q->Z;
}	

void GENESISCC geQuaternion_Scale(
	const geQuaternion *Q,
	geFloat Scale,
	geQuaternion *QScaled)
	// Q = Q * Scale  (result is not generally a unit quaternion!)
{
	assert( geQuaternion_IsValid(Q) != GE_FALSE );
	assert( (Scale * Scale) >=0.0f );
	assert( QScaled != NULL);

	QScaled->W = Q->W * Scale;
	QScaled->X = Q->X * Scale;
	QScaled->Y = Q->Y * Scale;
	QScaled->Z = Q->Z * Scale;
}

void GENESISCC geQuaternion_SetNoRotation(geQuaternion *Q)
	// sets Q to be a quaternion with no rotation (like an identity matrix)
{
	Q->W = 1.0f;
	Q->X = Q->Y = Q->Z = 0.0f;
	
	/* this is equivelant to:
		{
			geXForm3d M;
			geXForm3d_SetIdentity(&M);
			geQuaternionFromMatrix(&M,Q);
		}
	*/
}



geBoolean GENESISCC geQuaternion_Compare( geQuaternion *Q1, geQuaternion *Q2, geFloat Tolerance )
{
	assert( geQuaternion_IsValid(Q1) != GE_FALSE );
	assert( geQuaternion_IsValid(Q2) != GE_FALSE );
	assert ( Tolerance >= 0.0 );

	if (	// they are the same but with opposite signs
			(		(fabs(Q1->X + Q2->X) <= Tolerance )  
				&&  (fabs(Q1->Y + Q2->Y) <= Tolerance )  
				&&  (fabs(Q1->Z + Q2->Z) <= Tolerance )  
				&&  (fabs(Q1->W + Q2->W) <= Tolerance )  
			)
		  ||  // they are the same with same signs
			(		(fabs(Q1->X - Q2->X) <= Tolerance )  
				&&  (fabs(Q1->Y - Q2->Y) <= Tolerance )  
				&&  (fabs(Q1->Z - Q2->Z) <= Tolerance )  
				&&  (fabs(Q1->W - Q2->W) <= Tolerance )  
			)
		)
		return GE_TRUE;
	else
		return GE_FALSE;


	
}
