/****************************************************************************************/
/*  VKFRAME.C																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Vector keyframe implementation.										*/
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
/* geVKFrame  (Vector-Keyframe)
	This module handles interpolation for keyframes that contain a vector (a geVec3d)
	This is intended to support Path.c
	geTKArray supplies general support for a time-keyed array, and this supplements
	that support to include the two specific time-keyed arrays:
	  An array of geVec3d interpolated linearly
	  An array of geVec3d interpolated with hermite blending
	These are phycially separated and have different base structures because:
		linear blending requires less data.
		future blending might require more data.
	The two types of lists are created with different creation calls,
	interpolated with different calls, but insertion and queries share a call.
	
	Hermite interpolation requires additional computation after changes are
	made to the keyframe list.  Call geVKFrame_HermiteRecompute() to update the
	calculations.
*/
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vec3d.h"
#include "vkframe.h"
#include "errorlog.h"
#include "ram.h"

#define LINEAR_BLEND(a,b,t)  ( (t)*((b)-(a)) + (a) )	
			// linear blend of a and b  0<t<1 where  t=0 ->a and t=1 ->b

typedef struct
{
	geTKArray_TimeType	Time;		// Time for this keyframe
	geVec3d		V;					// vector for this keyframe
}  geVKFrame;		
	// This is the root structure that geVKFrame supports
	// all keyframe types must begin with this structure.  Time is first, so
	// that this structure can be manipulated by geTKArray

typedef struct
{
	geVKFrame Key;					// key values for this keyframe
	geVec3d		SDerivative;		// Hermite Derivative (Incoming) 
	geVec3d		DDerivative;		// Hermite Derivative (Outgoing) 
}	geVKFrame_Hermite;
	// keyframe data for hermite blending
	// The structure includes computed derivative information.  

typedef struct
{
	geVKFrame Key;				// key values for this keyframe
}	geVKFrame_Linear;
	// keyframe data for linear interpolation
	// The structure includes no additional information.

geTKArray *GENESISCC geVKFrame_LinearCreate(void)
	// creates a frame list for linear interpolation
{
	return geTKArray_Create(sizeof(geVKFrame_Linear) );
}


geTKArray *GENESISCC geVKFrame_HermiteCreate(void)
	// creates a frame list for hermite interpolation	
{
	return geTKArray_Create(sizeof(geVKFrame_Hermite) );
}


geBoolean GENESISCC geVKFrame_Insert(
	geTKArray **KeyList,			// keyframe list to insert into
	geTKArray_TimeType Time,		// time of new keyframe
	const geVec3d *V,				// vector at new keyframe
	int *Index)					// index of new key
	// inserts a new keyframe with the given time and vector into the list.
{
	assert( KeyList != NULL );
	assert( *KeyList != NULL );
	assert( V != NULL );
	assert(   sizeof(geVKFrame_Hermite) == geTKArray_ElementSize(*KeyList) 
	       || sizeof(geVKFrame_Linear) == geTKArray_ElementSize(*KeyList) );

	if (geTKArray_Insert(KeyList, Time, Index) == GE_FALSE)
		{
			geErrorLog_Add(ERR_VKARRAY_INSERT, NULL);
			return GE_FALSE;
		}
	else
		{
			geVKFrame *KF;
			KF = (geVKFrame *)geTKArray_Element(*KeyList,*Index);
			KF->V = *V;
			return GE_TRUE;
		}
}

void GENESISCC geVKFrame_Query(
	const geTKArray *KeyList,		// keyframe list
	int Index,						// index of frame to return
	geTKArray_TimeType *Time,		// time of the frame is returned
	geVec3d *V)						// vector from the frame is returned
	// returns the vector and the time at keyframe[index] 
{
	geVKFrame *KF;
	assert( KeyList != NULL );
	assert( Time != NULL );
	assert( V != NULL );
	assert( Index < geTKArray_NumElements(KeyList) );
	assert( Index >= 0 );
	assert(   sizeof(geVKFrame_Hermite) == geTKArray_ElementSize(KeyList) 
	       || sizeof(geVKFrame_Linear) == geTKArray_ElementSize(KeyList) );
		
	KF = (geVKFrame *)geTKArray_Element(KeyList,Index);
	*Time = KF->Time;
	*V    = KF->V;
}


void GENESISCC geVKFrame_Modify(
	geTKArray *KeyList,				// keyframe list
	int Index,						// index of frame to change
	const geVec3d *V)				// vector for the key
	// chganes the vector at keyframe[index] 
{
	geVKFrame *KF;
	assert( KeyList != NULL );
	assert( V != NULL );
	assert( Index < geTKArray_NumElements(KeyList) );
	assert( Index >= 0 );
	assert(   sizeof(geVKFrame_Hermite) == geTKArray_ElementSize(KeyList) 
	       || sizeof(geVKFrame_Linear) == geTKArray_ElementSize(KeyList) );
		
	KF = (geVKFrame *)geTKArray_Element(KeyList,Index);
	KF->V = *V;
}


void GENESISCC geVKFrame_LinearInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result)			// put the result in here (geVec3d)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates linearly
{
	geVec3d *Vec1,*Vec2;
	geVec3d *VNew = (geVec3d *)Result;
	
	assert( Result != NULL );
	assert( KF1 != NULL );
	assert( KF2 != NULL );
	
	assert( T >= (geFloat)0.0f );
	assert( T <= (geFloat)1.0f );
	
	if ( KF1 == KF2 )
		{
			*VNew = ((geVKFrame_Linear *)KF1)->Key.V;
			return;
		}

	Vec1 = &( ((geVKFrame_Linear *)KF1)->Key.V);
	Vec2 = &( ((geVKFrame_Linear *)KF2)->Key.V);
	
	VNew->X = LINEAR_BLEND(Vec1->X,Vec2->X,T);
	VNew->Y = LINEAR_BLEND(Vec1->Y,Vec2->Y,T);
	VNew->Z = LINEAR_BLEND(Vec1->Z,Vec2->Z,T);
}



void GENESISCC geVKFrame_HermiteInterpolation(
	const void *KF1,		// pointer to first keyframe
	const void *KF2,		// pointer to second keyframe
	geFloat T,				// 0 <= T <= 1   blending parameter
	void *Result)			// put the result in here (geVec3d)
		// interpolates to get a vector between the two vectors at the two
		// keyframes where T==0 returns the vector for KF1 
		// and T==1 returns the vector for KF2
		// interpolates using 'hermite' blending
{
	geVec3d *Vec1,*Vec2;
	geVec3d *VNew = (geVec3d *)Result;
	
	assert( Result != NULL );
	assert( KF1 != NULL );
	assert( KF2 != NULL );
	
	assert( T >= (geFloat)0.0f );
	assert( T <= (geFloat)1.0f );
	
	if ( KF1 == KF2 )
		{
			*VNew = ((geVKFrame_Hermite *)KF1)->Key.V;
			return;
		}

	Vec1 = &( ((geVKFrame_Hermite *)KF1)->Key.V);
	Vec2 = &( ((geVKFrame_Hermite *)KF2)->Key.V);

	{
		geFloat	t2;			// T sqaured
		geFloat	t3;			// T cubed
		geFloat   H1,H2,H3,H4;	// hermite basis function coefficients

		t2 = T * T;
		t3 = t2 * T;
	
		H2 = -(t3 + t3) + t2*3.0f;
		H1 = 1.0f - H2;
		H4 = t3 - t2;
		H3 = H4 - t2 + T;   //t3 - 2.0f * t2 + t;
		
		geVec3d_Scale(Vec1,H1,VNew);
		geVec3d_AddScaled(VNew,Vec2,H2,VNew);
		geVec3d_AddScaled(VNew,&( ((geVKFrame_Hermite *)KF1)->DDerivative),H3,VNew);
		geVec3d_AddScaled(VNew,&( ((geVKFrame_Hermite *)KF2)->SDerivative),H4,VNew);
	}
}


void GENESISCC geVKFrame_HermiteRecompute(
	int Looped,				 // if keylist has the first key connected to last key
	geBoolean ZeroDerivative,// if each key should have a zero derivatives (good for 2 point S curves)
	geTKArray *KeyList)		 // list of keys to recompute hermite values for
	// rebuild precomputed data for keyframe list.
{
	// compute the incoming and outgoing derivatives at each keyframe
	int i;
	geVec3d V0,V1,V2;
	geFloat Time0, Time1, Time2, N0, N1, N0N1;
	geVKFrame_Hermite *TK;
	geVKFrame_Hermite *Vector= NULL;
	int count;
	int Index0,Index1,Index2;

	assert( KeyList != NULL );
	assert( sizeof(geVKFrame_Hermite) == geTKArray_ElementSize(KeyList) );
	
			
	// Compute derivatives at the keyframe points:
	// The derivative is the average of the source chord p[i]-p[i-1]
	// and the destination chord p[i+1]-p[i]
	//     (where i is Index1 in this function)
	//  D = 1/2 * ( p[i+1]-p[i-1] ) = 1/2 *( (p[i+1]-p[i]) + (p[i]-p[i-1]) )
	//  The very first and last chords are simply the 
	// destination and source derivative.
	//   These 'averaged' D's are adjusted for variences in the time scale
	// between the Keyframes.  To do this, the derivative at each keyframe
	// is split into two parts, an incoming ('source' DS) 
	// and an outgoing ('destination' DD) derivative.
	// DD[i] = DD[i] * 2 * N[i]  / ( N[i-1] + N[i] )   
	// DS[i] = DS[i] * 2 * N[i-1]/ ( N[i-1] + N[i] )
	//    where N[i] is time between keyframes i and i+1
	// Since the chord dealt with on a given chord between key[i] and key[i+1], only
	// one of the derivates are needed for each keyframe.  For key[i] the outgoing
	// derivative at is needed (DD[i]).  For key[i+1], the incoming derivative
	// is needed (DS[i+1])   ( note that  (1/2) * 2 = 1 )
	count = geTKArray_NumElements(KeyList);
	if (count > 0)
		{
			Vector = (geVKFrame_Hermite *)geTKArray_Element(KeyList,0);
		}

	if (ZeroDerivative!=GE_FALSE)
		{	// in this case, just bang all derivatives to zero.
			for (i =0; i< count; i++)
				{
					TK = &(Vector[i]);
					geVec3d_Clear(&(TK->DDerivative));
					geVec3d_Clear(&(TK->SDerivative));
				}
			return;
		}

	if (count < 3)			
		{
			Looped = GE_FALSE;	
			// cant compute slopes without a closed loop: 
			// so compute slopes as if it is not closed.
		}
	for (i =0; i< count; i++)
		{
			TK = &(Vector[i]);
			Index0 = i-1;
			Index1 = i;
			Index2 = i+1;

			Time1 = Vector[Index1].Key.Time;
			if (Index1 == 0)
				{
					if (Looped != GE_TRUE)
						{
							Index0 = 0;			
							Time0 = Vector[Index0].Key.Time;
						}
					else
						{
							Index0 = count-2;
							Time0 = Time1 - (Vector[count-1].Key.Time - Vector[count-2].Key.Time);
						}
				}
			else
				{
					Time0 = Vector[Index0].Key.Time;
				}


			if (Index2 == count)
				{
					if (Looped != GE_TRUE)
						{
							Index2 = count-1;
							Time2 = Vector[Index2].Key.Time;
						}
					else
						{
							Index2 = 1;
							Time2 = Time1 + (Vector[1].Key.Time - Vector[0].Key.Time);
						}
				}
			else
				{
					Time2 = Vector[Index2].Key.Time;
				}

			V0 = Vector[Index0].Key.V;
			V1 = Vector[Index1].Key.V;
			V2 = Vector[Index2].Key.V;

			N0    = (Time1 - Time0);
			N1    = (Time2 - Time1);
			N0N1  = N0 + N1;

			if (( Looped != GE_TRUE) && (Index1 == 0) )
				{
					geVec3d_Subtract(&V2,&V1,&(TK->SDerivative));
					geVec3d_Copy( &(TK->SDerivative), &(TK->DDerivative));
				}
			else if (( Looped != GE_TRUE) && (Index1 == count-1))
				{
					geVec3d_Subtract(&V1,&V0,&(TK->SDerivative));
					geVec3d_Copy( &(TK->SDerivative), &(TK->DDerivative));
				}
			else
			{
				geVec3d Slope;
				geVec3d_Subtract(&V2,&V0,&Slope);
				geVec3d_Scale(&Slope, (N1 / N0N1), &(TK->DDerivative));
				geVec3d_Scale(&Slope, (N0 / N0N1), &(TK->SDerivative));
			}
		}	
}		


#define VKFRAME_LINEAR_ASCII_FILE 0x4C464B56	// 'VKFL'
#define VKFRAME_HERMITE_ASCII_FILE 0x48464B56	// 'VKFH'
#define CHECK_FOR_READ(uu, nn) if(uu != nn) { geErrorLog_Add(ERR_PATH_FILE_READ, NULL);  return GE_FALSE; }
#define CHECK_FOR_WRITE(uu) if (uu <= 0)    { geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL); return GE_FALSE; }

#define VKFRAME_KEYLIST_ID "Keys"


//-----------------------------------
// Support for 'old' 1.0 ascii data format

geBoolean GENESISCC geVKFrame_LinearRead(geVFile* pFile, void* geVKFrame)
{
	uint32	u;
	char	KeyString[64];
	geVKFrame_Linear* pLinear = (geVKFrame_Linear*)geVKFrame;

	assert( pFile != NULL );
	assert( geVKFrame != NULL );

	// Read the format/version flag
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	if(u != VKFRAME_LINEAR_ASCII_FILE)
	{
		geErrorLog_Add(ERR_PATH_FILE_VERSION, NULL);
		return GE_FALSE;
	}

	if	(geVFile_GetS(pFile, KeyString, sizeof(KeyString)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	u = sscanf(KeyString, "%f %f %f\n", &pLinear->Key.V.X,
										&pLinear->Key.V.Y,
										&pLinear->Key.V.Z);
	CHECK_FOR_READ(u, 3);

	return GE_TRUE;
}


geBoolean GENESISCC geVKFrame_HermiteRead(geVFile* pFile, void* geVKFrame)
{
	uint32	u;
	char	HermiteString[128];
	geVKFrame_Hermite* pHermite = (geVKFrame_Hermite*)geVKFrame;

	assert( pFile != NULL );
	assert( geVKFrame != NULL );

	// Read the format/version flag
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}

	if(u != VKFRAME_HERMITE_ASCII_FILE)
	{
		geErrorLog_Add(ERR_PATH_FILE_VERSION, NULL);
		return GE_FALSE;
	}

	if	(geVFile_GetS(pFile, HermiteString, sizeof(HermiteString)) == GE_FALSE)
	{
		geErrorLog_Add(ERR_PATH_FILE_READ, NULL);
		return GE_FALSE;
	}
	u = sscanf(HermiteString, "%f %f %f %f %f %f %f %f %f\n",	&pHermite->Key.V.X,
																&pHermite->Key.V.Y,
																&pHermite->Key.V.Z,
																&pHermite->SDerivative.X,
																&pHermite->SDerivative.Y,
																&pHermite->SDerivative.Z,
																&pHermite->DDerivative.X,
																&pHermite->DDerivative.Y,
																&pHermite->DDerivative.Z);
	CHECK_FOR_READ(u, 9);

	return GE_TRUE;
}


		// (above)Support for 'old' 1.0 ascii data format
//-----------------------------------
#define LINEARTIME_TOLERANCE (0.0001f)
#define VKFRAME_LINEARTIME_COMPRESSION 0x2



geBoolean GENESISCC geVKFrame_WriteToFile(geVFile *pFile, geTKArray *KeyList, 
		geVKFrame_InterpolationType InterpolationType, int Looping)
{
	int NumElements,i;
	geFloat Time,DeltaTime;
	int Compression=0;

	assert( pFile != NULL );
	assert( KeyList != NULL );

	NumElements = geTKArray_NumElements(KeyList);

	if (NumElements>2)
		{
			if ( geTKArray_SamplesAreTimeLinear(KeyList,LINEARTIME_TOLERANCE) != GE_FALSE )
				{
					Compression |= VKFRAME_LINEARTIME_COMPRESSION;
				}
		}


	if	(geVFile_Printf(pFile,
					  "%s %d %d %d %d\n",
					  VKFRAME_KEYLIST_ID,
					  NumElements,
					  InterpolationType,
					  Compression,
					  Looping) == GE_FALSE)
    {
		geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
		return GE_FALSE;
	}

	if (Compression & VKFRAME_LINEARTIME_COMPRESSION)
		{
			Time = geTKArray_ElementTime(KeyList, 0);
			DeltaTime = geTKArray_ElementTime(KeyList, 1)- Time;
			if(geVFile_Printf(pFile,"%f %f Start T,Delta T\n",Time,DeltaTime) == GE_FALSE)
				{
					geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
					return GE_FALSE;
				}
		}

	for(i=0;i<NumElements;i++)
		{
			geVKFrame_Linear* pLinear = (geVKFrame_Linear*)geTKArray_Element(KeyList, i);
			if (!(Compression & VKFRAME_LINEARTIME_COMPRESSION))
				{
					Time = geTKArray_ElementTime(KeyList, i);
					if	(geVFile_Printf(pFile, "%f ", Time ) == GE_FALSE)
						{
							geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
							return GE_FALSE;
						}
				}
			if	(geVFile_Printf(pFile,
							  "%f %f %f\n",
							  pLinear->Key.V.X,
							  pLinear->Key.V.Y,
							  pLinear->Key.V.Z) == GE_FALSE)
				{
					geErrorLog_Add(ERR_PATH_FILE_WRITE, NULL);
					return GE_FALSE;
				}
		}
	return GE_TRUE;
}


geTKArray *GENESISCC geVKFrame_CreateFromFile(geVFile *pFile, int *InterpolationType, int *Looping)
{
	#define ERROREXIT  {geErrorLog_Add( ERR_PATH_FILE_READ , NULL);if (KeyList != NULL){geTKArray_Destroy(&KeyList);}	return NULL;}
	int i,u,NumElements;
	int Compression;
	#define LINE_LENGTH 256
	char line[LINE_LENGTH];
	geTKArray *KeyList= NULL;
	geFloat StartTime=0.0f;
	geFloat DeltaTime=0.0f;


	assert( pFile != NULL );
	assert( InterpolationType != NULL );

	if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		ERROREXIT;
		
	if(strnicmp(line, VKFRAME_KEYLIST_ID, sizeof(VKFRAME_KEYLIST_ID)-1) != 0)
		ERROREXIT;

	if(sscanf(line + sizeof(VKFRAME_KEYLIST_ID)-1, "%d %d %d %d", 
					&NumElements,InterpolationType,&Compression,Looping) != 4)
		ERROREXIT;

	switch (*InterpolationType)
		{
			case (VKFRAME_LINEAR):
					KeyList = geTKArray_CreateEmpty(sizeof(geVKFrame_Linear),NumElements);
					break;
			case (VKFRAME_HERMITE):
			case (VKFRAME_HERMITE_ZERO_DERIV):
					KeyList = geTKArray_CreateEmpty(sizeof(geVKFrame_Hermite),NumElements);
					break;
			default:
				ERROREXIT;
		}
	if (KeyList == NULL)
		ERROREXIT;

	if (Compression & VKFRAME_LINEARTIME_COMPRESSION)
		{
			if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
				ERROREXIT;
			if (sscanf(line,"%f %f",&StartTime,&DeltaTime) != 2)
				ERROREXIT;
		}

	for(i=0;i<NumElements;i++)
		{
			geVKFrame_Linear* pLinear = (geVKFrame_Linear*)geTKArray_Element(KeyList, i);
			if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
				ERROREXIT;

			if (Compression & VKFRAME_LINEARTIME_COMPRESSION)
				{
					pLinear->Key.Time = StartTime + DeltaTime * i;
					u = sscanf(line,"%f %f %f",  &(pLinear->Key.V.X),
						  &(pLinear->Key.V.Y),&(pLinear->Key.V.Z));
					if (u!=3)
						ERROREXIT;
				}
			else
				{
					u = sscanf(line,"%f %f %f %f",&(pLinear->Key.Time), &(pLinear->Key.V.X),
									  &(pLinear->Key.V.Y),&(pLinear->Key.V.Z));
					if (u!=4)
						ERROREXIT;
				}
		}

	switch (*InterpolationType)
		{
			case (VKFRAME_LINEAR):
					break;
			case (VKFRAME_HERMITE):
				geVKFrame_HermiteRecompute(	*Looping, GE_FALSE, KeyList);
					break;
			case (VKFRAME_HERMITE_ZERO_DERIV):
				geVKFrame_HermiteRecompute(	*Looping, GE_TRUE, KeyList);
					break;
			default:
				assert(0);
		}

	return KeyList;	
}

uint32 GENESISCC geVKFrame_ComputeBlockSize(geTKArray *KeyList, int Compression)
{
	uint32 Size=0;
	int Count;

	assert( KeyList != NULL );
	assert( Compression < 0xFF);
	
	Count = geTKArray_NumElements(KeyList);

	Size += sizeof(uint32);		// flags
	Size += sizeof(uint32);		// count

	if (Compression & VKFRAME_LINEARTIME_COMPRESSION)
		{
			Size += sizeof(geFloat) * 2;
		}
	else
		{
			Size += sizeof(geFloat) * Count;
		}

	Size += sizeof(geVec3d) * Count;
	return Size;
}


geTKArray *GENESISCC geVKFrame_CreateFromBinaryFile(geVFile *pFile, int *InterpolationType, int *Looping)
{
	uint32 u;
	int BlockSize;
	int Compression;
	int Count,i;
	int FieldSize;
	char *Block;
	geFloat *Data;
	geTKArray *KeyList;
	geVKFrame_Linear* pLinear0;
	geVKFrame_Linear* pLinear;
	
	assert( pFile != NULL );
	assert( InterpolationType != NULL );
	assert( Looping != NULL );
	
	if (geVFile_Read(pFile, &BlockSize, sizeof(int)) == GE_FALSE)
		{
			geErrorLog_AddString(-1,"Failure to read binary VKFrame header", NULL);
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
			geErrorLog_AddString(-1,"Failure to read binary VKFrame header", NULL);
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
			case (VKFRAME_LINEAR):
					FieldSize = sizeof(geVKFrame_Linear);
					break;
			case (VKFRAME_HERMITE):
			case (VKFRAME_HERMITE_ZERO_DERIV):
					FieldSize = sizeof(geVKFrame_Hermite);
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
			
	pLinear0 = (geVKFrame_Linear*)geTKArray_Element(KeyList, 0);

	pLinear = pLinear0;

	if (Compression & VKFRAME_LINEARTIME_COMPRESSION)
		{
			geFloat fi;
			geFloat fCount = (geFloat)Count;
			geFloat Time,DeltaTime;
			Time = *(Data++);
			DeltaTime = *(Data++);
			for(fi=0.0f;fi<fCount;fi+=1.0f)
				{
					pLinear->Key.Time = Time + fi*DeltaTime;
					pLinear = (geVKFrame_Linear *)  ( ((char *)pLinear) + FieldSize );
				}
		}
	else
		{
			for(i=0;i<Count;i++)
				{
					pLinear->Key.Time = *(Data++);
					pLinear = (geVKFrame_Linear *)  ( ((char *)pLinear) + FieldSize );
				}
		}

	pLinear = pLinear0;
	for(i=0;i<Count;i++)
		{
			pLinear->Key.V = *(geVec3d *)Data;
			Data += 3;
			pLinear = (geVKFrame_Linear *)  ( ((char *)pLinear) + FieldSize );
		}

	switch (*InterpolationType)
		{
			case (VKFRAME_LINEAR):
					break;
			case (VKFRAME_HERMITE):
				geVKFrame_HermiteRecompute(	*Looping, GE_FALSE, KeyList);
					break;
			case (VKFRAME_HERMITE_ZERO_DERIV):
				geVKFrame_HermiteRecompute(	*Looping, GE_TRUE, KeyList);
					break;
			default:
				assert(0);
		}
	geRam_Free(Block);
	return KeyList;	

}

geBoolean GENESISCC geVKFrame_WriteToBinaryFile(geVFile *pFile, geTKArray *KeyList, 
		geVKFrame_InterpolationType InterpolationType, int Looping)
{
	#define WBERREXIT  {geErrorLog_AddString( ERR_PATH_FILE_WRITE,"Failure to write binary key data", NULL);return GE_FALSE;}
	uint32 u,BlockSize;
	int Compression=0;
	int Count,i;
	geFloat Time,DeltaTime;

	assert( pFile != NULL );
	assert( InterpolationType < 0xFF);
	assert( (Looping == 0) || (Looping == 1) );

	if (geTKArray_NumElements(KeyList)>2)
		{
			if ( geTKArray_SamplesAreTimeLinear(KeyList,LINEARTIME_TOLERANCE) != GE_FALSE )
				{
					Compression |= VKFRAME_LINEARTIME_COMPRESSION;
				}
		}

	u = (InterpolationType << 16) |  (Compression << 8) |  Looping;
	
	BlockSize = geVKFrame_ComputeBlockSize(KeyList,Compression);

	if (geVFile_Write(pFile, &BlockSize,sizeof(uint32)) == GE_FALSE)
		WBERREXIT;
	
	if (geVFile_Write(pFile, &u, sizeof(uint32)) == GE_FALSE)
		WBERREXIT;
	
	Count = geTKArray_NumElements(KeyList);
	if (geVFile_Write(pFile, &Count, sizeof(uint32)) == GE_FALSE)
		WBERREXIT;

	if (Compression & VKFRAME_LINEARTIME_COMPRESSION)
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

	for(i=0;i<Count;i++)
		{
			geVKFrame_Linear* pLinear = (geVKFrame_Linear*)geTKArray_Element(KeyList, i);
			if (geVFile_Write(pFile, &(pLinear->Key.V),sizeof(geVec3d)) == GE_FALSE)
				WBERREXIT;
		}

	return GE_TRUE;
}
