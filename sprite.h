/****************************************************************************************/
/*  SPRITE.H                                                                            */
/*                                                                                      */
/*  Author: Michael R. Brumm	                                                          */
/*  Description:  Sprite interface		                                                  */
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
/****************************************************************************************/


#ifndef GE_SPRITE_H
#define GE_SPRITE_H

#include "genesis.h"				
#include "basetype.h"
#include "extbox.h"
#include "bitmap.h"

#ifdef GE_WORLD_H
#include "camera.h"
#include "Frustum.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// GENESIS_PUBLIC_APIS

typedef struct geSprite geSprite;			// an instance of a sprite


//---------------------------------------------------------------------------------
//   Creation/Destruction functions
//---------------------------------------------------------------------------------

	// create a sprite instance associated with the given bitmaps as faces
	// pass in a NULL bitmap for a gouraud face
	// backface bitmap can be the same as the front face bitmap
GENESISAPI geSprite *GENESISCC geSprite_Create(geBitmap *SpriteBitmap, geBitmap *SpriteBackfaceBitmap);

	// create an additional reference (owner) for the sprite
GENESISAPI void GENESISCC geSprite_CreateRef(geSprite *Sprite);

	// destroy a sprite
GENESISAPI void GENESISCC geSprite_Destroy(geSprite **pS);

	// returns number of sprites that are currently created
GENESISAPI int32 GENESISCC geSprite_GetCount();

	// checks to see if a sprite is valid or not
GENESISAPI geBoolean GENESISCC geSprite_IsValid(const geSprite *S);

//---------------------------------------------------------------------------------
//   Queries 
//---------------------------------------------------------------------------------
	// In general: Objects retuned from Get functions should not not be destroyed. 
	// if ownership is desired, call the objects _CreateRef() function to create another owner. 
	// (An 'owner' has access to the object regardless of the number of other owners, and 
	// an owner must call the object's _Destroy() function to relinquish ownership )

	// Returns the Bitmap associated with the sprite's front face
GENESISAPI geBitmap *GENESISCC geSprite_GetBitmap(const geSprite *S);

	// Returns the Bitmap associated with the sprite's backface
GENESISAPI geBitmap *GENESISCC geSprite_GetBackfaceBitmap(const geSprite *S);

	// Gets backface parameters
GENESISAPI void GENESISCC geSprite_GetBackface(const geSprite *S, geBoolean *Enabled, geBoolean *MirrorImage);

	// Sets the backface parameters
GENESISAPI void GENESISCC geSprite_SetBackface(geSprite *S, const geBoolean Enabled, const geBoolean MirrorImage);

	// Gets whether the sprite always faces the camera
GENESISAPI void GENESISCC geSprite_GetFaceCamera(const geSprite *S, geBoolean *Enabled);

	// Sets whether the sprite always faces the camera
GENESISAPI void GENESISCC geSprite_SetFaceCamera(geSprite *S, geBoolean Enabled);

	// Gets the position of the sprite
GENESISAPI void GENESISCC geSprite_GetPosition(const geSprite *S, geVec3d *Pos);

	// Sets the position of the sprite
	//
	// For easy modification of the sprite position if the sprite always faces the camera
GENESISAPI void GENESISCC geSprite_SetPosition(geSprite *S, const geVec3d *Pos);

	// Gets the current transform for the sprite
GENESISAPI void GENESISCC geSprite_GetTransform(const geSprite *S, geXForm3d *Transform);

	// Sets the current transform for the sprite
	//
	// Rotation information is ignored if the sprite always faces the camera
GENESISAPI void GENESISCC geSprite_SetTransform(geSprite *S, const geXForm3d *Transform);

	// Gets the internal transform for the sprite
GENESISAPI void GENESISCC geSprite_GetInternalTransform(const geSprite *S, geXForm3d *Transform);

	// Sets the internal transform for the sprite
	//
	// Allows the sprite to be rendered offset from its main transform. For example,
	// translation could make the bottom of the sprite its center of rotation.
	//
	// This is totally ignored if the sprite always faces the camera
GENESISAPI void GENESISCC geSprite_SetInternalTransform(geSprite *S, const geXForm3d *Transform);

	// Gets the scale of the sprite (width and height)
GENESISAPI void GENESISCC geSprite_GetScale(const geSprite *S, geFloat *ScaleX, geFloat *ScaleY);

	// Sets the scale of the sprite (width and height)
GENESISAPI void GENESISCC geSprite_SetScale(geSprite *S, geFloat ScaleX, geFloat ScaleY);

	// Gets an assigned general non changing bounding box from the sprite
GENESISAPI void GENESISCC geSprite_GetExtBox(const geSprite *S, geExtBox *ExtBox);

	// Gets the bounding box in non-world coordinates
	// Whatever you put in with geSprite_SetExtBox, you get out with this function
GENESISAPI void GENESISCC geSprite_GetNonWorldExtBox(const geSprite *S, geExtBox *ExtBox);

	// Sets an assigned general non changing bounding box from the sprite
GENESISAPI void GENESISCC geSprite_SetExtBox(geSprite *S, const geExtBox *ExtBox);

	// Gets the texture parameters for the sprite
GENESISAPI void GENESISCC geSprite_GetTextureParameters(const geSprite *S,
									geFloat *OffsetX,
									geFloat *OffsetY,
									geFloat *ScaleX,
									geFloat *ScaleY
									);

	// Sets the texture parameters for the sprite
GENESISAPI void GENESISCC geSprite_SetTextureParameters(geSprite *S,
									geFloat OffsetX,
									geFloat OffsetY,
									geFloat ScaleX,
									geFloat ScaleY
									);

	// Gets the lighting options for the sprite
GENESISAPI void GENESISCC geSprite_GetLightingOptions(const geSprite *S,
									geFloat *AmbientLightRed,			// 0 .. 255
									geFloat *AmbientLightGreen,			// 0 .. 255
									geFloat *AmbientLightBlue,			// 0 .. 255
									geBoolean *UseFillLight,			// GE_TRUE or GE_FALSE
									geVec3d *FillLightNormal,			// normalized vector
									geFloat *FillLightRed,				// 0 .. 255
									geFloat *FillLightGreen,			// 0 .. 255
									geFloat *FillLightBlue,				// 0 .. 255
									geBoolean *UseLightFromFloor,// GE_TRUE or GE_FALSE
									int32 *MaximumDynamicLightsToUse
									);

	// Sets the lighting options for the sprite
GENESISAPI void GENESISCC geSprite_SetLightingOptions(geSprite *S,
									geFloat AmbientLightRed,			// 0 .. 255
									geFloat AmbientLightGreen,			// 0 .. 255
									geFloat AmbientLightBlue,			// 0 .. 255
									geBoolean UseFillLight,				// GE_TRUE or GE_FALSE
									const geVec3d *FillLightNormal,		// normalized vector
									geFloat FillLightRed,				// 0 .. 255
									geFloat FillLightGreen,				// 0 .. 255
									geFloat FillLightBlue,				// 0 .. 255
									geBoolean UseLightFromFloor,	// GE_TRUE or GE_FALSE
									int32 MaximumDynamicLightsToUse		// 0 for none
									);			

	// Gets the alpha transparency of the sprite
GENESISAPI void GENESISCC geSprite_GetAlpha(const geSprite *S, geFloat *Alpha, geFloat *BackfaceAlpha);

	// Sets the alpha transparency of the sprite
GENESISAPI void GENESISCC geSprite_SetAlpha(geSprite *S, geFloat Alpha, geFloat BackfaceAlpha);

	// Returns the pointer which was set with geSprite_SetUserData. NULL if not set.
GENESISAPI void *GENESISCC geSprite_GetUserData(const geSprite *S);

	// Sets the sprites user data pointer to the given value. For clients only.
GENESISAPI void GENESISCC geSprite_SetUserData(geSprite *S, void *UserData);


//--------------------------------------------------------------------------------
//   Rendering
//--------------------------------------------------------------------------------

// GENESIS_PRIVATE_APIS

#ifdef GE_WORLD_H
	// Prepares the geSprite for rendering and posing.  Call Once once the sprite is fully created.
	// Must be called prior to render/pose/setworldtransform 
geBoolean GENESISCC geSprite_RenderPrep( geSprite *A, geWorld *World);

	// Draws the geSprite.  (RenderPrep must be called first)
geBoolean GENESISCC geSprite_RenderThroughFrustum(geSprite *S, geEngine *Engine, geWorld *World, geCamera *Camera, Frustum_Info *FInfo);
#endif


#ifdef __cplusplus
}
#endif


#endif
