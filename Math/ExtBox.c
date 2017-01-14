/****************************************************************************************/
/*  EXTBOX.C                                                                            */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Axial aligned bounding box support                                     */
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
#include "ExtBox.h"
#include <assert.h>

#define MAX(aa,bb)   ( ((aa)>(bb))?(aa):(bb) )
#define MIN(aa,bb)   ( ((aa)<(bb))?(aa):(bb) )

geBoolean GENESISCC geExtBox_IsValid(  const geExtBox *B )
{
	if (B == NULL) return GE_FALSE;
	
	if (geVec3d_IsValid(&(B->Min)) == GE_FALSE)
		return GE_FALSE;
	if (geVec3d_IsValid(&(B->Max)) == GE_FALSE)
		return GE_FALSE;


	if (    (B->Min.X <= B->Max.X) &&
			(B->Min.Y <= B->Max.Y) &&
			(B->Min.Z <= B->Max.Z)   	)
		return GE_TRUE;
	else
		return GE_FALSE;
}

void GENESISCC geExtBox_Set(  geExtBox *B,
					geFloat X1,geFloat Y1,geFloat Z1,
					geFloat X2,geFloat Y2,geFloat Z2)
{
	assert (B != NULL);

	//geVec3d_Set	(&B->Min, MIN (x1, x2),	MIN (y1, y2),MIN (z1, z2));
	//geVec3d_Set (&B->Max, MAX (x1, x2),	MAX (y1, y2),MAX (z1, z2));

	if ( X1 > X2 )
		{	B->Max.X = X1;	B->Min.X = X2;	}
	else
		{	B->Max.X = X2;  B->Min.X = X1;  }
	
	if ( Y1 > Y2 )
		{	B->Max.Y = Y1;	B->Min.Y = Y2;	}
	else
		{	B->Max.Y = Y2;  B->Min.Y = Y1;  }
	
	if ( Z1 > Z2 )
		{	B->Max.Z = Z1;	B->Min.Z = Z2;	}
	else
		{	B->Max.Z = Z2;  B->Min.Z = Z1;  }

	assert( geVec3d_IsValid(&(B->Min)) != GE_FALSE );
	assert( geVec3d_IsValid(&(B->Max)) != GE_FALSE );

}

// Set box Min and Max to the passed point
void GENESISCC geExtBox_SetToPoint ( geExtBox *B, const geVec3d *Point )
{
	assert( B     != NULL );
	assert( Point != NULL );
	assert( geVec3d_IsValid(Point) != GE_FALSE );

	
	B->Max = *Point;
	B->Min = *Point;
}

// Extend a box to encompass the passed point
void GENESISCC geExtBox_ExtendToEnclose( geExtBox *B, const geVec3d *Point )
{
	assert ( geExtBox_IsValid(B) != GE_FALSE );
	assert( Point != NULL );
	assert( geVec3d_IsValid(Point) != GE_FALSE );

	if (Point->X > B->Max.X ) B->Max.X = Point->X;
	if (Point->Y > B->Max.Y ) B->Max.Y = Point->Y;
	if (Point->Z > B->Max.Z ) B->Max.Z = Point->Z;

	if (Point->X < B->Min.X ) B->Min.X = Point->X;
	if (Point->Y < B->Min.Y ) B->Min.Y = Point->Y;
	if (Point->Z < B->Min.Z ) B->Min.Z = Point->Z;

}

static geBoolean GENESISCC geExtBox_Intersects(  const geExtBox *B1,  const geExtBox *B2 )
{
	assert ( geExtBox_IsValid (B1) != GE_FALSE );
	assert ( geExtBox_IsValid (B2) != GE_FALSE );

	if ((B1->Min.X > B2->Max.X) || (B1->Max.X < B2->Min.X)) return GE_FALSE;
	if ((B1->Min.Y > B2->Max.Y) || (B1->Max.Y < B2->Min.Y)) return GE_FALSE;
	if ((B1->Min.Z > B2->Max.Z) || (B1->Max.Z < B2->Min.Z)) return GE_FALSE;
	return GE_TRUE;
}


	
geBoolean GENESISCC geExtBox_Intersection( const geExtBox *B1, const geExtBox *B2, geExtBox *Result )
{
	geBoolean rslt;

	assert ( geExtBox_IsValid (B1) != GE_FALSE );
	assert ( geExtBox_IsValid (B2) != GE_FALSE );

	rslt = geExtBox_Intersects (B1, B2);
	if ( (rslt != GE_FALSE) && (Result != NULL))
		{
			geExtBox_Set ( Result,
						MAX (B1->Min.X, B2->Min.X),
						MAX (B1->Min.Y, B2->Min.Y),
						MAX (B1->Min.Z, B2->Min.Z),
						MIN (B1->Max.X, B2->Max.X),
						MIN (B1->Max.Y, B2->Max.Y),
						MIN (B1->Max.Z, B2->Max.Z) );
		}
	return rslt;
}

void GENESISCC geExtBox_Union( const geExtBox *B1, const geExtBox *B2, geExtBox *Result )
{
	assert ( geExtBox_IsValid (B1) != GE_FALSE );
	assert ( geExtBox_IsValid (B2) != GE_FALSE );
	assert (Result != NULL);

	geExtBox_Set (	Result,
				MIN (B1->Min.X, B2->Min.X),
				MIN (B1->Min.Y, B2->Min.Y),
				MIN (B1->Min.Z, B2->Min.Z),
				MAX (B1->Max.X, B2->Max.X),
				MAX (B1->Max.Y, B2->Max.Y),
				MAX (B1->Max.Z, B2->Max.Z) );
}

geBoolean GENESISCC geExtBox_ContainsPoint(  const geExtBox *B,  const geVec3d *Point )
{
	assert (geExtBox_IsValid (B) != GE_FALSE);
	assert( geVec3d_IsValid(Point) != GE_FALSE );

	if (    (Point->X >= B->Min.X) && (Point->X <= B->Max.X) &&
			(Point->Y >= B->Min.Y) && (Point->Y <= B->Max.Y) &&
			(Point->Z >= B->Min.Z) && (Point->Z <= B->Max.Z)     )
		{
			return GE_TRUE;
		}
	else
		{
			return GE_FALSE;
		}
}


void GENESISCC geExtBox_GetTranslation( const geExtBox *B, geVec3d *pCenter )
{
	assert (geExtBox_IsValid (B) != GE_FALSE);
	assert (pCenter != NULL);

	geVec3d_Set( pCenter,
				(B->Min.X + B->Max.X)/2.0f,
				(B->Min.Y + B->Max.Y)/2.0f,
				(B->Min.Z + B->Max.Z)/2.0f );
}

void GENESISCC geExtBox_Translate(  geExtBox *B,  geFloat DX,  geFloat DY,  geFloat DZ	)
{
	geVec3d VecDelta;

	assert (geExtBox_IsValid (B) != GE_FALSE);

	geVec3d_Set (&VecDelta, DX, DY, DZ);
		assert( geVec3d_IsValid(&VecDelta) != GE_FALSE );
	geVec3d_Add (&B->Min, &VecDelta, &B->Min);
	geVec3d_Add (&B->Max, &VecDelta, &B->Max);
}

void GENESISCC geExtBox_SetTranslation( geExtBox *B, const geVec3d *pCenter )
{
	geVec3d Center,Translation;

	assert (geExtBox_IsValid (B) != GE_FALSE);
	assert (pCenter != NULL);
	assert( geVec3d_IsValid(pCenter) != GE_FALSE );

	geExtBox_GetTranslation( B, &Center );
	geVec3d_Subtract( pCenter, &Center, &Translation);

	geExtBox_Translate( B, Translation.X, Translation.Y, Translation.Z );
}

void GENESISCC geExtBox_GetScaling( const geExtBox *B, geVec3d *pScale )
{
	assert (geExtBox_IsValid (B) != GE_FALSE );
	assert (pScale != NULL);

	geVec3d_Subtract( &(B->Max), &(B->Min), pScale );
}

void GENESISCC geExtBox_Scale( geExtBox *B, geFloat ScaleX, geFloat ScaleY, geFloat ScaleZ )
{
	geVec3d Center;
	geVec3d Scale;
	geFloat DX,DY,DZ;

	assert (geExtBox_IsValid (B) != GE_FALSE );
	assert (ScaleX >= 0.0f );
	assert (ScaleY >= 0.0f );
	assert (ScaleZ >= 0.0f );
	assert (ScaleX * ScaleX >= 0.0f );		// check for NANS
	assert (ScaleY * ScaleY >= 0.0f );
	assert (ScaleZ * ScaleZ >= 0.0f );

	geExtBox_GetTranslation( B, &Center );
	geExtBox_GetScaling    ( B, &Scale  );
	
	DX = ScaleX * Scale.X * 0.5f;
	DY = ScaleY * Scale.Y * 0.5f;
	DZ = ScaleZ * Scale.Z * 0.5f;

	B->Min.X = Center.X - DX;
	B->Min.Y = Center.Y - DY;
	B->Min.Z = Center.Z - DZ;
	
	B->Max.X = Center.X + DX;
	B->Max.Y = Center.Y + DY;
	B->Max.Z = Center.Z + DZ;
	
	assert (geExtBox_IsValid (B) != GE_FALSE);
}

void GENESISCC geExtBox_SetScaling( geExtBox *B, const geVec3d *pScale )
{
	geVec3d Center;
	geFloat DX,DY,DZ;

	assert (geExtBox_IsValid (B) != GE_FALSE );
	assert (pScale != NULL );
	assert (geVec3d_IsValid( pScale )!= GE_FALSE);
	assert (pScale->X >= 0.0f );
	assert (pScale->Y >= 0.0f );
	assert (pScale->Z >= 0.0f );

	geExtBox_GetTranslation( B, &Center );

	DX = pScale->X / 2.0f;
	DY = pScale->Y / 2.0f;
	DZ = pScale->Z / 2.0f;

	B->Min.X = Center.X - DX;
	B->Min.Y = Center.Y - DY;
	B->Min.Z = Center.Z - DZ;
	
	B->Max.X = Center.X + DX;
	B->Max.Y = Center.Y + DY;
	B->Max.Z = Center.Z + DZ;
}

void GENESISCC geExtBox_LinearSweep(	const geExtBox *BoxToSweep, 
						const geVec3d *StartPoint, 
						const geVec3d *EndPoint, 
						geExtBox *EnclosingBox )
{

	assert (geExtBox_IsValid (BoxToSweep) != GE_FALSE );
	assert (StartPoint   != NULL );
	assert (EndPoint     != NULL );
	assert (geVec3d_IsValid( StartPoint )!= GE_FALSE);
	assert (geVec3d_IsValid( EndPoint   )!= GE_FALSE);
	assert (EnclosingBox != NULL );

	*EnclosingBox = *BoxToSweep;

	if (EndPoint->X > StartPoint->X)
		{
			EnclosingBox->Min.X += StartPoint->X; 
			EnclosingBox->Max.X += EndPoint->X; 
		}
	else
		{
			EnclosingBox->Min.X += EndPoint->X; 
			EnclosingBox->Max.X += StartPoint->X; 
		}

	if (EndPoint->Y > StartPoint->Y)
		{
			EnclosingBox->Min.Y += StartPoint->Y; 
			EnclosingBox->Max.Y += EndPoint->Y; 
		}
	else
		{
			EnclosingBox->Min.Y += EndPoint->Y; 
			EnclosingBox->Max.Y += StartPoint->Y; 
		}

	if (EndPoint->Z > StartPoint->Z)
		{
			EnclosingBox->Min.Z += StartPoint->Z; 
			EnclosingBox->Max.Z += EndPoint->Z; 
		}
	else
		{
			EnclosingBox->Min.Z += EndPoint->Z; 
			EnclosingBox->Max.Z += StartPoint->Z; 
		}
	assert (geExtBox_IsValid (EnclosingBox) != GE_FALSE );
}

static geBoolean GENESISCC geExtBox_XFaceDist(  const geVec3d *Start, 
												const geVec3d *Delta, const geExtBox *B, geFloat *T, geFloat X)
{
	geFloat t;
	geFloat Y,Z;
	assert( Start != NULL );
	assert( Delta != NULL );
	assert( B     != NULL );
	assert( T     != NULL );

	//if ( (Start->X <= X) && (X <= Delta->X + Start->X) )
		{
			t = (X - Start->X)/Delta->X;
			Y  = Start->Y + Delta->Y * t;
			if ( ( B->Min.Y <= Y) && (Y <= B->Max.Y) )
				{
					Z = Start->Z + Delta->Z * t;
					if ( ( B->Min.Z <= Z) && (Z <= B->Max.Z) )
						{
							*T = t;
							return GE_TRUE;
						}
				}
		}
	return GE_FALSE;
}

static geBoolean GENESISCC geExtBox_YFaceDist(  const geVec3d *Start, const geVec3d *Delta, const geExtBox *B, geFloat *T, geFloat Y)
{
	geFloat t;
	geFloat X,Z;
	assert( Start != NULL );
	assert( Delta != NULL );
	assert( B     != NULL );
	assert( T     != NULL );

	//if ( (Start->Y <= Y) && (Y <= Delta->Y + Start->Y) )
		{
			t = (Y - Start->Y)/Delta->Y;
			Z  = Start->Z + Delta->Z * t;
			if ( ( B->Min.Z <= Z) && (Z <= B->Max.Z) )
				{
					X = Start->X + Delta->X * t;
					if ( ( B->Min.X <= X) && (X <= B->Max.X) )
						{
							*T = t;
							return GE_TRUE;
						}
				}
		}
	return GE_FALSE;
}


static geBoolean GENESISCC geExtBox_ZFaceDist(  const geVec3d *Start, const geVec3d *Delta, const geExtBox *B, geFloat *T, geFloat Z)
{
	geFloat t;
	geFloat X,Y;
	assert( Start != NULL );
	assert( Delta != NULL );
	assert( B     != NULL );
	assert( T     != NULL );

	//if ( (Start->Z <= Z) && (Z <= Delta->Z + Start->Z) )
		{
			t = (Z - Start->Z)/Delta->Z;
			X  = Start->X + Delta->X * t;
			if ( ( B->Min.X <= X) && (X <= B->Max.X) )
				{
					Y = Start->Y + Delta->Y * t;
					if ( ( B->Min.Y <= Y) && (Y <= B->Max.Y) )
						{
							*T = t;
							return GE_TRUE;
						}
				}
		}
	return GE_FALSE;
}



geBoolean GENESISCC geExtBox_RayCollision( const geExtBox *B, const geVec3d *Start, const geVec3d *End, 
								geFloat *T, geVec3d *Normal )
{
	// only detects rays going 'in' to the box
	geFloat t;
	geVec3d Delta;
	geVec3d LocalNormal;
	geFloat LocalT;

	assert( B != NULL );
	assert( Start != NULL );
	assert( End != NULL );
	assert (geVec3d_IsValid( Start )!= GE_FALSE);
	assert (geVec3d_IsValid( End   )!= GE_FALSE);
	assert (geExtBox_IsValid( B )!= GE_FALSE );

	geVec3d_Subtract(End,Start,&Delta);
	
	if (Normal == NULL)
		Normal = &LocalNormal;
	if (T == NULL)
		T = &LocalT;
	
	// test x end of box, facing away from ray direction.
	if (Delta.X > 0.0f)
		{
			if ( (Start->X <= B->Min.X) && (B->Min.X <= End->X) &&
				 (geExtBox_XFaceDist(  Start ,&Delta, B, &t, B->Min.X ) != GE_FALSE) )
					{
						geVec3d_Set( Normal,  -1.0f, 0.0f, 0.0f );
						*T = t;
						return GE_TRUE;
					}
		}
	else if (Delta.X < 0.0f)
		{
			if ( (End->X <= B->Max.X) && (B->Max.X <= Start->X) &&
				 (geExtBox_XFaceDist(  Start ,&Delta, B, &t, B->Max.X ) != GE_FALSE) )
					{
						geVec3d_Set( Normal,  1.0f, 0.0f, 0.0f );
						*T = t;
						return GE_TRUE;
					}
		}
	
	// test y end of box, facing away from ray direction.
	if (Delta.Y > 0.0f)
		{	
			if ( (Start->Y <= B->Min.Y) && (B->Min.Y <= End->Y) &&
				 (geExtBox_YFaceDist(  Start ,&Delta, B, &t, B->Min.Y ) != GE_FALSE) )
				{
					geVec3d_Set( Normal,  0.0f, -1.0f, 0.0f );
					*T = t;
					return GE_TRUE;
				}
		}
	else if (Delta.Y < 0.0f)
		{
			if ( (End->Y <= B->Max.Y) && (B->Max.Y <= Start->Y) &&
				 (geExtBox_YFaceDist(  Start ,&Delta, B, &t, B->Max.Y ) != GE_FALSE) )
				{
					geVec3d_Set( Normal,  0.0f, 1.0f, 0.0f );
					*T = t;
					return GE_TRUE;
				}
		}
	
	// test z end of box, facing away from ray direction.
	if (Delta.Z > 0.0f)
		{	
			if ( (Start->Z <= B->Min.Z) && (B->Min.Z <= End->Z) &&
			     (geExtBox_ZFaceDist(  Start ,&Delta, B, &t, B->Min.Z ) != GE_FALSE) )
				{
					geVec3d_Set( Normal,  0.0f, 0.0f, -1.0f );
					*T = t;
					return GE_TRUE;
				}
		}
	else if (Delta.Z < 0.0f)
		{			
			if ( (End->Z <= B->Max.Z) && (B->Max.Z <= Start->Z) &&
				 (geExtBox_ZFaceDist(  Start ,&Delta, B, &t, B->Max.Z ) != GE_FALSE) )
				{
					geVec3d_Set( Normal,  0.0f, 0.0f, 1.0f );
					*T = t;
					return GE_TRUE;
				}
		}
	return GE_FALSE;	
}
