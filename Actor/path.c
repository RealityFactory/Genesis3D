/****************************************************************************************/
/*  PATH.C																				*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Time-indexed keyframe creation, maintenance, and sampling.				*/
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
#include <math.h>   //fmod()
#include <string.h>
#include <stdio.h>  //sscanf

#include "path.h"
#include "Quatern.h"
#include "errorlog.h"
#include "ram.h"
#include "tkarray.h"
#include "VKFrame.h"
#include "QKFrame.h"
#include "vec3d.h"

#define min(aa,bb)  (( (aa)>(bb) ) ? (bb) : (aa) )
#define max(aa,bb)  (( (aa)>(bb) ) ? (aa) : (bb) )


#define gePath_TimeType geFloat

typedef int8 Bool8;

typedef void (GENESISCC *InterpolationFunction)(
	const void *KF1,
	const void *KF2, 
	gePath_TimeType T,
	void *Result);


#define FLAG_DIRTY   (0x01)
#define FLAG_LOOPED  (0x01)
#define FLAG_OTHER	 (0x696C6345)
#define FLAG_EMPTY	 (0x21657370)

typedef struct
{
	geTKArray *KeyList;
	int InterpolationType;				// type of interpolation for channel

	gePath_TimeType StartTime;			// First time in channel's path
	gePath_TimeType EndTime;			// Last time in channel's path

	// --remember keys used for last sample--
	int32 LastKey1;						// smaller key
	int32 LastKey2;						// larger key (keys may be equal)
	gePath_TimeType LastKey1Time;		// Time at LastKey1
	gePath_TimeType LastKey2Time;		// Time at LastKey2
									// if last key is not valid: LastKey1Time > LastKey2Time
} gePath_Channel;


typedef struct _gePath
{
	gePath_Channel Rotation;
	gePath_Channel Translation;
	unsigned int Dirty   : 1;						
	unsigned int Looped  : 1;
	unsigned int RefCount:30;
} gePath;

typedef enum
{
	GE_PATH_VK_LINEAR,
	GE_PATH_VK_HERMITE,
	GE_PATH_VK_HERMITE_ZERO_DERIV,
	GE_PATH_QK_LINEAR,
	GE_PATH_QK_SLERP,
	GE_PATH_QK_SQUAD,
	GE_PATH_MANY_INTERPOLATORS
} gePath_InterpolationType;

typedef struct 
{
	InterpolationFunction InterpolationTable[GE_PATH_MANY_INTERPOLATORS];
	int32 Flags[2];
} gePath_StaticType;

gePath_StaticType gePath_Statics = 
{
	{ 	geVKFrame_LinearInterpolation,
		geVKFrame_HermiteInterpolation,
		geVKFrame_HermiteInterpolation,
		geQKFrame_LinearInterpolation,
		geQKFrame_SlerpInterpolation,
		geQKFrame_SquadInterpolation
	},
	{FLAG_OTHER,FLAG_EMPTY}
};


static geVKFrame_InterpolationType GENESISCC gePath_PathToVKInterpolation(gePath_InterpolationType I)
{
	switch (I)
		{
			case (GE_PATH_VK_LINEAR):			  return VKFRAME_LINEAR;
			case (GE_PATH_VK_HERMITE):			  return VKFRAME_HERMITE;
			case (GE_PATH_VK_HERMITE_ZERO_DERIV): return VKFRAME_HERMITE_ZERO_DERIV;
			default: assert(0);
		}
	return VKFRAME_LINEAR;  // this is just for warning removal
}
			
static gePath_InterpolationType GENESISCC gePath_VKToPathInterpolation(geVKFrame_InterpolationType I)
{
	switch (I)
		{
			case (VKFRAME_LINEAR):				return GE_PATH_VK_LINEAR;
			case (VKFRAME_HERMITE):				return GE_PATH_VK_HERMITE;
			case (VKFRAME_HERMITE_ZERO_DERIV):  return GE_PATH_VK_HERMITE_ZERO_DERIV;
			default: assert(0);
		}
	return GE_PATH_VK_LINEAR; // this is just for warning removal
}

static geQKFrame_InterpolationType GENESISCC gePath_PathToQKInterpolation(gePath_InterpolationType I)
{
	switch (I)
		{
			case (GE_PATH_QK_LINEAR):	return QKFRAME_LINEAR;
			case (GE_PATH_QK_SLERP):	return QKFRAME_SLERP;
			case (GE_PATH_QK_SQUAD):	return QKFRAME_SQUAD;
			default: assert(0);
		}
	return QKFRAME_LINEAR;  // this is just for warning removal
}
			
static gePath_InterpolationType GENESISCC gePath_QKToPathInterpolation(geQKFrame_InterpolationType I)
{
	switch (I)
		{
			case (QKFRAME_LINEAR):	return GE_PATH_QK_LINEAR;
			case (QKFRAME_SLERP):	return GE_PATH_QK_SLERP;
			case (QKFRAME_SQUAD):	return GE_PATH_QK_SQUAD;
			default: assert(0);
		}
	return GE_PATH_QK_LINEAR; // this is just for warning removal
}


GENESISAPI void GENESISCC gePath_CreateRef( gePath *P )
{
	assert( P != NULL );
	P->RefCount++;
}

GENESISAPI gePath *GENESISCC gePath_Create(
	gePath_Interpolator TranslationInterpolation,	// type of interpolation for translation channel
	gePath_Interpolator RotationInterpolation,	// type of interpolation for rotation channel
	geBoolean Looped)				// GE_TRUE if end of path is connected to head
	
{
	gePath *P;

	P = geRam_Allocate(sizeof(gePath));

	if ( P == NULL )
	{
		geErrorLog_Add(ERR_PATH_CREATE_ENOMEM, NULL);
		return NULL;
	}

	P->Rotation.KeyList    = NULL;
	P->Translation.KeyList = NULL;
	
	P->RefCount = 0;
	P->Dirty    = FLAG_DIRTY;

	if (Looped==GE_TRUE)
		P->Looped = FLAG_LOOPED;
	else
		P->Looped = 0;


	switch (RotationInterpolation)
		{
			case (GE_PATH_INTERPOLATE_LINEAR):
				P->Rotation.InterpolationType = GE_PATH_QK_LINEAR;
				break;
			case (GE_PATH_INTERPOLATE_SLERP):
				P->Rotation.InterpolationType = GE_PATH_QK_SLERP; 
				break;
			case (GE_PATH_INTERPOLATE_SQUAD):
				P->Rotation.InterpolationType = GE_PATH_QK_SQUAD;
				break;
			default:
				assert(0);
		}
	
	P->Rotation.KeyList = NULL;

	switch (TranslationInterpolation)
		{
			case (GE_PATH_INTERPOLATE_LINEAR):
				P->Translation.InterpolationType = GE_PATH_VK_LINEAR;
				break;
			case (GE_PATH_INTERPOLATE_HERMITE):
				P->Translation.InterpolationType = GE_PATH_VK_HERMITE;
				break;
			case (GE_PATH_INTERPOLATE_HERMITE_ZERO_DERIV):
				P->Translation.InterpolationType = GE_PATH_VK_HERMITE_ZERO_DERIV;
				break;
			default:
				assert(0);
		}

	P->Translation.KeyList = NULL;

	return P;
}

static geBoolean GENESISCC gePath_SetupRotationKeyList(gePath *P)
{
	assert( P != NULL );
	switch (P->Rotation.InterpolationType)
		{
			case (GE_PATH_QK_LINEAR):
				P->Rotation.KeyList = geQKFrame_LinearCreate();
				break;
			case (GE_PATH_QK_SLERP):
				P->Rotation.KeyList = geQKFrame_SlerpCreate();
				break;
			case (GE_PATH_QK_SQUAD):
				P->Rotation.KeyList = geQKFrame_SquadCreate();
				break;
			default:
				assert(0);
		}
	if (P->Rotation.KeyList == NULL)
		{
			return GE_FALSE;
		}
	return GE_TRUE;	
}

static geBoolean GENESISCC gePath_SetupTranslationKeyList(gePath *P)
{
	assert( P != NULL );
	switch (P->Translation.InterpolationType)
		{
			case (GE_PATH_VK_LINEAR):
				P->Translation.KeyList = geVKFrame_LinearCreate();
				break;
			case (GE_PATH_VK_HERMITE):
				P->Translation.KeyList = geVKFrame_HermiteCreate();
				break;
			case (GE_PATH_VK_HERMITE_ZERO_DERIV):
				P->Translation.KeyList = geVKFrame_HermiteCreate();
				break;
			default:
				assert(0);
		}
	if (P->Translation.KeyList == NULL)
		{
			return GE_FALSE;
		}
	return GE_TRUE;
}

GENESISAPI gePath *GENESISCC gePath_CreateCopy(const gePath *Src)
{
	gePath *P;
	gePath_TimeType Time;
	geBoolean Looped;

	int i,Count;
	int RInterp=0;
	int TInterp=0;

	assert ( Src != NULL );

	switch (Src->Rotation.InterpolationType)
		{
			case (GE_PATH_QK_LINEAR):
				RInterp = GE_PATH_INTERPOLATE_LINEAR;
				break;
			case (GE_PATH_QK_SLERP):
				RInterp = GE_PATH_INTERPOLATE_SLERP;
				break;
			case (GE_PATH_QK_SQUAD):
				RInterp = GE_PATH_INTERPOLATE_SQUAD;
				break;
			default:
				assert(0);
		}
	
	switch (Src->Translation.InterpolationType)
		{
			case (GE_PATH_VK_LINEAR):
				TInterp = GE_PATH_INTERPOLATE_LINEAR;
				break;
			case (GE_PATH_VK_HERMITE):
				TInterp = GE_PATH_INTERPOLATE_HERMITE;
				break;
			case (GE_PATH_VK_HERMITE_ZERO_DERIV):
				TInterp = GE_PATH_INTERPOLATE_HERMITE_ZERO_DERIV;
				break;
			default:
				assert(0);
		}
	
	if (Src->Looped)
		Looped = GE_TRUE;
	else
		Looped = GE_FALSE;

	P = gePath_Create(TInterp, RInterp, Looped);	
	if (P == NULL)
		{
			geErrorLog_Add(ERR_PATH_CREATE_ENOMEM, NULL);
			return NULL;
		}

	{
		geVec3d V;
		Count = 0;
		if (Src->Translation.KeyList != NULL)
			{
				Count = geTKArray_NumElements(Src->Translation.KeyList);
			}
		if (Count>0)
			{
				if (gePath_SetupTranslationKeyList(P)==GE_FALSE)
					{
						geErrorLog_Add(ERR_PATH_CREATE_ENOMEM, NULL);
						gePath_Destroy(&P);
						return NULL;
					}

				for (i=0; i<Count; i++)
					{
						int Index;
						geVKFrame_Query(Src->Translation.KeyList, i, &Time, &V);
						if (geVKFrame_Insert(&(P->Translation.KeyList), Time, &V,&Index) == GE_FALSE)
							{
								geErrorLog_Add(ERR_PATH_CREATE_ENOMEM, NULL);
								gePath_Destroy(&P);
								return NULL;
							}
					}
			}
	}

	{
		geQuaternion Q;
		Count = 0;
		if (Src->Rotation.KeyList != NULL)
			{
				Count = geTKArray_NumElements(Src->Rotation.KeyList);
			}
		if (Count>0)
			{
				if (gePath_SetupRotationKeyList(P)==GE_FALSE)
					{
						geErrorLog_Add(ERR_PATH_CREATE_ENOMEM, NULL);
						gePath_Destroy(&P);
						return NULL;
					}

				for (i=0; i<Count; i++)
					{
						int Index;
						geQKFrame_Query(Src->Rotation.KeyList, i, &Time, &Q);
						if (geQKFrame_Insert(&(P->Rotation.KeyList), Time, &Q, &Index) == GE_FALSE)
							{
								geErrorLog_Add(ERR_PATH_CREATE_ENOMEM, NULL);
								gePath_Destroy(&P);
								return NULL;
							}
					}
			}
	}
	return P;
}
	


GENESISAPI void GENESISCC gePath_Destroy(gePath **PP)
{
	gePath *P;
	
	assert( PP  != NULL );
	assert( *PP != NULL );
	
	P = *PP;

	if ( P->RefCount > 0)
		{
			P->RefCount -- ;
			return;
		}
	if ( P->Rotation.KeyList != NULL)
	{
		geTKArray_Destroy(&(P->Rotation.KeyList));
		P->Rotation.KeyList = NULL;
	}

	if ( P->Translation.KeyList != NULL)
	{
		geTKArray_Destroy(&(P->Translation.KeyList));
		P->Translation.KeyList = NULL;
	}

	geRam_Free(*PP);

	*PP = NULL;
}


static void GENESISCC gePath_Recompute(gePath *P)
	// Recompute any pre-computed constants for the current path.
{
	geBoolean Looped;
	assert(P);

	P->Dirty = 0;

	P->Translation.LastKey1Time = 0.0f;
	P->Translation.LastKey2Time = -1.0f;
	if (P->Looped)
		Looped = GE_TRUE;
	else
		Looped = GE_FALSE;

	if (P->Translation.KeyList != NULL)
	{
		if (geTKArray_NumElements(P->Translation.KeyList) > 0 )
		{
			P->Translation.StartTime =	geTKArray_ElementTime(P->Translation.KeyList,0);
			P->Translation.EndTime   =	geTKArray_ElementTime(P->Translation.KeyList,
										geTKArray_NumElements(P->Translation.KeyList) - 1);
		}
		if(P->Translation.InterpolationType == GE_PATH_VK_HERMITE)
			geVKFrame_HermiteRecompute(Looped, GE_FALSE, P->Translation.KeyList);
		else if (P->Translation.InterpolationType == GE_PATH_VK_HERMITE_ZERO_DERIV)
			geVKFrame_HermiteRecompute(Looped, GE_TRUE, P->Translation.KeyList);
	}
	
	P->Rotation.LastKey1Time = 0.0f;
	P->Rotation.LastKey2Time = -1.0f;

	if (P->Rotation.KeyList != NULL)
	{
		if (geTKArray_NumElements(P->Rotation.KeyList) > 0 )
		{
			P->Rotation.StartTime = geTKArray_ElementTime(P->Rotation.KeyList,0);
			P->Rotation.EndTime   = geTKArray_ElementTime(P->Rotation.KeyList,
									geTKArray_NumElements(P->Rotation.KeyList) - 1);
		}
		if (P->Rotation.InterpolationType == GE_PATH_QK_SQUAD)
			geQKFrame_SquadRecompute(Looped, P->Rotation.KeyList);
		else if (P->Rotation.InterpolationType == GE_PATH_QK_SLERP)
			geQKFrame_SlerpRecompute(P->Rotation.KeyList);
	}
}	

//------------------ time based keyframe operations
GENESISAPI geBoolean GENESISCC gePath_InsertKeyframe(
	gePath *P, 
	int ChannelMask, 
	gePath_TimeType Time, 
	const geXForm3d *Matrix)
{
	int VIndex;
	int QIndex;
	assert( P != NULL );
	assert( Matrix != NULL );
	assert( ( ChannelMask & GE_PATH_ROTATION_CHANNEL    ) ||
			( ChannelMask & GE_PATH_TRANSLATION_CHANNEL ) );
	
	if (ChannelMask & GE_PATH_ROTATION_CHANNEL)
	{	
		geQuaternion Q;
		geQuaternion_FromMatrix(Matrix, &Q);
		geQuaternion_Normalize(&Q);
		if (P->Rotation.KeyList==NULL)
		{
			if (gePath_SetupRotationKeyList(P)==GE_FALSE)
				{
					geErrorLog_Add(ERR_PATH_INSERT_R_KEYFRAME, NULL);
					return GE_FALSE;
				}
		}
		if (geQKFrame_Insert(&(P->Rotation.KeyList), Time, &Q, &QIndex) == GE_FALSE)
		{
			geErrorLog_Add(ERR_PATH_INSERT_R_KEYFRAME, NULL);
			return GE_FALSE;
		}
	}

	
	if (ChannelMask & GE_PATH_TRANSLATION_CHANNEL)
	{
		geBoolean ErrorOccured = GE_FALSE;
		if (P->Translation.KeyList == NULL)
			{
				if (gePath_SetupTranslationKeyList(P)==GE_FALSE)
					{
						geErrorLog_Add(ERR_PATH_INSERT_R_KEYFRAME, NULL);
						ErrorOccured = GE_TRUE;
					}
			}
		if (ErrorOccured == GE_FALSE)
			{
				if (geVKFrame_Insert( &(P->Translation.KeyList), Time, &(Matrix->Translation), &VIndex) == GE_FALSE)
					{
						geErrorLog_Add(ERR_PATH_INSERT_T_KEYFRAME, NULL);
						ErrorOccured = GE_TRUE;
					}
			}
		if (ErrorOccured != GE_FALSE)
			{
				if (ChannelMask & GE_PATH_ROTATION_CHANNEL)
					{	// clean up previously inserted rotation
						if (geTKArray_DeleteElement(&(P->Rotation.KeyList),QIndex)==GE_FALSE)
							{
								geErrorLog_Add(ERR_PATH_DELETE_T_KEYFRAME, NULL);
							}
					}
				P->Dirty = FLAG_DIRTY;
				return GE_FALSE;
			}
	}

	P->Dirty = FLAG_DIRTY;

	return GE_TRUE;
}
	
GENESISAPI geBoolean GENESISCC gePath_DeleteKeyframe(
	gePath *P,
	int Index,
	int ChannelMask)
{
	int ErrorOccured= 0;

	assert( P != NULL );
	assert( ( ChannelMask & GE_PATH_ROTATION_CHANNEL    ) ||
			( ChannelMask & GE_PATH_TRANSLATION_CHANNEL ) );

	if (ChannelMask & GE_PATH_ROTATION_CHANNEL)
	{
		if (geTKArray_DeleteElement( &(P->Rotation.KeyList), Index) == GE_FALSE)
		{
			ErrorOccured = 1;
			geErrorLog_Add(ERR_PATH_DELETE_R_KEYFRAME, NULL);
		}
	}
			
	if (ChannelMask & GE_PATH_TRANSLATION_CHANNEL)
	{
		if (geTKArray_DeleteElement( &(P->Translation.KeyList), Index) == GE_FALSE)
		{
			ErrorOccured = 1;
			geErrorLog_Add(ERR_PATH_DELETE_T_KEYFRAME, NULL);
		}
	}

	P->Dirty = FLAG_DIRTY;


	if (ErrorOccured)
	{
		return GE_FALSE;
	}

	return GE_TRUE;
}


GENESISAPI void GENESISCC gePath_GetKeyframe(
	const gePath *P, 
	int Index,				// gets keyframe[index]
	int Channel,			// for this channel
	gePath_TimeType *Time,	// returns the time of the keyframe
	geXForm3d *Matrix) 		// returns the matrix of the keyframe
{
	assert( P != NULL );
	assert( Index >= 0 );
	assert( Time != NULL );
	assert( Matrix != NULL );

	geXForm3d_SetIdentity(Matrix);

	switch (Channel)
	{
	case (GE_PATH_ROTATION_CHANNEL):
		{
			geQuaternion Q;
			assert( Index < geTKArray_NumElements(P->Rotation.KeyList) );
			geQKFrame_Query(P->Rotation.KeyList, Index, Time, &Q);
			geQuaternion_ToMatrix(&Q, Matrix);
		}
		break;

	case (GE_PATH_TRANSLATION_CHANNEL):
		{
			assert( Index < geTKArray_NumElements(P->Translation.KeyList) );
			geVKFrame_Query(P->Translation.KeyList, Index, Time, &(Matrix->Translation));
		}
		break;

	default:
		assert(0);
	}
}

GENESISAPI geBoolean GENESISCC gePath_ModifyKeyframe(
	gePath *P, 
	int Index,						// keyframe[index]
	int ChannelMask,				// for this channel
	const geXForm3d *Matrix) 		// new matrix for the keyframe
{
	assert( P != NULL );
	assert( Index >= 0 );
	assert( Matrix != NULL );
	assert( ( ChannelMask & GE_PATH_ROTATION_CHANNEL    ) ||
			( ChannelMask & GE_PATH_TRANSLATION_CHANNEL ) );


	if (ChannelMask & GE_PATH_ROTATION_CHANNEL)
		{
			geQuaternion Q;
			assert( Index < geTKArray_NumElements(P->Rotation.KeyList) );
			geQuaternion_FromMatrix(Matrix, &Q);
			geQuaternion_Normalize(&Q);
			geQKFrame_Modify(P->Rotation.KeyList, Index, &Q);
		}

	if (ChannelMask & GE_PATH_TRANSLATION_CHANNEL)
		{
			assert( Index < geTKArray_NumElements(P->Translation.KeyList) );
			geVKFrame_Modify(P->Translation.KeyList, Index, &(Matrix->Translation));
		}

	P->Dirty = FLAG_DIRTY;
	return GE_TRUE;
}


GENESISAPI int GENESISCC gePath_GetKeyframeCount(const gePath *P, int Channel)
{
	assert( P != NULL );

	switch (Channel)
	{
		case (GE_PATH_ROTATION_CHANNEL):
			if (P->Rotation.KeyList!=NULL)
				{
					return geTKArray_NumElements(P->Rotation.KeyList);
				}
			else
				{
					return 0;
				}
			break;

		case (GE_PATH_TRANSLATION_CHANNEL):
			if (P->Translation.KeyList!=NULL)
				{
					return geTKArray_NumElements(P->Translation.KeyList);
				}
			else
				{
					return 0;
				}
			break;

		default:
			assert(0);
	}
	return 0; // this is just for warning removal
}

GENESISAPI int GENESISCC gePath_GetKeyframeIndex(const gePath *P, int Channel, geFloat Time)
	// retrieves the index of the keyframe at a specific time for a specific channel
{
	int KeyIndex;
	geTKArray *Array = NULL;

	assert ((Channel == GE_PATH_TRANSLATION_CHANNEL) ||
			(Channel == GE_PATH_ROTATION_CHANNEL));

	switch (Channel)
	{
		case GE_PATH_ROTATION_CHANNEL :
			Array = P->Rotation.KeyList;
			break;

		case GE_PATH_TRANSLATION_CHANNEL :
			Array = P->Translation.KeyList;
			break;
	}

	// find the time in the channel's array
	KeyIndex = geTKArray_BSearch (Array, Time);
	if (KeyIndex != -1)
	{
		// since geTKArray_BSearch will return the "closest" key,
		// I need to make sure that it's exact...
		if (fabs (Time - geTKArray_ElementTime (Array, KeyIndex)) > GE_TKA_TIME_TOLERANCE)
		{
			KeyIndex = -1;
		}
	}

	return KeyIndex;
}


static gePath_TimeType GENESISCC gePath_AdjustTimeForLooping(
	geBoolean Looped,
	gePath_TimeType Time, 
	gePath_TimeType TStart, 
	gePath_TimeType TEnd)
{
	if (Looped!=GE_FALSE)
	{
		if (Time < TStart)
		{
			return (gePath_TimeType)fmod(Time - TStart, TEnd - TStart) + TStart + TEnd;
		}
		else
		{
			if (Time >= TEnd)
			{
				if(TStart + GE_TKA_TIME_TOLERANCE > TEnd)
					return TStart;

				return (gePath_TimeType)fmod(Time - TStart, TEnd - TStart) + TStart;
			}
			else
			{
				return Time;
			}
		}
	}
	else
	{
		return Time;
	}
}


static geBoolean GENESISCC gePath_SampleChannel(
	const gePath_Channel *Channel,			// channel to sample
	geBoolean Looped,
	gePath_TimeType Time, 
	void *Result)
				// return GE_TRUE if sample was made,
				// return GE_FALSE if no sample was made (no keyframes)
{
	int Index1,Index2;				// index of keyframe just before and after Time
	gePath_TimeType Time1, Time2;	// Times in those keyframes	
	gePath_TimeType T;				// 0..1 blending factor
	gePath_TimeType AdjTime;		// parameter Time adjusted for looping.
	int Length;
	
	assert( Channel != NULL );
	assert( Result != NULL );

	if (Channel->KeyList == NULL)	
		return GE_FALSE;
	
	Length = geTKArray_NumElements( Channel->KeyList );
			
	if ( Length == 0 )
	{
		//Interpolate(Channel,NULL,NULL,Time,Result);
		return GE_FALSE;
	}

	AdjTime = gePath_AdjustTimeForLooping(Looped,Time,
			Channel->StartTime,Channel->EndTime);

	if (	( Channel->LastKey1Time <= AdjTime ) && 
			( AdjTime < Channel->LastKey2Time  ) )
	{  
		Index1 = Channel->LastKey1;
		Index2 = Channel->LastKey2;
		Time1  = Channel->LastKey1Time;
		Time2  = Channel->LastKey2Time;
	}
	else
	{
		Index1 = geTKArray_BSearch( Channel->KeyList,
								AdjTime);
		Index2 = Index1 + 1;

		// edge conditions: if Time is off end of path's time, use end point twice
		if ( Index1 < 0 )	
		{
			if (Looped!=GE_FALSE) 
			{
				Index1 = Length -1;
			}
			else
			{
				Index1 = 0;
			}
		}
		if ( Index2 >= Length )
		{
			if (Looped!=GE_FALSE)
			{
				Index2 = 0;
			}
			else
			{
				Index2 = Length - 1;
			}
		}
		((gePath_Channel *)Channel)->LastKey1 = Index1;
		((gePath_Channel *)Channel)->LastKey2 = Index2;
		Time1 = ((gePath_Channel *)Channel)->LastKey1Time = geTKArray_ElementTime(Channel->KeyList, Index1);
		Time2 = ((gePath_Channel *)Channel)->LastKey2Time = geTKArray_ElementTime(Channel->KeyList, Index2);
	}
	
	if (Index1 == Index2)
		T=0.0f;			// Time2 == Time1 !
	else
		T = (AdjTime-Time1) / (Time2 - Time1);
	
	gePath_Statics.InterpolationTable[Channel->InterpolationType](
				geTKArray_Element(Channel->KeyList,Index1),
				geTKArray_Element(Channel->KeyList,Index2),
				T,Result);

	return GE_TRUE;
}


GENESISAPI void GENESISCC gePath_Sample(const gePath *P, gePath_TimeType Time, geXForm3d *Matrix)
{
	geQuaternion	Rotation;
	geVec3d		Translation;
	geBoolean Looped;

	assert( P != NULL );
	assert( Matrix != NULL );


	if (P->Dirty)
		{
			gePath_Recompute((gePath *)P);
		}
	if (P->Looped)
		Looped = GE_TRUE;
	else
		Looped = GE_FALSE;

	if(gePath_SampleChannel(&(P->Rotation), Looped, Time, (void*)&Rotation) == GE_TRUE)
	{
		geQuaternion_ToMatrix(&Rotation, Matrix);
	}
	else
	{
		geXForm3d_SetIdentity(Matrix);
	}

	if(gePath_SampleChannel(&(P->Translation), Looped, Time, (void*)&Translation) == GE_TRUE)
	{
		Matrix->Translation = Translation;
	}
	else
	{
		Matrix->Translation.X = Matrix->Translation.Y = Matrix->Translation.Z = 0.0f;
	}

}

void GENESISCC gePath_SampleChannels(const gePath *P, gePath_TimeType Time, geQuaternion *Rotation, geVec3d *Translation)
{
	geBoolean Looped;
	assert( P != NULL );
	assert( Rotation != NULL );
	assert( Translation != NULL );

	if (P->Dirty)
		{
			gePath_Recompute((gePath *)P);
		}

	if (P->Looped)
		Looped = GE_TRUE;
	else
		Looped = GE_FALSE;
	
	if(gePath_SampleChannel(&(P->Rotation), Looped, Time, (void*)Rotation) == GE_FALSE)
	{
		geQuaternion_SetNoRotation(Rotation);
	}

	if(gePath_SampleChannel(&(P->Translation), Looped, Time, (void*)Translation) == GE_FALSE)
	{
		Translation->X  = Translation->Y = Translation->Z = 0.0f;
	}
}


GENESISAPI geBoolean GENESISCC gePath_GetTimeExtents(const gePath *P, gePath_TimeType *StartTime, gePath_TimeType *EndTime)
	// returns false and times are unchanged if there is no extent (no keys)
{
	gePath_TimeType TransStart,TransEnd,RotStart,RotEnd;

	int RCount,TCount;
	assert( P != NULL );
	assert( StartTime != NULL );
	assert( EndTime != NULL );
	// this is a pain because each channel may have 0,1, or more keys
	
	if (P->Rotation.KeyList!=NULL)
		RCount = geTKArray_NumElements( P->Rotation.KeyList );
	else
		RCount = 0;

	if (P->Translation.KeyList!=NULL)
		TCount = geTKArray_NumElements( P->Translation.KeyList );
	else
		TCount = 0;
	
	if (RCount>0)
		{	
			RotStart = geTKArray_ElementTime(P->Rotation.KeyList, 0);
			if (RCount>1)
				{
					RotEnd = geTKArray_ElementTime(P->Rotation.KeyList, RCount-1);
				}
			else
				{
					RotEnd = RotStart;
				}
			if (TCount>0)
				{	// Rotation and Translation keys
					TransStart = geTKArray_ElementTime(P->Translation.KeyList, 0);
					if (TCount>1)
						{
							TransEnd = geTKArray_ElementTime(P->Translation.KeyList,TCount-1);
						}
					else
						{
							TransEnd = TransStart;
						}

					*StartTime = min(TransStart,RotStart);
					*EndTime   = max(TransEnd,RotEnd);
				}
			else
				{	// No Translation Keys
					*StartTime = RotStart;
					*EndTime   = RotEnd;
				}
		}
	else
		{  // No Rotation Keys
			if (TCount>0)
				{
					*StartTime = geTKArray_ElementTime(P->Translation.KeyList, 0);
					if (TCount>1)
						{
							*EndTime = geTKArray_ElementTime(P->Translation.KeyList,TCount-1);
						}
					else
						{
							*EndTime = *StartTime;
						}
				}
			else
				{	// No Rotation or Translation keys
					return GE_FALSE;
				}
		}
	return GE_TRUE;	
}

// First uint32 of ASCII and Binary formats flags the file type.  If the value
// matches the token, it's ASCII.  Binary files use this uint32 for versions
// and flags.

#define GE_PATH_BINARY_FILE_VERSION 0x1001
static gePath *GENESISCC gePath_CreateFromBinaryFile(geVFile *F,uint32 Header);


#define CHECK_FOR_WRITE(uu) if(uu <= 0) { geErrorLog_Add( ERR_PATH_FILE_WRITE , NULL); return GE_FALSE; }

#define GE_PATH_CHANNEL_ASCII_FILE_TYPE 0x4C4E4843	// 'CHNL'
#define GE_PATH_CHANNEL_FILE_VERSION 0x00F0		// Restrict version to 16 bits

#define GE_PATH_CHANNEL_INTERPOLATE_ID "InterpolationType"
#define GE_PATH_CHANNEL_STARTTIME_ID "StartTime"
#define GE_PATH_CHANNEL_ENDTIME_ID "EndTime"
#define GE_PATH_CHANNEL_KEYLIST_ID "KeyList"
#define GE_PATH_CHANNEL_NUM_ASCII_IDS 4	// Keep this up to date

geBoolean GENESISCC gePath_ReadChannel_F0_(int ChannelMask, gePath_Channel *C, geVFile* pFile)
{
	uint32 u, v;
	int NumItemsNeeded=0;
	int NumItemsRead = 0;
	#define LINE_LENGTH 256
	char line[LINE_LENGTH];
	geBoolean (GENESISCC *FrameRead)(geVFile*, void*);
	gePath_TimeType Time;
	int Interp=0;
	void* pElement;
	char	VersionString[32];

	assert( C != NULL );
	assert( pFile != NULL );

	// Read the format/version flag
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
		return GE_FALSE;
	}

	if(u != GE_PATH_CHANNEL_ASCII_FILE_TYPE)
	{
		geErrorLog_Add( ERR_PATH_FILE_VERSION , NULL);
		return GE_FALSE;
	}

	// Read and build the version.  Then determine the number of items to read.
	if	(geVFile_GetS(pFile, VersionString, sizeof(VersionString)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
		return GE_FALSE;
	}

	if	(sscanf(VersionString, "%X.%X\n", &u, &v) != 2)
	{
		geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
		return GE_FALSE;
	}
	v |= (u << 8);
	if(v >= GE_PATH_CHANNEL_FILE_VERSION)
	{
		NumItemsNeeded = GE_PATH_CHANNEL_NUM_ASCII_IDS;
	}

	// Set InterpolationType to something less than valid so the KeyList will
	// be assured of reading properly.
	C->InterpolationType = -1;

	// reset sample optimization bracket
	C->LastKey1Time = 0.0f;
	C->LastKey2Time = -1.0f;

	while(NumItemsRead < NumItemsNeeded)
	{
		if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
			{						
				geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
				break;		 // got to read something
			}

		if(strnicmp(line, GE_PATH_CHANNEL_INTERPOLATE_ID, sizeof(GE_PATH_CHANNEL_INTERPOLATE_ID)-1) == 0)
		{
			if(sscanf(line + sizeof(GE_PATH_CHANNEL_INTERPOLATE_ID)-1, "%d", &Interp) != 1)
				{						
					geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
					break;		 
				}
			NumItemsRead++;
		}
		else if(strnicmp(line, GE_PATH_CHANNEL_STARTTIME_ID, sizeof(GE_PATH_CHANNEL_STARTTIME_ID)-1) == 0)
		{
			if(sscanf(line + sizeof(GE_PATH_CHANNEL_STARTTIME_ID)-1, "%f", &C->StartTime) != 1)
				{						
					geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
					break;		 
				}
			NumItemsRead++;
		}
		else if(strnicmp(line, GE_PATH_CHANNEL_ENDTIME_ID, sizeof(GE_PATH_CHANNEL_ENDTIME_ID)-1) == 0)
		{
			if(sscanf(line + sizeof(GE_PATH_CHANNEL_ENDTIME_ID)-1, "%f", &C->EndTime) != 1)
				{						
					geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
					break;		 
				}
			NumItemsRead++;
		}
		else if(strnicmp(line, GE_PATH_CHANNEL_KEYLIST_ID, sizeof(GE_PATH_CHANNEL_KEYLIST_ID)-1) == 0)
		{
			assert(C->KeyList == NULL);

			// v = number of elements
			if(sscanf(line + sizeof(GE_PATH_CHANNEL_KEYLIST_ID)-1, "%d", &v) != 1)
				{						
					geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
					break;		 
				}

			if(ChannelMask == GE_PATH_ROTATION_CHANNEL)
			{
				switch(Interp)
				{
				case GE_PATH_INTERPOLATE_LINEAR:
					C->KeyList = geQKFrame_LinearCreate();
					C->InterpolationType = GE_PATH_QK_LINEAR;
					FrameRead = geQKFrame_LinearRead;
					break;

				case GE_PATH_INTERPOLATE_SLERP:
					C->KeyList = geQKFrame_SlerpCreate();
					C->InterpolationType = GE_PATH_QK_SLERP;
					FrameRead = geQKFrame_SlerpRead;
					break;

				case GE_PATH_INTERPOLATE_SQUAD:
					C->KeyList = geQKFrame_SquadCreate();
					C->InterpolationType = GE_PATH_QK_SQUAD;
					FrameRead = geQKFrame_SquadRead;
					break;

				default:
					geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
					return GE_FALSE;
				}
			}
			else
			{
				assert(ChannelMask == GE_PATH_TRANSLATION_CHANNEL);

				switch(Interp)
				{
				case GE_PATH_INTERPOLATE_LINEAR:
					C->KeyList = geVKFrame_LinearCreate();
					C->InterpolationType = GE_PATH_VK_LINEAR;
					FrameRead = geVKFrame_LinearRead;
					break;

				case GE_PATH_INTERPOLATE_HERMITE:
					C->KeyList = geVKFrame_HermiteCreate();
					C->InterpolationType = GE_PATH_VK_HERMITE;
					FrameRead = geVKFrame_HermiteRead;
					break;

				case GE_PATH_INTERPOLATE_HERMITE_ZERO_DERIV:
					C->KeyList = geVKFrame_HermiteCreate();
					C->InterpolationType = GE_PATH_VK_HERMITE_ZERO_DERIV;
					FrameRead = geVKFrame_HermiteRead;
					break;

				default:
					geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
					return GE_FALSE;
				}
			}

			while( v > 0 )
			{
				char	TimeString[32];
				v--;

				if(geVFile_GetS(pFile, TimeString, sizeof(TimeString)) == GE_FALSE)
					{						
						geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
						break;		 
					}
				if(sscanf(TimeString, "%f ", &Time) != 1)
					{						
						geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
						break;		 
					}

				{
					int NewlyAddedElement;  // u = newly added element
					if(geTKArray_Insert(&C->KeyList, Time, &NewlyAddedElement) == GE_FALSE)
						break;
					pElement = geTKArray_Element(C->KeyList, NewlyAddedElement);
				}
				if(FrameRead(pFile, pElement) == GE_FALSE)
					break;
			}

			if( v > 0 )
			{
				// must have aborted early
				geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
				break;
			}

			NumItemsRead++;
		}
		else
		{
			// Bad news, unknown line, kill the loop
			geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
			break;
		}
	}
	if (NumItemsNeeded == NumItemsRead)
		{
			return GE_TRUE;
		}
	else
		{
			geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
			return GE_FALSE;
		}
}


#define GE_PATH_ASCII_FILE_TYPE 0x48544150	// 'PATH'
#define GE_PATH_FILE_VERSION0 0x00F0		// Restrict version to 16 bits
#define GE_PATH_FILE_VERSION1 0x00F1		// Restrict version to 16 bits
#define GE_PATH_FILE_VERSION  0x00F2		// Restrict version to 16 bits

#define GE_PATH_ROTATION_ID "Rotation"
#define GE_PATH_TRANSLATION_ID "Translation"
#define GE_PATH_LOOPED_ID "Looped"
#define GE_PATH_NUM_ASCII_IDS 2	// Keep this up to date

#define EXIT_ERROR { gePath_Destroy(&P); geErrorLog_Add( ERR_PATH_FILE_READ , NULL); return NULL; }

gePath* GENESISCC gePath_CreateFromFile_F0_(geVFile* pFile)
{
	gePath* P;
	int flag;
		
	#define LINE_LENGTH 256
	char line[LINE_LENGTH];

	assert( pFile != NULL );
	
	P = gePath_Create(0, 0, GE_FALSE);
	if( P == NULL )
		{
			return NULL;	// error logged already in gePath_Create
		}

	P->Translation.InterpolationType = 0;
	P->Rotation.InterpolationType    = 0;

	if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		EXIT_ERROR
	if(strnicmp(line, GE_PATH_LOOPED_ID, sizeof(GE_PATH_LOOPED_ID)-1) != 0)
		EXIT_ERROR

	if(sscanf(line + sizeof(GE_PATH_LOOPED_ID)-1, "%d", &flag) != 1)
		EXIT_ERROR
	
	if (flag == GE_TRUE)
		{
			P->Looped = FLAG_LOOPED;
		}

	if(!geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		EXIT_ERROR
	if(strnicmp(line, GE_PATH_ROTATION_ID, sizeof(GE_PATH_ROTATION_ID)-1) != 0)
		EXIT_ERROR

	if(sscanf(line + sizeof(GE_PATH_ROTATION_ID)-1, "%d", &flag) != 1)
		EXIT_ERROR
	if (flag!=GE_FALSE)
		{
			if(!gePath_ReadChannel_F0_(GE_PATH_ROTATION_CHANNEL, &P->Rotation, pFile))
				EXIT_ERROR
		}
	if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		EXIT_ERROR
	if(strnicmp(line, GE_PATH_TRANSLATION_ID, sizeof(GE_PATH_TRANSLATION_ID)-1) != 0)
		EXIT_ERROR
	if(sscanf(line + sizeof(GE_PATH_TRANSLATION_ID)-1, "%d", &flag) != 1)
		EXIT_ERROR
					
	if (flag!=GE_FALSE)
		if(!gePath_ReadChannel_F0_(GE_PATH_TRANSLATION_CHANNEL, &P->Translation, pFile))
			EXIT_ERROR
	
	P->Dirty = FLAG_DIRTY;
	return P;
}

GENESISAPI gePath* GENESISCC gePath_CreateFromFile(geVFile* pFile)
{
	uint32 u, v, flag;
	int Interp,Loop;
	gePath* P;
	#define LINE_LENGTH 256
	char line[LINE_LENGTH];

	assert( pFile != NULL );
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
		return NULL;
	}

	if ( (u>>16) == GE_PATH_BINARY_FILE_VERSION)
		return gePath_CreateFromBinaryFile(pFile,u);
		
		
	if(u != GE_PATH_ASCII_FILE_TYPE)
		{
			geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
			return NULL;
		}
		
	// Read the version.
	if	(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		{
			geErrorLog_Add( ERR_PATH_FILE_READ , NULL);
			return NULL;
		}
	if	(sscanf(line, "%X.%X\n", &u, &v) != 2)
		{
			geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
			return NULL;
		}
	v |= (u << 8);
	if(v == GE_PATH_FILE_VERSION0)
		return gePath_CreateFromFile_F0_(pFile);
	if (! ((v == GE_PATH_FILE_VERSION1) || (v== GE_PATH_FILE_VERSION)) )
		{
			geErrorLog_Add( ERR_PATH_FILE_PARSE , NULL);
			return NULL;
		}

	P = gePath_Create(0, 0, GE_FALSE);
	if( P == NULL )
		{
			return NULL;	// error logged already in gePath_Create
		}

	if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		EXIT_ERROR
	if(strnicmp(line, GE_PATH_ROTATION_ID, sizeof(GE_PATH_ROTATION_ID)-1) != 0)
		EXIT_ERROR
	if (v==GE_PATH_FILE_VERSION1)
		{
			P->Rotation.InterpolationType    = 0;
			if(sscanf(line + sizeof(GE_PATH_ROTATION_ID)-1, "%d", &flag) != 1)
				EXIT_ERROR
		}
	else
		{
			if (v==GE_PATH_FILE_VERSION)
				{
					if(sscanf(line + sizeof(GE_PATH_ROTATION_ID)-1, "%d %d", &flag, &(P->Rotation.InterpolationType)) != 2)
						EXIT_ERROR
				}
			else
				EXIT_ERROR
		}
	if (flag!=GE_FALSE)
		{
			P->Rotation.KeyList = geQKFrame_CreateFromFile(pFile,&Interp,&Loop);
			if (P->Rotation.KeyList == NULL)
				EXIT_ERROR
			P->Rotation.InterpolationType = gePath_QKToPathInterpolation(Interp);
			if (Loop)
				P->Looped = FLAG_LOOPED;
		}

	if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
		EXIT_ERROR
	if(strnicmp(line, GE_PATH_TRANSLATION_ID, sizeof(GE_PATH_TRANSLATION_ID)-1) != 0)
		EXIT_ERROR
	if (v==GE_PATH_FILE_VERSION1)
		{
			P->Translation.InterpolationType = 0;
			if(sscanf(line + sizeof(GE_PATH_TRANSLATION_ID)-1, "%d", &flag) != 1)
				EXIT_ERROR
		}
	else
		{
			if (v==GE_PATH_FILE_VERSION)
				{
					if(sscanf(line + sizeof(GE_PATH_TRANSLATION_ID)-1, "%d %d", &flag, &(P->Translation.InterpolationType)) != 2)
						EXIT_ERROR
				}
			else
				EXIT_ERROR
		}
					
	if (flag!=GE_FALSE)
		{
			P->Translation.KeyList = geVKFrame_CreateFromFile(pFile,&Interp,&Loop);
			if (P->Translation.KeyList == NULL)
				EXIT_ERROR
			P->Translation.InterpolationType = gePath_VKToPathInterpolation(Interp);
			if (Loop)
				P->Looped = FLAG_LOOPED;
		}

	P->Dirty = FLAG_DIRTY;
	return P;
}

GENESISAPI geBoolean GENESISCC gePath_WriteToFile(const gePath *P,geVFile *pFile)
{
	uint32 u;
	int Looped=0;

	assert( P != NULL );
	assert( pFile != NULL );

	if (P->Dirty)
		gePath_Recompute((gePath *)P);

	// Write the format flag
	u = GE_PATH_ASCII_FILE_TYPE;
	if(geVFile_Write(pFile, &u,sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_PATH_FILE_WRITE , NULL);
		return GE_FALSE;
	}

	// Write the version
	if	(geVFile_Printf(pFile, " %X.%.2X\n", (GE_PATH_FILE_VERSION & 0xFF00) >> 8, 
									GE_PATH_FILE_VERSION & 0x00FF) == GE_FALSE)
		{
			geErrorLog_Add( ERR_PATH_FILE_WRITE , NULL);
			return GE_FALSE;
		}

	{
		int flag;
		flag = GE_FALSE;

		if (P->Rotation.KeyList != NULL)
			{
				if (geTKArray_NumElements(P->Rotation.KeyList)>0)
					{
						flag = GE_TRUE;
					}
			}
		if	(geVFile_Printf(pFile,
						  "%s %d %d\n",
						  GE_PATH_ROTATION_ID,
						  flag,
						  P->Rotation.InterpolationType) == GE_FALSE)
			{
				geErrorLog_Add( ERR_PATH_FILE_WRITE , NULL);
				return GE_FALSE;
			}

		if (P->Looped)
			Looped = 1;

		if (flag!=GE_FALSE)
			if (geQKFrame_WriteToFile( pFile, P->Rotation.KeyList, 
										gePath_PathToQKInterpolation(P->Rotation.InterpolationType),
										Looped)==GE_FALSE)
				{
					return GE_FALSE;
				}
		
		flag = GE_FALSE;
		if (P->Translation.KeyList != NULL)
			{
				if (geTKArray_NumElements(P->Translation.KeyList)>0)
					{
						flag = GE_TRUE;
					}
			}
		if	(geVFile_Printf(pFile,
						  "%s %d %d\n",
						  GE_PATH_TRANSLATION_ID,
						  flag,
						  P->Translation.InterpolationType) == GE_FALSE)
			{
				geErrorLog_Add( ERR_PATH_FILE_WRITE , NULL);
				return GE_FALSE;
			}

		if (flag!=GE_FALSE)
			if (geVKFrame_WriteToFile( pFile, P->Translation.KeyList, 
										gePath_PathToVKInterpolation(P->Translation.InterpolationType),
										Looped)==GE_FALSE)
				{
					return GE_FALSE;
				}
	}
		
			
	return GE_TRUE;
}

/*
	Binary file header:
	 13 bit version id, 8 bit Rotation InterpolationType,
	 8 bit Translation InterpolationType, 1 bits for looped flag,
	 1 bit for translation keys exist, 1 bit for rotation keys exist
*/
#define GE_PATH_MAX_INT_TYPE_COUNT      (127)		// 7 bits 
#define GE_PATH_TRANS_SHIFT_INTO_HEADER (9)			// 7 bits shifted into bits 9..15
#define GE_PATH_ROT_SHIFT_INTO_HEADER   (2)			// 7 bits shifted into bits 2..8

GENESISAPI geBoolean GENESISCC gePath_WriteToBinaryFile(const gePath *P, geVFile *F)
{
	uint32 Header;
	int R,T,Looped;

	assert( F != NULL );
	assert( P != NULL );
	assert( GE_PATH_BINARY_FILE_VERSION < 0xFFFF );

	R=T=0;

	if (P->Rotation.KeyList != NULL)
		{
			if (geTKArray_NumElements(P->Rotation.KeyList)>0)
				{
					R = GE_TRUE;
				}
		}
				
	if (P->Translation.KeyList != NULL)
		{
			if (geTKArray_NumElements(P->Translation.KeyList)>0)
				{
					T = GE_TRUE;
				}
		}

	if (P->Looped)
		Looped = 1;
	else
		Looped = 0;
	assert( P->Translation.InterpolationType <= GE_PATH_MAX_INT_TYPE_COUNT);	
	assert( P->Rotation.InterpolationType <= GE_PATH_MAX_INT_TYPE_COUNT);		

	Header = 
		(GE_PATH_BINARY_FILE_VERSION << 16) |
		(T<<1)  | 
		(R) 	| 
		(P->Translation.InterpolationType << GE_PATH_TRANS_SHIFT_INTO_HEADER) | 
		(P->Rotation.InterpolationType    << GE_PATH_ROT_SHIFT_INTO_HEADER  );

	if	(geVFile_Write(F, &Header,sizeof(uint32)) == GE_FALSE)
		{
			geErrorLog_AddString( -1 ,"Failure to write Path Binary File Header", NULL);
			return GE_FALSE;
		}

	if (T==1)
		{
			if (geVKFrame_WriteToBinaryFile( F, P->Translation.KeyList, 
										gePath_PathToVKInterpolation(P->Translation.InterpolationType),
										Looped)==GE_FALSE)
				{
					geErrorLog_AddString( -1 ,"Failure to write Path data", NULL);
					return GE_FALSE;
				}
		}
	if (R==1)
		{
			if (geQKFrame_WriteToBinaryFile( F, P->Rotation.KeyList, 
										gePath_PathToQKInterpolation(P->Rotation.InterpolationType),
										Looped)==GE_FALSE)
				{
					geErrorLog_AddString( -1 ,"Failure to write Path data", NULL);
					return GE_FALSE;
				}
		}
	
	return GE_TRUE;
}



static gePath * GENESISCC gePath_CreateFromBinaryFile(geVFile *F,uint32 Header)
{
	gePath *P;
	int Interp,Looping;

	assert( F != NULL );

	if ((Header>>16) != GE_PATH_BINARY_FILE_VERSION)
		{
			geErrorLog_AddString( -1, "Bad path binary file version" , NULL);
			return NULL;
		}

	P = geRam_Allocate(sizeof(gePath));
	P->Translation.KeyList = NULL;
	P->Rotation.KeyList = NULL;
	if (P == NULL)
		{
			geErrorLog_AddString( -1, "Failure to allocate memory for path" , NULL);
			return NULL;
		}
	
	P->Translation.InterpolationType = (Header >> GE_PATH_TRANS_SHIFT_INTO_HEADER) & GE_PATH_MAX_INT_TYPE_COUNT;
	P->Rotation.InterpolationType    = (Header >> GE_PATH_ROT_SHIFT_INTO_HEADER) & GE_PATH_MAX_INT_TYPE_COUNT;
	// this will be replaced by the path reader (if the path has keys)

	P->Translation.LastKey1Time = 0.0f;
	P->Translation.LastKey2Time = -1.0f;

	P->Rotation.LastKey1Time = 0.0f;
	P->Rotation.LastKey2Time = -1.0f;
	P-> Dirty    = 0;
	P-> Looped   = 0;
	P-> RefCount = 0;

	if ((Header >> 1) & 0x1)
		{
			P->Translation.KeyList = geVKFrame_CreateFromBinaryFile(F,&Interp,&Looping);
			if (P->Translation.KeyList == NULL)
				{
					geErrorLog_AddString( -1, "Failure to read translation keys" , NULL);
					geRam_Free(P);
					return NULL;
				}
			P->Translation.InterpolationType = gePath_VKToPathInterpolation(Interp);
			if( Looping != 0 )
				P->Looped = FLAG_LOOPED;
		}

	if (Header & 0x1)
		{
			P->Rotation.KeyList = geQKFrame_CreateFromBinaryFile(F,&Interp,&Looping);
			if (P->Rotation.KeyList == NULL)
				{
					geErrorLog_AddString( -1, "Failure to read rotation keys" , NULL);
					if (P->Translation.KeyList != NULL)
						{
							geTKArray_Destroy(&P->Translation.KeyList);
						}
					geRam_Free(P);
					return NULL;
				}
			P->Rotation.InterpolationType = gePath_QKToPathInterpolation(Interp);
			if( Looping != 0 )
				P->Looped = FLAG_LOOPED;

		}
	P->Dirty = FLAG_DIRTY;
	return P;
}
		