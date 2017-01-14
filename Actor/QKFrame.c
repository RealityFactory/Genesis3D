/****************************************************************************************/
/*  QKFRAME.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Quaternion keyframe implementation.									*/
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
/* geQKFrame   (geQuaternion - Keyframe)
	This module handles interpolation for keyframes that contain a quaternion
	This is intended to support Path.c
	geTKArray supplies general support for a time-keyed array, and this supplements
	that support to include the specific time-keyed arrays:
	  An array of geQuaternion interpolated linearly
	  An array of geQuaternion with spherical linear interpolation (SLERP)
	  An array of geQuaternion with spherical quadrangle 
		interpolation (SQUAD) as defined by:
	    Advanced Animation and Rendering Techniques by Alan Watt and Mark Watt

	These are phycially separated and have different base structures because
	the different interpolation techniques requre different additional data.
	
	The two lists are created with different creation calls,
	interpolated with different calls, but insertion and queries share a call.
	
	Quadrangle interpolation requires additional computation after changes are
	made to the keyframe list.  Call geQKFrame_SquadRecompute() to update the
	calculations.
*/
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vec3d.h"
#include "QKFrame.h"
#include "errorlog.h"
#include "ram.h"

#define LINEAR_BLEND(a,b,t)  ( (t)*((b)-(a)) + (a) )	
			// linear blend of a and b  0<t<1 where  t=0 ->a and t=1 ->b

typedef struct
{
	geTKArray_TimeType	Time;				// Time for this keyframe
	geQuaternion	Q;					// quaternion for this keyframe
}  QKeyframe;		
	// This is the root structure that geQKFrame supports
	// all keyframe types must begin with this structure.  Time is first, so
	// that this structure can be manipulated by geTKArray

typedef struct
{
	QKeyframe Key;				// key values for this keyframe
}	geQKFrame_Linear;
	// keyframe data for linear interpolation
	// The structure includes no additional information.

typedef struct
{
	QKeyframe Key;				// key values for this keyframe
}	geQKFrame_Slerp;
	// keyframe data for spherical linear interpolation
	// The structure includes no additional information.

typedef struct
{
	QKeyframe Key;				// key values for this keyframe
	geQuaternion  QuadrangleCorner;	
}	geQKFrame_Squad;
	// keyframe data for spherical quadratic interpolation


geTKArray *GENESISCC geQKFrame_LinearCreate()
	// creates a frame list for linear interpolation
{
	return geTKArray_Create(sizeof(geQKFrame_Linear) );
}


geTKArray *GENESISCC geQKFrame_SlerpCreate()
	// creates a frame list for spherical linear interpolation	
{
	return geTKArray_Create(sizeof(geQKFrame_Slerp) );
}

geTKArray *GENESISCC geQKFrame_SquadCreate()
	// creates a frame list for spherical linear interpolation	
{
	return geTKArray_Create(sizeof(geQKFrame_Squad) );
}


geBoolean GENESISCC geQKFrame_Insert(
	geTKArray **KeyList,			// keyframe list to insert into
	geTKArray_TimeType Time,		// time of new keyframe
	const geQuaternion *Q,			// quaternion at new keyframe
	int *Index)						// index of new key
	// inserts a new keyframe with the given time and vector into the list.
{
	assert( KeyList != NULL );
	assert( *KeyList != NULL );
	assert( Q != NULL );
	assert(   sizeof(geQKFrame_Squad) == geTKArray_ElementSize(*KeyList) 
	       || sizeof(geQKFrame_Slerp) == geTKArray_ElementSize(*KeyList) 
		   || sizeof(geQKFrame_Linear) == geTKArray_ElementSize(*KeyList) );

	if (geTKArray_Insert(KeyList, Time, Index) == GE_FALSE)
		{
			geErrorLog_Add(ERR_QKARRAY_INSERT, NULL);
			return GE_FALSE;
		}
	else
		{
			QKeyframe *KF;
			KF = (QKeyframe *)geTKArray_Element(*KeyList,*Index);
			KF->Q = *Q;
			return GE_TRUE;
		}
}

void GENESISCC geQKFrame_Query(
	const geTKArray *KeyList,		// keyframe list
	int Index,						// index of frame to return
	geTKArray_TimeType *Time,		// time of the frame is returned
	geQuaternion *Q)					// vector from the frame is returned
	// returns the vector and the time at keyframe[index] 
{
	QKeyframe *KF;
	assert( KeyList != NULL );
	assert( Time != NULL );
	assert( Q != NULL );
	assert( Index < geTKArray_NumElements(KeyList) );
	assert( Index >= 0 );
	assert(   sizeof(geQKFrame_Squad) == geTKArray_ElementSize(KeyList) 
	       || sizeof(geQKFrame_Slerp) == geTKArray_ElementSize(KeyList) 
		   || sizeof(geQKFrame_Linear) == geTKArray_ElementSize(KeyList) );
	
	KF = (QKeyframe *)geTKArray_Element(KeyList,Index);
	*Time = KF->Time;
	*Q    = KF->Q;
}

void GENESISCC geQKFrame_Modify(
	geTKArray *KeyList,				// keyframe list
	int Index,						// index of frame to change
	const geQuaternion *Q)			// vector for the new key
{
	QKeyframe *KF;
	assert( KeyList != NULL );
	assert( Q != NULL );
	assert( Index < geTKArray_NumElements(KeyList) );
	assert( Index >= 0 );
	assert(   sizeof(geQKFrame_Squad) == geTKArray_ElementSize(KeyList) 
	       || sizeof(geQKFrame_Slerp) == geTKArray_ElementSize(KeyList) 
		   || sizeof(geQKFrame_Linear) == geTKArray_ElementSize(KeyList) );
	
	KF = (QKeyframe *)geTKArray_Element(KeyList,Index);
	KF->Q  = *Q;
}



void GENESISCC geQKFrame_LinearInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result)			// put the result in here (geQuaternion)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates linearly
{
	geQuaternion *Q1,*Q2;
	geQuaternion *QNew = (geQuaternion *)Result;
	
	assert( Result != NULL );
	assert( KF1 != NULL );
	assert( KF2 != NULL );
	
	assert( T >= (geFloat)0.0f );
	assert( T <= (geFloat)1.0f );
	
	if ( KF1 == KF2 )
		{
			*QNew = ((geQKFrame_Linear *)KF1)->Key.Q;
			return;
		}

	Q1 = &( ((geQKFrame_Linear *)KF1)->Key.Q);
	Q2 = &( ((geQKFrame_Linear *)KF2)->Key.Q);
	
	QNew->X = LINEAR_BLEND(Q1->X,Q2->X,T);
	QNew->Y = LINEAR_BLEND(Q1->Y,Q2->Y,T);
	QNew->Z = LINEAR_BLEND(Q1->Z,Q2->Z,T);
	QNew->W = LINEAR_BLEND(Q1->W,Q2->W,T);
	if (geQuaternion_Normalize(QNew)==0.0f)
		{
			geQuaternion_SetNoRotation(QNew);
		}

}



void GENESISCC geQKFrame_SlerpInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result)			// put the result in here (geQuaternion)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates using spherical linear blending
{
	geQuaternion *Q1,*Q2;
	geQuaternion *QNew = (geQuaternion *)Result;
	
	assert( Result != NULL );
	assert( KF1 != NULL );
	assert( KF2 != NULL );
	
	assert( T >= (geFloat)0.0f );
	assert( T <= (geFloat)1.0f );
	
	if ( KF1 == KF2 )
		{
			*QNew = ((geQKFrame_Slerp *)KF1)->Key.Q;
			return;
		}
 
	Q1 = &( ((geQKFrame_Slerp *)KF1)->Key.Q);
	Q2 = &( ((geQKFrame_Slerp *)KF2)->Key.Q);
	geQuaternion_SlerpNotShortest(Q1,Q2,T,QNew);
}




void GENESISCC geQKFrame_SquadInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result)			// put the result in here (geQuaternion)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates using spherical quadratic blending
{
	geQuaternion *Q1,*Q2;
	geQuaternion *QNew = (geQuaternion *)Result;
	
	assert( Result != NULL );
	assert( KF1 != NULL );
	assert( KF2 != NULL );
	
	assert( T >= (geFloat)0.0f );
	assert( T <= (geFloat)1.0f );
	
	if ( KF1 == KF2 )
		{
			*QNew = ((geQKFrame_Squad *)KF1)->Key.Q;
			return;
		}

	Q1 = &( ((geQKFrame_Squad *)KF1)->Key.Q);
	Q2 = &( ((geQKFrame_Squad *)KF2)->Key.Q);
	
	{
		geQuaternion *A1,*B2;
		geQuaternion SL1,SL2;
				
		A1 = &( ((geQKFrame_Squad *)KF1)->QuadrangleCorner);
		B2 = &( ((geQKFrame_Squad *)KF2)->QuadrangleCorner);

		geQuaternion_SlerpNotShortest(Q1,   Q2,   T, &SL1);
				assert( geQuaternion_IsUnit(&SL1) == GE_TRUE);
		geQuaternion_SlerpNotShortest(A1,   B2,   T, &SL2);
				assert( geQuaternion_IsUnit(&SL2) == GE_TRUE);
		geQuaternion_SlerpNotShortest(&SL1, &SL2, (2.0f*T*(1.0f-T)), QNew);
				assert( geQuaternion_IsUnit(QNew) == GE_TRUE);
	}
}


static void GENESISCC geQKFrame_QuadrangleCorner(
	const geQuaternion *Q0,
	const geQuaternion *Q1,
	const geQuaternion *Q2,
	geQuaternion *Corner)
	// compute quadrangle corner for a keyframe containing Q1.
	//  Q0 and Q2 are the quaternions for the previous and next keyframes 
	// corner is the newly computed quaternion
{
	geQuaternion Q1Inv,LnSum;

	assert( Q0 != NULL );
	assert( Q1 != NULL );
	assert( Q2 != NULL );
	assert( Corner != NULL );

	assert( geQuaternion_IsUnit(Q1) == GE_TRUE );

	Q1Inv.W = Q1->W;
	Q1Inv.X = -Q1->X;
	Q1Inv.Y = -Q1->Y;
	Q1Inv.Z = -Q1->Z;
				
	{
		geQuaternion Q1InvQ2, Q1InvQ0;
		geQuaternion Ln1,Ln2;

		geQuaternion_Multiply(&Q1Inv,Q2,&Q1InvQ2);
		geQuaternion_Multiply(&Q1Inv,Q0,&Q1InvQ0);
		geQuaternion_Ln(&Q1InvQ0,&Ln1);
		geQuaternion_Ln(&Q1InvQ2,&Ln2);
		geQuaternion_Add(&Ln1,&Ln2,&LnSum);
		geQuaternion_Scale(&LnSum,-0.25f,&LnSum);
	}

	geQuaternion_Exp(&LnSum,Corner);
	geQuaternion_Multiply(Q1,Corner,Corner);
}

static void GENESISCC geQKFrame_ChooseBestQuat(const geQuaternion *Q0,geQuaternion *Q1)
	// adjusts the sign of Q1:  to either Q1 or -Q1
	// adjusts Q1 such that Q1 is the 'closest' of the two choices to Q0.
{
	geQuaternion pLessQ,pPlusQ;
	geFloat MagpLessQ,MagpPlusQ;

	assert( Q0 != NULL );
	assert( Q1 != NULL );
	
	geQuaternion_Add(Q0,Q1,&pPlusQ);
	geQuaternion_Subtract(Q0,Q1,&pLessQ);
		
	geQuaternion_Multiply(&pPlusQ,&pPlusQ,&pPlusQ);
	geQuaternion_Multiply(&pLessQ,&pLessQ,&pLessQ);

	MagpLessQ=   (pLessQ.W * pLessQ.W) + (pLessQ.X * pLessQ.X) 
					  + (pLessQ.Y * pLessQ.Y) + (pLessQ.Z * pLessQ.Z);

	MagpPlusQ=   (pPlusQ.W * pPlusQ.W) + (pPlusQ.X * pPlusQ.X) 
					  + (pPlusQ.Y * pPlusQ.Y) + (pPlusQ.Z * pPlusQ.Z);

	if (MagpLessQ >= MagpPlusQ)
		{
			Q1->X = -Q1->X;
			Q1->Y = -Q1->Y;
			Q1->Z = -Q1->Z;
			Q1->W = -Q1->W;
		}
}




void GENESISCC geQKFrame_SquadRecompute(
	int Looped,				// if keylist has the first key connected to last key
	geTKArray *KeyList)		// list of keys to recompute hermite values for
	// rebuild precomputed data for keyframe list.
{

	// compute the extra interpolation points at each keyframe
	// see Advanced Animation and Rendering Techniques 
	//     by Alan Watt and Mark Watt, pg 366
	int i;
	geQKFrame_Squad *QList=NULL;
	int count;

	int Index0,Index1,Index2;
	assert( KeyList != NULL );

	count = geTKArray_NumElements(KeyList);

	if (count > 0)
		{
			QList = (geQKFrame_Squad *)geTKArray_Element(KeyList,0);

			for (i =0; i< count-1; i++)
				{
					geQKFrame_ChooseBestQuat(&(QList[i].Key.Q),&(QList[i+1].Key.Q) );
				}
		}

	if (count<3)
		{
			Looped = 0;
			// cant compute 'slopes' without enough points to loop. 
			// so treat path as non-looped.
		}
	for (i =0; i< count; i++)
		{
			Index0 = i-1;
			Index1 = i;
			Index2 = i+1;

			if (Index1 == 0)
				{
					if (Looped != GE_TRUE)
						{
							Index0 = 0;
						}
					else
						{
							Index0 = count-2;
						}
				}

			if (Index2 == count)
				{
					if (Looped != GE_TRUE)
						{
							Index2 = count-1;
						}
					else
						{
							Index2 = 1;
						}
				}

			if (( Looped != GE_TRUE) && (Index1 == 0) )
				{
					geQuaternion_Copy(
						&(QList[i].Key.Q),
						&(QList[i].QuadrangleCorner) );
				}
			else if (( Looped != GE_TRUE) && (Index1 == count-1))
				{
					geQuaternion_Copy(
						&(QList[i].Key.Q),
						&(QList[i].QuadrangleCorner) );
				}
			else
			{
				geQKFrame_QuadrangleCorner( 
					&(QList[Index0].Key.Q),
					&(QList[Index1].Key.Q),
					&(QList[Index2].Key.Q),
					&(QList[i].QuadrangleCorner) );
	
			}
		}	
}					



void GENESISCC geQKFrame_SlerpRecompute(
	geTKArray *KeyList)		// list of keys to recompute hermite values for
	// rebuild precomputed data for keyframe list.
	// also make sure that each successive key is the 'closest' quaternion choice
	// to the previous one.
{

	int i;
	geQKFrame_Slerp *QList;
	int count;
	assert( KeyList != NULL );

	count = geTKArray_NumElements(KeyList);

	if (count > 0)
		{
			QList = (geQKFrame_Slerp  *)geTKArray_Element(KeyList,0);
			for (i =0; i< count-1; i++)
				{
					geQKFrame_ChooseBestQuat(&(QList[i].Key.Q),&(QList[i+1].Key.Q) );
				}
		}
}


#define QKFRAME_LINEAR_ASCII_FILE 0x4C464B51	// 'QKFL'
#define QKFRAME_SLERP_ASCII_FILE 0x53464B51		// 'QKFS'
#define QKFRAME_SQUAD_ASCII_FILE 0x51464B51		// 'QKFQ'
#define CHECK_FOR_WRITE(uu) if(uu <= 0)     { geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL); return GE_FALSE; }
#define CHECK_FOR_READ(uu, nn) if(uu != nn) { geErrorLog_Add(ERR_PATH_FILE_READ, NULL);  return GE_FALSE; }

#define QKFRAME_KEYLIST_ID "Keys"

//------------------------------------------------------------------------
// support for older data files

geBoolean GENESISCC geQKFrame_LinearRead(geVFile* pFile, void* geQKFrame)
{
	uint32 	u;
	char	QKeyString[64];
	geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geQKFrame;

	assert( pFile != NULL );
	assert( geQKFrame != NULL );

	// Read the format/version flag
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	if(u != QKFRAME_LINEAR_ASCII_FILE)
	{
		geErrorLog_Add(ERR_PATH_FILE_VERSION, NULL);
		return GE_FALSE;
	}

	if	(geVFile_GetS(pFile, QKeyString, sizeof(QKeyString)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	u = sscanf(QKeyString, "%f %f %f %f\n",	&pLinear->Key.Q.W,
											&pLinear->Key.Q.X,
											&pLinear->Key.Q.Y,
											&pLinear->Key.Q.Z);
	CHECK_FOR_READ(u, 4);

	return GE_TRUE;
}

geBoolean GENESISCC geQKFrame_SlerpRead(geVFile* pFile, void* geQKFrame)
{
	uint32	u;
	char	QKeyString[64];
	geQKFrame_Slerp* pSlerp = (geQKFrame_Slerp*)geQKFrame;

	assert( pFile != NULL );
	assert( geQKFrame != NULL );

	// Read the format/version flag
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	if(u != QKFRAME_SLERP_ASCII_FILE)
	{
		geErrorLog_Add(ERR_PATH_FILE_VERSION, NULL);
		return GE_FALSE;
	}

	if	(geVFile_GetS(pFile, QKeyString, sizeof(QKeyString)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	u = sscanf(QKeyString, "%f %f %f %f\n",	&pSlerp->Key.Q.W,
											&pSlerp->Key.Q.X,
											&pSlerp->Key.Q.Y,
											&pSlerp->Key.Q.Z);
	CHECK_FOR_READ(u, 4);

	return GE_TRUE;
}


geBoolean GENESISCC geQKFrame_SquadRead(geVFile* pFile, void* geQKFrame)
{
	uint32	u;
	char	SQuadKeyString[128];
	geQKFrame_Squad* pSquad = (geQKFrame_Squad*)geQKFrame;

	assert( pFile != NULL );
	assert( geQKFrame != NULL );

	// Read the format/version flag
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	if(u != QKFRAME_SQUAD_ASCII_FILE)
	{
		geErrorLog_Add(ERR_PATH_FILE_VERSION, NULL);
		return GE_FALSE;
	}

	if	(geVFile_GetS(pFile, SQuadKeyString, sizeof(SQuadKeyString)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	u = sscanf(SQuadKeyString, "%f %f %f %f %f %f %f %f\n", &pSquad->Key.Q.W,
															&pSquad->Key.Q.X,
															&pSquad->Key.Q.Y,
															&pSquad->Key.Q.Z,
															&pSquad->QuadrangleCorner.W,
															&pSquad->QuadrangleCorner.X,
															&pSquad->QuadrangleCorner.Y,
															&pSquad->QuadrangleCorner.Z);
	CHECK_FOR_READ(u, 8);

	return GE_TRUE;
}

//------------------------------------------------------------------------
#define QKFRAME_HINGE_COMPRESSION 0x1
#define QKFRAME_LINEARTIME_COMPRESSION 0x2


#define HINGE_TOLERANCE (0.0001f)
#define LINEARTIME_TOLERANCE (0.0001f)

static geBoolean GENESISCC geQKFrame_PathIsHinged(geTKArray *KeyList, geFloat Tolerance)
{
	int i,Count;
	geVec3d Axis;
	geVec3d NextAxis;
	geFloat Angle; 
	geQKFrame_Linear* pLinear;

	assert( KeyList != NULL );

	Count = geTKArray_NumElements(KeyList);
	
	if (Count<2)
		return GE_FALSE;
	pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, 0);
	if (geQuaternion_GetAxisAngle(&(pLinear->Key.Q),&Axis,&Angle)==GE_FALSE)
		{
			return GE_FALSE;
		}
		
	for (i=1; i<Count; i++)
		{
			pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, i);
			if (geQuaternion_GetAxisAngle(&(pLinear->Key.Q),&NextAxis,&Angle)==GE_FALSE)
				{
					return GE_FALSE;
				}
				
			if (geVec3d_Compare(&Axis,&NextAxis,Tolerance) == GE_FALSE)
				{	
					return GE_FALSE;
				}
		}
	return GE_TRUE;
}


static int GENESISCC geQKFrame_DetermineCompressionType(geTKArray *KeyList)
{
	int Compression=0;
	int NumElements=0;

	assert( KeyList != NULL );

	NumElements = geTKArray_NumElements(KeyList);

	if (NumElements>2)
		{
			if ( geTKArray_SamplesAreTimeLinear(KeyList,LINEARTIME_TOLERANCE) != GE_FALSE )
				{
					Compression |= QKFRAME_LINEARTIME_COMPRESSION;
				}
		}


	if (NumElements>3)
		{
			 if ( geQKFrame_PathIsHinged(KeyList,HINGE_TOLERANCE)!=GE_FALSE )
				{
					Compression |= QKFRAME_HINGE_COMPRESSION;
				}
		}

	return Compression;
}



geBoolean GENESISCC geQKFrame_WriteToFile(geVFile *pFile, geTKArray *KeyList, 
		geQKFrame_InterpolationType InterpolationType, int Looping)
{
	int NumElements,i;
	geFloat Time,DeltaTime;
	int Compression;

	assert( pFile != NULL );
	assert( KeyList != NULL );

	NumElements = geTKArray_NumElements(KeyList);
	
	Compression = geQKFrame_DetermineCompressionType(KeyList);

	if	(geVFile_Printf(pFile,
					  "%s %d %d %d %d\n",
					  QKFRAME_KEYLIST_ID,
					  NumElements,
					  InterpolationType,
					  Compression,
					  Looping) == GE_FALSE)
		{
			geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
			return GE_FALSE;
		}

	if (Compression & QKFRAME_LINEARTIME_COMPRESSION)
		{
			Time = geTKArray_ElementTime(KeyList, 0);
			DeltaTime = geTKArray_ElementTime(KeyList, 1)- Time;
			if	(geVFile_Printf(pFile,"%f %f Start T,Delta T\n",Time,DeltaTime) == GE_FALSE)
				{
					geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
					return GE_FALSE;
				}
		}

	switch (Compression & (~QKFRAME_LINEARTIME_COMPRESSION) )
		{
			case (0):
				{
					for(i=0;i<NumElements;i++)
						{
							geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, i);
							if (!(Compression & QKFRAME_LINEARTIME_COMPRESSION))
								{
									Time = geTKArray_ElementTime(KeyList, i);
									if	(geVFile_Printf(pFile, "%f ",Time) == GE_FALSE)
										{
											geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
											return GE_FALSE;
										}
								}
							if	(geVFile_Printf(pFile,
											  "%f %f %f %f\n",	pLinear->Key.Q.W,  pLinear->Key.Q.X,  
																pLinear->Key.Q.Y,  pLinear->Key.Q.Z) == GE_FALSE)
								{
									geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
									return GE_FALSE;
								}
						}
				}
				break;
			case (QKFRAME_HINGE_COMPRESSION):
				{
					geVec3d Hinge;
					geFloat Angle;

					geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, 0);
					geQuaternion_GetAxisAngle(&(pLinear->Key.Q),&Hinge,&Angle);
					geVec3d_Normalize(&Hinge);
					if	(geVFile_Printf(pFile,"%f %f %f Axis\n",Hinge.X,Hinge.Y,Hinge.Z) == GE_FALSE)
						{
							geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
							return GE_FALSE;
						}
					for(i=0;i<NumElements;i++)
						{
							geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, i);
							if (!(Compression & QKFRAME_LINEARTIME_COMPRESSION))
								{
									Time = geTKArray_ElementTime(KeyList, i);
									if	(geVFile_Printf(pFile, "%f ",Time) == GE_FALSE)
										{
											geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
											return GE_FALSE;
										}
								}
							geQuaternion_GetAxisAngle(&(pLinear->Key.Q),&Hinge,&Angle);
							if	(geVFile_Printf(pFile,"%f\n",	Angle) == GE_FALSE)
								{
									geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
									return GE_FALSE;
								}
						}
				}
				break;
			default:
				assert(0);

		}
	return GE_TRUE;
}


geTKArray *GENESISCC geQKFrame_CreateFromFile(geVFile *pFile, int *InterpolationType, int *Looping)
{
	int i,u,NumElements;
	int Compression;
	geFloat StartTime=0.0f;
	geFloat DeltaTime=0.0f;

	#define ERROREXIT  {geErrorLog_Add( ERR_PATH_FILE_READ, NULL);if (KeyList != NULL){geTKArray_Destroy(&KeyList);}	return NULL;}

	#define LINE_LENGTH 256
	char line[LINE_LENGTH];
	geTKArray *KeyList=NULL;

	assert( pFile != NULL );
	assert( InterpolationType != NULL );
	
	if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		ERROREXIT;
	if(strnicmp(line, QKFRAME_KEYLIST_ID, sizeof(QKFRAME_KEYLIST_ID)-1) != 0)
		ERROREXIT;

	if(sscanf(line + sizeof(QKFRAME_KEYLIST_ID)-1, "%d %d %d %d", 
					&NumElements,InterpolationType,&Compression,Looping) != 4)
		ERROREXIT;

	if (!( (*InterpolationType == QKFRAME_LINEAR) || (*InterpolationType == QKFRAME_SLERP) || (*InterpolationType == QKFRAME_SQUAD) ))
		ERROREXIT;
	
	if ( Compression > 0xFF)
		ERROREXIT;
		


	switch (*InterpolationType)
		{
			case (QKFRAME_LINEAR):
					KeyList = geTKArray_CreateEmpty(sizeof(geQKFrame_Linear),NumElements);
					break;
			case (QKFRAME_SLERP):
					KeyList = geTKArray_CreateEmpty(sizeof(geQKFrame_Slerp),NumElements);
					break;
			case (QKFRAME_SQUAD):
					KeyList = geTKArray_CreateEmpty(sizeof(geQKFrame_Squad),NumElements);
					break;
			default:
				ERROREXIT;
		}
	if (KeyList == NULL)
		ERROREXIT;

	if (Compression & QKFRAME_LINEARTIME_COMPRESSION)
		{
			if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
				ERROREXIT;
			if (sscanf(line,"%f %f",&StartTime,&DeltaTime) != 2)
				ERROREXIT;
		}
	switch (Compression & (~QKFRAME_LINEARTIME_COMPRESSION) )
		{
			case (0):
				{
					for(i=0;i<NumElements;i++)
						{
							geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, i);
							if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
								ERROREXIT;
							if (Compression & QKFRAME_LINEARTIME_COMPRESSION)
								{
									pLinear->Key.Time = StartTime + DeltaTime * i;
									u = sscanf(line,"%f %f %f %f", &(pLinear->Key.Q.W),
										&(pLinear->Key.Q.X),&(pLinear->Key.Q.Y),&(pLinear->Key.Q.Z));
									if (u==4) u = 5;
								}
							else
								{
									u = sscanf(line,"%f %f %f %f %f",&(pLinear->Key.Time), &(pLinear->Key.Q.W),
										&(pLinear->Key.Q.X),&(pLinear->Key.Q.Y),&(pLinear->Key.Q.Z));
								}
							if (u!=5)
								ERROREXIT;
						}
				}
				break;
			case (QKFRAME_HINGE_COMPRESSION):
				{
					geVec3d Hinge;
					geFloat Angle;
					if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
						ERROREXIT;
					u = sscanf(line,"%f %f %f",&(Hinge.X),&(Hinge.Y),&(Hinge.Z) );
					if (u != 3)
						ERROREXIT;
					for(i=0;i<NumElements;i++)
						{
							geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, i);
							if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
								ERROREXIT;
							if (Compression & QKFRAME_LINEARTIME_COMPRESSION)
								{
									pLinear->Key.Time = StartTime + DeltaTime * i;
									u = sscanf(line,"%f", &(Angle));
									if (u==1) u = 2;
								}
							else
								{
									u = sscanf(line,"%f %f",&(pLinear->Key.Time), &(Angle) );
								}
							if (u!=2)
								ERROREXIT;
							geQuaternion_SetFromAxisAngle(&(pLinear->Key.Q),&Hinge,Angle);
						}
				}
				break;
			default:
				assert(0);

		}

	switch (*InterpolationType)
		{
			case (QKFRAME_LINEAR):
					break;
			case (QKFRAME_SLERP):
				geQKFrame_SlerpRecompute( KeyList);
					break;
			case (QKFRAME_SQUAD):
				geQKFrame_SquadRecompute( *Looping, KeyList);
					break;
			default:
				assert(0);
		}
	return KeyList;	

}

uint32 GENESISCC geQKFrame_ComputeBlockSize(geTKArray *KeyList, int Compression)
{
	uint32 Size=0;
	int Count;
	assert( KeyList != NULL );
	assert( Compression < 0xFF);
	
	Count = geTKArray_NumElements(KeyList);

	Size += sizeof(uint32);		// flags
	Size += sizeof(uint32);		// count

	if (Compression & QKFRAME_LINEARTIME_COMPRESSION)
		{
			Size += sizeof(geFloat) * 2;
		}
	else
		{
			Size += sizeof(geFloat) * Count;
		}

	switch (Compression & (~QKFRAME_LINEARTIME_COMPRESSION) )
		{
			case 0:
				Size += sizeof(geQuaternion) * Count;
				break;
			case QKFRAME_HINGE_COMPRESSION:
				Size += (sizeof(geFloat) * 3) + sizeof(geFloat) * Count;
				break;
			default:
				assert(0);
		}	
	return Size;
}

geTKArray *GENESISCC geQKFrame_CreateFromBinaryFile(geVFile *pFile, int *InterpolationType, int *Looping)
{
	uint32 u;
	int BlockSize;
	int Compression;
	int Count,i;
	int FieldSize;
	char *Block;
	geFloat *Data;
	geTKArray *KeyList;
	geQKFrame_Linear* pLinear0;
	geQKFrame_Linear* pLinear;

	assert( pFile != NULL );
	assert( InterpolationType != NULL );
	assert( Looping != NULL );
	
	if (geVFile_Read(pFile, &BlockSize, sizeof(int)) == GE_FALSE)
		{
			geErrorLog_AddString(-1,"Failure to read binary QKFrame header", NULL);
			return NULL;
		}
	if (BlockSize<0)
		{
			geErrorLog_AddString(-1,"Bad Blocksize", NULL);
			return NULL;
		}
			
	Block = geRam_Allocate(BlockSize);
	if(geVFile_Read(pFile, Block, BlockSize) == GE_FALSE)
		{
			geErrorLog_AddString(-1,"Failure to read binary QKFrame header", NULL);
			return NULL;
		}
	u = *(uint32 *)Block;
	*InterpolationType = (u>>16)& 0xFF;
	Compression = (u>>8) & 0xFF;
	*Looping           = (u & 0x1);		
	Count = *(((uint32 *)Block)+1);
	
	if (Compression > 0xFF)
		{
			geRam_Free(Block);	
			geErrorLog_AddString(-1,"Bad Compression Flag", NULL);
			return NULL;
		}
	switch (*InterpolationType)
		{
			case (QKFRAME_LINEAR):
				FieldSize = sizeof(geQKFrame_Linear);
				break;
			case (QKFRAME_SLERP):
				FieldSize = sizeof(geQKFrame_Slerp);
				break;
			case (QKFRAME_SQUAD):
				FieldSize = sizeof(geQKFrame_Squad);
				break;
			default:
				geRam_Free(Block);
				geErrorLog_AddString(-1,"Bad InterpolationType", NULL);
				return NULL;
		}
	
	KeyList = geTKArray_CreateEmpty(FieldSize,Count);
	if (KeyList == NULL)
		{
			geRam_Free(Block);	
			geErrorLog_AddString(-1,"Failed to allocate tkarray", NULL);
			return NULL;
		}

	Data = (geFloat *)(Block + sizeof(uint32)*2);
			
	pLinear0 = (geQKFrame_Linear*)geTKArray_Element(KeyList, 0);

	pLinear = pLinear0;

	if (Compression & QKFRAME_LINEARTIME_COMPRESSION)
		{
			geFloat fi;
			geFloat fCount = (geFloat)Count;
			geFloat Time,DeltaTime;
			Time = *(Data++);
			DeltaTime = *(Data++);
			for(fi=0.0f;fi<fCount;fi+=1.0f)
				{
					pLinear->Key.Time = Time + fi*DeltaTime;
					pLinear = (geQKFrame_Linear *)  ( ((char *)pLinear) + FieldSize );
				}
		}
	else
		{
			for(i=0;i<Count;i++)
				{
					pLinear->Key.Time = *(Data++);
					pLinear = (geQKFrame_Linear *)  ( ((char *)pLinear) + FieldSize );
				}
		}

	pLinear = pLinear0;

	if (Compression & QKFRAME_HINGE_COMPRESSION)
		{
			geVec3d Hinge;
			Hinge.X = *(Data++);
			Hinge.Y = *(Data++);
			Hinge.Z = *(Data++);

			for(i=0;i<Count;i++)
				{
					geQuaternion_SetFromAxisAngle(&(pLinear->Key.Q),&Hinge,*(Data++));
					pLinear = (geQKFrame_Linear *)  ( ((char *)pLinear) + FieldSize );
				}
		}
	else
		{
			for(i=0;i<Count;i++)
				{
					pLinear->Key.Q = *(geQuaternion *)Data;
					Data += 4;
					pLinear = (geQKFrame_Linear *)  ( ((char *)pLinear) + FieldSize );
				}
		}
	
	switch (*InterpolationType)
		{
			case (QKFRAME_LINEAR):
					break;
			case (QKFRAME_SLERP):
				geQKFrame_SlerpRecompute( KeyList);
					break;
			case (QKFRAME_SQUAD):
				geQKFrame_SquadRecompute( *Looping, KeyList);
					break;
			default:
				assert(0);
		}
	geRam_Free(Block);	
	return KeyList;						
}

geBoolean GENESISCC geQKFrame_WriteToBinaryFile(geVFile *pFile, geTKArray *KeyList, 
		geQKFrame_InterpolationType InterpolationType, int Looping)
{
	#define WBERREXIT  {geErrorLog_AddString( ERR_PATH_FILE_WRITE,"Failure to write binary key data", NULL);return GE_FALSE;}
	uint32 u,BlockSize;
	int Compression;
	int Count,i;
	geFloat Time,DeltaTime;
	assert( pFile != NULL );
	assert( InterpolationType < 0xFF);
	assert( (Looping == 0) || (Looping == 1) );


	Compression = geQKFrame_DetermineCompressionType(KeyList);
	u = (InterpolationType << 16) | (Compression << 8) |  Looping;
	
	BlockSize = geQKFrame_ComputeBlockSize(KeyList,Compression);

	if (geVFile_Write(pFile, &BlockSize,sizeof(uint32)) == GE_FALSE)
		WBERREXIT;
	
	if (geVFile_Write(pFile, &u, sizeof(uint32)) == GE_FALSE)
		WBERREXIT;
	
	Count = geTKArray_NumElements(KeyList);
	if (geVFile_Write(pFile, &Count, sizeof(uint32)) == GE_FALSE)
		WBERREXIT;
	
	if (Compression & QKFRAME_LINEARTIME_COMPRESSION)
		{
			Time = geTKArray_ElementTime(KeyList, 0);
			DeltaTime = geTKArray_ElementTime(KeyList, 1)- Time;
			if (geVFile_Write(pFile, &Time,sizeof(geFloat)) == GE_FALSE)
				WBERREXIT;
			if (geVFile_Write(pFile, &DeltaTime,sizeof(geFloat)) == GE_FALSE)
				WBERREXIT;
		}
	else
		{
			for(i=0;i<Count;i++)
				{
					Time = geTKArray_ElementTime(KeyList, i);
					if (geVFile_Write(pFile, &Time,sizeof(geFloat)) == GE_FALSE)
						WBERREXIT;
				}
		}

	if (Compression & QKFRAME_HINGE_COMPRESSION)
		{
			geVec3d Hinge;
			geFloat Angle;

			geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, 0);
			geQuaternion_GetAxisAngle(&(pLinear->Key.Q),&Hinge,&Angle);
			geVec3d_Normalize(&Hinge);
			if (geVFile_Write(pFile, &Hinge,sizeof(geVec3d)) == GE_FALSE)
				WBERREXIT;

			for(i=0;i<Count;i++)
				{
					geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, i);
					geQuaternion_GetAxisAngle(&(pLinear->Key.Q),&Hinge,&Angle);
					if (geVFile_Write(pFile, &Angle,sizeof(geFloat)) == GE_FALSE)
						WBERREXIT;
				}
		}
	else
		{
			for(i=0;i<Count;i++)
				{
					geQKFrame_Linear* pLinear = (geQKFrame_Linear*)geTKArray_Element(KeyList, i);
					if (geVFile_Write(pFile, &(pLinear->Key.Q),sizeof(geQuaternion)) == GE_FALSE)
						WBERREXIT;
				}
		}
		
	return GE_TRUE;
}
