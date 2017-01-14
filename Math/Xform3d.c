/****************************************************************************************/
/*  XFORM3D.C                                                                           */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: 3D transform implementation                                            */
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

#include "XForm3d.h"


#ifndef NDEBUG
	static geXForm3d_MaximalAssertionMode = GE_TRUE;
	#define geXForm3d_Assert if (geXForm3d_MaximalAssertionMode) assert

GENESISAPI 	void GENESISCC geXForm3d_SetMaximalAssertionMode( geBoolean Enable )
	{
		assert( (Enable == GE_TRUE) || (Enable == GE_FALSE) );
		geXForm3d_MaximalAssertionMode = Enable;
	}
#else
	#define geXForm3d_Assert assert
#endif


GENESISAPI geBoolean GENESISCC geXForm3d_IsValid(const geXForm3d *M)
	// returns GE_TRUE if M is 'valid'  
	// 'valid' means that M is non NULL, and there are no NAN's in the matrix.
{

	if (M == NULL)
		return GE_FALSE;
	if (geVec3d_IsValid(&(M->Translation)) == GE_FALSE)
		return GE_FALSE;

	if ((M->AX * M->AX) < 0.0f) 
		return GE_FALSE;
	if ((M->AY * M->AY) < 0.0f) 
		return GE_FALSE;
	if ((M->AZ * M->AZ) < 0.0f) 
		return GE_FALSE;

	if ((M->BX * M->BX) < 0.0f) 
		return GE_FALSE;
	if ((M->BY * M->BY) < 0.0f) 
		return GE_FALSE;
	if ((M->BZ * M->BZ) < 0.0f) 
		return GE_FALSE;
	
	if ((M->CX * M->CX) < 0.0f) 
		return GE_FALSE;
	if ((M->CY * M->CY) < 0.0f) 
		return GE_FALSE;
	if ((M->CZ * M->CZ) < 0.0f) 
		return GE_FALSE;
	return GE_TRUE;
}




GENESISAPI void GENESISCC geXForm3d_SetIdentity(geXForm3d *M)
	// sets M to an identity matrix (clears it)
{
	assert( M != NULL );			
	
	M->AX = M->BY = M->CZ = 1.0f;
	M->AY = M->AZ = M->BX = M->BZ = M->CX = M->CY = 0.0f;
	M->Translation.X = M->Translation.Y = M->Translation.Z = 0.0f;

	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
}
	
GENESISAPI void GENESISCC geXForm3d_SetXRotation(geXForm3d *M,geFloat RadianAngle)
	// sets up a transform that rotates RadianAngle about X axis
{
	geFloat Cos,Sin;
	assert( M != NULL );
	assert( RadianAngle * RadianAngle >= 0.0f );

	Cos = (geFloat)cos(RadianAngle);
	Sin = (geFloat)sin(RadianAngle);
	M->BY =  Cos;
	M->BZ = -Sin;
	M->CY =  Sin;
	M->CZ =  Cos;
	M->AX = 1.0f;
	M->AY = M->AZ = M->BX = M->CX = 0.0f;
	M->Translation.X = M->Translation.Y = M->Translation.Z = 0.0f;

	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
}  
	
GENESISAPI void GENESISCC geXForm3d_SetYRotation(geXForm3d *M,geFloat RadianAngle)
	// sets up a transform that rotates RadianAngle about Y axis
{
	geFloat Cos,Sin;
	assert( M != NULL );
	assert( RadianAngle * RadianAngle >= 0.0f );

	Cos = (geFloat)cos(RadianAngle);
	Sin = (geFloat)sin(RadianAngle);
	
	M->AX =  Cos;
	M->AZ =  Sin;
	M->CX = -Sin;
	M->CZ =  Cos;
	M->BY = 1.0f;
	M->AY = M->BX = M->BZ = M->CY = 0.0f;
	M->Translation.X = M->Translation.Y = M->Translation.Z = 0.0f;


	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_SetZRotation(geXForm3d *M,geFloat RadianAngle)
	// sets up a transform that rotates RadianAngle about Z axis
{
	geFloat Cos,Sin;
	assert( M != NULL );
	assert( RadianAngle * RadianAngle >= 0.0f );

	Cos = (geFloat)cos(RadianAngle);
	Sin = (geFloat)sin(RadianAngle);
	
	M->AX =  Cos;
	M->AY = -Sin;
	M->BX =  Sin;
	M->BY =  Cos;
	M->CZ = 1.0f;
	M->AZ = M->BZ = M->CX = M->CY = 0.0f;
	M->Translation.X = M->Translation.Y = M->Translation.Z = 0.0f;


	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_SetTranslation(geXForm3d *M,geFloat x, geFloat y, geFloat z)
	// sets up a transform that translates x,y,z
{
	assert( M != NULL );

	M->Translation.X = x;
	M->Translation.Y = y;
	M->Translation.Z = z;
	assert( geVec3d_IsValid(&M->Translation)!=GE_FALSE);

	M->AX = M->BY = M->CZ = 1.0f;
	M->AY = M->AZ = 0.0f;
	M->BX = M->BZ = 0.0f;
	M->CX = M->CY = 0.0f;


	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_SetScaling(geXForm3d *M,geFloat x, geFloat y, geFloat z)
	// sets up a transform that scales by x,y,z
{
	assert( M != NULL );
	assert( x * x >= 0.0f);
	assert( y * y >= 0.0f);
	assert( z * z >= 0.0f);
	assert( x > GEXFORM3D_MINIMUM_SCALE );
	assert( y > GEXFORM3D_MINIMUM_SCALE );
	assert( z > GEXFORM3D_MINIMUM_SCALE );

	M->AX = x;
	M->BY = y;
	M->CZ = z;

	M->AY = M->AZ = 0.0f;
	M->BX = M->BZ = 0.0f;
	M->CX = M->CY = 0.0f;
	M->Translation.X = M->Translation.Y = M->Translation.Z = 0.0f;


	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_RotateX(geXForm3d *M,geFloat RadianAngle)
	// Rotates M by RadianAngle about X axis
{
	geXForm3d R;
	assert( M != NULL );
	assert( RadianAngle * RadianAngle >= 0.0f );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	geXForm3d_SetXRotation(&R,RadianAngle);
	geXForm3d_Multiply(&R, M, M);
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_RotateY(geXForm3d *M,geFloat RadianAngle)
	// Rotates M by RadianAngle about Y axis
{
	geXForm3d R;
	assert( M != NULL );
	assert( RadianAngle * RadianAngle >= 0.0f );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	geXForm3d_SetYRotation(&R,RadianAngle);
	geXForm3d_Multiply(&R, M, M);

	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_RotateZ(geXForm3d *M,geFloat RadianAngle)
	// Rotates M by RadianAngle about Z axis
{
	geXForm3d R;
	assert( M != NULL );
	assert( RadianAngle * RadianAngle >= 0.0f );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	geXForm3d_SetZRotation(&R,RadianAngle);
	geXForm3d_Multiply(&R, M, M);

	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
}


GENESISAPI void GENESISCC geXForm3d_Translate(geXForm3d *M,geFloat x, geFloat y, geFloat z)
	// Translates M by x,y,z
{
	geXForm3d T;
	assert( M != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	geXForm3d_SetTranslation(&T,x,y,z);
	geXForm3d_Multiply(&T, M, M);

	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_Scale(geXForm3d *M,geFloat x, geFloat y, geFloat z)
	// Scales M by x,y,z
{
	geXForm3d S;
	assert( M != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	geXForm3d_SetScaling(&S,x,y,z);
	geXForm3d_Multiply(&S, M, M);

	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_Multiply(
	const geXForm3d *M1, 
	const geXForm3d *M2, 
	geXForm3d *MProduct)
	// MProduct = matrix multiply of M1*M2
{
	geXForm3d M1L;
	geXForm3d M2L;

	assert( M1       != NULL );
	assert( M2       != NULL );
	assert( MProduct != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M1) == GE_TRUE );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M2) == GE_TRUE );

	M1L = *M1;
	M2L = *M2;

	MProduct->AX =  M1L.AX * M2L.AX;
	MProduct->AX += M1L.AY * M2L.BX;
	MProduct->AX += M1L.AZ * M2L.CX;

	MProduct->AY =  M1L.AX * M2L.AY;
	MProduct->AY += M1L.AY * M2L.BY;
	MProduct->AY += M1L.AZ * M2L.CY;

	MProduct->AZ =  M1L.AX * M2L.AZ;
	MProduct->AZ += M1L.AY * M2L.BZ;
	MProduct->AZ += M1L.AZ * M2L.CZ;

	MProduct->BX =  M1L.BX * M2L.AX;
	MProduct->BX += M1L.BY * M2L.BX;
	MProduct->BX += M1L.BZ * M2L.CX;

	MProduct->BY =  M1L.BX * M2L.AY;
	MProduct->BY += M1L.BY * M2L.BY;
	MProduct->BY += M1L.BZ * M2L.CY;

	MProduct->BZ =  M1L.BX * M2L.AZ;
	MProduct->BZ += M1L.BY * M2L.BZ;
	MProduct->BZ += M1L.BZ * M2L.CZ;

	MProduct->CX =  M1L.CX * M2L.AX;
	MProduct->CX += M1L.CY * M2L.BX;
	MProduct->CX += M1L.CZ * M2L.CX;

	MProduct->CY =  M1L.CX * M2L.AY;
	MProduct->CY += M1L.CY * M2L.BY;
	MProduct->CY += M1L.CZ * M2L.CY;

	MProduct->CZ =  M1L.CX * M2L.AZ;
	MProduct->CZ += M1L.CY * M2L.BZ;
	MProduct->CZ += M1L.CZ * M2L.CZ;

	MProduct->Translation.X =  M1L.AX * M2L.Translation.X;
	MProduct->Translation.X += M1L.AY * M2L.Translation.Y;
	MProduct->Translation.X += M1L.AZ * M2L.Translation.Z;
	MProduct->Translation.X += M1L.Translation.X;

	MProduct->Translation.Y =  M1L.BX * M2L.Translation.X;
	MProduct->Translation.Y += M1L.BY * M2L.Translation.Y;
	MProduct->Translation.Y += M1L.BZ * M2L.Translation.Z;
	MProduct->Translation.Y += M1L.Translation.Y;

	MProduct->Translation.Z =  M1L.CX * M2L.Translation.X;
	MProduct->Translation.Z += M1L.CY * M2L.Translation.Y;
	MProduct->Translation.Z += M1L.CZ * M2L.Translation.Z;
	MProduct->Translation.Z += M1L.Translation.Z;
	
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(MProduct) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_Transform(
	const geXForm3d *M,
	const geVec3d *V, 
	geVec3d *Result)
	// Result is Matrix M * Vector V:  V Tranformed by M 
{
	geVec3d VL;
	assert( M != NULL );
	assert( geVec3d_IsValid(V)!=GE_FALSE);

	assert( Result != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	VL = *V;

	Result->X = (VL.X * M->AX) + (VL.Y * M->AY) + (VL.Z * M->AZ) + M->Translation.X;
	Result->Y = (VL.X * M->BX) + (VL.Y * M->BY) + (VL.Z * M->BZ) + M->Translation.Y;
	Result->Z = (VL.X * M->CX) + (VL.Y * M->CY) + (VL.Z * M->CZ) + M->Translation.Z;
	geXForm3d_Assert( geVec3d_IsValid(Result)!=GE_FALSE);

}


//========================================================================================
//	geXForm3d_TransformArray
//	Assembly version 
//========================================================================================
GENESISAPI void GENESISCC geXForm3d_TransformArray(const geXForm3d *XForm, const geVec3d *Source, geVec3d *Dest, int32 Count)
{
	#define FSIZE	4
	static int32 Lookup[]={0x696C6345,0x21657370};
	assert( XForm != NULL );
	assert( Source != NULL );
	assert( Dest != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(XForm) == GE_TRUE );


	if (Count <= 0)								// Early out if possible
		return;

	_asm 
	{
	mov     ecx,Count							// get item count
	mov     esi,Source							// get source array pointer
	mov     ebx,Dest							// get dest array pointer
	mov     edi,XForm							// point to matrix
	imul    ecx,ecx,3*FSIZE						// ecx is size of array
	add     esi,ecx								// esi points to source end
	add     ebx,ecx								// edi pointe to dest end
	neg     ecx									// ecx ready for count-up
          
Again:	
	// Multiply
	fld   dword ptr [esi+ecx+0*FSIZE]			// 1;i1
	fmul  dword ptr [edi+(0+0*3)*FSIZE]			// 1;m11
	fld   dword ptr [esi+ecx+1*FSIZE]			// 1;m11 i2
	fmul  dword ptr [edi+(1+0*3)*FSIZE]			// 1;m11 m12
	fld   dword ptr [esi+ecx+2*FSIZE]			// 1;m11 m12 i3
	fmul  dword ptr [edi+(2+0*3)*FSIZE]			// 1;m11 m12 m13
	fxch  st(1)									// 0;m11 m13 m12
	faddp st(2),st								// 1;s1a m13
	fld   dword ptr [esi+ecx+0*FSIZE]			// 1;s1a m13 i1
	fmul  dword ptr [edi+(0+1*3)*FSIZE]			// 1;s1a m13 m21
	fxch  st(1)									// 0;s1a m21 m13
	faddp st(2),st								// 1;s1b m21
	fld   dword ptr [esi+ecx+1*FSIZE]			// 1;s1b m21 i2
	fmul  dword ptr [edi+(1+1*3)*FSIZE]			// 1;s1b m21 m22
	fld   dword ptr [esi+ecx+2*FSIZE]			// 1;s1b m21 m22 i3
	fmul  dword ptr [edi+(2+1*3)*FSIZE]			// 1;s1b m21 m22 m23
	fxch  st(1)									// 0;s1b m21 m23 m22
	faddp st(2),st								// 1;s1b s2a m23
	fld   dword ptr [esi+ecx+0*FSIZE]			// 1;s1b s2a m23 i1
	fmul  dword ptr [edi+(0+2*3)*FSIZE]			// 1;s1b s2a m23 m31
	fxch  st(1)									// 0;s1b s2a m31 m23
	faddp st(2),st								// 1;s1b s2b m31
	fld   dword ptr [esi+ecx+1*FSIZE]			// 1;s1b s2b m31 i2
	fmul  dword ptr [edi+(1+2*3)*FSIZE]			// 1;s1b s2b m31 m32
	fld   dword ptr [esi+ecx+2*FSIZE]			// 1;s1b s2b m31 m32 i3
	fmul  dword ptr [edi+(2+2*3)*FSIZE]			// 1;s1b s2b m31 m32 m33
	// Add translation
	fxch  st(1)									// 0;s1b s2b m31 m33 m32
	faddp st(2),st								// 1;s1b s2b s3a m33
	fxch  st(3)									// 0;m33 s2b s3a s1b
	fadd  dword ptr [edi+(9+0)*FSIZE]			// 1;m33 s2b s3a s1c
	fxch  st(1)									// 0;m33 s2b s1c s3a 
	faddp st(3),st								// 1;s3b s2b s1c
	fxch  st(1)									// 0;s3b s1c s2b
	fadd  dword ptr [edi+(9+1)*FSIZE]			// 1;s3b s1c s2c
	fxch  st(2)									// 0;s2c s1c s3b
	fadd  dword ptr [edi+(9+2)*FSIZE]			// 1;s2c s1c s3c
	fxch  st(1)									// 0;s2c s3c s1c
	fstp  dword ptr [ebx+ecx+0*FSIZE]			// 2;s2c s3c    
	fxch  st(1)									// 0;s3c s2c    
	fstp  dword ptr [ebx+ecx+1*FSIZE]			// 2;s3c
	fstp  dword ptr [ebx+ecx+2*FSIZE]			// 2;
	add   ecx,3*FSIZE							// 1;

	cmp ecx, 0
	jne Again
	}

	// 34 cycles predicted (per loop)
	// 39 cycles measured
}

GENESISAPI void GENESISCC geXForm3d_Rotate(
	const geXForm3d *M,
	const geVec3d *V, 
	geVec3d *Result)
	// Result is Matrix M * Vector V:  V Rotated by M (no translation)
{
	geVec3d VL;
	assert( M != NULL );
	assert( Result != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
	assert( geVec3d_IsValid(V)!=GE_FALSE);

	VL = *V;

	Result->X = (VL.X * M->AX) + (VL.Y * M->AY) + (VL.Z * M->AZ);
	Result->Y = (VL.X * M->BX) + (VL.Y * M->BY) + (VL.Z * M->BZ);
	Result->Z = (VL.X * M->CX) + (VL.Y * M->CY) + (VL.Z * M->CZ);
	geXForm3d_Assert( geVec3d_IsValid(Result)!=GE_FALSE);
}


GENESISAPI void GENESISCC geXForm3d_GetLeft(const geXForm3d *M, geVec3d *Left)
	// Gets a vector that is 'left' in the frame of reference of M (facing -Z)
{
	assert( M     != NULL );
	assert( Left != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
	
	Left->X = -M->AX;
	Left->Y = -M->BX;
	Left->Z = -M->CX;
	geXForm3d_Assert( geVec3d_IsValid(Left)!=GE_FALSE);
}

GENESISAPI void GENESISCC geXForm3d_GetUp(const geXForm3d *M,    geVec3d *Up)
	// Gets a vector that is 'up' in the frame of reference of M (facing -Z)
{
	assert( M  != NULL );
	assert( Up != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );
	
	Up->X = M->AY;
	Up->Y = M->BY;
	Up->Z = M->CY;
	geXForm3d_Assert( geVec3d_IsValid(Up)!=GE_FALSE);
}

GENESISAPI void GENESISCC geXForm3d_GetIn(const geXForm3d *M,  geVec3d *In)
	// Gets a vector that is 'in' in the frame of reference of M (facing -Z)
{
	assert( M    != NULL );
	assert( In != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	In->X = -M->AZ;
	In->Y = -M->BZ;
	In->Z = -M->CZ;
	geXForm3d_Assert( geVec3d_IsValid(In)!=GE_FALSE);
}

GENESISAPI void GENESISCC geXForm3d_GetTranspose(const geXForm3d *M, geXForm3d *MInv)
{
	geXForm3d M1;
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	M1 = *M;

	MInv->AX = M1.AX;
	MInv->AY = M1.BX;
	MInv->AZ = M1.CX;

	MInv->BX = M1.AY;
	MInv->BY = M1.BY;
	MInv->BZ = M1.CY;

	MInv->CX = M1.AZ;
	MInv->CY = M1.BZ;
	MInv->CZ = M1.CZ;

	MInv->Translation.X = 0.0f;
	MInv->Translation.Y = 0.0f;
	MInv->Translation.Z = 0.0f;

	{
		geXForm3d T;
		geXForm3d_SetTranslation(&T,-M1.Translation.X,-M1.Translation.Y,-M1.Translation.Z);
		geXForm3d_Multiply(MInv,&T,MInv);
	}

	geXForm3d_Assert ( geXForm3d_IsOrthogonal(MInv) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_TransposeTransform(
	const geXForm3d *M, 
	const geVec3d *V, 
	geVec3d *Result)
	// applies the Transpose transform of M to V.  Result = (M^T) * V
{
	geVec3d V1;

	assert( M      != NULL );
	assert( geVec3d_IsValid(V)!=GE_FALSE);

	assert( Result != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(M) == GE_TRUE );

	V1.X = V->X - M->Translation.X;
	V1.Y = V->Y - M->Translation.Y;
	V1.Z = V->Z - M->Translation.Z;

	Result->X = (V1.X * M->AX) + (V1.Y * M->BX) + (V1.Z * M->CX);
	Result->Y = (V1.X * M->AY) + (V1.Y * M->BY) + (V1.Z * M->CY);
	Result->Z = (V1.X * M->AZ) + (V1.Y * M->BZ) + (V1.Z * M->CZ);
	geXForm3d_Assert( geVec3d_IsValid(Result)!=GE_FALSE);
}


GENESISAPI void GENESISCC geXForm3d_Copy(
	const geXForm3d *Src, 
	geXForm3d *Dst)
{	
	assert( Src != NULL );
	assert( Dst != NULL );
	geXForm3d_Assert ( geXForm3d_IsOrthogonal(Src) == GE_TRUE );

	*Dst = *Src;
}    

GENESISAPI void GENESISCC geXForm3d_GetEulerAngles(const geXForm3d *M, geVec3d *Angles)
	// order of angles z,y,x
{
	geFloat AZ;
	assert( M      != NULL );
	assert( Angles != NULL );

	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
	
	//ack.  due to floating point error, the value can drift away from 1.0 a bit
	//      this will clamp it.  The _IsOrthonormal test will pass because it allows
	//      for a tolerance.

	AZ = M->AZ;
	if (AZ > 1.0f) 
		AZ = 1.0f;
	if (AZ < -1.0f) 
		AZ = -1.0f;

	Angles->Y = -(geFloat)asin(-AZ);

	if ( cos(Angles->Y) != 0 )
	{
		Angles->X = -(geFloat)atan2(M->BZ, M->CZ);
		Angles->Z = -(geFloat)atan2(M->AY, M->AX);
	}
	else
	{
		Angles->X = -(geFloat)atan2(M->BX, M->BY);
		Angles->Z = 0.0f;
	}
	assert( geVec3d_IsValid(Angles)!=GE_FALSE);
}


GENESISAPI void GENESISCC geXForm3d_SetEulerAngles(geXForm3d *M, const geVec3d *Angles)
	// order of angles z,y,x
{
	geXForm3d XM, YM, ZM;							            

	assert( M      != NULL );
	assert( geVec3d_IsValid(Angles)!=GE_FALSE);
	
	geXForm3d_SetXRotation(&XM,Angles->X);
	geXForm3d_SetYRotation(&YM,Angles->Y);
	geXForm3d_SetZRotation(&ZM,Angles->Z);
	
	geXForm3d_Multiply(&XM, &YM, M);
	geXForm3d_Multiply(M, &ZM, M);
	

	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );

}

GENESISAPI geBoolean GENESISCC geXForm3d_IsOrthonormal(const geXForm3d *M)
	// returns GE_TRUE if M is orthonormal 
	// (if the rows and columns are all normalized (transform has no scaling or shearing)
	// and is orthogonal (row1 cross row2 = row3 & col1 cross col2 = col3)
{
#define ORTHONORMAL_TOLERANCE ((geFloat)(0.001f))
	geVec3d Col1,Col2,Col3;
	geVec3d Col1CrossCol2;
	geBoolean IsOrthonormal;
	assert( M != NULL );

	geXForm3d_Assert ( geXForm3d_IsValid(M) == GE_TRUE );

	Col1.X = M->AX;
	Col1.Y = M->BX;
	Col1.Z = M->CX;
	
	Col2.X = M->AY;
	Col2.Y = M->BY;
	Col2.Z = M->CY;

	Col3.X = M->AZ;
	Col3.Y = M->BZ;
	Col3.Z = M->CZ;

	geVec3d_CrossProduct(&Col1,&Col2,&Col1CrossCol2);

	IsOrthonormal = geVec3d_Compare(&Col1CrossCol2,&Col3,ORTHONORMAL_TOLERANCE);
	if (IsOrthonormal == GE_FALSE)
		{
			geVec3d_Inverse(&Col3);
			IsOrthonormal = geVec3d_Compare(&Col1CrossCol2,&Col3,ORTHONORMAL_TOLERANCE);
		}

	if ( geVec3d_IsValid(&(M->Translation)) ==GE_FALSE)
		return GE_FALSE;

	return IsOrthonormal;
}


GENESISAPI void GENESISCC geXForm3d_Orthonormalize(geXForm3d *M)
	// essentially removes scaling (or other distortions) from 
	// an orthogonal (or nearly orthogonal) matrix 
{
	geVec3d Col1,Col2,Col3;
	assert( M != NULL );
	geXForm3d_Assert ( geXForm3d_IsValid(M) == GE_TRUE );

	Col1.X = M->AX;
	Col1.Y = M->BX;
	Col1.Z = M->CX;
	geVec3d_Normalize(&Col1);
	M->AX = Col1.X;
	M->BX = Col1.Y;
	M->CX = Col1.Z;
	
	Col2.X = M->AY;
	Col2.Y = M->BY;
	Col2.Z = M->CY;
	geVec3d_Normalize(&Col2);
	M->AY = Col2.X;
	M->BY = Col2.Y;
	M->CY = Col2.Z;

	geVec3d_CrossProduct(&Col1,&Col2,&Col3);

	M->AZ = Col3.X;
	M->BZ = Col3.Y;
	M->CZ = Col3.Z;

	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
}




GENESISAPI geBoolean GENESISCC geXForm3d_IsOrthogonal(const geXForm3d *M)
	// returns GE_TRUE if M is orthogonal
	// (row1 cross row2 = row3 & col1 cross col2 = col3)
{
#define ORTHOGONAL_TOLERANCE ((geFloat)(0.001f))
	geVec3d Col1,Col2,Col3;
	geVec3d Col1CrossCol2;
	geBoolean IsOrthogonal;
	assert( M != NULL );
	geXForm3d_Assert ( geXForm3d_IsValid(M) == GE_TRUE );

	//return GE_TRUE;

	Col1.X = M->AX;
	Col1.Y = M->BX;
	Col1.Z = M->CX;
	//geVec3d_Normalize(&Col1);
	
	Col2.X = M->AY;
	Col2.Y = M->BY;
	Col2.Z = M->CY;
	//geVec3d_Normalize(&Col2);
	
	Col3.X = M->AZ;
	Col3.Y = M->BZ;
	Col3.Z = M->CZ;
	geVec3d_Normalize(&Col3);
	
	geVec3d_CrossProduct(&Col1,&Col2,&Col1CrossCol2);
		
	geVec3d_Normalize(&Col1CrossCol2);
	
	IsOrthogonal = geVec3d_Compare(&Col1CrossCol2,&Col3,ORTHOGONAL_TOLERANCE);
	if (IsOrthogonal == GE_FALSE)
		{
			geVec3d_Inverse(&Col3);
			IsOrthogonal = geVec3d_Compare(&Col1CrossCol2,&Col3,ORTHOGONAL_TOLERANCE);
		}

	if ( geVec3d_IsValid(&(M->Translation)) ==GE_FALSE)
		return GE_FALSE;

	return IsOrthogonal;
}

GENESISAPI void GENESISCC geXForm3d_SetFromLeftUpIn(
	geXForm3d *M,
	const geVec3d *Left, 
	const geVec3d *Up, 
	const geVec3d *In)
{
	assert(M);
	assert(Left);
	assert(Up);
	assert(In);
	geXForm3d_Assert(geVec3d_IsNormalized(Left));
	geXForm3d_Assert(geVec3d_IsNormalized(Up));
	geXForm3d_Assert(geVec3d_IsNormalized(In));

	M->AX = -Left->X;
	M->BX = -Left->Y;
	M->CX = -Left->Z;
	M->AY =  Up->X;
	M->BY =  Up->Y;
	M->CY =  Up->Z;
	M->AZ = -In->X;
	M->BZ = -In->Y;
	M->CZ = -In->Z;

	geVec3d_Clear(&M->Translation);


	geXForm3d_Assert ( geXForm3d_IsOrthonormal(M) == GE_TRUE );
}

GENESISAPI void GENESISCC geXForm3d_Mirror(
	const		geXForm3d *Source, 
	const		geVec3d *PlaneNormal, 
	float		PlaneDist, 
	geXForm3d	*Dest)
{
	float			Dist;
	geVec3d			In, Left, Up;
	geXForm3d		Original;
	geVec3d			MirrorTranslation;

	geXForm3d_Assert ( geXForm3d_IsOrthogonal(Source) == GE_TRUE );
	assert( PlaneNormal != NULL );
	assert( Dest        != NULL );

	geXForm3d_Copy(Source, &Original);

	// Mirror the translation portion of the matrix
	Dist = geVec3d_DotProduct(&Original.Translation, PlaneNormal) - PlaneDist;
	geVec3d_AddScaled(&Original.Translation, PlaneNormal, -Dist*2.0f, &MirrorTranslation);

	// Mirror the Rotational portion of the xform first
	geXForm3d_GetIn(&Original, &In);
	geVec3d_Add(&Original.Translation, &In, &In);
	Dist = geVec3d_DotProduct(&In, PlaneNormal) - PlaneDist;
	geVec3d_AddScaled(&In, PlaneNormal, -Dist*2.0f, &In);
	geVec3d_Subtract(&In, &MirrorTranslation, &In);
	geVec3d_Normalize(&In);

	geXForm3d_GetLeft(&Original, &Left);
	geVec3d_Add(&Original.Translation, &Left, &Left);
	Dist = geVec3d_DotProduct(&Left, PlaneNormal) - PlaneDist;
	geVec3d_AddScaled(&Left, PlaneNormal, -Dist*2.0f, &Left);
	geVec3d_Subtract(&Left, &MirrorTranslation, &Left);
	geVec3d_Normalize(&Left);

	geXForm3d_GetUp(&Original, &Up);
	geVec3d_Add(&Original.Translation, &Up, &Up);
	Dist = geVec3d_DotProduct(&Up, PlaneNormal) - PlaneDist;
	geVec3d_AddScaled(&Up, PlaneNormal, -Dist*2.0f, &Up);
	geVec3d_Subtract(&Up, &MirrorTranslation, &Up);
	geVec3d_Normalize(&Up);

	geXForm3d_SetFromLeftUpIn(Dest, &Left, &Up, &In);

	// Must set the mirror translation here since geXForm3d_SetFromLeftUpIn cleared the translation portion
	geVec3d_Set(&Dest->Translation, MirrorTranslation.X, MirrorTranslation.Y, MirrorTranslation.Z);

	geXForm3d_Assert ( geXForm3d_IsOrthogonal(Dest) == GE_TRUE );
}

