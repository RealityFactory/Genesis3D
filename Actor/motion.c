/****************************************************************************************/
/*  MOTION.C	                                                                        */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Motion implementation.				                                    */
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
/*	motion.c

	This object is a list of (named) gePath objects, 
	and an associated event list 

*/
 
#include <assert.h>
#include <string.h>		// strcmp, strnicmp

#include "basetype.h"
#include "ram.h"
#include "errorlog.h"
#include "motion.h"
#include "tkevents.h"
#include "StrBlock.h"

#pragma warning(disable : 4201)		// we're using nameless structures

#define gePath_TimeType geFloat

#define MIN(aa,bb)  (( (aa)>(bb) ) ? (bb) : (aa) )
#define MAX(aa,bb)  (( (aa)>(bb) ) ? (aa) : (bb) )

typedef enum { MOTION_NODE_UNDECIDED, MOTION_NODE_BRANCH, MOTION_NODE_LEAF } geMotion_NodeType;

#define MOTION_BLEND_PART_OF_TRANSFORM(TForm)  ((TForm).Translation.X)						
#define MOTION_BLEND_PART_OF_VECTOR(Vec)  ((Vec).X)						


typedef struct geMotion_Leaf
{
	int			PathCount;		
	int32		NameChecksum;	// checksum based on names and list order
	geTKEvents *Events;
	geStrBlock *NameArray;
	gePath	  **PathArray;
} geMotion_Leaf;


typedef struct geMotion_Mixer
{
	geFloat   TimeScale;		// multipler for time
	geFloat   TimeOffset;		// already scaled.
	gePath   *Blend;			// path used to interpolate blending amounts. 
	geXForm3d Transform;		// base transform for this motion (if TransformUsed==GE_TRUE)
	geBoolean TransformUsed;	// GE_FALSE if there is no base transform.
	geMotion *Motion;			
} geMotion_Mixer;

typedef struct geMotion_Branch
{
	int				MixerCount;
	int				CurrentEventIterator;
	geMotion_Mixer *MixerArray;
} geMotion_Branch;


typedef struct geMotion
{
	char			 *Name;
	int				  CloneCount;
	geBoolean		  MaintainNames;		
	geMotion_NodeType NodeType;
	union 
		{
			geMotion_Leaf   Leaf;
			geMotion_Branch Branch;
		};
	geMotion *SanityCheck;
} geMotion;


GENESISAPI geBoolean GENESISCC geMotion_IsValid(const geMotion *M)
{
	if (M == NULL)
		return GE_FALSE;
	if (M->SanityCheck!=M)
		return GE_FALSE;
	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geMotion_SetName(geMotion *M, const char *Name)
{
	char *NewName;

	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	NewName = geRam_Allocate( strlen(Name)+1 );
	if (NewName == NULL )
		{
			geErrorLog_Add(ERR_MOTION_CREATE_ENOMEM, NULL);
			return GE_FALSE;
		}
	if (M->Name!=NULL)
		{
			geRam_Free(M->Name);
		}
	M->Name = NewName;
	strcpy(M->Name, Name);
	return GE_TRUE;
}

GENESISAPI const char * GENESISCC geMotion_GetName(const geMotion *M)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	return M->Name;
}
		
static geBoolean GENESISCC geMotion_InitNodeAsLeaf(geMotion *M,geBoolean SetupStringBlock)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( M->NodeType == MOTION_NODE_UNDECIDED );

	M->NodeType = MOTION_NODE_LEAF;
	
	M->Leaf.PathCount     = 0;
	M->Leaf.Events        = NULL;
	M->Leaf.PathArray     = NULL;
	M->Leaf.NameChecksum  = 0;
	if ((M->MaintainNames != GE_FALSE) && (SetupStringBlock!=GE_FALSE))
		{
			M->Leaf.NameArray = geStrBlock_Create();
			if (M->Leaf.NameArray == NULL)	
				{
					geErrorLog_Add(ERR_MOTION_CREATE_ENOMEM, NULL);
					return GE_FALSE;
				}
		}
	else
		{
			M->Leaf.NameArray  = NULL;
		}
	return GE_TRUE;
}

static geBoolean GENESISCC geMotion_InitNodeAsBranch(geMotion *M)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( M->NodeType == MOTION_NODE_UNDECIDED );

	M->NodeType = MOTION_NODE_BRANCH;
	
	M->Branch.MixerCount           = 0;
	M->Branch.CurrentEventIterator = 0;
	M->Branch.MixerArray           = NULL;
	return GE_TRUE;
}


GENESISAPI geMotion * GENESISCC geMotion_Create(geBoolean WithNames)
{
	geMotion *M;
	assert( (WithNames==GE_TRUE) || (WithNames==GE_FALSE) );

	M = GE_RAM_ALLOCATE_STRUCT(geMotion);

	if ( M == NULL )
		{
			geErrorLog_Add(ERR_MOTION_CREATE_ENOMEM, NULL);
			return NULL;
		}

	M->Name          = NULL;
	M->CloneCount	 = 0;
	M->MaintainNames = WithNames;
	M->NodeType      = MOTION_NODE_UNDECIDED;
	M->SanityCheck   = M;
	return M;
}


GENESISAPI geBoolean GENESISCC geMotion_RemoveNames(geMotion *M)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->CloneCount > 0)
		{
			geErrorLog_AddString(-1,"Can't remove names from a cloned motion.", NULL);
			return GE_FALSE;
		}

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				break;
			case (MOTION_NODE_BRANCH):
				geErrorLog_AddString(-1,"Can't remove names from a compound motion.", NULL);
				return GE_FALSE;
				break;
			case (MOTION_NODE_LEAF):
				assert( M->Leaf.PathCount >= 0 );
				
				if ( M->Leaf.NameArray != NULL )
					{	
						geStrBlock_Destroy(&(M->Leaf.NameArray));
					}
				M->Leaf.NameArray = NULL;
				break;
			default:
				assert(0);
		}

	M->MaintainNames = GE_FALSE;
	return GE_TRUE;
}



GENESISAPI void GENESISCC geMotion_Destroy(geMotion **PM)
{
	int i;
	geMotion *M;
	
	assert(PM   != NULL );
	assert(*PM  != NULL );
	M = *PM;
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->CloneCount > 0 )
		{
			M->CloneCount--;
			return;
		}

	if (M->Name != NULL)
		{
			geRam_Free(M->Name);
			M->Name = NULL;
		}

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				break;
			case (MOTION_NODE_BRANCH):
				for (i=0; i<M->Branch.MixerCount; i++)
					{
						assert( M->Branch.MixerArray[i].Motion != NULL );
						geMotion_Destroy( &(M->Branch.MixerArray[i].Motion));
						M->Branch.MixerArray[i].Motion = NULL;

						if (M->Branch.MixerArray[i].Blend != NULL )
							{
								gePath_Destroy( &(M->Branch.MixerArray[i].Blend));
								M->Branch.MixerArray[i].Blend = NULL;
							}
						
					}
				if (M->Branch.MixerArray != NULL)
					{
						geRam_Free(M->Branch.MixerArray);
						M->Branch.MixerArray = NULL;
					}
				M->Branch.MixerCount = 0;
				M->Branch.CurrentEventIterator = 0;
				break;
			case (MOTION_NODE_LEAF):
				if (M->MaintainNames == GE_TRUE)
					{	
						geBoolean Test=	geMotion_RemoveNames(M);
						assert( Test != GE_FALSE );
						Test;
					}
				for (i=0; i< M->Leaf.PathCount; i++)
					{
						assert( M->Leaf.PathArray[i] );
						gePath_Destroy( &( M->Leaf.PathArray[i] ) );
						M->Leaf.PathArray[i] = NULL;
					}
				if (M->Leaf.PathArray!=NULL)
					{
						geRam_Free(M->Leaf.PathArray);
						M->Leaf.PathArray = NULL;
					}
				M->Leaf.PathCount = 0;
				if ( M->Leaf.Events != NULL )
					{
						geTKEvents_Destroy( &(M->Leaf.Events) );
					}
				break;
			default:
				assert(0);
		}
	M->NodeType = MOTION_NODE_UNDECIDED;
	geRam_Free( *PM );
	*PM = NULL;
}

GENESISAPI geBoolean GENESISCC geMotion_AddPath(geMotion *M,
	gePath *P,const char *Name,int *PathIndex)
{
	int PathCount;
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				if (geMotion_InitNodeAsLeaf(M,GE_TRUE)==GE_FALSE)
					{
						geErrorLog_Add(-1, NULL);
						return GE_FALSE;
					}
				break;
			case (MOTION_NODE_BRANCH):
				geErrorLog_Add(ERR_MOTION_ADDPATH_ENOMEM, NULL);	//FIXME!
				return GE_FALSE;
			case (MOTION_NODE_LEAF):
				break;
			default:
				assert(0);
		}

	assert( M->Leaf.PathCount >= 0 );

	if (Name!=NULL)
		{
			if (geMotion_GetPathNamed( M, Name) != NULL )
				{
					geErrorLog_Add(ERR_MOTION_ADDPATH_BAD_NAME, NULL);
					return GE_FALSE;
				}
		}

	PathCount = M->Leaf.PathCount;

	{
		gePath **NewPathArray;

		NewPathArray = geRam_Realloc(M->Leaf.PathArray, (1+PathCount) * sizeof(gePath*) );

		if ( NewPathArray == NULL )
			{	
				geErrorLog_Add(ERR_MOTION_ADDPATH_ENOMEM, NULL);
				return GE_FALSE;
			}
		M->Leaf.PathArray = NewPathArray;
	}

	M->Leaf.PathArray[PathCount] = P;

	if ( M->MaintainNames == GE_TRUE )
		{
			
			assert (M->Leaf.NameArray != NULL);
			if (geStrBlock_Append(&(M->Leaf.NameArray),Name)==GE_FALSE)
				{
					geErrorLog_Add(ERR_MOTION_ADDPATH_ENOMEM, NULL);
					assert(M->Leaf.PathArray[PathCount]);
					gePath_Destroy(&(M->Leaf.PathArray[PathCount]));
					return GE_FALSE;
				}
			M->Leaf.NameChecksum = geStrBlock_GetChecksum(M->Leaf.NameArray);
		}						
															
	M->Leaf.PathCount = PathCount+1;
	*PathIndex = PathCount;
	gePath_CreateRef(P);
	return GE_TRUE;
}


// returns 0 if there is no name information... or if children don't all share the same checksum.
GENESISAPI int32 GENESISCC geMotion_GetNameChecksum(const geMotion *M)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				return 0;
			case (MOTION_NODE_BRANCH):
				{
					int i;
					int32 Checksum,FirstChecksum;
					if (M->Branch.MixerCount<1)
						return 0;
					assert( M->Branch.MixerArray[0].Motion );
					FirstChecksum = geMotion_GetNameChecksum( M->Branch.MixerArray[0].Motion );
					
					for (i=1; i<M->Branch.MixerCount; i++)
						{
							assert( M->Branch.MixerArray[i].Motion );
							Checksum = geMotion_GetNameChecksum( M->Branch.MixerArray[i].Motion );
							if (Checksum != FirstChecksum)
								return 0;
						}
					return FirstChecksum;
				}
			case (MOTION_NODE_LEAF):
				return M->Leaf.NameChecksum;
			default:
				assert(0);
		}
	return 0;
}	

GENESISAPI geBoolean GENESISCC geMotion_HasNames(const geMotion *M)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( (M->MaintainNames == GE_TRUE) || (M->MaintainNames == GE_FALSE) );
	// if M has names, all children of M have names. 
	return M->MaintainNames;
}

GENESISAPI gePath * GENESISCC geMotion_GetPathNamed(const geMotion *M,const char *Name)
{
	int i;

	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	
	if (M->NodeType != MOTION_NODE_LEAF)
		{	// not an error condition.
			return NULL;
		}
			
	assert( M->Leaf.PathCount >=0 );
	
	if (Name != NULL)	
		{
			if ( M->MaintainNames == GE_TRUE )
				{
					for (i=0; i<M->Leaf.PathCount; i++)
						{
							if ( strcmp(Name,geStrBlock_GetString(M->Leaf.NameArray,i))==0 )
								{
									return M->Leaf.PathArray[i];
								}
						}
				}
		}
	return NULL;
}
			

#define LINEAR_BLEND(a,b,t)  ( (t)*((b)-(a)) + (a) )	
			// linear blend of a and b  0<t<1 where  t=0 ->a and t=1 ->b



GENESISAPI void GENESISCC geMotion_Sample(const geMotion *M, int PathIndex, gePath_TimeType Time, geXForm3d *Transform)
{
	geQuaternion Rotation;
	geVec3d		 Translation;
	assert( M           != NULL);
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( Transform   != NULL );

	geMotion_SampleChannels(M,PathIndex,Time,&Rotation,&Translation);
	geQuaternion_ToMatrix(&Rotation,Transform);
	Transform->Translation = Translation;
}


GENESISAPI void GENESISCC geMotion_SampleChannels(const geMotion *M, int PathIndex, gePath_TimeType Time, geQuaternion *Rotation, geVec3d *Translation)
{
	assert( M           != NULL);
	assert( Rotation    != NULL );
	assert( Translation != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				assert(0);
				break;
			case (MOTION_NODE_BRANCH):
				{
					geQuaternion R;
					geVec3d      T;
					geMotion_Mixer *Mixer;
					int i;

					if ( M->Branch.MixerCount == 0 )
						{
							geVec3d_Clear(Translation);
							geQuaternion_SetNoRotation(Rotation);
							return;
						}

					assert( M->Branch.MixerCount > 0);
					Mixer = &(M->Branch.MixerArray[0]);
					
					assert(Mixer->Motion != NULL );
					geMotion_SampleChannels(Mixer->Motion,PathIndex,
											(Time - Mixer->TimeOffset) * Mixer->TimeScale,
											Rotation,Translation);
				
					for (i=1; i<M->Branch.MixerCount; i++)
						{
							geFloat BlendAmount;
							geFloat MixTime;

							Mixer = &(M->Branch.MixerArray[i]);

							assert( Mixer->Motion != NULL );
							assert( Mixer->Blend  != NULL );

							MixTime = (Time - Mixer->TimeOffset) * Mixer->TimeScale;

							geMotion_SampleChannels(Mixer->Motion,PathIndex,MixTime,&R,&T);
							{
								geVec3d BlendVector;
								geQuaternion Dummy;
								gePath_SampleChannels(Mixer->Blend,MixTime,&Dummy,&BlendVector);
								BlendAmount = MOTION_BLEND_PART_OF_VECTOR(BlendVector);
							}
							geQuaternion_Slerp(Rotation,&R,BlendAmount,Rotation);
							Translation->X = LINEAR_BLEND(Translation->X,T.X,BlendAmount);
							Translation->Y = LINEAR_BLEND(Translation->Y,T.Y,BlendAmount);
							Translation->Z = LINEAR_BLEND(Translation->Z,T.Z,BlendAmount);
						}
				}
				break;
			case (MOTION_NODE_LEAF):
				{
					gePath *P;
					assert( ( PathIndex >=0 ) && ( PathIndex < M->Leaf.PathCount ) );
					P= M->Leaf.PathArray[PathIndex];
					assert( P != NULL );
					gePath_SampleChannels(P,Time,Rotation,Translation);
				}
				break;
			default:
				assert(0);
		}
}		

GENESISAPI geBoolean GENESISCC geMotion_SampleNamed(const geMotion *M, const char *PathName, gePath_TimeType Time, geXForm3d *Transform)
{
	geQuaternion Rotation;
	geVec3d		 Translation;
	assert( M           != NULL);
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( Transform   != NULL );

	if (geMotion_SampleChannelsNamed(M,PathName,Time,&Rotation,&Translation)==GE_FALSE)
		{
			return GE_FALSE;
		}

	geQuaternion_ToMatrix(&Rotation,Transform);
	Transform->Translation = Translation;
	return GE_TRUE;
}



GENESISAPI geBoolean GENESISCC geMotion_SampleChannelsNamed(const geMotion *M, const char *PathName, gePath_TimeType Time, geQuaternion *Rotation, geVec3d *Translation)
{
	geBoolean AnyChannels=GE_FALSE;
	assert( M           != NULL);
	assert( Rotation    != NULL );
	assert( Translation != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				return GE_FALSE;
				break;
			case (MOTION_NODE_BRANCH):
				{
					int i;
					geQuaternion R;
					geVec3d T;
					geMotion_Mixer *Mixer;

					if ( M->Branch.MixerCount == 0 )
						{
							geVec3d_Clear(Translation);
							geQuaternion_SetNoRotation(Rotation);
							return GE_TRUE;
						}

					assert( M->Branch.MixerCount > 0 );

					for (i=0; i<M->Branch.MixerCount; i++)
						{
							geFloat BlendAmount;
							geFloat MixTime;

							Mixer = &(M->Branch.MixerArray[i]);

							assert( Mixer->Motion != NULL );
							assert( Mixer->Blend  != NULL );

							MixTime = (Time - Mixer->TimeOffset) * Mixer->TimeScale;

							// hmm. is BlendAmount still good if there is no path?
							if ( geMotion_SampleChannelsNamed(Mixer->Motion,PathName,MixTime,&R,&T)
								 != GE_FALSE )
								{
									if (AnyChannels != GE_FALSE)
										{
											{
												geVec3d BlendVector;
												geQuaternion Dummy;
												gePath_SampleChannels(Mixer->Blend,MixTime,&Dummy,&BlendVector);
												BlendAmount = MOTION_BLEND_PART_OF_VECTOR(BlendVector);
											}
											geQuaternion_Slerp(Rotation,&R,BlendAmount,Rotation);
											Translation->X = LINEAR_BLEND(Translation->X,T.X,BlendAmount);
											Translation->Y = LINEAR_BLEND(Translation->Y,T.Y,BlendAmount);
											Translation->Z = LINEAR_BLEND(Translation->Z,T.Z,BlendAmount);
										}
									else
										{
											*Rotation = R;
											*Translation = T;
											AnyChannels = GE_TRUE;
										}
								}
						}
				}
				break;
			case (MOTION_NODE_LEAF):
				{
					gePath *P;
					P = geMotion_GetPathNamed(M, PathName);
					if (P == NULL)
						{
							return GE_FALSE;
						}
					gePath_SampleChannels(P,Time,Rotation,Translation);
					AnyChannels = GE_TRUE;
				}
				break;
			default:
				assert(0);
		}
	return AnyChannels;
}		


GENESISAPI gePath * GENESISCC geMotion_GetPath(const geMotion *M,int Index)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
 	
	if (M->NodeType != MOTION_NODE_LEAF)
		{	// not an error condition.
			return NULL;
		}
			
	assert( M->Leaf.PathCount >=0 );
	assert( Index <= M->Leaf.PathCount );
	assert( Index >= 0 );

	return M->Leaf.PathArray[Index];
}

GENESISAPI const char * GENESISCC geMotion_GetNameOfPath(const geMotion *M, int Index)
{
	gePath *P;
	assert( M != NULL );

	if (M->NodeType!=MOTION_NODE_LEAF)
		{
			return NULL;
		}
	if (geMotion_HasNames(M)==GE_FALSE)
		{
			return NULL;
		}

	P = geMotion_GetPath(M,Index);
	if (P==NULL)
		{
			return NULL;
		}
	assert( M->Leaf.NameArray!=NULL );

	return geStrBlock_GetString(M->Leaf.NameArray,Index);

}
			
	

GENESISAPI int GENESISCC geMotion_GetPathCount(const geMotion *M)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->NodeType != MOTION_NODE_LEAF)
		{	// not an error condition.
			return 0;
		}
	assert( M->Leaf.PathCount >=0 );
	return M->Leaf.PathCount;
}


GENESISAPI geBoolean GENESISCC geMotion_GetTimeExtents(const geMotion *M,gePath_TimeType *StartTime,gePath_TimeType *EndTime)
{
	int i,found;
	gePath_TimeType Start,End;
	assert( M != NULL );
	assert( StartTime != NULL );
	assert( EndTime != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	found = 0;

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				break;
			case (MOTION_NODE_BRANCH):
				for (i=0; i<M->Branch.MixerCount; i++)
					{
						if (geMotion_GetTimeExtents(M->Branch.MixerArray[i].Motion,&Start,&End)!=GE_FALSE)
							{
								found++;

								// Assertions in AddSubMotion and SetTimeScale prevent TimeScale from being 0.
								End = M->Branch.MixerArray[i].TimeOffset + ((End - Start) / M->Branch.MixerArray[i].TimeScale);
								Start += M->Branch.MixerArray[i].TimeOffset;

								// If time scale is negative, then End will be < Start, which violates
								// the entire idea of extents.  So we'll swap them.
								if (End < Start)
								{
									geFloat Temp = Start;
									Start = End;
									End = Temp;
								}
								if (found==1)
									{
										*StartTime = Start;
										*EndTime   = End;
									}
								else
									{	//found>1
										*StartTime = MIN(*StartTime,Start);
										*EndTime   = MAX(*EndTime,End);
									}
							}
					}										
				break;			
			case (MOTION_NODE_LEAF):
				found = 0;
				for (i=0; i<M->Leaf.PathCount; i++)
					{
						if (gePath_GetTimeExtents(M->Leaf.PathArray[i],&Start,&End)!=GE_FALSE)
							{
								found++;
								if (found==1)
									{
										*StartTime = Start;
										*EndTime   = End;
									}
								else
									{	//found>1
										*StartTime = MIN(*StartTime,Start);
										*EndTime   = MAX(*EndTime,End);
									}
							}
					}
				break;
			default:
				assert(0);
		}
	if (found>0)
		{
			return GE_TRUE;
		}
	return GE_FALSE;
}			


GENESISAPI int GENESISCC geMotion_GetSubMotionCount(const geMotion *M)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			return M->Branch.MixerCount;
		}
	return 0;
}


#pragma message ("do we want to copy these before returning them?")
GENESISAPI geMotion * GENESISCC geMotion_GetSubMotion(const geMotion *M,int SubMotionIndex)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	if (M->NodeType != MOTION_NODE_BRANCH )
		{
			return NULL;
		}
	assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
	assert( M->Branch.MixerArray != NULL );

	return M->Branch.MixerArray[SubMotionIndex].Motion;
}

GENESISAPI geMotion * GENESISCC geMotion_GetSubMotionNamed(const geMotion *M,const char *Name)
{
	int i;
	assert( M != NULL);	
	assert( Name != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->NodeType != MOTION_NODE_BRANCH)
		{
			return NULL;
		}
	assert( M->Branch.MixerArray != NULL );
	for (i=0; i<M->Branch.MixerCount; i++)
		{
			geMotion *MI = M->Branch.MixerArray[i].Motion;
			assert( MI != NULL );
			if (MI->Name!=NULL)
				{
					if (strcmp(MI->Name,Name)==0)
						{
							return MI;
						}
				}
		}
	return NULL;
}

static geBoolean GENESISCC geMotion_SearchForSubMotion(const geMotion *Parent, const geMotion*Child)
{
	int i;
	assert( Parent != NULL );
	assert( Child  != NULL );
	assert( geMotion_IsValid(Parent) != GE_FALSE );
	assert( geMotion_IsValid(Child) != GE_FALSE );

	if (Parent == Child)
		return GE_TRUE;

	if (Parent->NodeType != MOTION_NODE_BRANCH)
		return GE_FALSE;

	assert( Parent->Branch.MixerArray != NULL );

	for (i=0; i<Parent->Branch.MixerCount; i++)
		{
			assert( Parent->Branch.MixerArray[i].Motion != NULL );
			if (geMotion_SearchForSubMotion(Parent->Branch.MixerArray[i].Motion,Child)==GE_TRUE)
				return GE_TRUE;
		}
	return GE_FALSE;
}

GENESISAPI geBoolean GENESISCC geMotion_AddSubMotion(geMotion *ParentMotion, 
								geFloat TimeScale, 
								geFloat TimeOffset,
								geMotion *SubMotion, 
								geFloat StartTime, geFloat StartMagnitude,
								geFloat EndTime,   geFloat EndMagnitude,
								const geXForm3d *Transform,
								int *Index)

{

	int Count;
	geMotion_Mixer *NewMixerArray;
	assert( ParentMotion != NULL );
	assert( TimeScale	 != 0.0f );
	assert( SubMotion    != NULL );
	assert( Index        != NULL );
	//assert( Transform    != NULL );
	assert( ( StartMagnitude >= 0.0f) && ( StartMagnitude <=1.0f ));
	assert( ( EndMagnitude   >= 0.0f) && ( EndMagnitude   <=1.0f ));
	assert( geMotion_IsValid(ParentMotion) != GE_FALSE );
	assert( geMotion_IsValid(SubMotion) != GE_FALSE );

	switch (ParentMotion->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				if (geMotion_InitNodeAsBranch(ParentMotion)==GE_FALSE)
					{
						geErrorLog_Add(-1, NULL);
						return GE_FALSE;
					}
				break;
			case (MOTION_NODE_LEAF):
				{
					geErrorLog_Add(-1, NULL);
					return GE_FALSE;
				}
			case (MOTION_NODE_BRANCH):
				break;
			default:
				assert(0);
		}

	if (ParentMotion->MaintainNames != SubMotion->MaintainNames)
		{
			geErrorLog_Add(-1, NULL);  //?
			return GE_FALSE;
		}
		
	if (geMotion_SearchForSubMotion(SubMotion,ParentMotion)!=GE_FALSE)
		{
			geErrorLog_Add(-1, NULL);
			return GE_FALSE;
		}
			
	Count = ParentMotion->Branch.MixerCount;
	NewMixerArray = geRam_Realloc(ParentMotion->Branch.MixerArray, (1+Count) * sizeof(geMotion_Mixer) );
	if ( NewMixerArray == NULL )
		{	
			geErrorLog_Add(-1, NULL);
			return GE_FALSE;
		}
		
	ParentMotion->Branch.MixerArray = NewMixerArray;
	{
		geMotion_Mixer *Mixer;
		geXForm3d BlendKeyTransform;
		Mixer = &(ParentMotion->Branch.MixerArray[Count]);
	
		Mixer->Motion     = SubMotion;
		Mixer->TimeScale  = TimeScale;
		Mixer->TimeOffset = TimeOffset;
		
		Mixer->Blend = gePath_Create(GE_PATH_INTERPOLATE_HERMITE_ZERO_DERIV,GE_PATH_INTERPOLATE_SLERP,GE_FALSE);
		if (Mixer->Blend==NULL)
			{	
				geErrorLog_Add(-1, NULL);
				return GE_FALSE;
			}
		MOTION_BLEND_PART_OF_TRANSFORM(BlendKeyTransform) = StartMagnitude;
		if (gePath_InsertKeyframe(Mixer->Blend,
						GE_PATH_TRANSLATION_CHANNEL,StartTime,&BlendKeyTransform)==GE_FALSE)
			{
				geErrorLog_Add(-1, NULL);
				gePath_Destroy(&(Mixer->Blend));
				return GE_FALSE;
			}

		MOTION_BLEND_PART_OF_TRANSFORM(BlendKeyTransform) = EndMagnitude;
		if (gePath_InsertKeyframe(Mixer->Blend,
						GE_PATH_TRANSLATION_CHANNEL,EndTime,&BlendKeyTransform)==GE_FALSE)
			{
				geErrorLog_Add(-1, NULL);
				gePath_Destroy(&(Mixer->Blend));
				return GE_FALSE;
			}
		if (Transform == NULL)
			{
				Mixer->TransformUsed = GE_FALSE;
			}
		else
			{
				Mixer->TransformUsed = GE_TRUE;
				Mixer->Transform = *Transform;
			}
	}
	
	*Index = Count;
	SubMotion->CloneCount++;
	ParentMotion->Branch.MixerCount++;

	return GE_TRUE;
}

GENESISAPI geMotion * GENESISCC geMotion_RemoveSubMotion(geMotion *ParentMotion, int SubMotionIndex)
{
	int Count;
	geMotion *M;
	assert( ParentMotion != NULL );
	assert( geMotion_IsValid(ParentMotion) != GE_FALSE );

	if (ParentMotion->NodeType != MOTION_NODE_BRANCH)
		{
			return NULL;
		}
	
	Count = ParentMotion->Branch.MixerCount;
	assert( (SubMotionIndex>=0) && (SubMotionIndex<Count));
	
	M = ParentMotion->Branch.MixerArray[SubMotionIndex].Motion;
	assert( ParentMotion->Branch.MixerArray[SubMotionIndex].Blend != NULL );
	gePath_Destroy( &(ParentMotion->Branch.MixerArray[SubMotionIndex].Blend) );
	
	if (Count>1)
		{
			memcpy( &(ParentMotion->Branch.MixerArray[SubMotionIndex]),
					&(ParentMotion->Branch.MixerArray[SubMotionIndex+1]),
					sizeof(geMotion_Mixer) * (Count-(SubMotionIndex+1)));
		}
	ParentMotion->Branch.MixerCount--;

	{
		geMotion_Mixer *NewMixerArray;
		if (ParentMotion->Branch.MixerCount == 0)
			{
				geRam_Free(ParentMotion->Branch.MixerArray);
				ParentMotion->Branch.MixerArray = NULL;
			}
		else
			{
				NewMixerArray = geRam_Realloc(ParentMotion->Branch.MixerArray, 
									(ParentMotion->Branch.MixerCount) * sizeof(geMotion_Mixer) );
				if ( NewMixerArray != NULL )
					{	
						ParentMotion->Branch.MixerArray = NewMixerArray;
					}
			}
	}
	geMotion_Destroy( &M );
	return M;
}
	


GENESISAPI geFloat   GENESISCC geMotion_GetTimeOffset( const geMotion *M,int SubMotionIndex )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	// wrong node type is neither error nor invalid.  return value is just 0

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			return M->Branch.MixerArray[SubMotionIndex].TimeOffset;
		}
	return 0.0f;
}

GENESISAPI geBoolean  GENESISCC geMotion_SetTimeOffset( geMotion *M,int SubMotionIndex,geFloat TimeOffset )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			M->Branch.MixerArray[SubMotionIndex].TimeOffset = TimeOffset;
			return GE_TRUE;
		}
	return GE_FALSE;
}

GENESISAPI geFloat   GENESISCC geMotion_GetTimeScale( const geMotion *M,int SubMotionIndex )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	// wrong node type is neither error nor invalid.  return value is just 1

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			return M->Branch.MixerArray[SubMotionIndex].TimeScale;
		}
	return 1.0f;
}

GENESISAPI geBoolean  GENESISCC geMotion_SetTimeScale( geMotion *M,int SubMotionIndex,geFloat TimeScale )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( TimeScale != 0.0f);

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			M->Branch.MixerArray[SubMotionIndex].TimeScale = TimeScale;
			return GE_TRUE;
		}
	return GE_FALSE;
}

GENESISAPI geFloat    GENESISCC geMotion_GetBlendAmount( const geMotion *M, int SubMotionIndex, geFloat Time)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	// wrong node type is neither error nor invalid.  return value is just 0

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			geQuaternion Dummy;
			geVec3d BlendVector;
			geFloat BlendAmount;

			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			assert( M->Branch.MixerArray[SubMotionIndex].Blend != NULL );
			gePath_SampleChannels(M->Branch.MixerArray[SubMotionIndex].Blend,
								  ( Time - M->Branch.MixerArray[SubMotionIndex].TimeOffset )
								    * M->Branch.MixerArray[SubMotionIndex].TimeScale,
								   &Dummy,&BlendVector);
			BlendAmount = MOTION_BLEND_PART_OF_VECTOR(BlendVector);
			return BlendAmount;
		}
	return 0.0f;
}

GENESISAPI gePath    * GENESISCC geMotion_GetBlendPath( const geMotion *M,int SubMotionIndex )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	// wrong node type is neither error nor invalid.  return value is just NULL

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			return M->Branch.MixerArray[SubMotionIndex].Blend;
		}
	return NULL;
}

GENESISAPI geBoolean  GENESISCC geMotion_SetBlendPath( geMotion *M,int SubMotionIndex, gePath *Blend )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			gePath *P;
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			assert( Blend != NULL );
			P = M->Branch.MixerArray[SubMotionIndex].Blend;
			gePath_Destroy(&P);
			P = gePath_CreateCopy(Blend);
			if ( P == NULL )
				{
					geErrorLog_Add(-1, NULL);
					return GE_FALSE;
				}
			M->Branch.MixerArray[SubMotionIndex].Blend = P;
			return GE_TRUE;
		}
	return GE_FALSE;
}


GENESISAPI const geXForm3d * GENESISCC geMotion_GetBaseTransform( const geMotion *M,int SubMotionIndex )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	// wrong node type is neither error nor invalid.  return value is just NULL

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			if (M->Branch.MixerArray[SubMotionIndex].TransformUsed != GE_FALSE)
				{
					return &(M->Branch.MixerArray[SubMotionIndex].Transform);
				}
			else
				{
					return NULL;
				}
		}
	return NULL;
}

GENESISAPI geBoolean  GENESISCC geMotion_SetBaseTransform( geMotion *M,int SubMotionIndex, geXForm3d *BaseTransform )
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->NodeType == MOTION_NODE_BRANCH)
		{
			assert( (SubMotionIndex>=0) && (SubMotionIndex<M->Branch.MixerCount));
			assert( BaseTransform != NULL );
			if (BaseTransform!=NULL)
				{
					M->Branch.MixerArray[SubMotionIndex].Transform     = *BaseTransform;
					M->Branch.MixerArray[SubMotionIndex].TransformUsed = GE_TRUE;
				}
			else
				{
					M->Branch.MixerArray[SubMotionIndex].TransformUsed = GE_FALSE;
				}
					
			return GE_TRUE;
		}
	return GE_FALSE;
}


#pragma warning( disable : 4701)	// don't want to set Translation until we are ready
GENESISAPI geBoolean GENESISCC geMotion_GetTransform( const geMotion *M, geFloat Time, geXForm3d *Transform)
{
	assert( M         != NULL);
	assert( geMotion_IsValid(M) != GE_FALSE );

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				{
					return GE_FALSE;
				}
				break;
			case (MOTION_NODE_BRANCH):
				{
					geQuaternion R,Rotation;
					geVec3d      T,Translation;
					geMotion_Mixer *Mixer;
					geFloat MixTime;
					int i;
					int MixCount=0;

					if ( M->Branch.MixerCount == 0 )
						{
							return GE_FALSE;
						}
					assert( M->Branch.MixerCount > 0 );

					for (i=0; i<M->Branch.MixerCount; i++)
						{
							geFloat BlendAmount;
							geBoolean DoMix=GE_FALSE;

							Mixer = &(M->Branch.MixerArray[i]);

							assert( Mixer->Motion != NULL );
							assert( Mixer->Blend  != NULL );
							
							MixTime = (Time - Mixer->TimeOffset) * Mixer->TimeScale;
							if (geMotion_GetTransform(Mixer->Motion,MixTime,Transform)!=GE_FALSE)
								{
									DoMix=GE_TRUE;
									if (Mixer->TransformUsed!=GE_FALSE)
										{
											geXForm3d_Multiply(&(Mixer->Transform),Transform,Transform);
										}
								}
							else
								{
									if (Mixer->TransformUsed!=GE_FALSE)
										{
											DoMix = GE_TRUE;
											*Transform = Mixer->Transform;
										}
								}
							if (DoMix!=GE_FALSE)
								{
									if (MixCount==0)
										{
											geQuaternion_FromMatrix(Transform,&Rotation);
											Translation = Transform->Translation;
										}
									else
										{
											geQuaternion_FromMatrix(Transform,&R);
											T = Transform->Translation;
											{
												geVec3d BlendVector;
												geQuaternion Dummy;
												gePath_SampleChannels(Mixer->Blend,MixTime,&Dummy,&BlendVector);
												BlendAmount = MOTION_BLEND_PART_OF_VECTOR(BlendVector);
											}
											geQuaternion_Slerp(&Rotation,&R,BlendAmount,&Rotation);
											Translation.X = LINEAR_BLEND(Translation.X,T.X,BlendAmount);
											Translation.Y = LINEAR_BLEND(Translation.Y,T.Y,BlendAmount);
											Translation.Z = LINEAR_BLEND(Translation.Z,T.Z,BlendAmount);
										}
									
									MixCount++;
								}
						}
					if (MixCount>0)
						{
							geQuaternion_ToMatrix(&Rotation,Transform);
							Transform->Translation = Translation;
							return GE_TRUE;
						}
					return GE_FALSE;
				}
				break;
			case (MOTION_NODE_LEAF):
				{
					return GE_FALSE;
				}
				break;
			default:
				assert(0);
		}
	return GE_FALSE;
}
#pragma warning( default : 4701)	


//--------------------------------------------------------------------------------------------
//   Event Support

GENESISAPI geBoolean GENESISCC geMotion_GetEventExtents(const geMotion *M,geFloat *FirstEventTime,geFloat *LastEventTime)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( FirstEventTime != NULL );
	assert( LastEventTime != NULL );

	return geTKEvents_GetExtents(M->Leaf.Events,FirstEventTime,LastEventTime);
}	


	// Inserts the new event and corresponding string.
GENESISAPI geBoolean GENESISCC geMotion_InsertEvent(geMotion *M, gePath_TimeType tKey, const char* String)
{
	assert( M != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );
	assert( String != NULL );

	if (M->NodeType != MOTION_NODE_LEAF )
		{
			geErrorLog_Add(ERR_MOTION_INSERT_EVENT, NULL);
			return GE_FALSE;
		}

	if (M->Leaf.Events == NULL)
		{
			M->Leaf.Events = geTKEvents_Create();
			if ( M->Leaf.Events == NULL )
				{
					geErrorLog_Add(ERR_MOTION_INSERT_EVENT, NULL);
					return GE_FALSE;
				}
		}
	if (geTKEvents_Insert(M->Leaf.Events, tKey,String)==GE_FALSE)
		{
			geErrorLog_Add(ERR_MOTION_INSERT_EVENT, NULL);
			return GE_FALSE;
		};
	return GE_TRUE;
}
	

			
	// Deletes the event
GENESISAPI geBoolean GENESISCC geMotion_DeleteEvent(geMotion *M, gePath_TimeType tKey)
{
	assert( M != NULL);
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->NodeType != MOTION_NODE_LEAF )
		{
			geErrorLog_Add(ERR_MOTION_INSERT_EVENT, NULL);
			return GE_FALSE;
		}
	if ( M->Leaf.Events == NULL )
		{
			geErrorLog_Add(ERR_MOTION_DELETE_EVENT, NULL);
			return GE_FALSE;
		}
	if (geTKEvents_Delete(M->Leaf.Events,tKey)==GE_FALSE)
		{
			geErrorLog_Add(ERR_MOTION_DELETE_EVENT, NULL);
			return GE_FALSE;
		}
	return GE_TRUE;
}

GENESISAPI void GENESISCC geMotion_SetupEventIterator(
	geMotion *M,
	gePath_TimeType StartTime,				// Inclusive search start
	gePath_TimeType EndTime)				// Non-inclusive search stop
	// For searching or querying the array for events between two times
	// times are compaired [StartTime,EndTime), '[' is inclusive, ')' is 
	// non-inclusive.  This prepares the geMotion_GetNextEvent() function.
{
	int i;
	assert( M != NULL);
	assert( geMotion_IsValid(M) != GE_FALSE );

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				break;
			case (MOTION_NODE_LEAF):
				if ( M->Leaf.Events != NULL )
					{
						geTKEvents_SetupIterator(M->Leaf.Events,StartTime,EndTime);
					}	
				break;
			case (MOTION_NODE_BRANCH):
				for (i=0; i<M->Branch.MixerCount; i++)
					{
						geMotion_Mixer *Mixer;
				
						Mixer = &(M->Branch.MixerArray[i]);

						geMotion_SetupEventIterator(Mixer->Motion,
							(StartTime - Mixer->TimeOffset) * Mixer->TimeScale,
							(EndTime - Mixer->TimeOffset) * Mixer->TimeScale);
					}
				M->Branch.CurrentEventIterator =0;
				break;
			default:
				assert(0);
		}
}		


GENESISAPI geBoolean GENESISCC geMotion_GetNextEvent(
	geMotion *M,						// Event list to iterate
	gePath_TimeType *pTime,				// Return time, if found
	const char **ppEventString)		// Return data, if found
	// Iterates from StartTime to EndTime as setup in geMotion_SetupEventIterator()
	// and for each event between these times [StartTime,EndTime)
	// this function will return Time and EventString returned for that event
	// and the iterator will be positioned for the next search.  When there 
	// are no more events in the range, this function will return GE_FALSE (Time
	// will be 0 and ppEventString will be empty).
{
	assert( M != NULL);
	assert( geMotion_IsValid(M) != GE_FALSE );

	assert( pTime != NULL );
	assert( ppEventString != NULL );

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				{
					return GE_FALSE;
				}
				break;
			case (MOTION_NODE_LEAF):
				if ( M->Leaf.Events != NULL )
					{
						return geTKEvents_GetNextEvent(M->Leaf.Events,pTime,ppEventString);
					}	
				break;
			case (MOTION_NODE_BRANCH):
				while (M->Branch.CurrentEventIterator < M->Branch.MixerCount)
					{
						if (geMotion_GetNextEvent(
									M->Branch.MixerArray[M->Branch.CurrentEventIterator].Motion,
									pTime,ppEventString) !=GE_FALSE)
							return GE_TRUE;
						M->Branch.CurrentEventIterator++;
					}
				break;
			default:
				assert(0);
		}

	return GE_FALSE;
}
	
//------------------------------------------------------------------------------------------------------
//    Read/Write support


#define CHECK_FOR_WRITE(uu) if(uu <= 0) { geErrorLog_Add( ERR_MOTION_FILE_WRITE, NULL ); return GE_FALSE; }

#define MOTION_ASCII_FILE_TYPE 0x4E544F4D 	// 'MOTN'
#define MOTION_BIN_FILE_TYPE 0x424E544D 	// 'MTNB'
#define MOTION_FILE_VERSION 0x00F0			// Restrict version to 16 bits


#define MOTION_NAME_ID			"NameID"
#define MOTION_MAINTAINNAMES_ID "MaintainNames"

#define MOTION_NUM_ASCII_IDS     6	// Keep this up to date

#define MOTION_PATHCOUNT_ID     "PathCount"
#define MOTION_NAMECHECKSUM_ID  "NameChecksum"
#define MOTION_EVENTS_ID		"Events"
#define MOTION_NAMEARRAY_ID		"NameArray"
#define MOTION_PATHARRAY_ID		"PathArray"
#define LEAF_NUM_ASCII_IDS		5  // keep this up to date

#define MOTION_MixerCount_ID   "MixerCount"
#define MOTION_MOTION_ARRAY		"MotionArray"
#define MOTION_BLEND_ARRAY		"BlendArray"
#define BRANCH_NUM_ASCII_IDS	3  // keep this up to date

static geMotion *GENESISCC geMotion_CreateFromBinaryFile(geVFile *pFile);


GENESISAPI geMotion* GENESISCC geMotion_CreateFromFile(geVFile* pFile)
{
	uint32 u, v;
	geMotion* M;

	assert( pFile != NULL );

	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
		return NULL;
	}

	if(u == MOTION_ASCII_FILE_TYPE)
	{
		int NumItemsNeeded=0;
		int NumItemsRead = 0;
		#define LINE_LENGTH 256
		char line[LINE_LENGTH];

		M = geMotion_Create(GE_FALSE);
		if( M == NULL )
		{
			geErrorLog_Add(-1, NULL);
			return NULL;		// error logged already in geMotion_Create
		}
	
		// Read and build the version.  Then determine the number of items to read.
		if	(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
			{
				geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
				return NULL;
			}
		if	(sscanf(line, "%X.%X\n", &u, &v) != 2)
			{
				geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
				return NULL;
			}
		v |= (u << 8);
		if(v >= MOTION_FILE_VERSION)
		{
			NumItemsNeeded = MOTION_NUM_ASCII_IDS;
		}

		// remove this when leaf/branch split happens.
		if (geMotion_InitNodeAsLeaf(M,GE_FALSE)==GE_FALSE)
		{
			geErrorLog_Add(-1, NULL);
			return GE_FALSE;
		}


		while(NumItemsRead < NumItemsNeeded)
		{
			if(geVFile_GetS(pFile, line, LINE_LENGTH) == GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
					break; // got to read something
				}
			else if(strnicmp(line, MOTION_NAME_ID, sizeof(MOTION_NAME_ID)-1) == 0)
			{
				//line[strlen(line)-1]=0;  // zap off cr
				if ( line[0] != 0 )
					line[strlen(line)-1] = 0;	// remove trailing /n  (textmode)
				if ( line[0] != 0 )
					{
						int len = strlen(line)-1;
						if (line[len] == 13)  // remove trailing /r  (binary file mode)
							{
								line[len] = 0;
							}
					}
				if (strlen(line) > sizeof(MOTION_NAME_ID))
					{
						if (geMotion_SetName(M,line+sizeof(MOTION_NAME_ID))==GE_FALSE)
							{						
								geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
								break;		 
							}
					}
				//NumItemsRead++;
			}
			
			else if(strnicmp(line, MOTION_MAINTAINNAMES_ID, sizeof(MOTION_MAINTAINNAMES_ID)-1) == 0)
			{
				if(sscanf(line + sizeof(MOTION_MAINTAINNAMES_ID)-1, "%d", &M->MaintainNames) != 1)
					{						
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						break;
					}
				NumItemsRead++;
			}
			else if(strnicmp(line, MOTION_PATHCOUNT_ID, sizeof(MOTION_PATHCOUNT_ID)-1) == 0)
			{
				if(sscanf(line + sizeof(MOTION_PATHCOUNT_ID)-1, "%d", &M->Leaf.PathCount) != 1)
					{						
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						break;		 
					}
				NumItemsRead++;
			}
			else if(strnicmp(line, MOTION_NAMECHECKSUM_ID, sizeof(MOTION_NAMECHECKSUM_ID)-1) == 0)
			{
				if(sscanf(line + sizeof(MOTION_NAMECHECKSUM_ID)-1, "%d", &M->Leaf.NameChecksum) != 1)
					{						
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						break;
					}
				NumItemsRead++;
			}
			else if(strnicmp(line, MOTION_EVENTS_ID, sizeof(MOTION_EVENTS_ID)-1) == 0)
			{
				int flag;
				if(sscanf(line + sizeof(MOTION_EVENTS_ID)-1, "%d", &flag) != 1)
					{						
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						break;
					}
				if (flag!=GE_FALSE)
					{
						M->Leaf.Events = geTKEvents_CreateFromFile(pFile);
						if (M->Leaf.Events == NULL)
							{						
								geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
								break;
							}
					}
				NumItemsRead++;
			}
			else if(strnicmp(line, MOTION_NAMEARRAY_ID, sizeof(MOTION_NAMEARRAY_ID)-1) == 0)
			{
				int flag;
				if(sscanf(line + sizeof(MOTION_NAMEARRAY_ID)-1, "%d", &flag) != 1)
					{						
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						break;
					}
				if (flag!=GE_FALSE)
					{
						M->Leaf.NameArray = geStrBlock_CreateFromFile(pFile);
						if (M->Leaf.NameArray == NULL)
							{						
								geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
								break;
							}
					}
				NumItemsRead++;
			}
			else if(strnicmp(line, MOTION_PATHARRAY_ID, sizeof(MOTION_PATHARRAY_ID)-1) == 0)
			{
				int i,count;
				if(sscanf(line + sizeof(MOTION_PATHARRAY_ID)-1, "%d", &count) != 1)
					{						
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						break;
					}

				M->Leaf.PathArray = geRam_Allocate( count * sizeof(gePath*) );

				if ( M->Leaf.PathArray == NULL )
					{	
						geErrorLog_Add(ERR_MOTION_ADDPATH_ENOMEM, NULL);
						break;
					}

				for (i=0; i<count; i++)
					{
							
						M->Leaf.PathArray[i] = gePath_CreateFromFile(pFile);
						if (M->Leaf.PathArray[i] == NULL )
							{
								geErrorLog_Add(ERR_MOTION_FILE_READ, NULL);
								break;
							}
	
					}
				NumItemsRead++;
			}


		}
		
		if(NumItemsNeeded == NumItemsRead)
			{
				return M;
			}
		else
			{
				geErrorLog_Add( ERR_MOTION_FILE_PARSE , NULL);
				geMotion_Destroy(&M); // try to destroy it
				return NULL;
			}
	}
	else
		{
			if (u==MOTION_BIN_FILE_TYPE)
				{
					return geMotion_CreateFromBinaryFile(pFile);
				}
		}

	geErrorLog_Add( ERR_MOTION_FILE_PARSE , NULL);
	return NULL;
}

static geBoolean GENESISCC geMotion_WriteLeaf(const geMotion *M, geVFile *pFile)
{
	int i;
	int flag;

	assert( M != NULL );
	assert( pFile != NULL );
	assert( M->NodeType == MOTION_NODE_LEAF);
	assert( geMotion_IsValid(M) != GE_FALSE );

	if (M->Leaf.Events == NULL)
		flag = GE_FALSE;
	else
		flag = GE_TRUE;

	if	(geVFile_Printf(pFile, "%s %d\n", MOTION_PATHCOUNT_ID,M->Leaf.PathCount) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
			return GE_FALSE; 
		}
	if	(geVFile_Printf(pFile, "%s %d\n", MOTION_NAMECHECKSUM_ID,M->Leaf.NameChecksum) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
			return GE_FALSE; 
		}

	if	(geVFile_Printf(pFile, "%s %d\n", MOTION_EVENTS_ID, flag) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
			return GE_FALSE; 
		}

	if (flag != GE_FALSE)
		{
			if (geTKEvents_WriteToFile(M->Leaf.Events,pFile)==GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
					return GE_FALSE; 
				}
		}

	if (M->Leaf.NameArray == NULL)
		flag = GE_FALSE;
	else
		flag = GE_TRUE;
	if	(geVFile_Printf(pFile, "%s %d\n", MOTION_NAMEARRAY_ID, flag) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
			return GE_FALSE; 
		}
	if (flag != GE_FALSE)
		{
			if (geStrBlock_WriteToFile(M->Leaf.NameArray,pFile)==GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
					return GE_FALSE; 
				}
		}

	if	(geVFile_Printf(pFile, "%s %d\n", MOTION_PATHARRAY_ID, M->Leaf.PathCount) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
			return GE_FALSE; 
		}
	for (i=0; i<M->Leaf.PathCount; i++)
		{
			if (gePath_WriteToFile(M->Leaf.PathArray[i],pFile) == GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
					return GE_FALSE; 
				}
		}
	return GE_TRUE;
}

static geBoolean GENESISCC geMotion_WriteBranch(const geMotion *M, geVFile *pFile)
{
	assert( M != NULL );
	assert( pFile != NULL );
	assert( M->NodeType == MOTION_NODE_BRANCH);
	assert( geMotion_IsValid(M) != GE_FALSE );
	#pragma message("finish this")
	return GE_FALSE;
}


GENESISAPI geBoolean GENESISCC geMotion_WriteToFile(const geMotion *M, geVFile *pFile)
{
	uint32 u;

	assert( M != NULL );
	assert( pFile != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );


	// Write the format flag
	u = MOTION_ASCII_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
		return GE_FALSE;
	}


	// Write the version
	if	(geVFile_Printf(pFile, " %X.%.2X\n", (MOTION_FILE_VERSION & 0xFF00) >> 8, 
									MOTION_FILE_VERSION & 0x00FF) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
			return GE_FALSE;
		}

	if	(geVFile_Printf(pFile, "%s %s\n", MOTION_NAME_ID,(M->Name==NULL)?(""):(M->Name)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
			return GE_FALSE;
		}

	if	(geVFile_Printf(pFile, "%s %d\n", MOTION_MAINTAINNAMES_ID,M->MaintainNames) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
			return GE_FALSE;
		}

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				break;
			case (MOTION_NODE_BRANCH):
				if (geMotion_WriteBranch(M,pFile)==GE_FALSE)
					{
						geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
						return GE_FALSE;
					}
				break;
			case (MOTION_NODE_LEAF):
				if (geMotion_WriteLeaf(M,pFile)==GE_FALSE)
					{
						geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
						return GE_FALSE;
					}
				break;
			default:
				assert(0);
				break;
		}
	return GE_TRUE;
}



typedef struct
{
	int PathCount;
	int32 NameChecksum;
	uint32 Flags;
} geMotion_BinaryFileLeafHeader;

static geBoolean GENESISCC geMotion_ReadBinaryBranch(geMotion *M, geVFile *pFile)
{
	assert( M != NULL );
	assert( pFile != NULL );
	if (geMotion_InitNodeAsBranch(M)==GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
			return GE_FALSE;
		}

	#pragma message("finish this")
	return GE_FALSE;
}

static geBoolean GENESISCC geMotion_ReadBinaryLeaf(geMotion *M, geVFile *pFile)
{
	int i;
	geMotion_BinaryFileLeafHeader Header;
	assert( M != NULL );
	assert( pFile != NULL );
	if (geMotion_InitNodeAsLeaf(M,GE_FALSE)==GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
			return GE_FALSE;
		}

	if (geVFile_Read(pFile, &Header, sizeof(geMotion_BinaryFileLeafHeader)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_READ , NULL); 
			return GE_FALSE; 
		}
	M->Leaf.NameChecksum = Header.NameChecksum;
	
	if (Header.Flags & 0x1)
		{
			M->Leaf.Events = geTKEvents_CreateFromFile(pFile);
			if (M->Leaf.Events == NULL )
				{
					geErrorLog_Add( ERR_MOTION_FILE_READ , NULL); 
					return GE_FALSE; 
				}
		}
	else
		{
			M->Leaf.Events = NULL;
		}

	if (Header.Flags & 0x2)
		{
			M->Leaf.NameArray = geStrBlock_CreateFromFile(pFile);
			if (M->Leaf.NameArray == NULL)
				{
					geErrorLog_Add( ERR_MOTION_FILE_READ , NULL); 
					return GE_FALSE; 
				}
		}
	else
		{
			M->Leaf.NameArray = NULL;
		}

	M->Leaf.PathCount = 0;
	M->Leaf.PathArray = geRam_Allocate( Header.PathCount * sizeof(gePath*) );

	if ( M->Leaf.PathArray == NULL )
		{	
			geErrorLog_Add(ERR_MOTION_CREATE_ENOMEM, NULL);
			return GE_TRUE;
		}

	for (i=0; i<Header.PathCount; i++)
		{
			M->Leaf.PathArray[i] = gePath_CreateFromFile(pFile);
			if (M->Leaf.PathArray[i] == NULL )
				{
					geErrorLog_Add( ERR_MOTION_FILE_READ , NULL); 
					return GE_FALSE; 
				}
			M->Leaf.PathCount++;
		}
	return GE_TRUE;
}

static geMotion *GENESISCC geMotion_CreateFromBinaryFile(geVFile *pFile)
{
	uint32 u;	
	geBoolean MaintainNames;
	int NodeType;
	int NameLength;
	geMotion *M;

	assert( pFile != NULL );

	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
			return NULL;
		}
	if (u!=MOTION_FILE_VERSION)
		{
			geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
			return NULL;
		}
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
			return NULL;
		}

	if (u & (1<<16)) 
		{
			MaintainNames = GE_TRUE;
		}
	else
		{
			MaintainNames = GE_FALSE;
		}

	NameLength = (u & 0xFFFF);
	NodeType   = (u >> 24);
	M = geMotion_Create(MaintainNames);
	if ( M == NULL )
		{
			geErrorLog_Add(ERR_MOTION_CREATE_ENOMEM, NULL);
			return NULL;
		}
	if (NameLength>0)
		{
			M->Name = geRam_Allocate(NameLength);
			if ( M->Name == NULL )
				{
					geErrorLog_Add(ERR_MOTION_CREATE_ENOMEM, NULL);
					geMotion_Destroy(&M);
					return NULL;
				}
 			if ( geVFile_Read (pFile, M->Name, NameLength ) == GE_FALSE )
				{
					geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
					geMotion_Destroy(&M);
					return NULL;
				}
		}
	else
		{
			M->Name = NULL;
		}
	switch (NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				break;
			case (MOTION_NODE_BRANCH):
				if (geMotion_ReadBinaryBranch(M,pFile)==GE_FALSE)
					{
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						geMotion_Destroy(&M);
						return NULL;
					}
				break;
			case (MOTION_NODE_LEAF):
				if (geMotion_ReadBinaryLeaf(M,pFile)==GE_FALSE)
					{
						geErrorLog_Add( ERR_MOTION_FILE_READ , NULL);
						geMotion_Destroy(&M);
						return NULL;
					}
				break;
			default:
				assert(0);
				break;
		}
	return M;
}


static geBoolean GENESISCC geMotion_WriteBinaryLeaf(const geMotion *M, geVFile *pFile)
{
	int i;
	geMotion_BinaryFileLeafHeader Header;

	#define MOTION_LEAF_EVENTS_FLAG    (1)
	#define MOTION_LEAF_NAMEARRAY_FLAG (2)

	assert( M != NULL );
	assert( pFile != NULL );
	assert( M->NodeType == MOTION_NODE_LEAF);
	assert( geMotion_IsValid(M) != GE_FALSE );

	Header.PathCount = M->Leaf.PathCount;
	Header.NameChecksum = M->Leaf.NameChecksum;
	Header.Flags = 0;

	if (M->Leaf.Events != NULL)
		{
			Header.Flags |= MOTION_LEAF_EVENTS_FLAG;
		}

	if (M->Leaf.NameArray != NULL)
		{
			Header.Flags |= MOTION_LEAF_NAMEARRAY_FLAG;
		}
		
		
	if (geVFile_Write(pFile, &Header, sizeof(geMotion_BinaryFileLeafHeader)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
			return GE_FALSE; 
		}
			

	if (Header.Flags & MOTION_LEAF_EVENTS_FLAG)
		{
			if (geTKEvents_WriteToBinaryFile(M->Leaf.Events,pFile)==GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
					return GE_FALSE; 
				}
		}

	
	if (Header.Flags & MOTION_LEAF_NAMEARRAY_FLAG)
		{
			if (geStrBlock_WriteToBinaryFile(M->Leaf.NameArray,pFile)==GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
					return GE_FALSE; 
				}
		}

	for (i=0; i<M->Leaf.PathCount; i++)
		{
			if (gePath_WriteToBinaryFile(M->Leaf.PathArray[i],pFile) == GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL); 
					return GE_FALSE; 
				}
		}
	return GE_TRUE;
}

static geBoolean GENESISCC geMotion_WriteBinaryBranch(const geMotion *M, geVFile *pFile)
{
	assert( M != NULL );
	assert( pFile != NULL );
	assert( M->NodeType == MOTION_NODE_BRANCH);
	assert( geMotion_IsValid(M) != GE_FALSE );
	#pragma message("finish this")
	return GE_FALSE;
}


GENESISAPI geBoolean GENESISCC geMotion_WriteToBinaryFile(const geMotion *M,geVFile *pFile)
{
	uint32 u;

	assert( M != NULL );
	assert( pFile != NULL );
	assert( geMotion_IsValid(M) != GE_FALSE );


	// Write the format flag
	u = MOTION_BIN_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
			return GE_FALSE;
		}

	u = MOTION_FILE_VERSION;
	if (geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
			return GE_FALSE;
		}
	if ( M->Name != NULL )
		{
			u = strlen(M->Name)+1;
		}
	else
		{
			u = 0;
		}
	assert( u < 0xFFFF );
	
	if (M->MaintainNames != GE_FALSE)
		{
			u |= (1<<16);
		}
	assert( M->NodeType < 0xFF );
	u |= (M->NodeType << 24);
	if (geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
			return GE_FALSE;
		}
	if ((u&0xFFFF) > 0)
		{
			if (geVFile_Write(pFile, M->Name, (u&0xFFFF)) == GE_FALSE)
				{
					geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
					return GE_FALSE;
				}
		}

	switch (M->NodeType)
		{
			case (MOTION_NODE_UNDECIDED):
				break;
			case (MOTION_NODE_BRANCH):
				if (geMotion_WriteBinaryBranch(M,pFile)==GE_FALSE)
					{
						geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
						return GE_FALSE;
					}
				break;
			case (MOTION_NODE_LEAF):
				if (geMotion_WriteBinaryLeaf(M,pFile)==GE_FALSE)
					{
						geErrorLog_Add( ERR_MOTION_FILE_WRITE , NULL);
						return GE_FALSE;
					}
				break;
			default:
				assert(0);
				break;
		}
	return GE_TRUE;
}
