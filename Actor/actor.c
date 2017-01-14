/****************************************************************************************/
/*  ACTOR.C                                                                             */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description:  Actor implementation                                                  */
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
/*

	TODO:
	  make cued motions keyed to a 'root' bone.  Register the root bone, and then 
	  all requests are relative to that bone, rather than the current 'anchor' point.
	  actually, this doesn't really change much, just _AnimationCue() - and it allows
	  a more efficient _TestStep()

	
*/
#include <assert.h>
#include <string.h>  //strnicmp		memmove()
#include <math.h>	 // fabs()
#include <stdio.h>	 //sscanf

#include "world.h"	// to expose _Render apis in actor.h

#include "Actor.h"
#include "Ram.h"
#include "Puppet.h"
#include "Body.h"
#include "Motion.h"
#include "ErrorLog.h"
#include "strblock.h"


/* to do:
		need to utilize extbox module rather than hard coding vector corners of boxes
		(BoundingBoxMinCorner,BoundingBoxMaxCorner)
*/

#define ACTOR_MOTIONS_MAX 0x0FFFF		// really arbitrary. just for sanity checking
#define ACTOR_CUES_MAX    0x0FFFF		// arbitrary. 

typedef struct geActor
{
	int32				 RefCount;				// this is the number of owners.
	gePuppet			*Puppet;
	gePose				*Pose;
	geActor_BlendingType BlendingType;
	geActor_Def			*ActorDefinition;		// actor definition this is an instance of
	
	geMotion			*CueMotion;
	geVec3d				BoundingBoxMinCorner;
	geVec3d				BoundingBoxMaxCorner;
	int					BoundingBoxCenterBoneIndex;
	int					StepBoneIndex;			// used for single-bone motion optimization.
	void *				UserData;

	geExtBox			RenderHintExtBox;
	int					RenderHintExtBoxCenterBoneIndex;
	geBoolean			RenderHintExtBoxEnabled;
} geActor;


typedef struct geActor_Def
{
	geBody				*Body;
	geVFile *			 TextureFileContext;
	
	int32				 MotionCount;
	geMotion		   **MotionArray;

	int32				 RefCount;				// this is the number of owners.

	geActor_Def			*ValidityCheck;
} geActor_Def;

	// these are useful globals to monitor resources
int geActor_Count       = 0;
int geActor_RefCount    = 0;
int geActor_DefCount    = 0;
int geActor_DefRefCount = 0;

	// returns number of actors that are currently created.
GENESISAPI int GENESISCC geActor_GetCount(void)
{
	return geActor_Count;
}

GENESISAPI geBoolean GENESISCC geActor_IsValid(const geActor *A)
{
	if (A==NULL)
		return GE_FALSE;
	if (geActor_DefIsValid(A->ActorDefinition)==GE_FALSE)
		return GE_FALSE;
	if (A->Pose == NULL)
		return GE_FALSE;
	if (A->CueMotion == NULL)
		return GE_FALSE;
	if (geBody_IsValid(A->ActorDefinition->Body) == GE_FALSE )
		return GE_FALSE;

	return GE_TRUE;
}
	

GENESISAPI geBoolean GENESISCC geActor_DefIsValid(const geActor_Def *A)
{
	if (A==NULL)
		return GE_FALSE;
	if (A->ValidityCheck != A)
		return GE_FALSE;
	return GE_TRUE;
}

static geBoolean GENESISCC geActor_GetBoneIndex(const geActor *A, const char *BoneName, int *BoneIndex)
{
	geXForm3d Dummy;
	int ParentBoneIndex;
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( BoneIndex != NULL );

	if ( BoneName != NULL )
		{
			if (geBody_GetBoneByName(A->ActorDefinition->Body,
									 BoneName,
									 BoneIndex,
									 &Dummy,
									 &ParentBoneIndex) ==GE_FALSE)
				{
					geErrorLog_AddString(-1,"Named bone not found:", BoneName);
					//geErrorLog_AppendString(BoneName);
					return GE_FALSE;			
				}
		}
	else
		{
			*BoneIndex = GE_POSE_ROOT_JOINT;
		}
	return GE_TRUE;
}


GENESISAPI geActor_Def *GENESISCC geActor_GetActorDef(const geActor *A)
{
	assert( geActor_DefIsValid(A->ActorDefinition) != GE_FALSE );
	return A->ActorDefinition;
}

GENESISAPI void GENESISCC geActor_DefCreateRef(geActor_Def *A)
{
	assert( geActor_DefIsValid(A) != GE_FALSE );
	A->RefCount++;
	geActor_DefRefCount++;
}

GENESISAPI geActor_Def *GENESISCC geActor_DefCreate(void)
{
	geActor_Def *Ad;

	Ad = GE_RAM_ALLOCATE_STRUCT( geActor_Def );
	if ( Ad == NULL )
		{
			geErrorLog_Add( ERR_ACTOR_ENOMEM , NULL);
			return NULL;
		}

	Ad->Body				= NULL;
	Ad->MotionCount			= 0;
	Ad->MotionArray			= NULL;
	Ad->ValidityCheck		= Ad;
	Ad->RefCount            = 0;
	geActor_DefCount++;
	return Ad;
}

GENESISAPI void GENESISCC geActor_CreateRef(geActor *Actor)
{
	assert( geActor_IsValid(Actor) );
	Actor->RefCount ++;
	geActor_RefCount++;
}

GENESISAPI geActor *GENESISCC geActor_Create(geActor_Def *ActorDefinition)
{
	geActor *A;
	assert( geActor_DefIsValid(ActorDefinition)      != GE_FALSE );
	
	if (ActorDefinition->Body == NULL)
		{
			geErrorLog_AddString(-1,"geActor_Def must have a body before Actors can be created", NULL);
			return NULL;
		}
	assert( geBody_IsValid(ActorDefinition->Body) != GE_FALSE );

	A = GE_RAM_ALLOCATE_STRUCT( geActor );
	if ( A == NULL )
		{
			geErrorLog_Add( ERR_ACTOR_ENOMEM , NULL);
			goto ActorCreateFailure;
		}
	A->Puppet = NULL;
	A->Pose   = NULL;
	A->CueMotion = NULL;

	A->Pose = gePose_Create();
	if (A->Pose == NULL)
		{
			geErrorLog_Add(ERR_ACTOR_ENOMEM, NULL);
			goto ActorCreateFailure;
		}
	
	A->RefCount          = 0;
	A->BlendingType		 = GE_ACTOR_BLEND_HERMITE;
	A->ActorDefinition   = ActorDefinition;
	A->CueMotion		 = geMotion_Create(GE_TRUE);
	A->BoundingBoxCenterBoneIndex = GE_POSE_ROOT_JOINT;
	A->RenderHintExtBoxCenterBoneIndex = GE_POSE_ROOT_JOINT;
	A->RenderHintExtBoxEnabled = GE_FALSE;
	A->StepBoneIndex     = GE_POSE_ROOT_JOINT;
	geExtBox_Set(&(A->RenderHintExtBox), 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f);
	if (A->CueMotion == NULL)
		{
			geErrorLog_Add( ERR_ACTOR_ENOMEM , NULL);
			goto ActorCreateFailure;
		}


	{
		int i; 
		int BoneCount;

		BoneCount = geBody_GetBoneCount(A->ActorDefinition->Body);
		for (i=0; i<BoneCount; i++)
			{
				const char *Name;
				geXForm3d Attachment;
				int ParentBone;
				int Index;
				geBody_GetBone( A->ActorDefinition->Body, i, &Name,&Attachment, &ParentBone );
				if (gePose_AddJoint( A->Pose,
									ParentBone,Name,&Attachment,&Index)==GE_FALSE)
					{
						geErrorLog_Add(ERR_ACTOR_ENOMEM, NULL);
						goto ActorCreateFailure;
					}
			}
	}


	geVec3d_Clear(&(A->BoundingBoxMinCorner));
	geVec3d_Clear(&(A->BoundingBoxMaxCorner));
	assert( geActor_IsValid(A) != GE_FALSE );
	geActor_DefCreateRef(ActorDefinition);
	geActor_Count++;
	return A;

	ActorCreateFailure:
	if ( A!= NULL)
		{
			if (A->Pose != NULL)
				gePose_Destroy(&(A->Pose));
			if (A->CueMotion != NULL)
				geMotion_Destroy(&(A->CueMotion));
			geRam_Free( A );
		}
	return NULL;
}	

GENESISAPI geBoolean GENESISCC geActor_DefDestroy(geActor_Def **pActorDefinition)
{
	int i;
	geActor_Def *Ad;
	assert(  pActorDefinition != NULL );
	assert( *pActorDefinition != NULL );
	assert( geActor_DefIsValid( *pActorDefinition ) != GE_FALSE );

	Ad = *pActorDefinition;

	if (Ad->RefCount > 0)
		{
			Ad->RefCount--;
			geActor_DefRefCount--;
			return GE_FALSE;
		}

	if (Ad->Body != NULL)
		{
			geBody_Destroy( &(Ad->Body) );
			Ad->Body = NULL;
		}
	if (Ad->MotionArray != NULL)
		{
			for (i=0; i<Ad->MotionCount; i++)
				{
					geMotion_Destroy( &(Ad->MotionArray[i]) );
					Ad->MotionArray[i] = NULL;
				}
			geRam_Free( Ad->MotionArray );
			Ad->MotionArray = NULL;
		}
				
	Ad->MotionCount = 0;

	geRam_Free(*pActorDefinition);
	*pActorDefinition = NULL;
	geActor_DefCount--;
	return GE_TRUE;
}


GENESISAPI void GENESISCC geActor_Destroy(geActor **pA)
{
	geActor *A;
	assert(  pA != NULL );
	assert( *pA != NULL );
	assert( geActor_IsValid(*pA) != GE_FALSE );
	
	A = *pA;
	if (A->RefCount > 0)
		{
			A->RefCount --;
			geActor_RefCount--;
			return;
		}

	geActor_DefDestroy(&(A->ActorDefinition));
	if (A->Puppet != NULL)
		{
			gePuppet_Destroy( &(A->Puppet) );
			A->Puppet = NULL;
		}
	if ( A->Pose != NULL )
		{
			gePose_Destroy( &( A->Pose ) );
			A->Pose = NULL;
		}
	if ( A->CueMotion != NULL )
		{
			geMotion_Destroy(&(A->CueMotion));
			A->CueMotion = NULL;
		}
	geRam_Free(*pA);
	geActor_Count--;
	*pA = NULL;
}

GENESISAPI geBoolean GENESISCC geActor_SetBody( geActor_Def *ActorDefinition, geBody *BodyGeometry)
{
	assert( geBody_IsValid(BodyGeometry) != GE_FALSE );
	
	if (ActorDefinition->RefCount > 0)
		{	
			geErrorLog_Add(-1, NULL);  // ActorDef already used by a body. cant change now
			return GE_FALSE;
		}

	if (ActorDefinition->Body != NULL)
		{
			geBody_Destroy( &(ActorDefinition->Body) );
		}
	
	ActorDefinition->Body          = BodyGeometry;
	return GE_TRUE;
}


#pragma message ("consider removing this and related parameters to setpose")
GENESISAPI void GENESISCC geActor_SetBlendingType( geActor *A, geActor_BlendingType BlendingType )
{
	assert( geActor_IsValid(A) != GE_FALSE );

	assert( (BlendingType == GE_ACTOR_BLEND_LINEAR) || 
			(BlendingType == GE_ACTOR_BLEND_HERMITE) );

	if (BlendingType == GE_ACTOR_BLEND_LINEAR)
		{
			A->BlendingType = GE_POSE_BLEND_LINEAR;
		}
	else
		{
			A->BlendingType = GE_POSE_BLEND_HERMITE;
		}
}

GENESISAPI geVFile *geActor_DefGetFileContext(const geActor_Def *A)
{
	assert( geActor_DefIsValid(A) != GE_FALSE );
	return A->TextureFileContext;
}



GENESISAPI geBoolean GENESISCC geActor_AddMotion(geActor_Def *Ad, geMotion *NewMotion, int *Index)
{
	geMotion **NewMArray;
	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	assert( NewMotion != NULL );
	assert( Index != NULL );

	if (Ad->MotionCount >= ACTOR_MOTIONS_MAX)
		{
			geErrorLog_Add(ERR_ACTOR_TOO_MANY_MOTIONS, NULL);
			return GE_FALSE;
		}
	NewMArray = GE_RAM_REALLOC_ARRAY( Ad->MotionArray, geMotion*, Ad->MotionCount +1 );
	if ( NewMArray == NULL )
		{
			geErrorLog_Add(ERR_ACTOR_ENOMEM, NULL);
			return GE_FALSE;
		}

	Ad->MotionArray = NewMArray;

	Ad->MotionArray[Ad->MotionCount]= NewMotion;
	Ad->MotionCount++;
	*Index = Ad->MotionCount;
	return GE_TRUE;
};

GENESISAPI void GENESISCC geActor_ClearPose(geActor *A, const geXForm3d *Transform)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	assert ( (Transform==NULL) || (geXForm3d_IsOrthonormal(Transform) != GE_FALSE) );
	gePose_Clear( A->Pose ,Transform);
}

GENESISAPI void GENESISCC geActor_SetPose(geActor *A, const geMotion *M, 
								geFloat Time, const geXForm3d *Transform)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	assert( M != NULL );
	assert ( (Transform==NULL) || (geXForm3d_IsOrthonormal(Transform) != GE_FALSE) );

	gePose_SetMotion( A->Pose,M,Time,Transform);
}

GENESISAPI void GENESISCC geActor_BlendPose(geActor *A, const geMotion *M, 
								geFloat Time,  
								const geXForm3d *Transform,
								geFloat BlendAmount)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	assert( M != NULL );
	assert ( (Transform==NULL) || (geXForm3d_IsOrthonormal(Transform) != GE_FALSE) );

	gePose_BlendMotion( A->Pose,M,Time,Transform,
						BlendAmount,A->BlendingType);
}


GENESISAPI int GENESISCC geActor_GetMotionCount(const geActor_Def *Ad)
{
	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	return Ad->MotionCount;
}
	

GENESISAPI geMotion *GENESISCC geActor_GetMotionByIndex(const geActor_Def *Ad, int Index )
{
	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	assert( Index >= 0 );
	assert( Index < Ad->MotionCount );
	assert( Ad->MotionArray != NULL );

	return Ad->MotionArray[Index];
}

GENESISAPI geMotion *GENESISCC geActor_GetMotionByName(const geActor_Def *Ad, const char *Name )
{
	int i;
	const char *TestName;
	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	assert( Name != NULL );
	for (i=0; i<Ad->MotionCount; i++)
		{
			TestName = geMotion_GetName(Ad->MotionArray[i]);
			if (TestName != NULL)
				{
					if (strcmp(TestName,Name)==0)
						return Ad->MotionArray[i];
				}

		}
	return NULL;
}

GENESISAPI const char *GENESISCC geActor_GetMotionName(const geActor_Def *Ad, int Index )
{
	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	assert( Index >= 0 );
	assert( Index < Ad->MotionCount );
	assert( Ad->MotionArray != NULL );
	return geMotion_GetName(Ad->MotionArray[Index]);
}
	
GENESISAPI geBody *GENESISCC geActor_GetBody(const geActor_Def *Ad)
{
	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	return Ad->Body;
}
	
#pragma message ("Keep this function: geActor_DefHasBoneNamed()?")
// Returns GE_TRUE if the actor definition has a bone named 'Name'
GENESISAPI geBoolean GENESISCC geActor_DefHasBoneNamed(const geActor_Def *Ad, const char *Name )
{
	int DummyIndex,DummyParent;
	geXForm3d DummyAttachment;

	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	assert( Name != NULL );
	if (geBody_GetBoneByName(geActor_GetBody(Ad),Name,
			&DummyIndex, &DummyAttachment, &DummyParent ) == GE_FALSE )
		{
			return GE_FALSE;
		}
	return GE_TRUE;
}


#define GE_ACTOR_BODY_NAME       "Body"
#define GE_ACTOR_HEADER_NAME     "Header"
#define GE_MOTION_DIRECTORY_NAME "Motions"

#define ACTOR_FILE_TYPE 0x52544341      // 'ACTR'
#define ACTOR_FILE_VERSION 0x00F1		// Restrict version to 16 bits



static geActor_Def * GENESISCC geActor_DefCreateHeader(geVFile *pFile, geBoolean *HasBody)
{
	uint32 u;
	uint32 version;
	geActor_Def *Ad;

	assert( pFile != NULL );
	assert( HasBody != NULL );

	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	return NULL;}
	if (u != ACTOR_FILE_TYPE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	return NULL;}

	if(geVFile_Read(pFile, &version, sizeof(version)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	return NULL;}
	if ( (version != ACTOR_FILE_VERSION) )
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	return NULL;}

	Ad = geActor_DefCreate();
	if (Ad==NULL)
		{	geErrorLog_Add( ERR_ACTOR_ENOMEM , NULL); return NULL; }

	Ad->TextureFileContext = geVFile_GetContext(pFile);
	assert(Ad->TextureFileContext);

	if(geVFile_Read(pFile, HasBody, sizeof(*HasBody)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	geActor_DefDestroy(&Ad); return NULL;}

	if(geVFile_Read(pFile, &(Ad->MotionCount), sizeof(Ad->MotionCount)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	geActor_DefDestroy(&Ad); return NULL;}

	return Ad;
}


static geBoolean GENESISCC geActor_DefWriteHeader(const geActor_Def *Ad, geVFile *pFile)
{
	uint32 u;
	geBoolean Flag;
	
	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	assert( pFile != NULL );

	// Write the format flag
	u = ACTOR_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	return GE_FALSE; }


	u = ACTOR_FILE_VERSION;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	return GE_FALSE;}

	if (Ad->Body != NULL)
		Flag = GE_TRUE;
	else 
		Flag = GE_FALSE;

	if(geVFile_Write(pFile, &Flag, sizeof(Flag)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	return GE_FALSE;}

	if(geVFile_Write(pFile, &(Ad->MotionCount), sizeof(Ad->MotionCount)) == GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	return GE_FALSE;}

	return GE_TRUE;
}
	


GENESISAPI geActor_Def *GENESISCC geActor_DefCreateFromFile(geVFile *pFile)
{
	int i;
	geActor_Def *Ad   = NULL;
	geVFile *VFile    = NULL;
	geVFile *SubFile  = NULL;
	geVFile *MotionDirectory = NULL;	
	geBoolean HasBody = GE_FALSE;
	geBody * Body     = NULL;
			
	assert( pFile != NULL );

	VFile = geVFile_OpenNewSystem(pFile,GE_VFILE_TYPE_VIRTUAL, NULL, 
									NULL, GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_READONLY);
	if (VFile == NULL)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	goto CreateError;}
	
	SubFile = geVFile_Open(VFile,GE_ACTOR_HEADER_NAME,GE_VFILE_OPEN_READONLY);
	if (SubFile == NULL)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	goto CreateError;}

	Ad = geActor_DefCreateHeader(SubFile,&HasBody);
	if (Ad == NULL)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	goto CreateError;}
	geVFile_Close(SubFile);


	if (HasBody != GE_FALSE)
		{
			SubFile = geVFile_Open(VFile,GE_ACTOR_BODY_NAME,GE_VFILE_OPEN_READONLY);
			if (SubFile == NULL)
				{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	goto CreateError;}

			Body = geBody_CreateFromFile(SubFile);
			if (Body == NULL)
				{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	goto CreateError; }
			if (geActor_SetBody(Ad,Body)==GE_FALSE)
				{	geErrorLog_Add( ERR_ACTOR_ENOMEM , NULL);	goto CreateError; }
			geVFile_Close(SubFile);
		}

	MotionDirectory = geVFile_Open(VFile,GE_MOTION_DIRECTORY_NAME, 
									GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_READONLY);
	if (MotionDirectory == NULL)
		{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	return NULL;}

	if (Ad->MotionCount>0)
		{
			Ad->MotionArray = GE_RAM_ALLOCATE_ARRAY( geMotion*, Ad->MotionCount);
			for (i=0; i<Ad->MotionCount; i++)
				Ad->MotionArray[i] = NULL;
	
			for (i=0; i<Ad->MotionCount; i++)
				{
					char FName[1000];
					sprintf(FName,"%d",i);

					SubFile = geVFile_Open(MotionDirectory,FName,GE_VFILE_OPEN_READONLY);
					if (SubFile == NULL)
						{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	goto CreateError;}
					Ad->MotionArray[i] = geMotion_CreateFromFile(SubFile);
					if (Ad->MotionArray[i] == NULL)
						{	geErrorLog_Add( ERR_ACTOR_FILE_READ , NULL);	goto CreateError;}
					geVFile_Close(SubFile);
				}
		}
	else
		{
			Ad->MotionArray = NULL;
		}
	geVFile_Close(MotionDirectory);
	geVFile_Close(VFile);
	return Ad;

	CreateError:
		if (SubFile != NULL)
			geVFile_Close(SubFile);
		if (MotionDirectory != NULL)
			geVFile_Close(MotionDirectory);
		if (VFile != NULL)
			geVFile_Close(VFile);
		if (Ad != NULL)
			geActor_DefDestroy(&Ad);
		return NULL;
}


GENESISAPI geBoolean GENESISCC geActor_DefWriteToFile(const geActor_Def *Ad, geVFile *pFile)
{
	int i;
	geVFile *VFile;
	geVFile *SubFile;
	geVFile *MotionDirectory;

	assert( geActor_DefIsValid(Ad) != GE_FALSE );
	assert( pFile != NULL );

	VFile = geVFile_OpenNewSystem(pFile,GE_VFILE_TYPE_VIRTUAL, NULL, 
									NULL, GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_CREATE);
	if (VFile == NULL)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
	
	SubFile = geVFile_Open(VFile,GE_ACTOR_HEADER_NAME,GE_VFILE_OPEN_CREATE);
	if (SubFile == NULL)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}

	if (geActor_DefWriteHeader(Ad,SubFile)==GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
	if (geVFile_Close(SubFile)==GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}

	if (Ad->Body != NULL)
		{
			SubFile = geVFile_Open(VFile,GE_ACTOR_BODY_NAME,GE_VFILE_OPEN_CREATE);
			if (SubFile == NULL)
				{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
			if (geBody_WriteToFile(Ad->Body,SubFile)==GE_FALSE)
				{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
			if (geVFile_Close(SubFile)==GE_FALSE)
				{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
		}

	MotionDirectory = geVFile_Open(VFile,GE_MOTION_DIRECTORY_NAME, 
									GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_CREATE);
	if (MotionDirectory == NULL)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
	
	for (i=0; i<Ad->MotionCount; i++)
		{
			char FName[1000];
			sprintf(FName,"%d",i);

			SubFile = geVFile_Open(MotionDirectory,FName,GE_VFILE_OPEN_CREATE);
			if (SubFile == NULL)
				{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
			if (geMotion_WriteToBinaryFile(Ad->MotionArray[i],SubFile)==GE_FALSE)
				{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
			if (geVFile_Close(SubFile)==GE_FALSE)
				{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
		}

	if (geVFile_Close(MotionDirectory)==GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
	if (geVFile_Close(VFile)==GE_FALSE)
		{	geErrorLog_Add( ERR_ACTOR_FILE_WRITE , NULL);	goto WriteError;}
	
	return GE_TRUE;
	WriteError:
		return GE_FALSE;
}

GENESISAPI geBoolean GENESISCC geActor_GetBoneTransform(const geActor *A, const char *BoneName, geXForm3d *Transform)
{
	int BoneIndex;

	assert( geActor_IsValid(A)!=GE_FALSE );
	assert( Transform!= NULL );
	
	if (geActor_GetBoneIndex(A,BoneName,&BoneIndex)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone not found", BoneName);
			return GE_FALSE;
		}

	gePose_GetJointTransform(   A->Pose, BoneIndex,	Transform);
	assert ( geXForm3d_IsOrthonormal(Transform) != GE_FALSE );

	return GE_TRUE;
}


static void GENESISCC geActor_AccumulateMinMax(
	geVec3d *P,geVec3d *Mins,geVec3d *Maxs)
{
	assert( geVec3d_IsValid( P  ) != GE_FALSE );
	assert( geVec3d_IsValid(Mins) != GE_FALSE );
	assert( geVec3d_IsValid(Maxs) != GE_FALSE );
	
	if (P->X < Mins->X) Mins->X = P->X;
	if (P->Y < Mins->Y) Mins->Y = P->Y;
	if (P->Z < Mins->Z) Mins->Z = P->Z;

	if (P->X > Maxs->X) Maxs->X = P->X;
	if (P->Y > Maxs->Y) Maxs->Y = P->Y;
	if (P->Z > Maxs->Z) Maxs->Z = P->Z;
}


static geBoolean GENESISCC geActor_GetBoneBoundingBoxByIndex(
	const geActor *A, 
	int BoneIndex,
	geVec3d   *Corner,
	geVec3d   *DX,
	geVec3d   *DY,
	geVec3d   *DZ)
{
	geVec3d Min,Max;
	geVec3d Orientation;
	geXForm3d Transform;
	
	assert( geActor_IsValid(A) != GE_FALSE );	
	assert( geActor_DefIsValid(A->ActorDefinition) != GE_FALSE );
	assert( A->ActorDefinition->Body   != NULL );

	assert( Corner      );
	assert( DX          );
	assert( DY          );
	assert( DZ          );
	assert( (BoneIndex < gePose_GetJointCount(A->Pose)) || (BoneIndex ==GE_POSE_ROOT_JOINT));
	assert( (BoneIndex >=0)                             || (BoneIndex ==GE_POSE_ROOT_JOINT));
	
	if (geBody_GetBoundingBox( A->ActorDefinition->Body, BoneIndex, &Min, &Max )==GE_FALSE)
		{
			return GE_FALSE;
		}

	// scale bounding box:
	{
		geVec3d Scale;
		gePose_GetScale(A->Pose, &Scale);
		assert( geVec3d_IsValid(&Scale) != GE_FALSE );

		Min.X *= Scale.X;
		Min.Y *= Scale.Y;
		Min.Z *= Scale.Z;
		
		Max.X *= Scale.X;
		Max.Y *= Scale.Y;
		Max.Z *= Scale.Z;
	}


	gePose_GetJointTransform(A->Pose,BoneIndex,&(Transform));

	geVec3d_Subtract(&Max,&Min,&Orientation);
			
	DX->X = Orientation.X;	DX->Y = DX->Z = 0.0f;
	DY->Y = Orientation.Y;	DY->X = DY->Z = 0.0f;
	DZ->Z = Orientation.Z;	DZ->X = DZ->Y = 0.0f;
			
	// transform into world space
	geXForm3d_Transform(&(Transform),&Min,&Min);
	geXForm3d_Rotate(&(Transform),DX,DX);
	geXForm3d_Rotate(&(Transform),DY,DY);
	geXForm3d_Rotate(&(Transform),DZ,DZ);

	*Corner = Min;
	return GE_TRUE;
}



static geBoolean GENESISCC geActor_GetBoneExtBoxByIndex(
	const geActor *A, 
	int BoneIndex,
	geExtBox *ExtBox)
{
	geVec3d Min;
	geVec3d DX,DY,DZ,Corner;

	assert( ExtBox );
		
	if (geActor_GetBoneBoundingBoxByIndex(A,BoneIndex,&Min,&DX,&DY,&DZ)==GE_FALSE)
		{
			return GE_FALSE;
		}

	ExtBox->Min = Min;
	ExtBox->Max = Min;
	Corner = Min;
	// should use extent box (extbox) methods rather than this
	geVec3d_Add(&Corner,&DX,&Corner);
	geActor_AccumulateMinMax(&Corner,&(ExtBox->Min),&(ExtBox->Max));
	geVec3d_Add(&Corner,&DZ,&Corner);
	geActor_AccumulateMinMax(&Corner,&(ExtBox->Min),&(ExtBox->Max));
	geVec3d_Subtract(&Corner,&DX,&Corner);
	geActor_AccumulateMinMax(&Corner,&(ExtBox->Min),&(ExtBox->Max));
	geVec3d_Add(&Corner,&DY,&Corner);
	geActor_AccumulateMinMax(&Corner,&(ExtBox->Min),&(ExtBox->Max));
	geVec3d_Add(&Corner,&DX,&Corner);
	geActor_AccumulateMinMax(&Corner,&(ExtBox->Min),&(ExtBox->Max));
	geVec3d_Subtract(&Corner,&DZ,&Corner);
	geActor_AccumulateMinMax(&Corner,&(ExtBox->Min),&(ExtBox->Max));
	geVec3d_Subtract(&Corner,&DX,&Corner);
	geActor_AccumulateMinMax(&Corner,&(ExtBox->Min),&(ExtBox->Max));

	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geActor_GetBoneExtBox(const geActor *A,
									 const char *BoneName,
									 geExtBox *ExtBox)
{
	int BoneIndex;
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( ExtBox != NULL );
	
	if (geActor_GetBoneIndex(A,BoneName,&BoneIndex)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for bounding box not found", BoneName);
			return GE_FALSE;
		}
	return geActor_GetBoneExtBoxByIndex(A,BoneIndex,ExtBox);
}


GENESISAPI geBoolean GENESISCC geActor_GetBoneBoundingBox(const geActor *A,
								 const char *BoneName,
								 geVec3d *Corner,
								 geVec3d *DX,
								 geVec3d *DY,
								 geVec3d *DZ)
{
	int BoneIndex;
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( Corner    != NULL );
	assert( DX        != NULL );
	assert( DY        != NULL );
	assert( DZ        != NULL );

	if (geActor_GetBoneIndex(A,BoneName,&BoneIndex)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for bounding box not found", BoneName);
			return GE_FALSE;
		}
	if (geActor_GetBoneBoundingBoxByIndex(A,BoneIndex,Corner,DX,DY,DZ)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Failed to get bounding box named:", BoneName);
			//geErrorLog_AppendString(BoneName);
			return GE_FALSE;
		}
	return GE_TRUE;
}



GENESISAPI geBoolean GENESISCC geActor_GetExtBox(const geActor *A, geExtBox *ExtBox)
{
	geXForm3d Transform;
	
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( ExtBox != NULL );
	
	gePose_GetJointTransform(   A->Pose,
								A->BoundingBoxCenterBoneIndex,
								&Transform);	
	assert ( geXForm3d_IsOrthonormal(&Transform) != GE_FALSE );
	geVec3d_Add( &(Transform.Translation), &(A->BoundingBoxMinCorner), &(ExtBox->Min));
	geVec3d_Add( &(Transform.Translation), &(A->BoundingBoxMaxCorner), &(ExtBox->Max));
	return GE_TRUE;
}


GENESISAPI geBoolean GENESISCC geActor_SetExtBox(geActor *A,
												 const geExtBox *ExtBox,
												 const char *CenterOnThisNamedBone)
{
	
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( geExtBox_IsValid(ExtBox) != GE_FALSE);
	
	A->BoundingBoxMinCorner = ExtBox->Min;
	A->BoundingBoxMaxCorner = ExtBox->Max;
	
	if (geActor_GetBoneIndex(A,CenterOnThisNamedBone,&(A->BoundingBoxCenterBoneIndex))==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for bounding box not found", CenterOnThisNamedBone);
			return GE_FALSE;
		}
	
	return GE_TRUE;
}


	// Gets the rendering hint bounding box from the actor
GENESISAPI geBoolean GENESISCC geActor_GetRenderHintExtBox(const geActor *A, geExtBox *Box, geBoolean *Enabled)
{
	geXForm3d Transform;

	assert( geActor_IsValid(A) != GE_FALSE);
	assert( Box != NULL );
	assert( Enabled != NULL );

	gePose_GetJointTransform( A->Pose,
								A->RenderHintExtBoxCenterBoneIndex,
								&Transform);	
	assert ( geXForm3d_IsOrthonormal(&Transform) != GE_FALSE );

	*Box = A->RenderHintExtBox;
	geExtBox_Translate ( Box, Transform.Translation.X,
							  Transform.Translation.Y,
							  Transform.Translation.Z );
	
	*Enabled = A->RenderHintExtBoxEnabled;
	return GE_TRUE;
}

	// Sets a rendering hint bounding box from the actor
GENESISAPI geBoolean GENESISCC geActor_SetRenderHintExtBox(geActor *A, const geExtBox *Box, 
												const char *CenterOnThisNamedBone)
{
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( Box != NULL );
	assert( Box->Max.X >= Box->Min.X );
	assert( Box->Max.Y >= Box->Min.Y );
	assert( Box->Max.Z >= Box->Min.Z );
	
	if (geActor_GetBoneIndex(A,CenterOnThisNamedBone,&(A->RenderHintExtBoxCenterBoneIndex))==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for render hint box not found", CenterOnThisNamedBone);
			return GE_FALSE;
		}
		
	A->RenderHintExtBox = *Box;
	if (   (Box->Min.X == 0.0f) && (Box->Max.X == 0.0f)
		&& (Box->Min.Y == 0.0f) && (Box->Max.Y == 0.0f) 
		&& (Box->Min.Z == 0.0f) && (Box->Max.Z == 0.0f) )
		{
			A->RenderHintExtBoxEnabled = GE_FALSE;
		}
	else
		{
			A->RenderHintExtBoxEnabled = GE_TRUE;
		}

	return GE_TRUE;
}


GENESISAPI void *GENESISCC geActor_GetUserData(const geActor *A)
{
	assert( geActor_IsValid(A) != GE_FALSE);
	return A->UserData;
}

GENESISAPI void GENESISCC geActor_SetUserData(geActor *A, void *UserData)
{
	assert( geActor_IsValid(A) != GE_FALSE);
	A->UserData = UserData;
}

#define MAX(aa,bb)   ( (aa)>(bb)?(aa):(bb) )
#define MIN(aa,bb)   ( (aa)<(bb)?(aa):(bb) )

static void GENESISCC geActor_StretchBoundingBox( geVec3d *Min, geVec3d *Max,
							const geVec3d *Corner, 
							const geVec3d *DX, const geVec3d *DY, const geVec3d *DZ)
{
	geVec3d P;

	P = *Corner;
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);

	geVec3d_Add     (Corner ,DX,&P);
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);

	geVec3d_Add     (&P, DZ,&P);
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);

	geVec3d_Subtract(&P,DX,&P);
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);

	geVec3d_Add     (&P,DY,&P);
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);

	geVec3d_Add     (&P,DX,&P);
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);

	geVec3d_Subtract(&P,DZ,&P);
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);

	geVec3d_Subtract(&P,DX,&P);
	Min->X = MIN(Min->X,P.X);	Min->Y = MIN(Min->Y,P.Y);	Min->Z = MIN(Min->Z,P.Z);
	Max->X = MAX(Max->X,P.X);	Max->Y = MAX(Max->Y,P.Y);	Max->Z = MAX(Max->Z,P.Z);
}

GENESISAPI geBoolean GENESISCC geActor_GetDynamicExtBox( const geActor *A, geExtBox *ExtBox)
{
#define GE_ACTOR_REALLY_BIG_NUMBER (9e9f)

	geVec3d Corner;
	geVec3d DX;
	geVec3d DY;
	geVec3d DZ;
	int Count,i,BCount;

	assert( geActor_IsValid(A) != GE_FALSE);
	assert( A->ActorDefinition->Body   != NULL );

	geVec3d_Set(&(ExtBox->Min),
			GE_ACTOR_REALLY_BIG_NUMBER,GE_ACTOR_REALLY_BIG_NUMBER,GE_ACTOR_REALLY_BIG_NUMBER);
	geVec3d_Set(&(ExtBox->Max),
			-GE_ACTOR_REALLY_BIG_NUMBER,-GE_ACTOR_REALLY_BIG_NUMBER,-GE_ACTOR_REALLY_BIG_NUMBER);
		
	BCount = 0;
	Count = geBody_GetBoneCount( A->ActorDefinition->Body );
	for (i=0; i< Count; i++)
		{
			if (geActor_GetBoneBoundingBoxByIndex(A,i,&Corner,&DX,&DY,&DZ)!=GE_FALSE)
				{
					geActor_StretchBoundingBox( &(ExtBox->Min),
												&(ExtBox->Max),&Corner,&DX,&DY,&DZ);
					BCount ++;
				}
		}
	if (BCount>0)
		{
			return GE_TRUE;
		}
	return GE_FALSE;
}



GENESISAPI geBoolean GENESISCC geActor_Attach( geActor *Slave,  const char *SlaveBoneName,
						const geActor *Master, const char *MasterBoneName, 
						const geXForm3d *Attachment)
{
	int SlaveBoneIndex,MasterBoneIndex;

	assert( geActor_IsValid(Slave) != GE_FALSE);
	assert( geActor_IsValid(Master) != GE_FALSE);
	assert( geXForm3d_IsOrthonormal(Attachment) != GE_FALSE );
	
	assert( MasterBoneName != NULL );		// might this be possible?
	
	if (geActor_GetBoneIndex(Slave,SlaveBoneName,&(SlaveBoneIndex))==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for slave not found", SlaveBoneName);
			return GE_FALSE;
		}
	
	if (geActor_GetBoneIndex(Master,MasterBoneName,&(MasterBoneIndex))==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for master not found", MasterBoneName);
			return GE_FALSE;
		}
	
	return gePose_Attach(   Slave->Pose,   SlaveBoneIndex,
							Master->Pose, MasterBoneIndex, 
							Attachment);
}


GENESISAPI void GENESISCC geActor_Detach(geActor *A)
{
	assert( geActor_IsValid(A) != GE_FALSE);

	gePose_Detach( A->Pose );
}


GENESISAPI geBoolean GENESISCC geActor_GetBoneAttachment(const geActor *A,
								const char *BoneName,
								geXForm3d *Attachment)
{

	int BoneIndex;
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( Attachment != NULL );
	
	if (geActor_GetBoneIndex(A,BoneName,&(BoneIndex))==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone not found", BoneName);
			return GE_FALSE;
		}
	
	gePose_GetJointAttachment(A->Pose,BoneIndex, Attachment);
	assert ( geXForm3d_IsOrthonormal(Attachment) != GE_FALSE );

	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geActor_SetBoneAttachment(geActor *A,
								const char *BoneName,
								geXForm3d *Attachment)
{

	int BoneIndex;

	assert( geActor_IsValid(A) != GE_FALSE);
	assert( geXForm3d_IsOrthonormal(Attachment) != GE_FALSE );
	
	if (geActor_GetBoneIndex(A,BoneName,&(BoneIndex))==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone not found", BoneName);
			return GE_FALSE;
		}
	
	gePose_SetJointAttachment(A->Pose,BoneIndex, Attachment);
	return GE_TRUE;
}


//-------------------------------------------------------------------------------------------------
// Actor Cuing system
//-------------------------------------------------------------------------------------------------
#define ACTOR_CUE_MINIMUM_BLEND (0.0001f)
#define ACTOR_CUE_MAXIMUM_BLEND (0.999f)


static geBoolean GENESISCC geActor_IsAnimationCueDead(geActor *A, int Index)
{
	geBoolean Kill= GE_FALSE;
	geFloat BlendAmount;
	geMotion *M;
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( (Index>=0) && (Index<geMotion_GetSubMotionCount(A->CueMotion)));

	M = A->CueMotion;

	BlendAmount = geMotion_GetBlendAmount(M,Index,0.0f);
	if (BlendAmount <= ACTOR_CUE_MINIMUM_BLEND)
		{
			int KeyCount;
			gePath *P; 
			geFloat KeyTime;

			P = geMotion_GetBlendPath(M,Index);
			assert( P != NULL );
			KeyCount = gePath_GetKeyframeCount(P,GE_PATH_TRANSLATION_CHANNEL);
			if (KeyCount>0)
				{
					geXForm3d Dummy;
					geFloat TimeOffset = -geMotion_GetTimeOffset( M, Index);
					gePath_GetKeyframe( P, KeyCount-1, GE_PATH_TRANSLATION_CHANNEL, &KeyTime, &Dummy );
	
					if ( KeyTime <= TimeOffset )
						{
							Kill = GE_TRUE;
						}
				}
			else
				{
					Kill = GE_TRUE;
				}
		}
	return Kill;
}


static void GENESISCC geActor_KillCue( geActor *A, int Index )
{
	geMotion *M;
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( (Index>=0) && (Index<geMotion_GetSubMotionCount(A->CueMotion)));
	M  = geMotion_RemoveSubMotion(A->CueMotion,Index);
}

GENESISAPI geBoolean GENESISCC geActor_AnimationNudge(geActor *A, geXForm3d *Offset)
{
	geMotion *M;
	int i,Count;
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( geXForm3d_IsOrthonormal(Offset) != GE_FALSE );
	M = A->CueMotion;
	Count = geMotion_GetSubMotionCount(M);
	
	for (i=Count-1; i>=0; i--)	
		{
			geXForm3d Transform;
			const geXForm3d *pTransform;
			pTransform = geMotion_GetBaseTransform( M, i );
			if ( pTransform != NULL )
				{
					Transform = *pTransform;
			
					geXForm3d_Multiply(Offset,&Transform,&Transform);
					geXForm3d_Orthonormalize(&Transform);

					geMotion_SetBaseTransform( M, i, &Transform);
				}
		}
	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geActor_AnimationRemoveLastCue( geActor *A )
{
	int Count;
	assert( geActor_IsValid(A) != GE_FALSE);
	Count = geMotion_GetSubMotionCount(A->CueMotion);
	if (Count>0)
		{
			geActor_KillCue( A, Count-1 );
			return GE_TRUE;
		}
	return GE_FALSE;
}

GENESISAPI geBoolean GENESISCC geActor_AnimationCue( geActor *A, 
								geMotion *Motion,
								geFloat TimeScaleFactor,
								geFloat TimeIntoMotion,
								geFloat BlendTime, 
								geFloat BlendFromAmount, 
								geFloat BlendToAmount,
								const geXForm3d *MotionTransform)
{
	int Index;

	assert( geActor_IsValid(A) != GE_FALSE);

	assert( (BlendFromAmount>=0.0f) && (BlendFromAmount<=1.0f));
	assert( (  BlendToAmount>=0.0f) && (  BlendToAmount<=1.0f));
	assert( (MotionTransform==NULL) || (geXForm3d_IsOrthonormal(MotionTransform)) != GE_FALSE );

	assert( Motion != NULL );
	
	assert( BlendTime >= 0.0f);
	if (BlendTime==0.0f)
		{
			BlendFromAmount = BlendToAmount;
			BlendTime = 1.0f;	// anything that is > GE_TKA_TIME_TOLERANCE
		}

	if (geMotion_AddSubMotion( A->CueMotion, TimeScaleFactor, -TimeIntoMotion, Motion, 
							TimeIntoMotion, BlendFromAmount, 
							TimeIntoMotion + BlendTime, BlendToAmount, 
							MotionTransform, &Index )==GE_FALSE)
		{	
			return GE_FALSE;
		}
		
	return GE_TRUE;
}


GENESISAPI geBoolean GENESISCC geActor_AnimationStep(geActor *A, geFloat DeltaTime )
{
	int i,Coverage,Count;
	geMotion *M;
	geMotion *SubM;
	
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( DeltaTime >= 0.0f );
	
	gePose_ClearCoverage(A->Pose,0);

	M = A->CueMotion;

	Count = geMotion_GetSubMotionCount(M);

	for (i=Count-1; i>=0; i--)	
		{
			geFloat TimeOffset = geMotion_GetTimeOffset( M, i );
			TimeOffset = TimeOffset - DeltaTime;
			geMotion_SetTimeOffset( M, i, TimeOffset);

			if (geActor_IsAnimationCueDead(A,i))
				{
					geActor_KillCue(A,i);
				}
			else
				{
					geBoolean SetWithBlending= GE_TRUE;
					geFloat BlendAmount;
					
					SubM = geMotion_GetSubMotion(M,i);
					assert( SubM != NULL );
					
					BlendAmount = geMotion_GetBlendAmount( M,i,0.0f );
					
					if (BlendAmount >= ACTOR_CUE_MAXIMUM_BLEND)
						{
							SetWithBlending = GE_FALSE;
						}
					Coverage = gePose_AccumulateCoverage(A->Pose,SubM, SetWithBlending);
					if ( Coverage == 0 )
						{
							geActor_KillCue(A,i);
						}
				}
		}

	gePose_SetMotion( A->Pose, M, 0.0f, NULL );
	geMotion_SetupEventIterator(M,-DeltaTime,0.0f);

	return GE_TRUE;
}


GENESISAPI geBoolean GENESISCC geActor_AnimationStepBoneOptimized(geActor *A, geFloat DeltaTime, const char *BoneName )
{
	int i,Coverage,Count;
	geMotion *M;
	geMotion *SubM;
	
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( DeltaTime >= 0.0f );
	
	if (BoneName == NULL)
		{
			A->StepBoneIndex = GE_POSE_ROOT_JOINT;
		}
	else
		{
			geBoolean LookupBoneName= GE_TRUE;
			const char *LastBoneName;
			geXForm3d Attachment;
			int LastParentBoneIndex;
			if (A->StepBoneIndex >= 0)
				{
					geBody_GetBone(	A->ActorDefinition->Body,A->StepBoneIndex,&LastBoneName,&Attachment,&LastParentBoneIndex);
					if (  (LastBoneName != NULL) )
						if (strcmp(LastBoneName,BoneName)==0) 
							LookupBoneName = GE_FALSE;
				}
			if (LookupBoneName != GE_FALSE)
				{
					if (geActor_GetBoneIndex(A,BoneName,&(A->StepBoneIndex))==GE_FALSE)
						{
							geErrorLog_AddString(-1,"Named bone not found", BoneName);
							return GE_FALSE;
						}
				}
		}
			

	gePose_ClearCoverage(A->Pose,0);

	M = A->CueMotion;

	Count = geMotion_GetSubMotionCount(M);

	for (i=Count-1; i>=0; i--)	
		{
			geFloat TimeOffset = geMotion_GetTimeOffset( M, i );
			TimeOffset = TimeOffset - DeltaTime;
			geMotion_SetTimeOffset( M, i, TimeOffset);

			if (geActor_IsAnimationCueDead(A,i))
				{
					geActor_KillCue(A,i);
				}
			else
				{
					geBoolean SetWithBlending= GE_TRUE;
					geFloat BlendAmount;
					
					SubM = geMotion_GetSubMotion(M,i);
					assert( SubM != NULL );
					
					BlendAmount = geMotion_GetBlendAmount( M,i,0.0f );
					
					if (BlendAmount >= ACTOR_CUE_MAXIMUM_BLEND)
						{
							SetWithBlending = GE_FALSE;
						}
					Coverage = gePose_AccumulateCoverage(A->Pose,SubM, SetWithBlending);
					if ( Coverage == 0 )
						{
							geActor_KillCue(A,i);
						}
				}
		}

	gePose_SetMotionForABone( A->Pose, M, 0.0f, NULL, A->StepBoneIndex );
	geMotion_SetupEventIterator(M,-DeltaTime,0.0f);

	return GE_TRUE;
}


		
GENESISAPI geBoolean GENESISCC geActor_AnimationTestStep(geActor *A, geFloat DeltaTime)
{
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( DeltaTime >= 0.0f );

	gePose_SetMotion( A->Pose, A->CueMotion , DeltaTime, NULL );

	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geActor_AnimationTestStepBoneOptimized(geActor *A, geFloat DeltaTime, const char *BoneName)
{
	assert( geActor_IsValid(A) != GE_FALSE);
	assert( DeltaTime >= 0.0f );

	if (BoneName == NULL)
		{
			A->StepBoneIndex = GE_POSE_ROOT_JOINT;
		}
	else
		{
			geBoolean LookupBoneName= GE_TRUE;
			const char *LastBoneName;
			geXForm3d Attachment;
			int LastParentBoneIndex;
			if (A->StepBoneIndex >= 0)
				{
					geBody_GetBone(	A->ActorDefinition->Body,A->StepBoneIndex,&LastBoneName,&Attachment,&LastParentBoneIndex);
					if (  (LastBoneName != NULL) )
						if (strcmp(LastBoneName,BoneName)==0) 
							LookupBoneName = GE_FALSE;
				}
			if (LookupBoneName != GE_FALSE)
				{
					if (geActor_GetBoneIndex(A,BoneName,&(A->StepBoneIndex))==GE_FALSE)
						{
							geErrorLog_AddString(-1,"Named bone not found", BoneName);
							return GE_FALSE;
						}
				}
		}
	gePose_SetMotionForABone( A->Pose, A->CueMotion , DeltaTime, NULL,A->StepBoneIndex );

	return GE_TRUE;
}



GENESISAPI geBoolean GENESISCC geActor_GetAnimationEvent(
	geActor *A,						
	const char **ppEventString)		// Return data, if found
	// returns the event string for the 'next' event that occured during the last 
	// animation step time delta.
	// if the return value is GE_FALSE, there are no more events, and ppEventString will be Empty
{
	geFloat Time;
	assert( geActor_IsValid(A) != GE_FALSE);

	return geMotion_GetNextEvent(A->CueMotion, &Time, ppEventString );
}


#ifndef NDEBUG
static geBoolean GENESISCC geActor_TransformCompare(const geXForm3d *T1, const geXForm3d *T2)
{
	#define SINGLE_TERM_ERROR_THRESHOLD (0.0001f)
	if (fabs(T1->AX - T2->AX)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->BX - T2->BX)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->CX - T2->CX)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->AY - T2->AY)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->BY - T2->BY)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->CY - T2->CY)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->AZ - T2->AZ)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->BZ - T2->BZ)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->CZ - T2->CZ)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	
	if (fabs(T1->Translation.X - T2->Translation.X)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->Translation.Y - T2->Translation.Y)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	if (fabs(T1->Translation.Z - T2->Translation.Z)>SINGLE_TERM_ERROR_THRESHOLD) return GE_FALSE;
	return GE_TRUE;
}
#endif



GENESISAPI geBoolean GENESISCC geActor_GetLightingOptions(const geActor *Actor,
	geBoolean *UseFillLight,
	geVec3d *FillLightNormal,
	geFloat *FillLightRed,				
	geFloat *FillLightGreen,				
	geFloat *FillLightBlue,				
	geFloat *AmbientLightRed,			
	geFloat *AmbientLightGreen,			
	geFloat *AmbientLightBlue,			
	geBoolean *UseAmbientLightFromFloor,
	int *MaximumDynamicLightsToUse,		
	const char **LightReferenceBoneName,
	geBoolean *PerBoneLighting)
{
	int BoneIndex;
	assert( geActor_IsValid(Actor)!=GE_FALSE );

	assert( UseFillLight != NULL );
	assert( FillLightNormal != NULL );
	assert( FillLightRed != NULL );	
	assert( FillLightGreen != NULL );	
	assert( FillLightBlue != NULL );	
	assert( AmbientLightRed != NULL );
	assert( AmbientLightGreen != NULL );			
	assert( AmbientLightBlue != NULL );			
	assert( UseAmbientLightFromFloor != NULL );
	assert( MaximumDynamicLightsToUse != NULL );	
	assert( LightReferenceBoneName != NULL );

	if (Actor->Puppet == NULL)
		{
			geErrorLog_AddString(-1,"Can't set lighting options until actor is prepared for rendering", NULL);
			return GE_FALSE;
		}
	
	gePuppet_GetLightingOptions(	Actor->Puppet,
									UseFillLight,
									FillLightNormal,
									FillLightRed,	
									FillLightGreen,	
									FillLightBlue,	
									AmbientLightRed,
									AmbientLightGreen,		
									AmbientLightBlue,		
									UseAmbientLightFromFloor,
									MaximumDynamicLightsToUse,
									&BoneIndex,
									PerBoneLighting);

	if (BoneIndex>=0 && (BoneIndex < geBody_GetBoneCount(Actor->ActorDefinition->Body)))
		{
			geXForm3d DummyAttachment;
			int DummyParentBoneIndex;
			geBody_GetBone(	Actor->ActorDefinition->Body,
							BoneIndex,
							LightReferenceBoneName,
							&DummyAttachment,
							&DummyParentBoneIndex);
		}
	else
		{
			LightReferenceBoneName = NULL;
		}

	return GE_TRUE; // CB
}

GENESISAPI geBoolean GENESISCC geActor_SetLightingOptions(geActor *A,
	geBoolean UseFillLight,
	const geVec3d *FillLightNormal,
	geFloat FillLightRed,				// 0 .. 255
	geFloat FillLightGreen,				// 0 .. 255
	geFloat FillLightBlue,				// 0 .. 255
	geFloat AmbientLightRed,			// 0 .. 255
	geFloat AmbientLightGreen,			// 0 .. 255
	geFloat AmbientLightBlue,			// 0 .. 255
	geBoolean AmbientLightFromFloor,
	int MaximumDynamicLightsToUse,		// 0 for none
	const char *LightReferenceBoneName,
	geBoolean PerBoneLighting	)
{
	int BoneIndex;

	assert( geActor_IsValid(A)!=GE_FALSE );
	assert( FillLightNormal != NULL );
	
	if (A->Puppet == NULL)
		{
			geErrorLog_AddString(-1,"Can't set lighting options until actor is prepared for rendering", NULL);
			return GE_FALSE;
		}
	if (geActor_GetBoneIndex(A,LightReferenceBoneName,&BoneIndex)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for light reference not found", LightReferenceBoneName);
			return GE_FALSE;
		}

	gePuppet_SetLightingOptions(	A->Puppet,
									UseFillLight,
									FillLightNormal,
									FillLightRed,	
									FillLightGreen,	
									FillLightBlue,	
									AmbientLightRed,
									AmbientLightGreen,		
									AmbientLightBlue,		
									AmbientLightFromFloor,
									MaximumDynamicLightsToUse,
									BoneIndex,
									PerBoneLighting);
	return GE_TRUE;
}

GENESISAPI void GENESISCC geActor_SetScale(geActor *A, geFloat ScaleX,geFloat ScaleY,geFloat ScaleZ)
{
	geVec3d S;
	assert( A != NULL );
		
	geVec3d_Set(&S,ScaleX,ScaleY,ScaleZ);
	gePose_SetScale(A->Pose,&S);
}



GENESISAPI geBoolean GENESISCC geActor_SetShadow(geActor *A, 
		geBoolean DoShadow, 
		geFloat Radius,
		const geBitmap *ShadowMap,
		const char *BoneName)
{
	int BoneIndex;

	assert( geActor_IsValid(A)!=GE_FALSE );
	assert( (DoShadow==GE_FALSE) || (DoShadow==GE_TRUE));
	assert( Radius >= 0.0f);
	
	if (A->Puppet == GE_FALSE)
		{
			geErrorLog_AddString(-1,"Can't set shadow options until actor is prepared for rendering", NULL);
			return GE_FALSE;
		}
	
	if (geActor_GetBoneIndex(A,BoneName,&BoneIndex)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Named bone for shadow not found", BoneName);
			return GE_FALSE;
		}

	gePuppet_SetShadow(A->Puppet,DoShadow,Radius,ShadowMap,BoneIndex);

	return GE_TRUE;
}


geBoolean GENESISCC geActor_RenderPrep( geActor *A, geWorld *World)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	#pragma message ("need to make a world method that does this. so World doesn't get passed in")
	assert( geActor_DefIsValid(A->ActorDefinition) != GE_FALSE );
	assert( geBody_IsValid(A->ActorDefinition->Body) != GE_FALSE );

	if (A->Puppet!=NULL)
		{
			gePuppet_Destroy(&(A->Puppet));
			A->Puppet =NULL;
		}
		
	A->Puppet = gePuppet_Create(A->ActorDefinition->TextureFileContext, A->ActorDefinition->Body,World);
	if ( A->Puppet == NULL )
		{
			geErrorLog_Add( ERR_ACTOR_RENDER_PREP , NULL);
			return GE_FALSE;
		}
	{
		geExtBox EB;
		if (geActor_GetBoneExtBoxByIndex(A,GE_POSE_ROOT_JOINT,&EB) == GE_FALSE)
			{
				geErrorLog_AddString(-1,"Failure to get Root Bounding box from puppet", NULL);
				return GE_FALSE;			
			}
		
		A->BoundingBoxMinCorner = EB.Min;
		A->BoundingBoxMaxCorner = EB.Max;
	}
	
	return GE_TRUE;
}

geBoolean GENESISCC geActor_RenderThroughFrustum(const geActor *A, geEngine *Engine, geWorld *World,geCamera *Camera, Frustum_Info *FInfo)
{
	geExtBox	Box;

	assert( geActor_IsValid(A) != GE_FALSE );
	assert( A->Puppet != NULL );

	if (!geActor_GetDynamicExtBox( A, &Box))
	{
			geErrorLog_AddString(-1, "geActor_RenderThroughFrustum:  geActor_GetDynamicAABoundingBox failed.", NULL);
			return GE_FALSE;
	}

	if (gePuppet_RenderThroughFrustum( A->Puppet, A->Pose, &Box, Engine, World, Camera, FInfo)==GE_FALSE)
		{
			geErrorLog_Add( ERR_ACTOR_RENDER_FAILED , NULL);
			return GE_FALSE;
		}
	return GE_TRUE;
}

geBoolean GENESISCC geActor_Render(const geActor *A, geEngine *Engine, geWorld *World,geCamera *Camera)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	assert( A->Puppet != NULL );

	if (A->RenderHintExtBoxEnabled)
		{
			geBoolean Enabled;
			geExtBox Box;
			if (geActor_GetRenderHintExtBox(A, &Box, &Enabled)==GE_FALSE)
				{
					geErrorLog_Add( -1 , NULL);	//?
					return GE_FALSE;
				}
			if (gePuppet_Render( A->Puppet, A->Pose, Engine,World, Camera, &Box )==GE_FALSE)
				{
					geErrorLog_Add( ERR_ACTOR_RENDER_FAILED , NULL);
					return GE_FALSE;
				}
		}
	else
		{
			if (gePuppet_Render( A->Puppet, A->Pose, Engine,World, Camera, NULL )==GE_FALSE)
				{
					geErrorLog_Add( ERR_ACTOR_RENDER_FAILED , NULL);
					return GE_FALSE;
				}
		}
	return GE_TRUE;
}

GENESISAPI int GENESISCC geActor_GetMaterialCount(const geActor *A)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	assert( A->Puppet != NULL );

	return gePuppet_GetMaterialCount( A->Puppet );
}

GENESISAPI geBoolean GENESISCC geActor_GetMaterial(const geActor *A, int MaterialIndex,
										geBitmap **Bitmap, geFloat *Red, geFloat *Green, geFloat *Blue)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	assert( A->Puppet != NULL );

	return gePuppet_GetMaterial(A->Puppet, MaterialIndex, Bitmap, Red, Green, Blue );
}


GENESISAPI geBoolean GENESISCC geActor_SetMaterial(geActor *A, int MaterialIndex,
										geBitmap *Bitmap,  geFloat Red,  geFloat Green,  geFloat Blue)
{
	assert( geActor_IsValid(A) != GE_FALSE );
	assert( A->Puppet != NULL );

	return gePuppet_SetMaterial(A->Puppet,MaterialIndex, Bitmap, Red, Green, Blue );
}
