/****************************************************************************************/
/*  ACTOR.H                                                                             */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description:  Actor interface		                                                */
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
/*   Actor
	
	This object is designed to support character animation.
	There are two basic objects to deal with.  
	
	Actor Definition (geActor_Def)
		A geActor_Def embodies the geometry (polygon, and bone information), 
		and a library of motions that can be applied to that geometry.

	Actor
		A geActor is an instance of an actor definition.  The definition is used for 
		the geometry, but all additional settings, such as the bone pose, lighting information,
		and cuing information is unique for a geActor.
// GENESIS_PRIVATE_API
	An Actor Definition is created either from an existing Actor Definition file, or from scratch by 
	first creating a geBody and geMotions and selecting these into an Actor.  If the Actor Definition
	is constructed from scratch, the objects selected into it (via SetBody and AddMotion) are
	then 'owned' by the actor and will be destroyed along with the Actor when it is destroyed.
    Of course, when the Actor is loaded from a file, the Body and Motion it creates as it is
	loaded are cleaned up when the Actor is destroyed.

	Once an Actor is created, prepare it for rendering and animating by calling 
	Actor_RenderPrep().  This must be called (and it must succeed) before any render or
	pose setting functions can be called.
// GENESIS_PUBLIC_API

	There are two ways to use an Actor.
	Direct Control
		One method is to directly control the skeleton configuration.  Use _SetPose() to set its 
		skeleton using a geMotion animation.  The pose is positioned in world space relative to the 
		transform given in SetPose().  Whenever a new skeleton pose is required, call _SetPose() 
		to reposition the skeleton for a new point in time. 

		More complex positioning can be achieved by blending more than one animation.  Use
		_BlendPose() after a _SetPose() to blend the second geMotion into the first.  Additional
		blends can be applied by additional _BlendPose() calls.  Each blend is performed on the
		the existing skeleton (the results of any previous blends).
	Cuing
		Another method is to 'cue' up motions that are applied with parameterized blending over time.
		A cued motion takes effect 'now' in time.  The Actor advances in time and repositions itself
		according to its currently cued motions with a call to _AnimationStep().  AnimationStep() 
		redefines what the actor thinks 'now' is.  This causes historical cues to be forgotten, and 
		motions that are no longer valid are cleaned up.  AnimationTestStep() can be used to position 
		the actor for potential queries with its currently cued motions at some arbitrary future time 
		- relative to the last AnimationTestStep() call.  AnimationNudge() applies a given transform 
		'instantly' to the current actor's cue list.  This is usefull for moving the actor as a 
		result of a collision with another object.

	If a motion contains joint information that does not exactly match the Actor's skeleton 
	joints, only the joints that match by name are applied.  So a geMotion can be applied to
	a portion of the Actor, or a geMotion that has more joint information than the skeleton can
	be applied and the extra joint information is ignored.  
	 
	Examples of this:  If the Actor is a biped and has no tail, but the motion is for a 
	biped with a tail, the geMotion can be applied, but the tail information will be ignored.
	Also if there is a geMotion for only a left arm, it can be applied and it will only affect
	the left arm of the Actor, and consequently its left hand and fingers, but no other 
	bones that are not children of the affected bones will be changed.

	 	
*/
#ifndef GE_ACTOR_H
#define GE_ACTOR_H

#include "genesis.h"				
#include "basetype.h"
#include "extbox.h"
#include "bitmap.h"

#include "Motion.h"

#ifdef GE_WORLD_H
#include "camera.h"
#include "Frustum.h"
#endif

#include "Body.h"

#ifdef __cplusplus
extern "C" {
#endif

// GENESIS_PUBLIC_APIS

#ifndef GE_ACTOR_ENUMS
#define GE_ACTOR_ENUMS
typedef enum 
{
		GE_ACTOR_BLEND_LINEAR,		// Treats the blending amount as a linear value
		GE_ACTOR_BLEND_HERMITE		// Applies a parametric smoothing curve to the blending amount
									//  so that a linear change in BlendAmount parameters will
									//  result in a smooth (non-linear) change in blending.
} geActor_BlendingType;

#endif

typedef struct geActor geActor;			// an instance of an actor
typedef struct geActor_Def geActor_Def;		// the deinition of an actor's geometry/bone structure


// GENESIS_PRIVATE_APIS

//---------------------------------------------------------------------------------
//   Creation/Destruction functions
//---------------------------------------------------------------------------------
	// Create an 'empty' Actor Definition.
GENESISAPI geActor_Def *GENESISCC geActor_DefCreate(void);

	// Create an Actor Definition from a file image.
GENESISAPI geActor_Def *GENESISCC geActor_DefCreateFromFile(geVFile *pFile);

	// Create an additional reference (owner) for the Actor_Definition
GENESISAPI void GENESISCC geActor_DefCreateRef(geActor_Def *pActorDefinition);

	// Destroy a geActor_Def (its geBody and its geMotions)  Actors that rely on this definition become invalid.
	// can fail if there are actors still referencing this definition.
GENESISAPI geBoolean GENESISCC geActor_DefDestroy(geActor_Def **pActorDefinition);

	// Create an Actor instance associated with the given Actor Definition 
GENESISAPI geActor *GENESISCC geActor_Create(geActor_Def *ActorDefinition);

	// Create an additional reference (owner) for the Actor
GENESISAPI void GENESISCC geActor_CreateRef(geActor *Actor);

	// Give the Actor Definition a Body.  geActor becomes responsible for its destruction.
	// sets up default materials as referenced by the Body.
GENESISAPI geBoolean GENESISCC geActor_SetBody( geActor_Def *ActorDefinition, geBody *geBodyGeometry);

	// Adds a geMotion to the Actor Definition's library.  The ActorDefinition becomes responsible for its destruction.
	// returns the library index to the new geMotion.
GENESISAPI geBoolean GENESISCC geActor_AddMotion(geActor_Def *ActorDefinition, geMotion *M, int *Index);

	// Destroy an Actor.  
GENESISAPI void GENESISCC geActor_Destroy(geActor **pA);

GENESISAPI geBoolean GENESISCC geActor_DefIsValid(const geActor_Def *A);
GENESISAPI geBoolean GENESISCC geActor_IsValid(const geActor *A);

// GENESIS_PUBLIC_APIS
//---------------------------------------------------------------------------------
//   Queries 
//---------------------------------------------------------------------------------
// GENESIS_PRIVATE_APIS

	// In general: Objects retuned from Get functions should not not be destroyed. 
	// if ownership is desired, call the objects _CreateRef() function to create another owner. 
	// (An 'owner' has access to the object regardless of the number of other owners, and 
	// an owner must call the object's _Destroy() function to relinquish ownership )

	// Returns the Actor Definition associated with Actor A
GENESISAPI geActor_Def *GENESISCC geActor_GetActorDef(const geActor *A);

	// Writes an existing geActor to a file image.  Returns GE_TRUE on success, GE_FALSE on failure.
GENESISAPI geBoolean GENESISCC geActor_DefWriteToFile(const geActor_Def *A, geVFile *pFile);
	
	// Returns a geBody pointer from the geActor 
GENESISAPI geBody *GENESISCC geActor_GetBody(const geActor_Def *ActorDefinition);

	// Returns GE_TRUE if the actor definition has a bone named 'Name'
GENESISAPI geBoolean GENESISCC geActor_DefHasBoneNamed(const geActor_Def *Ad, const char *Name );

	// Selects a blending type.  BlendingType only affects the meaning of the 
	// BlendAmount parameter for the blend functions.  Can be changed anytime.
GENESISAPI void GENESISCC geActor_SetBlendingType( geActor *A, geActor_BlendingType BlendingType );

// GENESIS_PUBLIC_APIS

	// Returns the number of geMotions in the geActors geMotion library.
GENESISAPI int GENESISCC geActor_GetMotionCount(const geActor_Def *ActorDefinition);

	// Returns a geMotion pointer from the geActors geMotion library
	//   This is an aliased pointer - Not a copy.  Changes to this motion will be reflected
	//   in the actor.  Destroying this return motion will confuse the actor.
	// Index must be in range [0..geActor_GetMotionCount-1]
GENESISAPI geMotion *GENESISCC geActor_GetMotionByIndex(const geActor_Def *ActorDefinition, int Index );

	// Returns a geMotion pointer from the geActors geMotion library
	//   This is an aliased pointer - Not a copy.  Changes to this motion will be reflected
	//   in the actor.  Destroying this return motion will confuse the actor.
	// if there is no motion that matches the given name, the return value will be NULL
GENESISAPI geMotion *GENESISCC geActor_GetMotionByName(const geActor_Def *ActorDefinition, const char *Name );

	// Returns a motion name given an ActorDef and a motion index.
GENESISAPI const char *GENESISCC geActor_GetMotionName(const geActor_Def *ActorDefinition, int Index );

	// Returns the number of materials for an instance of an actor.
GENESISAPI int GENESISCC geActor_GetMaterialCount(const geActor *A);

	// Returns the current material for an instance of an actor
GENESISAPI geBoolean GENESISCC geActor_GetMaterial(const geActor *Actor, int MaterialIndex,
										geBitmap **Bitmap, geFloat *Red, geFloat *Green, geFloat *Blue);

	// Allows a material to be overriden in an actor instance
GENESISAPI geBoolean GENESISCC geActor_SetMaterial(geActor *Actor, int MaterialIndex,
										geBitmap *Bitmap,  geFloat Red,  geFloat Green,  geFloat Blue);



	// Gets the current transform for a single bone in A.  (actor space->world space transform)
	// with a NULL BoneName, this returns the current 'root' transform
GENESISAPI geBoolean GENESISCC geActor_GetBoneTransform(const geActor *A, const char *BoneName, geXForm3d *Transform);
	
	// Gets the extent box (axial-aligned bounding box) for a given bone (for the current pose)
	// if BoneName is NULL, gets the a general bounding box from the body of the actor if it has been set.
GENESISAPI geBoolean GENESISCC geActor_GetBoneExtBox(const geActor *A,
										 const char *BoneName,geExtBox *ExtBox);

	// Gets the non-axial-aligned bounding box for a given bone (for the current pose)
	//  The box is specified by a corner, and
	//  a non-normalized orientation transform.  Add DX,DY,DZ components 
	//  of the orientation to get other corners of the box
	// if BoneName is NULL, gets the a general bounding box from the body of the actor if it has been set.
GENESISAPI geBoolean GENESISCC geActor_GetBoneBoundingBox(const geActor *A,
														 const char *BoneName,
														 geVec3d *Corner,
														 geVec3d *DX,
														 geVec3d *DY,
														 geVec3d *DZ);

	// Gets the current axial-aligned bounding box for an actor's bone configuration
	// takes all bones into account
GENESISAPI geBoolean GENESISCC geActor_GetDynamicExtBox( const geActor *A, geExtBox *ExtBox);
	
	// Gets an assigned general non changing bounding box from the actor
GENESISAPI geBoolean GENESISCC geActor_GetExtBox(const geActor *A, geExtBox *ExtBox);

	// Sets an assigned general non changing bounding box from the actor
GENESISAPI geBoolean GENESISCC geActor_SetExtBox(geActor *A, const geExtBox *ExtBox,
					const char *CenterBoxOnThisNamedBone);		// NULL uses root position of actor

	// Gets the rendering hint bounding box from the actor
	//   if the RenderHintExtBox is disabled, Enabled is GE_FALSE, and the box returned has zero dimensions, 
	//   centered at the root position of the actor.  If the RenderHintExtBox is enabled, Enabled is
	//   GE_TRUE, and the box returned is the one set with _SetRenderHintExtBox, offset by the 
	//   bone position of the bone named in _SetRenderHintExtBox().
GENESISAPI geBoolean GENESISCC geActor_GetRenderHintExtBox(const geActor *A, geExtBox *Box, geBoolean *Enabled);

	// Sets a rendering hint bounding box from the actor.  Increases performance by 
	//   enabling the rendering of the actor to occur only if the box is visible.
	//   If the box is not visible, a detailed analysis of the actor's current geometry is avoided.
	//   This does allow errors to occur: 
	//   If the actor has a bit of geometry that extends outside this box for some
	//   animation, that extended geometry may not be drawn, if the box if off-screen.   
	//   If the render hint box is not set, the engine will make no conservative assumptions 
	//   about the visibility of an actor - it will always be drawn if any portion of it is
	//   visible.
	//   To attach the box to the 'root' bone, pass NULL for CenterBoxOnThisNamedBone
	//   For disabling the hint box: (disabled is default) pass Box with zero mins and maxs
GENESISAPI geBoolean GENESISCC geActor_SetRenderHintExtBox(geActor *A, const geExtBox *Box,
												const char *CenterBoxOnThisNamedBone );


	// Returns the pointer which was set with geActor_SetUserData.  NULL if not set.
GENESISAPI void *GENESISCC geActor_GetUserData(const geActor *A);

	// Sets the actors user data pointer to the given value.  For clients only.
GENESISAPI void GENESISCC geActor_SetUserData(geActor *A, void *UserData);


//--------------------------------------------------------------------------------
//   Posing and Rendering
//--------------------------------------------------------------------------------

// GENESIS_PRIVATE_APIS

#ifdef GE_WORLD_H
	// Prepares the geActor for rendering and posing.  Call Once once the actor is fully created.
	// Must be called prior to render/pose/setworldtransform 
geBoolean GENESISCC geActor_RenderPrep( geActor *A, geWorld *World);

	// Draws the geActor.  (RenderPrep must be called first)
geBoolean GENESISCC geActor_RenderThroughFrustum(const geActor *A, geEngine *Engine, geWorld *World, geCamera *Camera, Frustum_Info *FInfo);
geBoolean GENESISCC geActor_Render(const geActor *A, geEngine *Engine, geWorld *World, geCamera *Camera);
#endif

// GENESIS_PUBLIC_APIS

	// Poses the actor in its default pose
	// Transform is where to position the root for this pose.
	//  if Transform is NULL, the root for the pose is assumed to be the root of the actor.
GENESISAPI void GENESISCC geActor_ClearPose(geActor *A, const geXForm3d *Transform);

	// Poses the actor using given motion M at a time offset of Time
	// Transform is where to position the root for this pose.
	//  if Transform is NULL, the root for the pose is assumed to be the root of the actor.
GENESISAPI void GENESISCC geActor_SetPose(geActor *A, const geMotion *Motion, geFloat Time, const geXForm3d *Transform);

	// Blends the current pose of the geActor with 
	//  a new pose using motion M at a time offset of Time
	// A BlendAmount of 0 will result in the existing pose, A BlendAmount of 1 will
	// result in the new pose from M.  The BlendingType set by _SetBlendingType() determines
	// the blending function between 0 and 1
	// Transform is where to position the root for this pose.
	//  if Transform is NULL, the root for the pose is assumed to be the root of the actor.
GENESISAPI void GENESISCC geActor_BlendPose(geActor *A, const geMotion *Motion, geFloat Time,
						const geXForm3d *Transform, geFloat BlendAmount);


GENESISAPI geBoolean GENESISCC geActor_GetBoneAttachment(const geActor *A, const char *BoneName, geXForm3d *Transform);
GENESISAPI geBoolean GENESISCC geActor_SetBoneAttachment(geActor *A, const char *BoneName, geXForm3d *Transform);

// GENESIS_PRIVATE_APIS

GENESISAPI geBoolean GENESISCC geActor_Attach( geActor *Slave,  const char *SlaveBoneName,
						const geActor *Master, const char *MasterBoneName, 
						const geXForm3d *Attachment);

GENESISAPI void GENESISCC geActor_Detach(geActor *Slave);


// GENESIS_PUBLIC_APIS
GENESISAPI geBoolean GENESISCC geActor_SetLightingOptions(geActor *A,
									geBoolean UseFillLight,				// GE_TRUE or GE_FALSE
									const geVec3d *FillLightNormal,		// normalized vector
									geFloat FillLightRed,				// 0 .. 255
									geFloat FillLightGreen,				// 0 .. 255
									geFloat FillLightBlue,				// 0 .. 255
									geFloat AmbientLightRed,			// 0 .. 255
									geFloat AmbientLightGreen,			// 0 .. 255
									geFloat AmbientLightBlue,			// 0 .. 255
									geBoolean AmbientLightFromFloor,	// GE_TRUE or GE_FALSE
									int MaximumDynamicLightsToUse,		// 0 for none
									const char *LightReferenceBoneName, //NULL for root
									geBoolean PerBoneLighting);			
									// if GE_TRUE, then dynamic lighting attenuation and direction is computed
									// for each bone.  if GE_FALSE, then the computations are relative to the 
									// single bone named by the LightReferenceBoneName

GENESISAPI geBoolean GENESISCC geActor_GetLightingOptions(const geActor *A,
									geBoolean *UseFillLight,			// GE_TRUE or GE_FALSE
									geVec3d *FillLightNormal,			// normalized vector
									geFloat *FillLightRed,				// 0 .. 255
									geFloat *FillLightGreen,			// 0 .. 255
									geFloat *FillLightBlue,				// 0 .. 255
									geFloat *AmbientLightRed,			// 0 .. 255
									geFloat *AmbientLightGreen,			// 0 .. 255
									geFloat *AmbientLightBlue,			// 0 .. 255
									geBoolean *UseAmbientLightFromFloor,// GE_TRUE or GE_FALSE
									int *MaximumDynamicLightsToUse,		
									const char **LightReferenceBoneName,
									geBoolean *PerBoneLighting);		// NULL for root


GENESISAPI void GENESISCC geActor_SetScale(geActor *A, geFloat ScaleX,geFloat ScaleY,geFloat ScaleZ);

GENESISAPI geBoolean GENESISCC geActor_SetShadow(geActor *A, 
						geBoolean DoShadow, 
						geFloat Radius,
						const geBitmap *ShadowMap,
						const char * BoneName);

//  Animation Cuing API:
// high level Actor animation:  The principle is that motions can be applied to an actor
// and the actor will keep track of which motions are currently appropriate.  Call 
//	_AnimationStep() to compute a new pose for an elapsed time interval.  The new pose
//  will take into account all motions that are 'currently' cued up to be set or blended.


	// cue up a new motion.  The motion begins at the current time.  The motion can be 
	// blended in or out over time and time scaled.  If the return value is GE_FALSE, the 
	// animation was not cued up (failure implies Actor is incompletely initialized).
GENESISAPI geBoolean GENESISCC geActor_AnimationCue( 
		geActor *A,						// actor to apply animation to
		geMotion *Motion,				// motion to Cue
		geFloat TimeScaleFactor,		// time scale to apply to cued motion
		geFloat TimeIntoMotion,			// time offset to begin motion with (Not TimeScaled)
		geFloat BlendTime,				// time to apply a blend. 
		geFloat BlendFromAmount,		// blend value at current time
		geFloat BlendToAmount,			// blend value after BlendTime time has elapsed
		const geXForm3d *MotionTransform);	// local transform to adjust motion by (NULL implies NO transform)

	// removes the last animation cue that was cued up.  Can be called repeatedly to successively
	// remove older and older cues.  Returns GE_TRUE when a cue was removed, GE_FALSE if there 
	// are no cues to remove.
GENESISAPI geBoolean GENESISCC geActor_AnimationRemoveLastCue( geActor *A );

	// applies a time step to actor A.  re-poses the actor according to all currently applicable
	// Animation Cues. (failure implies Actor is incompletely initialized)
GENESISAPI geBoolean GENESISCC geActor_AnimationStep(geActor *A, geFloat DeltaTime );

	// applies a 'temporary' time step to actor A.  re-poses the actor according to all 
	// currently appliciable cues.  (failure implies Actor is incompletely initialized)
	// DeltaTime is always relative to the the last AnimationStep()
GENESISAPI geBoolean GENESISCC geActor_AnimationTestStep(geActor *A, geFloat DeltaTime);

	// optimized version of geActor_AnimationStep.  Limits calculations to the bone named BoneName, and it's 
	// parents.  BoneName will be correctly computed, but the other bones will be wrong.  This is usefull for 
	// moving and animating an actor that is not actually visible.  Rendering and queries will be 'optimized'
	// until the actor is given any pose or animation that doesn't go through geActor_AnimationStepBoneOptimized() or 
	//  geActor_AnimationTestStepBoneOptimized().  BoneName can be NULL to compute only 'root' bone.
GENESISAPI geBoolean GENESISCC geActor_AnimationStepBoneOptimized(geActor *A, geFloat DeltaTime, const char *BoneName );

	// optimized version of geActor_AnimationTestStep.  Limits calculations to the bone named BoneName, and it's 
	// parents.  BoneName will be correctly computed, but the other bones will be wrong.  This is usefull for 
	// moving and animating an actor that is not actually visible.  Rendering and queries will be 'optimized'
	// until the actor is given any pose or animation that doesn't go through geActor_AnimationStepBoneOptimized() or 
	//  geActor_AnimationTestStepBoneOptimized().  BoneName can be NULL to compute only 'root' bone.
GENESISAPI geBoolean GENESISCC geActor_AnimationTestStepBoneOptimized(geActor *A, geFloat DeltaTime, const char *BoneName);


	// applies an 'immediate' offset to the animated actor
GENESISAPI geBoolean GENESISCC geActor_AnimationNudge(geActor *A, geXForm3d *Offset);


GENESISAPI geBoolean GENESISCC geActor_GetAnimationEvent(geActor *A,						
	const char **ppEventString);		// Return data, if return value is GE_TRUE

	// returns number of actors that are currently created.
GENESISAPI int GENESISCC geActor_GetCount(void);

// GENESIS_PRIVATE_APIS
	// call setscale and setshadow after preparing the actor for rendering (renderprep)


#ifdef __cplusplus
}
#endif


#endif

