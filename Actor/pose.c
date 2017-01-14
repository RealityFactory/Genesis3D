/****************************************************************************************/
/*  POSE.C																				*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Bone hierarchy implementation.							.				*/
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
#pragma message ("could optimize a name binded setPose by caching the mapping from motionpath[i] to joint[j]")

#include <assert.h>
#include <string.h>

#include "ram.h"
#include "errorlog.h"
#include "Pose.h"
#include "StrBlock.h"

#define GE_POSE_STARTING_JOINT_COUNT (1)


/* this object maintains a hierarchy of joints.
   the hierarchy is stored as an array of joints, with each joint having an number
   that is it's parent's index in the array.  
   This code assumes:  
   **The parent's index is always smaller than the child**
*/

typedef struct gePose_Joint
{
	int			 ParentJoint;		// parent of path
	geXForm3d    *Transform;		// matrix for path	(pointer into TransformArray)
	geQuaternion Rotation;			// quaternion representation for orientation of above Transform

	geVec3d		 UnscaledAttachmentTranslation;	
					// point of Attachment to parent (in parent frame of ref) **Unscaled
	geQuaternion AttachmentRotation;// rotation of attachement to parent (in parent frame of ref)
	geXForm3d    AttachmentTransform;	//------------

	geVec3d		 LocalTranslation;	// translation relative to attachment 
	geQuaternion LocalRotation;		// rotation relative to attachment 

	geBoolean    Touched;			// if this joint has been touched and needs recomputation
	geBoolean    NoAttachmentRotation; // GE_TRUE if there is no attachment rotation.
	int			 Covered;			// if joint has been 100% set (no blending)
} gePose_Joint;						// structure to bind a name and a path for a joint

typedef struct gePose
{
	int				  JointCount;	// number of joints in the motion
	int32			  NameChecksum;	// checksum based on joint names and list order
	geBoolean		  Touched;		// if any joint has been touched & needs recomputation	
	geStrBlock		 *JointNames;
	geVec3d			  Scale;		// current scaling. Used for scaling motion samples

	geBoolean		  Slave;			// if pose is 'slaved' to parent -vs- attached.
	int				  SlaveJointIndex;	// index of 'slaved' joint
	gePose			 *Parent;		
	gePose_Joint	  RootJoint;		
	geXForm3d		  ParentsLastTransform;	// Compared to parent's transform to see if it changed: recompute is needed
	geXForm3d		  RootTransform;
	geXFArray		 *TransformArray;	
	gePose_Joint	 *JointArray;
	int				  OnlyThisJoint;		// update only this joint (and it's parents) if this is >0
} gePose;



static void gePose_ReattachTransforms(gePose *P)
{
	int XFormCount;
	int JointCount;
	geXForm3d *XForms;
	int i;

	assert( P != NULL );

	JointCount = P->JointCount;
	if (JointCount > 0)
		{
			assert( P->TransformArray != NULL );

			XForms = geXFArray_GetElements(P->TransformArray,&XFormCount);
			
			assert( XForms != NULL );
			assert( XFormCount == JointCount );
			
			for (i=0; i<JointCount; i++)
				{
					P->JointArray[i].Transform=&(XForms[i]);
				}
		}
	P->RootJoint.Transform = &(P->RootTransform);
}
	

static const gePose_Joint *gePose_JointByIndex(const gePose *P, int Index)
{
	assert( P != NULL );
	assert( (Index >=0)                 || (Index==(GE_POSE_ROOT_JOINT)));
	assert( (Index < P->JointCount)     || (Index==(GE_POSE_ROOT_JOINT)));

	if (Index == GE_POSE_ROOT_JOINT)
		{
			return &(P->RootJoint);
		}
	else
		{
			return &(P->JointArray[Index]);
		}
}

static void GENESISCC gePose_SetAttachmentRotationFlag( gePose_Joint *Joint)
{
	geQuaternion Q = Joint->AttachmentRotation;
#define GE_POSE_ROTATION_THRESHOLD (0.0001)  // if the rotation is closer than this to zero for
										     // quaterion elements X,Y,Z -> no rotation computed
	if (     (  (Q.X<GE_POSE_ROTATION_THRESHOLD) && (Q.X>-GE_POSE_ROTATION_THRESHOLD) ) 
		  && (  (Q.Y<GE_POSE_ROTATION_THRESHOLD) && (Q.Y>-GE_POSE_ROTATION_THRESHOLD) ) 
		  && (  (Q.Z<GE_POSE_ROTATION_THRESHOLD) && (Q.Z>-GE_POSE_ROTATION_THRESHOLD) )  )
		{
			Joint->NoAttachmentRotation = GE_TRUE;
		}
	else
		{
			Joint->NoAttachmentRotation = GE_FALSE;
		}
}

static void GENESISCC gePose_InitializeJoint(gePose_Joint *Joint, int ParentJointIndex, const geXForm3d *Attachment)
{
	assert( Joint != NULL );
	
	Joint->ParentJoint = ParentJointIndex;
	if (Attachment != NULL)
		{
			geQuaternion_FromMatrix(Attachment,&(Joint->AttachmentRotation));
			Joint->AttachmentTransform = *Attachment;
			Joint->UnscaledAttachmentTranslation = Joint->AttachmentTransform.Translation;
		}
	else
		{
			geQuaternion_SetNoRotation(&(Joint->AttachmentRotation));
			geXForm3d_SetIdentity(&(Joint->AttachmentTransform));
			Joint->UnscaledAttachmentTranslation = Joint->AttachmentTransform.Translation;
		}

	geQuaternion_SetNoRotation(&(Joint->LocalRotation));
	
	geXForm3d_SetIdentity(Joint->Transform);
	geQuaternion_SetNoRotation(&(Joint->Rotation));
	
	geVec3d_Set( (&Joint->LocalTranslation),0.0f,0.0f,0.0f);
	geQuaternion_SetNoRotation(&(Joint->LocalRotation));
	Joint->Touched = GE_TRUE;		
	gePose_SetAttachmentRotationFlag(Joint);
}



gePose *GENESISCC gePose_Create(void)
{
	gePose *P;

	P = GE_RAM_ALLOCATE_STRUCT(gePose);

	if ( P == NULL )
		{
			geErrorLog_Add(ERR_POSE_CREATE_ENOMEM, NULL);
			goto PoseCreateFailure;
		}
	P->JointCount = 0;
	P->OnlyThisJoint = GE_POSE_ROOT_JOINT-1;		
	P->JointNames = geStrBlock_Create();
	P->Touched = GE_FALSE;
	if ( P->JointNames == NULL )
		{
			geErrorLog_Add(ERR_POSE_CREATE_ENOMEM, NULL);
			goto PoseCreateFailure;
		}
	P->JointArray = GE_RAM_ALLOCATE_STRUCT( gePose_Joint );
	if (P->JointArray == NULL)
		{
			geErrorLog_Add(ERR_POSE_CREATE_ENOMEM, NULL);
			goto PoseCreateFailure;
		}
	P->TransformArray=NULL;

	P->Slave = GE_FALSE;
	P->Parent = NULL;
	gePose_ReattachTransforms(P);
	gePose_InitializeJoint(&(P->RootJoint),GE_POSE_ROOT_JOINT,NULL);

	P->Scale.X = P->Scale.Y = P->Scale.Z = 1.0f;
	return P;
	PoseCreateFailure:
	if (P!=NULL)
		{
			if (P->JointNames != NULL)
				geStrBlock_Destroy(&(P->JointNames));
			if (P->JointArray != NULL)
				geRam_Free(P->JointArray);
			geRam_Free(P);
		}
	return NULL;
}

void GENESISCC gePose_Destroy(gePose **PP)
{
	assert(PP   != NULL );
	assert(*PP  != NULL );

	assert( (*PP)->JointNames != NULL );
	assert( geStrBlock_GetCount((*PP)->JointNames) == (*PP)->JointCount );
	geStrBlock_Destroy( &( (*PP)->JointNames ) );
	if ((*PP)->TransformArray!=NULL)
		{
			geXFArray_Destroy(&( (*PP)->TransformArray) );
		}
	if ((*PP)->JointArray != NULL)
		geRam_Free((*PP)->JointArray);
	geRam_Free( *PP );

	*PP = NULL;
}

// uses J->LocalRotation and J->LocalTranslation to compute 
//    J->Rotation,J->Translation and J->Transform
static void GENESISCC gePose_JointRelativeToParent(
		 const gePose_Joint *Parent,
		 gePose_Joint *J)
{
	
	#if 0
		// the math in clearer (but slower) matrix form.
		// W = PAK
		geXForm3d X;
		geXForm3d K;

		geQuaternion_ToMatrix(&(J->LocalRotation),&K);
		K.Translation = J->LocalTranslation;

		geXForm3d_Multiply((Parent->Transform),&(J->AttachmentTransform),&X);
		geXForm3d_Multiply(&X,&(K),J->Transform);
		
		J->LocalTranslation = K.Translation;
		geQuaternion_FromMatrix(J->Transform,&(J->LocalRotation));

	#endif


	geVec3d *Translation = &(J->Transform->Translation);
	if (J->NoAttachmentRotation != GE_FALSE)
		{
			//    ( no attachment rotation )
			//ROTATION:
			// concatenate local rotation to parent rotation for complete rotation
			geQuaternion_Multiply(&(Parent->Rotation), &(J->LocalRotation), &(J->Rotation));
			
			geQuaternion_ToMatrix(&(J->Rotation), (J->Transform));
			//TRANSLATION:
			geVec3d_Add(&(J->LocalTranslation),&(J->AttachmentTransform.Translation),Translation);
			geXForm3d_Transform((Parent->Transform),Translation,Translation);
		}
	else
		{
			//  (there is an attachment rotation)
			
			geQuaternion BaseRotation; // attachement transform applied to the parent transform:
			//ROTATION:
			// concatenate attachment rotation to parent rotation for base rotation
			geQuaternion_Multiply(&(Parent->Rotation),&(J->AttachmentRotation),&BaseRotation);
			// concatenate base rotation with local rotation for complete rotation
			geQuaternion_Multiply(&BaseRotation, &(J->LocalRotation), &(J->Rotation));

			geQuaternion_ToMatrix(&(J->Rotation), (J->Transform));

			//TRANSLATION:
			geXForm3d_Transform(&(J->AttachmentTransform),&(J->LocalTranslation),Translation);
			geXForm3d_Transform((Parent->Transform),Translation,Translation);
		}
}


geBoolean GENESISCC gePose_Attach(gePose *Slave, int SlaveBoneIndex,
				  gePose *Master, int MasterBoneIndex, 
				  const geXForm3d *Attachment)
{
	gePose *P;
	P = Master;

	assert( Slave != NULL );
	assert( Master != NULL );
	assert( MasterBoneIndex >= 0);
	assert( MasterBoneIndex < Master->JointCount);
	assert( Attachment != NULL );
	assert( Master != Slave );


	assert( (SlaveBoneIndex >=0)                 || (SlaveBoneIndex==(GE_POSE_ROOT_JOINT)));
	assert( (SlaveBoneIndex < Slave->JointCount) || (SlaveBoneIndex==(GE_POSE_ROOT_JOINT)));

	while (P!=NULL)
		{
			if (P==Slave)
				{
					geErrorLog_Add(-1, NULL);//FIXME
					return GE_FALSE;
				}
			P=P->Parent;
		}

	Slave->SlaveJointIndex = SlaveBoneIndex;
	Slave->Parent = Master;
	if (SlaveBoneIndex == GE_POSE_ROOT_JOINT)
		{
			Slave->Slave = GE_FALSE;
		}
	else
		{
			Slave->Slave = GE_TRUE;
		}

	gePose_InitializeJoint(&(Slave->RootJoint),MasterBoneIndex,Attachment);
	Slave->Touched = GE_TRUE;
	Slave->ParentsLastTransform = *(Master->RootJoint.Transform);
	
	return GE_TRUE;
}


void GENESISCC gePose_Detach(gePose *P)
{
	P->Parent = NULL;
	P->Slave = GE_FALSE;
	gePose_InitializeJoint(&(P->RootJoint),GE_POSE_ROOT_JOINT,NULL);
}


static geBoolean GENESISCC gePose_TransformCompare(const geXForm3d *T1, const geXForm3d *T2)
{
	if (T1->AX != T2->AX) return GE_FALSE;
	if (T1->BX != T2->BX) return GE_FALSE;
	if (T1->CX != T2->CX) return GE_FALSE;
	if (T1->AY != T2->AY) return GE_FALSE;
	if (T1->BY != T2->BY) return GE_FALSE;
	if (T1->CY != T2->CY) return GE_FALSE;
	if (T1->AZ != T2->AZ) return GE_FALSE;
	if (T1->BZ != T2->BZ) return GE_FALSE;
	if (T1->CZ != T2->CZ) return GE_FALSE;
	
	if (T1->Translation.X != T2->Translation.X) return GE_FALSE;
	if (T1->Translation.Y != T2->Translation.Y) return GE_FALSE;
	if (T1->Translation.Z != T2->Translation.Z) return GE_FALSE;
	return GE_TRUE;
}
	
	
static void GENESISCC gePose_UpdateRecursively(gePose *P,int Joint)
{
	gePose_Joint *J;
	assert( P != NULL );
	assert( Joint >= GE_POSE_ROOT_JOINT );

	J=&(P->JointArray[Joint]);

	assert( J->ParentJoint < Joint);

	if (J->ParentJoint != GE_POSE_ROOT_JOINT)
		gePose_UpdateRecursively(P,J->ParentJoint);

	gePose_JointRelativeToParent(gePose_JointByIndex(P, J->ParentJoint) ,J);
}

//  updates a node if node->touched or if any of it's parents have been touched.
//  returns GE_TRUE if any updates were made.
static void GENESISCC gePose_UpdateRelativeToParent(gePose *P)
{
	int i;
	gePose_Joint *J;
	const gePose_Joint *Parent;
	assert( P != NULL );
	
	if ( P->Parent != NULL )
		{
			gePose_UpdateRelativeToParent(P->Parent);
			if (gePose_TransformCompare(
						(P->Parent->RootJoint.Transform),&(P->ParentsLastTransform)) != GE_FALSE)
				{
					P->Touched = GE_TRUE;
					P->RootJoint.Touched = GE_TRUE;  // bubble touched down entire hierarchy
					P->ParentsLastTransform = *(P->Parent->RootJoint.Transform);
				}
				
			if (P->Slave == GE_FALSE)
				{
					Parent = gePose_JointByIndex(P->Parent, P->RootJoint.ParentJoint);
					gePose_JointRelativeToParent(Parent,&(P->RootJoint));
				}
			else
				{
					geXForm3d_SetIdentity(P->RootJoint.Transform);
					geQuaternion_SetNoRotation(&(P->RootJoint.Rotation));
				}
		}
	else
		{
			// No parent.  RootJoint is relative to nothing.
			J = &(P->RootJoint);
			if (J->Touched)
				{
					geQuaternion_Multiply(&(J->AttachmentRotation),&(J->LocalRotation),&(J->Rotation));
					geQuaternion_ToMatrix(&(J->Rotation), (J->Transform));
					geXForm3d_Transform(&(J->AttachmentTransform),&(J->LocalTranslation),&(J->Transform->Translation));
				}
		}


	if (P->Touched == GE_FALSE)
		{
			return;
		}


	if (P->OnlyThisJoint>=GE_POSE_ROOT_JOINT)
		{
			gePose_UpdateRecursively(P,P->OnlyThisJoint);
		}
	else
		{
			for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
				{
					assert( J->ParentJoint < i);

					Parent = gePose_JointByIndex(P, J->ParentJoint);
					if (J->Touched == GE_TRUE)
						{
							gePose_JointRelativeToParent(Parent ,J);
						}
					else
						{
							if (Parent->Touched)
								{
									J->Touched = GE_TRUE;
									gePose_JointRelativeToParent(Parent,J);
								}
						}
				}
			// touched flags don't mean anything when recursing backwards.  
			P->RootJoint.Touched = GE_FALSE;
			for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
				{
					J->Touched = GE_FALSE;
				}
		}

	if (P->Slave != GE_FALSE)
		{
			geXForm3d SlavedJointInverse;
			geXForm3d FullSlaveTransform;
			geXForm3d *MasterTransform;
			geXForm3d MasterAttachment;
			
			MasterTransform = (P->Parent->JointArray[P->RootJoint.ParentJoint].Transform);
			geXForm3d_GetTranspose((P->JointArray[P->SlaveJointIndex].Transform), &SlavedJointInverse);

			geQuaternion_ToMatrix(&(P->RootJoint.AttachmentRotation), &MasterAttachment);
			//MasterAttachment.Translation = P->RootJoint.AttachmentTranslation;
			MasterAttachment.Translation = P->RootJoint.AttachmentTransform.Translation;

			geXForm3d_Multiply(MasterTransform,&MasterAttachment,&FullSlaveTransform);
			
			*(P->RootJoint.Transform) = FullSlaveTransform;
			
			geXForm3d_Multiply(&FullSlaveTransform,&SlavedJointInverse,&FullSlaveTransform);

			for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
				{
					geXForm3d_Multiply(&FullSlaveTransform,
										(P->JointArray[i].Transform),
										(P->JointArray[i].Transform));
				}
			
		}
	P->Touched = GE_FALSE;
}	


geBoolean GENESISCC gePose_FindNamedJointIndex(const gePose *P, const char *JointName, int *Index)
{
	int i;

	assert( P != NULL );
	assert( Index!= NULL );
	if (JointName == NULL )
		return GE_FALSE;

	for (i=0; i<P->JointCount; i++)
		{
			const char *NthName = geStrBlock_GetString(P->JointNames,i);
			assert( NthName!= NULL );
			if ( strcmp(JointName,NthName)==0 )
				{
					*Index = i;
					return GE_TRUE;
				}	
		}
	return GE_FALSE;
}
	

geBoolean GENESISCC gePose_AddJoint(
	gePose *P,
	int ParentJointIndex,
	const char *JointName,
	const geXForm3d *Attachment,
	int *JointIndex)
{
	int JointCount;
	gePose_Joint *Joint;

	assert(  P != NULL );
	assert( JointIndex != NULL );
	assert( P->JointCount >= 0 );
	assert( (ParentJointIndex == GE_POSE_ROOT_JOINT) || 
			((ParentJointIndex >=0) && (ParentJointIndex <P->JointCount) ) );

	// Duplicate names ARE allowed

	JointCount = P->JointCount;
	{
		gePose_Joint *NewJoints;
		NewJoints = GE_RAM_REALLOC_ARRAY(P->JointArray,gePose_Joint,JointCount+1);
		if (NewJoints == NULL)
			{
				geErrorLog_Add(ERR_POSE_ADDJOINT_ENOMEM, NULL);
				return GE_FALSE;
			}
		P->JointArray = NewJoints;
	}
	
	assert( P->JointNames != NULL );
	assert( geStrBlock_GetCount(P->JointNames) == P->JointCount );

	if (geStrBlock_Append( &(P->JointNames), (JointName==NULL)?"":JointName )==GE_FALSE)
		{
			geErrorLog_Add(ERR_POSE_ADDJOINT_ENOMEM, NULL);
			return GE_FALSE;
		}
	

	{
		geXFArray *NewXFA;
		NewXFA = geXFArray_Create(JointCount+1);
		if (NewXFA == NULL)
			{
				geErrorLog_Add(ERR_POSE_ADDJOINT_ENOMEM, NULL);
				return GE_FALSE;
			}
		if (P->TransformArray != NULL)
			{
				geXFArray_Destroy(&(P->TransformArray));
			}
		P->TransformArray = NewXFA;
	}

	P->JointCount = JointCount+1;
	gePose_ReattachTransforms(P);
	
	Joint = &( P->JointArray[JointCount] );
	gePose_InitializeJoint(Joint,ParentJointIndex, Attachment);
	P->Touched = GE_TRUE;

	*JointIndex = JointCount;

	P->NameChecksum = geStrBlock_GetChecksum( P->JointNames );
	return GE_TRUE;
}

void GENESISCC gePose_GetJointAttachment(const gePose *P,int JointIndex, geXForm3d *AttachmentTransform)
{
	assert( P != NULL );
	assert( AttachmentTransform != NULL );
	{
		const gePose_Joint *J;
		J = gePose_JointByIndex(P, JointIndex);
		*AttachmentTransform = J->AttachmentTransform;
	}
}

void GENESISCC gePose_SetJointAttachment(gePose *P,
	int JointIndex, 
	const geXForm3d *AttachmentTransform)
{
	assert( P != NULL );
	assert( AttachmentTransform != NULL );
	{
		gePose_Joint *J;
		J = (gePose_Joint *)gePose_JointByIndex(P, JointIndex);
		geQuaternion_FromMatrix(AttachmentTransform,&(J->AttachmentRotation));
		J->Touched = GE_TRUE;
		J->AttachmentTransform = *AttachmentTransform;
		J->UnscaledAttachmentTranslation = J->AttachmentTransform.Translation;
		gePose_SetAttachmentRotationFlag(J);
	}
	P->Touched = GE_TRUE;
}

void GENESISCC gePose_GetJointTransform(const gePose *P, int JointIndex,geXForm3d *Transform)
{
	assert( P != NULL );
	assert( Transform != NULL );
	
	gePose_UpdateRelativeToParent((gePose *)P);
	
	{
		const gePose_Joint *J;
		J = gePose_JointByIndex(P, JointIndex);
		*Transform = *(J->Transform);
	}
}

void GENESISCC gePose_GetJointLocalTransform(const gePose *P, int JointIndex,geXForm3d *Transform)
{
	assert( P != NULL );
	assert( Transform != NULL );
	{
		const gePose_Joint *J;
		J = gePose_JointByIndex(P, JointIndex);
		geQuaternion_ToMatrix(&(J->LocalRotation), Transform);
		Transform->Translation = J->LocalTranslation;
	}
}

void GENESISCC gePose_SetJointLocalTransform(gePose *P, int JointIndex,const geXForm3d *Transform)
{
	assert( P != NULL );
	assert( Transform != NULL );
	{
		gePose_Joint *J;
		J = (gePose_Joint *)gePose_JointByIndex(P, JointIndex);
		geQuaternion_FromMatrix(Transform,&(J->LocalRotation));
		J->LocalTranslation = Transform->Translation;
		J->Touched = GE_TRUE;
	}
	P->Touched = GE_TRUE;
}

int GENESISCC gePose_GetJointCount(const gePose *P)
{
	assert( P != NULL );
	assert( P->JointCount >= 0 );
	
	return P->JointCount;
}

geBoolean GENESISCC gePose_MatchesMotionExactly(const gePose *P, const geMotion *M)
{
	if (geMotion_HasNames(M) != GE_FALSE)
		{
			if (geMotion_GetNameChecksum(M) == P->NameChecksum)
				return GE_TRUE;
			else
				return GE_FALSE;
		}
	return GE_FALSE;
}

// sets pose to it's base position: applies no modifier to the joints: only
// it's attachment positioning is used.
void GENESISCC gePose_Clear(gePose *P,const geXForm3d *Transform)
{
	int i;
	gePose_Joint *J;
	
	assert( P != NULL );
	assert( P->JointCount >= 0 );
	P->OnlyThisJoint = GE_POSE_ROOT_JOINT-1;		// calling this function disables one-joint optimizations
	if (P->Parent==NULL)
		{
			if (Transform!=NULL)
				{
					geQuaternion_FromMatrix(Transform,&(P->RootJoint.LocalRotation));
					P->RootJoint.LocalTranslation = Transform->Translation;
				}
			P->RootJoint.Touched = GE_TRUE;
		}
			
	for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
		{
			geVec3d_Set( (&J->LocalTranslation),0.0f,0.0f,0.0f);
			geQuaternion_SetNoRotation(&(J->LocalRotation));
			assert( J->ParentJoint < i);
			P->Touched = GE_TRUE;
		}	
	P->Touched = GE_TRUE;
}	

void GENESISCC gePose_SetMotion(gePose *P, const geMotion *M, geFloat Time,
							const geXForm3d *Transform)
{
	geBoolean NameBinding;
	int i;
	gePose_Joint *J;
	geXForm3d RootTransform;
	
	assert( P != NULL );

	P->OnlyThisJoint = GE_POSE_ROOT_JOINT-1;		// calling this function disables one-joint optimizations

	if (P->Parent==NULL)
		{
			geBoolean SetRoot = GE_FALSE;
			if (geMotion_GetTransform(M,Time,&RootTransform)!=GE_FALSE)
				{
					SetRoot = GE_TRUE;

					if ( Transform != NULL )
						{
							geXForm3d_Multiply(Transform,&RootTransform,&RootTransform);
						}
				}
			else
				{
					if ( Transform != NULL )
						{
							SetRoot = GE_TRUE;
							RootTransform = *Transform;
						}
				}

			if (SetRoot != GE_FALSE)
				{
					geQuaternion_FromMatrix(&RootTransform,&(P->RootJoint.LocalRotation));
					P->RootJoint.LocalTranslation = RootTransform.Translation;
					P->RootJoint.Touched = GE_TRUE;
				}
		}

	if (M==NULL)
		{
			return;
		}

	if (gePose_MatchesMotionExactly(P,M)==GE_TRUE)
		NameBinding = GE_FALSE;
	else
		NameBinding = GE_TRUE;

	P->Touched = GE_TRUE;
	#pragma message("could optimize this by looping two ways (min(jointcount,pathcount))")
	for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
		{
			//gePath *JointPath;
			if (NameBinding == GE_FALSE)
				{
					geMotion_SampleChannels(M,i,Time,&(J->LocalRotation),&(J->LocalTranslation));

					//JointPath = geMotion_GetPath(M,i);
					//assert( JointPath != NULL );
				}
			else
				{
					if (geMotion_SampleChannelsNamed(M,
						geStrBlock_GetString(P->JointNames,i),
						Time,&(J->LocalRotation),&(J->LocalTranslation))==GE_FALSE)
						continue;

				}
			J->Touched = GE_TRUE;
			J->LocalTranslation.X *= P->Scale.X;
			J->LocalTranslation.Y *= P->Scale.Y;
			J->LocalTranslation.Z *= P->Scale.Z;
		}
}

static void GENESISCC gePose_SetMotionForABoneRecursion(gePose *P, const geMotion *M, geFloat Time,
							int BoneIndex,geBoolean NameBinding)
{
	gePose_Joint *J;
	geBoolean Touched = GE_FALSE;
	assert(P!=NULL);
	assert(M!=NULL);
	assert( BoneIndex >= 0);

	J=&(P->JointArray[BoneIndex]);

	if (NameBinding == GE_FALSE)
		{
			geMotion_SampleChannels(M,BoneIndex,Time,&(J->LocalRotation),&(J->LocalTranslation));
			Touched = GE_TRUE;
		}
	else
		{
			if (geMotion_SampleChannelsNamed(M,
				geStrBlock_GetString(P->JointNames,BoneIndex),
				Time,&(J->LocalRotation),&(J->LocalTranslation))!=GE_FALSE)
				Touched = GE_TRUE;
		}
	if (Touched != GE_FALSE)
		{
			J->Touched = GE_TRUE;
			J->LocalTranslation.X *= P->Scale.X;
			J->LocalTranslation.Y *= P->Scale.Y;
			J->LocalTranslation.Z *= P->Scale.Z;
		}
	if (J->ParentJoint != GE_POSE_ROOT_JOINT)
		gePose_SetMotionForABoneRecursion(P,M,Time,J->ParentJoint,NameBinding);
	
}

void GENESISCC gePose_SetMotionForABone(gePose *P, const geMotion *M, geFloat Time,
							const geXForm3d *Transform,int BoneIndex)
{
	geBoolean NameBinding;
	geXForm3d RootTransform;
	
	assert( P != NULL );
	//assert( M != NULL );
	P->OnlyThisJoint = BoneIndex;		// calling this function enables single-joint optimizations
	
	if (P->Parent==NULL)
		{
			geBoolean SetRoot = GE_FALSE;
			if (geMotion_GetTransform(M,Time,&RootTransform)!=GE_FALSE)
				{
					SetRoot = GE_TRUE;

					if ( Transform != NULL )
						{
							geXForm3d_Multiply(Transform,&RootTransform,&RootTransform);
						}
				}
			else
				{
					if ( Transform != NULL )
						{
							SetRoot = GE_TRUE;
							RootTransform = *Transform;
						}
				}

			if (SetRoot != GE_FALSE)
				{
					geQuaternion_FromMatrix(&RootTransform,&(P->RootJoint.LocalRotation));
					P->RootJoint.LocalTranslation = RootTransform.Translation;
					P->RootJoint.Touched = GE_TRUE;
				}
		}

	if (M==NULL)
		{
			return;
		}
	if (BoneIndex == GE_POSE_ROOT_JOINT)
		{
			return;
		}

	if (gePose_MatchesMotionExactly(P,M)==GE_TRUE)
		NameBinding = GE_FALSE;
	else
		NameBinding = GE_TRUE;

	P->Touched = GE_TRUE;

	gePose_SetMotionForABoneRecursion(P, M, Time, BoneIndex, NameBinding);
}
	



#define LINEAR_BLEND(a,b,t)  ( (t)*((b)-(a)) + (a) )	
			// linear blend of a and b  0<t<1 where  t=0 ->a and t=1 ->b



void GENESISCC gePose_BlendMotion(	
	gePose *P, const geMotion *M, geFloat Time,
	const geXForm3d *Transform,
	geFloat BlendAmount, gePose_BlendingType BlendingType)
{
	int i;
	geBoolean NameBinding;
	gePose_Joint *J;
	geQuaternion R1;
	geVec3d      T1;
	geXForm3d    RootTransform;
	
	assert( P != NULL );
	//assert( M != NULL );  // M can be NULL
	assert( BlendingType == GE_POSE_BLEND_HERMITE || BlendingType == GE_POSE_BLEND_LINEAR);
	assert( BlendAmount >= 0.0f );
	assert( BlendAmount <= 1.0f );

	P->OnlyThisJoint = GE_POSE_ROOT_JOINT-1;		// calling this function disables one-joint optimizations
	if (BlendingType == GE_POSE_BLEND_HERMITE)
		{
			geFloat t2,t3;
			t2 = BlendAmount * BlendAmount;
			t3 = t2 * BlendAmount;
			BlendAmount = t2*3.0f -t3-t3;
		}

	if (P->Parent==NULL)
		{
			geBoolean SetRoot = GE_FALSE;
			if (geMotion_GetTransform(M,Time,&RootTransform)!=GE_FALSE)
				{
					SetRoot = GE_TRUE;

					if ( Transform != NULL )
						{
							geXForm3d_Multiply(Transform,&RootTransform,&RootTransform);
						}
				}
			else
				{
					if ( Transform != NULL )
						{
							SetRoot = GE_TRUE;
							RootTransform = *Transform;
						}
				}

			if (SetRoot != GE_FALSE)
				{
					geQuaternion_FromMatrix(&RootTransform,&R1);
					T1 = RootTransform.Translation;
					J  = &(P->RootJoint);
					geQuaternion_Slerp(&(J->LocalRotation),&(R1),BlendAmount,&(J->LocalRotation));
					{
						geVec3d      *LT = &(J->LocalTranslation);
						LT->X = LINEAR_BLEND(LT->X,T1.X,BlendAmount);
						LT->Y = LINEAR_BLEND(LT->Y,T1.Y,BlendAmount);
						LT->Z = LINEAR_BLEND(LT->Z,T1.Z,BlendAmount);
					}
					J->Touched = GE_TRUE;
				}
		}

	if (M==NULL)
		{
			return;
		}

	
	if (gePose_MatchesMotionExactly(P,M)==GE_TRUE)
		NameBinding = GE_FALSE;
	else
		NameBinding = GE_TRUE;
	
	P->Touched = GE_TRUE;

	for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
		{
			//gePath *JointPath;
							
			if (NameBinding == GE_FALSE)
				{
					geMotion_SampleChannels(M,i,Time,&R1,&T1);
					//JointPath = geMotion_GetPath(M,i);
					//assert( JointPath != NULL );
				}
			else
				{
					//JointPath = geMotion_GetPathNamed(M, geStrBlock_GetString(P->JointNames,i));
					//if (JointPath == NULL)
					//	continue;
					if (geMotion_SampleChannelsNamed(M,
						geStrBlock_GetString(P->JointNames,i),
						Time,&R1,&T1)==GE_FALSE)
						continue;

				}
			J->Touched = GE_TRUE;

			//gePath_SampleChannels(JointPath,Time,&(R1),&(T1));
			
			T1.X *= P->Scale.X;
			T1.Y *= P->Scale.Y;
			T1.Z *= P->Scale.Z;
			
			geQuaternion_Slerp(&(J->LocalRotation),&(R1),BlendAmount,&(J->LocalRotation));
						
			{
				geVec3d      *LT = &(J->LocalTranslation);
				LT->X = LINEAR_BLEND(LT->X,T1.X,BlendAmount);
				LT->Y = LINEAR_BLEND(LT->Y,T1.Y,BlendAmount);
				LT->Z = LINEAR_BLEND(LT->Z,T1.Z,BlendAmount);
			}
		}
}

const char* GENESISCC gePose_GetJointName(const gePose* P, int JointIndex)
{
	return geStrBlock_GetString(P->JointNames, JointIndex);
}

const geXFArray * GENESISCC gePose_GetAllJointTransforms(const gePose *P)
{
	assert( P != NULL );

	gePose_UpdateRelativeToParent((gePose *)P);
	return P->TransformArray;
}

void GENESISCC gePose_GetScale(const gePose *P, geVec3d *Scale)
{
	assert( P     != NULL );
	assert( Scale != NULL );
	*Scale = P->Scale;
}
	

void GENESISCC gePose_SetScale(gePose *P, const geVec3d *Scale )
{
	assert( P != NULL );
	assert( geVec3d_IsValid(Scale) != GE_FALSE );

	{
		int i;
		gePose_Joint *J;

		P->Scale = *Scale;
		//geVec3d_Set(&(P->Scale),ScaleX,ScaleY,ScaleZ);

		for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
			{	
				J->AttachmentTransform.Translation.X = J->UnscaledAttachmentTranslation.X * Scale->X;
				J->AttachmentTransform.Translation.Y = J->UnscaledAttachmentTranslation.Y * Scale->Y;
				J->AttachmentTransform.Translation.Z = J->UnscaledAttachmentTranslation.Z * Scale->Z;
				//J->AttachmentTransform.Translation = J->AttachmentTranslation;
				J->Touched = GE_TRUE;
			}
		P->Touched = GE_TRUE;
	}
}

void GENESISCC gePose_ClearCoverage(gePose *P, int ClearTo)
{
	int i;
	gePose_Joint *J;

	assert( P != NULL );
	assert( (ClearTo == GE_FALSE) || (ClearTo == GE_TRUE) );

	for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
		{	
			J->Covered = ClearTo;
		}
}

int GENESISCC gePose_AccumulateCoverage(gePose *P, const geMotion *M, geBoolean QueryOnly)
{
	int i,SubMotions;
	geBoolean NameBinding;
	int Covers=0;
	gePose_Joint *J;
	
	assert( P != NULL );
	if (M==NULL)
		{
			return P->JointCount;
		}

	SubMotions = geMotion_GetSubMotionCount(M);
	if (SubMotions>0)
		{
			for (i=0; i<SubMotions; i++)
				{
					int c = gePose_AccumulateCoverage(P, geMotion_GetSubMotion(M,i),QueryOnly);
					if (c > Covers)
						{
							Covers = c;
						}
				}
			return Covers;
		}
	
	if (gePose_MatchesMotionExactly(P,M)==GE_TRUE)
		NameBinding = GE_FALSE;
	else
		NameBinding = GE_TRUE;

	for (i=0, J=&(P->JointArray[0]); i<P->JointCount; i++,J++)
		{
			gePath *JointPath;
			if (J->Covered == GE_FALSE)
				{
					if (NameBinding == GE_TRUE)
						{
							JointPath = geMotion_GetPathNamed(M, geStrBlock_GetString(P->JointNames,i));
							if (JointPath == NULL)
								continue;
						}
					if (QueryOnly == GE_FALSE)
						{
							J->Covered = GE_TRUE;
						}
					Covers ++;
				}
		}
	return Covers;
}
	
