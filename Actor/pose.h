/****************************************************************************************/
/*  POSE.H																				*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Bone hierarchy interface.								.				*/
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef GE_POSE_H
#define GE_POSE_H

/*	gePose

	This object is a hierarchical set of attached joints.  The joints can have names.
	A 'gePose' keeps track of which children joints move in the hierarchy when a parent
	joint moves.  A gePose also remembers the position transform matrices for each joint.

	The gePose is set by applying a motion at a specific time.  This queries the motion
	to determine each joint's change and applies them to the hierarchy.  Each joint can
	then be queried for it's world transform (for drawing, etc.)

	Additional motions can modify or be blended into the pose.  A motion that describes 
	only a few joint changes can be applied to only those joints, or a motion can be
	blended with the current pose. 

	Something to watch for:  since setting the pose by applying a motion is powerful
	enough to resolve intentionally mismatched motion-pose sets, this can lead to 
	problems if the motion UNintentionally does not match the pose.  Use 
	gePose_MatchesgeMotionExactly() to test for an exact name-based match.
	

*/

#include <stdio.h>
#include "Motion.h"
#include "XFArray.h"

#ifdef __cplusplus
extern "C" {
#endif


#define GE_POSE_ROOT_JOINT (-1)

typedef enum 
{
		GE_POSE_BLEND_LINEAR,
		GE_POSE_BLEND_HERMITE
} gePose_BlendingType;

typedef struct gePose gePose;

	// Creates a new pose with no joints.
gePose *GENESISCC gePose_Create(void);

	// Destroys an existing pose.
void GENESISCC gePose_Destroy(gePose **PM);

	// Adds a new joint to a pose.
geBoolean GENESISCC gePose_AddJoint(
	gePose *P,
	int ParentJointIndex,
	const char *JointName,
	const geXForm3d *Attachment,
	int *JointIndex);


void GENESISCC gePose_GetScale(const gePose *P, geVec3d *Scale);
	// Retrieves current joint attachment scaling factors

void GENESISCC gePose_SetScale(gePose *P, const geVec3d *Scale);
	// Scales all joint attachments by component scaling factors in Scale

	// Returns the index of a joint named JointName.  Returns GE_TRUE if it is
	// located, and Index is set.  Returns GE_FALSE if not, and Index is not changed.
geBoolean GENESISCC gePose_FindNamedJointIndex(const gePose *P, const char *JointName, int *Index);

	// returns the number of joints in the pose
int GENESISCC gePose_GetJointCount(const gePose *P);

geBoolean GENESISCC gePose_MatchesMotionExactly(const gePose *P, const geMotion *M);

void GENESISCC gePose_Clear(gePose *P, const geXForm3d *Transform);

	// set the pose according to a motion.  Use the motion at time 'Time'.
	// if the motion does not describe motion for all joints, name-based resolution
	// will be used to decide which motion to attach to which joints.
	// joints that are unaffected are unchanged.
	// if Transform is non-NULL, it is applied to the Motion
void GENESISCC gePose_SetMotion(gePose *P, const geMotion *M,geFloat Time,const geXForm3d *Transform);

	// optimization:  if this is called, then all pose computations are limited to the BoneIndex'th bone, and
	// it's parents (including the root bone).  This is true for all queries until an entire motion is set or blended
	// into the pose.
void GENESISCC gePose_SetMotionForABone(gePose *P, const geMotion *M, geFloat Time,
							const geXForm3d *Transform,int BoneIndex);


	// blend in the pose according to a motion.  Use the motion at time 'Time'.
	// the blending is between the 'current' pose and the pose described by the motion.
	// a BlendAmount of 0 will result in the 'current' pose, and a BlendAmount of 1.0
	// will result in the pose according to the new motion.
	// if the motion does not describe motion for all joints, name-based resolution
	// will be used to decide which motion to attach to which joints.
	// joints that are unaffected are unchanged.
	// if Transform is non-NULL, it is applied to the Motion prior to blending
void GENESISCC gePose_BlendMotion(gePose *P, const geMotion *M, geFloat Time, 
					const geXForm3d *Transform,
					geFloat BlendAmount, gePose_BlendingType BlendingType);

	// get a joint's current transform (relative to world space)
void GENESISCC gePose_GetJointTransform(const gePose *P, int JointIndex,geXForm3d *Transform);

	// get the transforms for the entire pose. *TransformArray must not be changed.
const geXFArray *GENESISCC gePose_GetAllJointTransforms(const gePose *P);

	// query a joint's current transform relative to it's attachment to it's parent.
void GENESISCC gePose_GetJointLocalTransform(const gePose *P, int JointIndex,geXForm3d *Transform);

	// adjust a joint's current transform relative to it's attachment to it's parent.
	//   this is like setting a mini-motion into this joint only:  this will only affect
	//   the current pose 
void GENESISCC gePose_SetJointLocalTransform(gePose *P, int JointIndex,const geXForm3d *Transform);

	// query how a joint is attached to it's parent. (it's base attachment)
void GENESISCC gePose_GetJointAttachment(const gePose *P,int JointIndex,geXForm3d *AttachmentTransform);

	// adjust how a joint is attached to it's parent.  These changes are permanent:  all
	//  future pose motions will incorporate this joint's new relation to it's parent */
void GENESISCC gePose_SetJointAttachment(gePose *P,int JointIndex,const geXForm3d *AttachmentTransform);

const char* GENESISCC gePose_GetJointName(const gePose* P, int JointIndex);

geBoolean GENESISCC gePose_Attach(gePose *Slave, int SlaveBoneIndex,
				  gePose *Master, int MasterBoneIndex, 
				  const geXForm3d *Attachment);

void GENESISCC gePose_Detach(gePose *P);

	// a pose can also maintain a record of which joints are touched by a given motion.
	// these funtions set,clear and query the record.
	// ClearCoverage clears the coverage flag for all joints 
void GENESISCC gePose_ClearCoverage(gePose *P, int ClearTo);
	// AccumulateCoverage returns the number of joints that are not already 'covered' 
	// that will be affected by a motion M,  
	// if QueryOnly is GE_FALSE, affected joints are tagged as 'covered', otherwise no changes
	// are made to the joint coverage flags.
int GENESISCC gePose_AccumulateCoverage(gePose *P, const geMotion *M, geBoolean QueryOnly);


#ifdef __cplusplus
}
#endif


#endif
