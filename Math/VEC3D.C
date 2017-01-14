/****************************************************************************************/
/*  VEC3D.C                                                                             */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: 3D Vector implementation                                               */
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
#include <Math.h>
#include <assert.h>
#include "Vec3d.h"

#define VCOMPARE_EPSILON	(geFloat)0.0005

#ifdef NDEBUG
#ifdef _MSC_VER
#define ASM_NORMALIZE
#endif
#endif

#ifndef NDEBUG
GENESISAPI geFloat GENESISCC   geVec3d_GetElement(geVec3d *V, int Index)
{
	assert( V != NULL );
	assert( Index >= 0 );
	assert( Index <  3 );
	return (* ((&((V)->X)) +  (Index) ));
}
#endif

GENESISAPI geBoolean GENESISCC geVec3d_IsValid(const geVec3d *V)
{
	if (V == NULL)
		return GE_FALSE;
	if ((V->X * V->X) < 0.0f) 
		return GE_FALSE;
	if ((V->Y * V->Y) < 0.0f) 
		return GE_FALSE;
	if ((V->Z * V->Z) < 0.0f) 
		return GE_FALSE;
	return GE_TRUE;
}


GENESISAPI void GENESISCC		geVec3d_Set(geVec3d *V, geFloat X, geFloat Y, geFloat Z)
{
	assert ( V != NULL );
	V->X = X;
	V->Y = Y;
	V->Z = Z;
	assert( geVec3d_IsValid(V) != GE_FALSE );
}

GENESISAPI void GENESISCC		geVec3d_Get(const geVec3d *V, geFloat *X, geFloat *Y, geFloat *Z)
{
	assert ( V != NULL );
	assert ( X != NULL );
	assert ( Y != NULL );
	assert ( Z != NULL );
	assert( geVec3d_IsValid(V) != GE_FALSE );
	
	*X = V->X;
	*Y = V->Y;
	*Z = V->Z;
}


GENESISAPI geFloat GENESISCC	geVec3d_DotProduct(const geVec3d *V1, const geVec3d *V2)
{
	assert ( V1 != NULL );
	assert ( V2 != NULL );
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );
	
	return(V1->X*V2->X + V1->Y*V2->Y + V1->Z*V2->Z);
}

GENESISAPI void GENESISCC geVec3d_CrossProduct(const geVec3d *V1, const geVec3d *V2, geVec3d *VResult)
{
	geVec3d Result;
	assert ( V1 != NULL );
	assert ( V2 != NULL );
	assert ( VResult != NULL );
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );

	Result.X = V1->Y*V2->Z - V1->Z*V2->Y;
    Result.Y = V1->Z*V2->X - V1->X*V2->Z;
    Result.Z = V1->X*V2->Y - V1->Y*V2->X;

	*VResult = Result;
}

GENESISAPI geBoolean GENESISCC geVec3d_Compare(const geVec3d *V1, const geVec3d *V2, geFloat Tolerance)
{
	assert ( V1 != NULL );
	assert ( V2 != NULL );
	assert ( Tolerance >= 0.0 );
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );

	if (fabs(V2->X - V1->X) > Tolerance) //VCOMPARE_EPSILON)
		return GE_FALSE;
	if (fabs(V2->Y - V1->Y) > Tolerance) //VCOMPARE_EPSILON)
		return GE_FALSE;
	if (fabs(V2->Z - V1->Z) > Tolerance) //VCOMPARE_EPSILON)
		return GE_FALSE;

	return GE_TRUE;
}

#ifdef ASM_NORMALIZE

GENESISAPI geFloat GENESISCC geVec3d_Normalize(geVec3d *V1)
{
float *fPtr;
float Dist,OneOverDist;
	
	fPtr = (float *)V1;
	Dist =  (*fPtr) * (*fPtr); fPtr++;
	Dist += (*fPtr) * (*fPtr); fPtr++;
	Dist += (*fPtr) * (*fPtr);

	// Vtune shows the float <-> double conversions
	// required for the clib sqrt() are taking a lot of time.
	// hence we use asm to access the float fsqrt() directly

	__asm 
	{
		FLD Dist
		FSQRT
		FSTP Dist
	}

	if ( Dist == 0.0f )
		return 0.0f;

	OneOverDist = 1.0f/Dist;
		
	fPtr = (float *)V1;
	*fPtr *= OneOverDist; fPtr++;
	*fPtr *= OneOverDist; fPtr++;
	*fPtr *= OneOverDist;

return (geFloat)Dist;
}
#else
GENESISAPI geFloat GENESISCC geVec3d_Normalize(geVec3d *V1)
{
	geFloat OneOverDist;
	geFloat Dist;

	assert( geVec3d_IsValid(V1) != GE_FALSE );

	Dist = (geFloat)sqrt(geVec3d_DotProduct(V1, V1));

	if (Dist == 0.0)
		return 0.0f;
	OneOverDist = 1.0f/Dist;
	
	V1->X *= OneOverDist;
	V1->Y *= OneOverDist;
	V1->Z *= OneOverDist;

	return Dist;
}
#endif // ASM_NORMALIZE

GENESISAPI geBoolean GENESISCC	geVec3d_IsNormalized(const geVec3d *V)
{
	geFloat	length;

	assert( geVec3d_IsValid(V) != GE_FALSE );

	length = geVec3d_Length(V);
	if	((length >= 1.0f - VCOMPARE_EPSILON) && (length <= 1.0f + VCOMPARE_EPSILON))
		return GE_TRUE;

	return GE_FALSE;
}

GENESISAPI void GENESISCC geVec3d_Scale(const geVec3d *VSrc, geFloat Scale, geVec3d *VDst)
{
	assert ( VDst != NULL );
	assert( geVec3d_IsValid(VSrc) != GE_FALSE );

	VDst->X = VSrc->X * Scale;
	VDst->Y = VSrc->Y * Scale;
	VDst->Z = VSrc->Z * Scale;
	assert( geVec3d_IsValid(VDst) != GE_FALSE );
}

GENESISAPI geFloat GENESISCC geVec3d_LengthSquared(const geVec3d *V1)
{	
	return geVec3d_DotProduct(V1, V1);
}

GENESISAPI geFloat GENESISCC geVec3d_Length(const geVec3d *V1)
{	
	assert( geVec3d_IsValid(V1) != GE_FALSE );

	return (geFloat)sqrt(geVec3d_DotProduct(V1, V1));
}

GENESISAPI void GENESISCC geVec3d_Subtract(const geVec3d *V1, const geVec3d *V2, geVec3d *V1MinusV2)
{
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );
	assert ( V1MinusV2 != NULL );

	V1MinusV2->X = V1->X - V2->X;
	V1MinusV2->Y = V1->Y - V2->Y;
	V1MinusV2->Z = V1->Z - V2->Z;
}

GENESISAPI void GENESISCC geVec3d_Add(const geVec3d *V1, const geVec3d *V2, geVec3d *V1PlusV2)
{
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );
	assert ( V1PlusV2 != NULL );
	
	V1PlusV2->X = V1->X + V2->X;
	V1PlusV2->Y = V1->Y + V2->Y;
	V1PlusV2->Z = V1->Z + V2->Z;
}

GENESISAPI void GENESISCC geVec3d_MA(geVec3d *V1, geFloat Scale, const geVec3d *V2, geVec3d *V1PlusV2Scaled)
{
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );
	assert ( V1PlusV2Scaled != NULL );
	
	V1PlusV2Scaled->X = V1->X + V2->X*Scale;
	V1PlusV2Scaled->Y = V1->Y + V2->Y*Scale;
	V1PlusV2Scaled->Z = V1->Z + V2->Z*Scale;
}

GENESISAPI void GENESISCC geVec3d_AddScaled(const geVec3d *V1, const geVec3d *V2, geFloat Scale, geVec3d *V1PlusV2Scaled)
{
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );
	assert ( V1PlusV2Scaled != NULL );
	
	V1PlusV2Scaled->X = V1->X + V2->X*Scale;
	V1PlusV2Scaled->Y = V1->Y + V2->Y*Scale;
	V1PlusV2Scaled->Z = V1->Z + V2->Z*Scale;
}

GENESISAPI void GENESISCC geVec3d_Copy(const geVec3d *VSrc, geVec3d *VDst)
{
	assert ( VDst != NULL );
	assert( geVec3d_IsValid(VSrc) != GE_FALSE );
	
	*VDst = *VSrc;
}

GENESISAPI void GENESISCC geVec3d_Clear(geVec3d *V)
{
	assert ( V != NULL );
	
	V->X = 0.0f;
	V->Y = 0.0f;
	V->Z = 0.0f;
}

GENESISAPI void GENESISCC geVec3d_Inverse(geVec3d *V)
{
	assert( geVec3d_IsValid(V) != GE_FALSE );
	
	V->X = -V->X;
	V->Y = -V->Y;
	V->Z = -V->Z;
}

GENESISAPI geFloat GENESISCC		geVec3d_DistanceBetween(const geVec3d *V1, const geVec3d *V2)	// returns length of V1-V2	
{
	geVec3d B;
	
	assert( geVec3d_IsValid(V1) != GE_FALSE );
	assert( geVec3d_IsValid(V2) != GE_FALSE );

	geVec3d_Subtract(V1,V2,&B);
	return geVec3d_Length(&B);
}

