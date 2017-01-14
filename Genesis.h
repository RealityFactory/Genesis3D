/****************************************************************************************/
/*  Genesis.h                                                                           */
/*                                                                                      */
/*  Description: The master header for Genesis                                          */
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
#ifndef GENESIS_H
#define GENESIS_H

#include "BaseType.h"
#include "Vec3d.h"
#include "XForm3d.h"
#include "GETypes.h"
#include "ExtBox.h"
#include "vfile.h"
#include "Bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

//================================================================================
//	Constants / Defines / TypDefs
//================================================================================

typedef struct		geEngine			geEngine;

typedef struct		geDriver_System		geDriver_System;
typedef struct		geDriver			geDriver;
typedef struct		geDriver_Mode		geDriver_Mode;

typedef struct		geSound_System		geSound_System;
typedef struct		geSound_Cfg			geSound_Cfg;
typedef struct		geSound_Def			geSound_Def;
typedef struct		geSound				geSound;
										
typedef struct		geActor				geActor;
typedef struct		geActor_Def			geActor_Def;		// the definition of an actor's geometry/bone structure

typedef struct		geWorld				geWorld;

typedef struct		geWorld_Model		geWorld_Model;

typedef struct		geEntity			geEntity;
typedef struct		geEntity_EntitySet	geEntity_EntitySet;

typedef struct		geCamera			geCamera;

typedef struct		geCSNetMgr			geCSNetMgr;

typedef struct		gePoly				gePoly;

typedef struct		geLight				geLight;			

typedef struct		geFog				geFog;

typedef struct		geMesh_Def			geMesh_Def;			// Mesh def
typedef struct		geMesh				geMesh;				

#define GE_VERSION_MAJOR		(1UL)
#define GE_VERSION_MINOR		(3UL)
#define GE_VERSION_MINOR_MIN	(3UL)

#define GE_VERSION_MAJOR_SHIFT	(16)
#define GE_VERSION_MAJOR_MASK	((uint32)0xFFFF0000)

#define GE_VERSION				( (GE_VERSION_MAJOR << GE_VERSION_MAJOR_SHIFT) + GE_VERSION_MINOR )

// From here down, still needs to be fixed up for July4
typedef	struct		GE_ModelMotion		GE_ModelMotion;
typedef float		GE_TimeType;

// Polys
typedef enum
{
	GE_TEXTURED_POLY,
	GE_GOURAUD_POLY,
	GE_TEXTURED_POINT					
} gePoly_Type;

// Poly Fx flags
#define	GE_RENDER_DO_NOT_OCCLUDE_OTHERS	(1<<0)			// Poly will not occlude others
#define GE_RENDER_DO_NOT_OCCLUDE_SELF	(1<<1)			// Renders under any condition.  Useful for halos, etc...
#define	GE_RENDER_BACKFACED				(1<<2)			// Poly should be backfaced from the Camera's Pov
#define GE_RENDER_DEPTH_SORT_BF			(1<<3)			// Sorts relative to camera position, from back to front
#define GE_RENDER_CLAMP_UV				(1<<4)			// Clamp UV's in both directions

// World Add flags
#define GE_WORLD_RENDER				(1<<0)
#define GE_WORLD_COLLIDE			(1<<1)



#ifndef GE_CONTENTS_TYPES
#define GE_CONTENTS_TYPES

//
// Content types in GE_Contents structure (multiple contents can be mixed...)
//
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//	IF THESE FLAGS CHANGE, THEY MUST CHANGE IN GBSPFILE.H in Genesis AND GBSPLIB, and Genesis.H!!!!!
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
#define GE_CONTENTS_SOLID			(1<<0)		// Solid (Visible)
#define GE_CONTENTS_WINDOW			(1<<1)		// Window (Visible)
#define GE_CONTENTS_EMPTY			(1<<2)		// Empty but Visible (water, lava, etc...)

#define GE_CONTENTS_TRANSLUCENT		(1<<3)		// Vis will see through it
#define GE_CONTENTS_WAVY			(1<<4)		// Wavy (Visible)
#define GE_CONTENTS_DETAIL			(1<<5)		// Won't be included in vis oclusion

#define GE_CONTENTS_CLIP			(1<<6)		// Structural but not visible
#define GE_CONTENTS_HINT			(1<<7)		// Primary splitter (Non-Visible)
#define GE_CONTENTS_AREA			(1<<8)		// Area seperator leaf (Non-Visible)

#define GE_CONTENTS_FLOCKING		(1<<9)
#define GE_CONTENTS_SHEET			(1<<10)
#define GE_CONTENTS_AIR				(1<<11)		// No brush lives in this leaf
#define GE_RESERVED4				(1<<12)
#define GE_RESERVED5				(1<<13)
#define GE_RESERVED6				(1<<14)
#define GE_RESERVED7				(1<<15)

// 16-31 reserved for user contents
#define GE_CONTENTS_USER1			(1<<16)
#define GE_CONTENTS_USER2			(1<<17)
#define GE_CONTENTS_USER3			(1<<18)
#define GE_CONTENTS_USER4			(1<<19)
#define GE_CONTENTS_USER5			(1<<20)
#define GE_CONTENTS_USER6			(1<<21)
#define GE_CONTENTS_USER7			(1<<22)
#define GE_CONTENTS_USER8			(1<<23)
#define GE_CONTENTS_USER9			(1<<24)
#define GE_CONTENTS_USER10			(1<<25)
#define GE_CONTENTS_USER11			(1<<26)
#define GE_CONTENTS_USER12			(1<<27)
#define GE_CONTENTS_USER13			(1<<28)
#define GE_CONTENTS_USER14			(1<<29)
#define GE_CONTENTS_USER15			(1<<30)
#define GE_CONTENTS_USER16			(1<<31)
// 16-31 reserved for user contents


// These contents are all solid types
#define GE_CONTENTS_SOLID_CLIP		(GE_CONTENTS_SOLID | GE_CONTENTS_WINDOW | GE_CONTENTS_CLIP)
#define GE_CONTENTS_CANNOT_OCCUPY	GE_CONTENTS_SOLID_CLIP

// These contents are all visible types
#define GE_VISIBLE_CONTENTS			(	GE_CONTENTS_SOLID | \
										GE_CONTENTS_EMPTY | \
										GE_CONTENTS_WINDOW | \
										GE_CONTENTS_WAVY)

#endif
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

// NOTES - Fills in first Model/Mesh hit
// Exceptions:
//		Returns the last solid model hit...
typedef struct
{
	geMesh			*Mesh;
	geWorld_Model	*Model;
	geActor			*Actor;
	int32			Contents;
} GE_Contents;

typedef geBoolean GE_CollisionCB(geWorld_Model *Model, geActor *Actor, void *Context);

// Collision
typedef struct
{
	geVec3d	Normal;									// Orientation of plane
	float	Dist;									// Distance from origin
} GE_Plane;

typedef struct
{
	geWorld_Model	*Model;							// Pointer to what model was hit (if any)
	geMesh			*Mesh;							// Pointer to what mesh was hit (if any)
	geActor			*Actor;							// Pointer to what actor was hit (if any)	
	geVec3d			Impact;							// Impact Point
	float			Ratio;							// Percent from 0 to 1.0, how far along the line for the impact point
	GE_Plane		Plane;							// Impact Plane
} GE_Collision;

// If these render states change, they must change in DCommon.h too!!!
// These are still under construction, and are for debug purposes only.
// They are merely means of overriding ways the engine normally renders primitives, etc...
//
//	RenderState States
//
#define GE_RENDERSTATE_ZWRITE		0		// Z Writes
#define GE_RENDERSTATE_ZCMP			1		// Z Compares
#define GE_RENDERSTATE_BILINEAR		2		// Bilinear filtering
#define	GE_RENDERSTATE_ANTI_ALIAS	3		// Anti-Aliasing
#define GE_RENDERSTATE_POLYMODE		4		// Normal, Gouraud only, Lines only, etc

//
//	RenderState Flags
//
#define GE_RENDERFLAG_OFF		0
#define GE_RENDERFLAG_ON		1

//
//  PolyMode flags	(A method to override how polys are drawn for debugging purposes...)
//
#define GE_POLYMODE_NORMAL		1			// Draw as is
#define GE_POLYMODE_GOURAUD		2			// Gouraud only
#define GE_POLYMODE_LINES		3			// Outlines only

//================================================================================
//	Engine Management functions
//================================================================================


#ifdef _INC_WINDOWS	
	// Windows.h must be included before genesis.h for this api to be exposed.
	

GENESISAPI	geEngine	*geEngine_CreateWithVersion(HWND hWnd, const char *AppName, const char *DriverDirectory, uint32 Version);
							// use geEngine_Create, rather than calling this directly.

#define geEngine_Create( hWnd, AppName, DriverDirectory)   geEngine_CreateWithVersion(hWnd,AppName,DriverDirectory,GE_VERSION)

#endif

GENESISAPI void			geEngine_Free(geEngine *Engine);

GENESISAPI geBoolean	geEngine_AddWorld(geEngine *Engine, geWorld *World);
GENESISAPI geBoolean	geEngine_RemoveWorld(geEngine *Engine, geWorld *World);

GENESISAPI geBoolean	geEngine_AddBitmap(geEngine *Engine, geBitmap *Bitmap);
GENESISAPI geBoolean	geEngine_RemoveBitmap(geEngine *Engine, geBitmap *Bitmap);

GENESISAPI geDriver_System *geEngine_GetDriverSystem(geEngine *Engine);

GENESISAPI geBoolean	geEngine_SetDriverAndMode(	geEngine *Engine, 
													geDriver *Driver, 
													geDriver_Mode *DriverMode);

GENESISAPI geBoolean	geEngine_ShutdownDriver(geEngine *Engine);

GENESISAPI geBoolean	geEngine_BeginFrame(geEngine *Engine, geCamera *Camera, geBoolean ClearScreen);
GENESISAPI geBoolean	geEngine_EndFrame(geEngine *Engine);

GENESISAPI geBoolean	geEngine_RenderWorld(geEngine *Engine, geWorld *World, geCamera *Camera, geFloat Time);
GENESISAPI geBoolean	geEngine_Printf(geEngine *Engine, int32 x, int32 y, const char *String, ...);

GENESISAPI void			GENESISCC geEngine_RenderPoly(const geEngine *Engine, const GE_TLVertex *Points, int NumPoints, const geBitmap *Texture, uint32 Flags);
							//RenderPoly : if Texture is null, we Gouraud shade

GENESISAPI void			GENESISCC geEngine_RenderPolyArray(const geEngine *Engine, const GE_TLVertex ** pPoints, int * pNumPoints, int NumPolys, 
								const geBitmap *Texture, uint32 Flags);

GENESISAPI geBoolean	GENESISCC geEngine_DrawBitmap(const geEngine *Engine,const geBitmap *Bitmap,
								const geRect * Source, uint32 x, uint32 y);
							//DrawBitmap & RenderPoly : must Engine_AddBitmap first!


GENESISAPI void			geEngine_FillRect(geEngine *Engine, const GE_Rect *Rect, const GE_RGBA *Color);

GENESISAPI geBoolean	geEngine_SetGamma(geEngine *Engine, float Gamma);
GENESISAPI geBoolean	geEngine_GetGamma(geEngine *Engine, float *Gamma);

GENESISAPI geBoolean	geEngine_SetFogEnable(geEngine *Engine, geBoolean Enable, float r, float g, float b, float Start, float End);
						// enables/disables distance fogging.  (based on Enable)
						//  r,g,b is the color of the fog
						//  Start is how far out from the camera is not fogged - this is where the fog begins
						//  End is how far out from the camera where the fog fully obscures things
				

GENESISAPI geBoolean	geEngine_ScreenShot(geEngine *Engine, const char *FileName);

GENESISAPI void			geEngine_EnableFrameRateCounter(geEngine *Engine, geBoolean Enabled);

GENESISAPI geBoolean	geEngine_Activate(geEngine *Engine, geBoolean bActive);

#ifdef _INC_WINDOWS
	// Windows.h must be included before genesis.h for this api to be exposed.
GENESISAPI geBoolean	geEngine_UpdateWindow(geEngine *Engine);
#endif

// geDriver
GENESISAPI geDriver		*geDriver_SystemGetNextDriver(geDriver_System *DriverSystem, geDriver *Start);
GENESISAPI geDriver_Mode *geDriver_GetNextMode(geDriver *Driver, geDriver_Mode *Start);
GENESISAPI geBoolean	geDriver_GetName(geDriver *Driver, const char **Name);
GENESISAPI geBoolean	geDriver_ModeGetName(geDriver_Mode *Mode, const char **Name);
GENESISAPI geBoolean	geDriver_ModeGetWidthHeight(geDriver_Mode *Mode, int32 *Width, int32 *Height);

//================================================================================
//	Sound Management functions
//================================================================================
#ifdef _INC_WINDOWS
	// Windows.h must be included before genesis.h for this api to be exposed.
GENESISAPI 	geSound_System *geSound_CreateSoundSystem(HWND hWnd);
#endif


GENESISAPI void			geSound_DestroySoundSystem(geSound_System *Sound);


GENESISAPI geSound_Def	   *geSound_LoadSoundDef(geSound_System *SoundS, geVFile *File);
GENESISAPI void				geSound_FreeSoundDef(geSound_System *SoundS, geSound_Def *SoundDef);

GENESISAPI geSound		   *geSound_PlaySoundDef(geSound_System *SoundS, 
									geSound_Def *SoundDef, 
									geFloat Volume, 
									geFloat Pan, 
									geFloat Frequency, 
									geBoolean Loop);
GENESISAPI geBoolean		geSound_StopSound(geSound_System *SoundS, geSound *Sound);
GENESISAPI geBoolean		geSound_ModifySound(geSound_System *SoundS, 
									geSound *Sound, 
									geFloat Volume, 
									geFloat Pan, 
									geFloat Frequency);
GENESISAPI geBoolean		geSound_SoundIsPlaying(geSound_System *SoundS, geSound *Sound);
GENESISAPI geBoolean		geSound_SetMasterVolume( geSound_System *SoundS, geFloat Volume );

GENESISAPI void geSound3D_GetConfig(
			const geWorld *World, 
			const geXForm3d *CameraTransform, 
			const geVec3d *SoundPos, 
			geFloat Min, 
			geFloat Ds,
			geFloat *Volume,
			geFloat *Pan,
			geFloat *Frequency);

GENESISAPI	void geSound3D_GetConfigIgnoreObstructions(
		const geWorld *World, 
		const geXForm3d *MXForm, 
		const geVec3d *SndPos, 
		geFloat Min, 
		geFloat Ds,
		geFloat *Volume,
		geFloat *Pan,
		geFloat *Frequency);


//================================================================================
//  Path Support
//================================================================================
#include "Path.h"
//================================================================================
//  Motion Support
//================================================================================
#include "Motion.h"
//================================================================================
//  Actor Support
//================================================================================
#include "Actor.h"



//================================================================================
//	World Management functions
//================================================================================
GENESISAPI geWorld		*geWorld_Create(geVFile *File);
GENESISAPI void			geWorld_Free(geWorld *World);

// World Actors
GENESISAPI geBoolean	geWorld_RemoveActor    (geWorld *World, geActor *Actor);
GENESISAPI geBoolean    geWorld_AddActor       (geWorld *World, geActor *Actor, uint32 Flags, uint32 UserFlags);
GENESISAPI geBoolean	geWorld_SetActorFlags  (geWorld *World, geActor *Actor, uint32 Flags);

// World Bitmaps
GENESISAPI geBoolean	geWorld_AddBitmap(		geWorld *World, geBitmap *Bitmap);
GENESISAPI geBoolean	geWorld_RemoveBitmap(	geWorld *World, geBitmap *Bitmap);
GENESISAPI geBoolean	geWorld_HasBitmap(const geWorld *World, const geBitmap *Bitmap);
GENESISAPI geBitmap		*geWorld_GetBitmapByName(geWorld *World, const char *BitmapName);
GENESISAPI geBoolean	geWorld_BitmapIsVisible(geWorld *World, const geBitmap *Bitmap);

// World BModels
GENESISAPI geWorld_Model	*geWorld_GetNextModel(geWorld *World, geWorld_Model *Start);
GENESISAPI geBoolean		geWorld_SetModelXForm(geWorld *World, geWorld_Model *Model, const geXForm3d *XForm);
GENESISAPI geBoolean		geWorld_GetModelXForm(const geWorld *World, const geWorld_Model *Model, geXForm3d *XForm);
GENESISAPI geBoolean		geWorld_OpenModel(geWorld *World, geWorld_Model *Model, geBoolean Open);
GENESISAPI geBoolean		geWorld_GetModelRotationalCenter(const geWorld *World, const geWorld_Model *Model, geVec3d *Center);
GENESISAPI geBoolean		geWorld_ModelGetBBox(const geWorld *World, const geWorld_Model *Model, geVec3d *Mins, geVec3d *Maxs);
GENESISAPI geMotion *		geWorld_ModelGetMotion(geWorld_Model *Model);

GENESISAPI void			*geWorld_ModelGetUserData(const geWorld_Model *Model);
GENESISAPI void			geWorld_ModelSetUserData(geWorld_Model *Model, void *UserData);
GENESISAPI void			geWorld_ModelSetFlags(geWorld_Model *Model, uint32 ModelFlags);
GENESISAPI uint32		geWorld_ModelGetFlags(geWorld_Model *Model);

// World Lights
GENESISAPI geLight		*geWorld_AddLight(geWorld *World);
GENESISAPI void			geWorld_RemoveLight(geWorld *World, geLight *Light);
GENESISAPI geBoolean	geWorld_SetLightAttributes(	geWorld *World,
										geLight		*Light, 
										const		geVec3d *Pos, 
										const		GE_RGBA *RGBA, 
										float		Radius,
										geBoolean	CastShadow);
GENESISAPI geBoolean	geWorld_SetLTypeTable(geWorld *World, int32 LType, const char *Table);

// World fog
GENESISAPI geFog		*geWorld_AddFog(geWorld *World);
GENESISAPI geBoolean	geWorld_RemoveFog(geWorld *World, geFog *Fog);

GENESISAPI geBoolean geFog_SetAttributes(	geFog			*Fog, 
											const geVec3d	*Pos, 
											GE_RGBA			*Color,
											float			LightBrightness, 
											float			VolumeBrightness, 
											float			VolumeRadius);

// World Classes/Entities
GENESISAPI geEntity_EntitySet *geWorld_GetEntitySet(geWorld *World, const char *ClassName);
GENESISAPI geEntity		*geEntity_EntitySetGetNextEntity(geEntity_EntitySet *EntitySet, geEntity *Entity);
GENESISAPI void			*geEntity_GetUserData(geEntity *Entity);
GENESISAPI void			geEntity_GetName(const geEntity *Entity, char *Buff, int MaxLen);

// World collision
GENESISAPI geBoolean	geWorld_ModelCollision(	geWorld			*World, 
												geWorld_Model	*Model, 
												const geXForm3d	*DXForm, 
												GE_Collision	*Collision);
GENESISAPI geBoolean geWorld_TestModelMove(	geWorld			*World, 
											geWorld_Model	*Model, 
											const geXForm3d	*DXForm, 
											const geVec3d	*Mins, const geVec3d *Maxs,
											const geVec3d	*In, geVec3d *Out);

GENESISAPI geBoolean geWorld_Collision(	geWorld *World,				// World to collide with
										const geVec3d *Mins,		// Mins of object (in object-space).  This CAN be NULL
										const geVec3d *Maxs,		// Maxs of object (in object-space).  This CAN be NULL
										const geVec3d *Front,		// Front of line (in world-space)
										const geVec3d *Back,		// Back of line (in world-space)
										uint32 Contents,			// Contents to collide with (use GE_CONTENTS_SOLID_CLIP for default)
										uint32 CollideFlags,		// To mask out certain object types (GE_COLLIDE_ALL, etc...)
										uint32 UserFlags,			// To mask out actors (refer to geActor_SetUserFlags)
										GE_CollisionCB *CollisionCB, // A callback to allow user to reject collisions with certain objects)
										void *Context,				// User data passed through above callback
										GE_Collision *Col);			// Structure filled with info about what was collided with
	// NOTE - Mins/Maxs CAN be NULL.  If you are just testing a point, then use NULL (it's faster!!!).

GENESISAPI geBoolean geWorld_GetContents(geWorld *World, const geVec3d *Pos, const geVec3d *Mins, const geVec3d *Maxs, uint32 Flags, uint32 UserFlags, GE_CollisionCB *CollisionCB, void *Context, GE_Contents *Contents);

// World Polys
GENESISAPI	gePoly *geWorld_AddPolyOnce(geWorld *World, 
										GE_LVertex *Verts, 
										int32 NumVerts, 
										geBitmap *Bitmap,
										gePoly_Type Type, 
										uint32 RenderFlags,
										float Scale);
GENESISAPI	gePoly *geWorld_AddPoly(geWorld *World, 
									GE_LVertex *Verts, 
									int32 NumVerts, 
									geBitmap *Bitmap,
									gePoly_Type Type,
									uint32 RenderFlags,
									float Scale);

GENESISAPI	void geWorld_RemovePoly(geWorld *World, gePoly *Poly);
GENESISAPI	geBoolean gePoly_GetLVertex(gePoly *Poly, int32 Index, GE_LVertex *LVert);
GENESISAPI	geBoolean gePoly_SetLVertex(gePoly *Poly, int32 Index, const GE_LVertex *LVert);

// World visibility
GENESISAPI geBoolean	geWorld_GetLeaf(const geWorld *World, const geVec3d *Pos, int32 *Leaf);
GENESISAPI geBoolean	geWorld_MightSeeLeaf(const geWorld *World, int32 Leaf);

GENESISAPI geBoolean	geWorld_LeafMightSeeLeaf(const geWorld *World, int32 Leaf1, int32 Leaf2, uint32 VisFlags);
	// Checks to see if Leaf1 can see Leaf2
	// Currently VisFlags is not used yet.  It could be used for checking against areas, etc...
	// Eventually you could also pass in a VisObject, that is manipulated with a camera...

GENESISAPI geBoolean GENESISCC geWorld_IsActorPotentiallyVisible(const geWorld *World, const geActor *Actor, const geCamera *Camera);


//================================================================================
//	Camera Management functions
//================================================================================

GENESISAPI geCamera			*GENESISCC geCamera_Create(geFloat Fov, const geRect *Rect);
GENESISAPI void				GENESISCC geCamera_Destroy(geCamera **pCamera);
GENESISAPI void				GENESISCC geCamera_SetZScale(geCamera *Camera, geFloat ZScale);
GENESISAPI geFloat			GENESISCC geCamera_GetZScale(const geCamera *Camera);
GENESISAPI void				GENESISCC geCamera_SetFarClipPlane(geCamera *Camera, geBoolean Enable, geFloat ZFar);
							// sets a far clipping plane.  The world and objects aren't drawn if they lie beyond ZFar. 
							// Zfar is the distance out from the camera.
							// Polygons crossing the line are not nesessarily clipped exactly to the line.
GENESISAPI void				GENESISCC geCamera_GetFarClipPlane(const geCamera *Camera, geBoolean *Enable, geFloat *ZFar);
GENESISAPI void				GENESISCC geCamera_SetAttributes(geCamera *Camera,geFloat Fov, const geRect *Rect);
GENESISAPI void				GENESISCC geCamera_GetClippingRect(const geCamera *Camera, geRect *Rect);

GENESISAPI void				GENESISCC geCamera_ScreenPointToWorld(const geCamera	*Camera,
										int32			 ScreenX,
										int32			 ScreenY,
										geVec3d			*Vector	);
GENESISAPI void	GENESISCC geCamera_Project(const geCamera	*Camera, 
										const geVec3d	*PointInCameraSpace, 
										geVec3d			*ProjectedPoint);
GENESISAPI void GENESISCC geCamera_Transform(const geCamera *Camera, 
										const geVec3d *WorldSpacePoint, 
										  geVec3d *CameraSpacePoint);

GENESISAPI void GENESISCC geCamera_TransformArray(const geCamera *Camera, 
										const geVec3d *WorldSpacePointPtr, 
										      geVec3d *CameraSpacePointPtr,
											int count);

GENESISAPI void GENESISCC geCamera_TransformAndProject(const geCamera *Camera,
										const	geVec3d *Point, 
												geVec3d *ProjectedPoint);

GENESISAPI void GENESISCC geCamera_TransformAndProjectArray(const geCamera *Camera, 
										const geVec3d *WorldSpacePointPtr, 
										      geVec3d *ProjectedSpacePointPtr,
											int count);
	
GENESISAPI void GENESISCC geCamera_TransformAndProjectL(const geCamera *Camera,
										const GE_LVertex *Point, 
											GE_TLVertex *ProjectedPoint);
		
GENESISAPI void GENESISCC geCamera_TransformAndProjectLArray(const geCamera *Camera, 
										const GE_LVertex *WorldSpacePointPtr, 
										      GE_TLVertex *ProjectedSpacePointPtr,
											int count);


GENESISAPI geBoolean		GENESISCC geCamera_SetWorldSpaceXForm(geCamera *Camera, const geXForm3d *XForm);
GENESISAPI geBoolean		GENESISCC geCamera_SetWorldSpaceVisXForm(geCamera *Camera, const geXForm3d *XForm);
GENESISAPI const geXForm3d	*GENESISCC geCamera_GetWorldSpaceXForm( const geCamera *Camera);
GENESISAPI const geXForm3d * GENESISCC geCamera_GetWorldSpaceVisXForm( const geCamera *Camera);
GENESISAPI geBoolean GENESISCC geCamera_ConvertWorldSpaceToCameraSpace(const geXForm3d *WXForm, geXForm3d *CXForm);


//================================================================================
// NetPlay Management functions
//================================================================================

typedef uint32				geCSNetMgr_NetID;
#define	MAX_CLIENT_NAME		256

// Types for messages received from GE_ReceiveSystemMessage
typedef enum 
{
	NET_MSG_NONE,					// No msg
	NET_MSG_USER,					// User message
	NET_MSG_CREATE_CLIENT,			// A new client has joined in
	NET_MSG_DESTROY_CLIENT,			// An existing client has left
	NET_MSG_HOST,					// We are the server now
	NET_MSG_SESSIONLOST,			// Connection was lost
	NET_MSG_SERVER_ID,				// Internal, for hand shaking process
} geCSNetMgr_NetMsgType;


typedef struct
{
	char				Name[MAX_CLIENT_NAME];
	geCSNetMgr_NetID	Id;
} geCSNetMgr_NetClient;

#ifdef _INC_WINDOWS
	// Windows.h must be included before genesis.h for this api to be exposed.
	typedef struct geCSNetMgr_NetSession
	{
		char		SessionName[200];					// Description of Service provider
		GUID		Guid;								// Service Provider GUID
	} geCSNetMgr_NetSession;
GENESISAPI 	geBoolean		GENESISCC geCSNetMgr_FindSession(geCSNetMgr *M, const char *IPAdress, geCSNetMgr_NetSession **SessionList, int32 *SessionNum );
GENESISAPI 	geBoolean		GENESISCC geCSNetMgr_JoinSession(geCSNetMgr *M, const char *Name, const geCSNetMgr_NetSession* Session);
#endif


GENESISAPI geCSNetMgr	*	GENESISCC geCSNetMgr_Create(void);
GENESISAPI void				GENESISCC geCSNetMgr_Destroy(geCSNetMgr **ppM);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_ReceiveFromServer(geCSNetMgr *M, geCSNetMgr_NetMsgType *Type, int32 *Size, uint8 **Data);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_ReceiveFromClient(geCSNetMgr *M, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetID *IdClient, int32 *Size, uint8 **Data);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_ReceiveSystemMessage(geCSNetMgr *M, geCSNetMgr_NetID IdFor, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetClient *Client);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_ReceiveAllMessages(geCSNetMgr *M, geCSNetMgr_NetID *IdFrom, geCSNetMgr_NetID *IdTo, geCSNetMgr_NetMsgType *Type, int32 *Size, uint8 **Data);
GENESISAPI geCSNetMgr_NetID	GENESISCC geCSNetMgr_GetServerID(geCSNetMgr *M);
GENESISAPI geCSNetMgr_NetID	GENESISCC geCSNetMgr_GetOurID(geCSNetMgr *M);
GENESISAPI geCSNetMgr_NetID	GENESISCC geCSNetMgr_GetAllPlayerID(geCSNetMgr *M);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_WeAreTheServer(geCSNetMgr *M);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_StartSession(geCSNetMgr *M, const char *SessionName, const char *PlayerName );
GENESISAPI geBoolean		GENESISCC geCSNetMgr_StopSession(geCSNetMgr *M);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_SendToServer(geCSNetMgr *M, geBoolean Guaranteed, uint8 *Data, int32 DataSize);
GENESISAPI geBoolean		GENESISCC geCSNetMgr_SendToClient(geCSNetMgr *M, geCSNetMgr_NetID To, geBoolean Guaranteed, uint8 *Data, int32 DataSize);

#ifdef __cplusplus
}
#endif

#endif

