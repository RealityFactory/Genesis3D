/****************************************************************************************/
/*  Engine.h                                                                            */
/*                                                                                      */
/*  Author: Charles Bloom/John Pollard                                                  */
/*  Description: Maintains the driver interface, as well as the bitmaps attached        */
/*                  to the driver                                                       */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                    */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved                            */
/*                                                                                      */
/****************************************************************************************/
#ifndef GE_ENGINE_H
#define GE_ENGINE_H

#include "Genesis.h"
#include "System.h"
#include "World.h"
#include "Bitmap.h"
#include "BitmapList.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------
// fake out windows include
//-------------------------------------------------
#ifndef WINVER // if you want windows, you must include it first!
#ifdef STRICT
typedef struct HINSTANCE__ * HINSTANCE;
#else // STRICT
typedef void * HINSTANCE;
#endif // STRICT
#endif

//-------------------------------------------------
// Engine Functions
//-------------------------------------------------

//-------- engine world list funcs
GENESISAPI geBoolean geEngine_AddWorld(geEngine *Engine, geWorld *World);
GENESISAPI geBoolean geEngine_RemoveWorld(geEngine *Engine, geWorld *World);
geBoolean geEngine_RemoveAllWorlds(geEngine *Engine);
geBoolean geEngine_HasWorld(const geEngine *Engine, const geWorld *World);
void geEngine_SetAllWorldChangedFlag(geEngine *Engine, geBoolean Flag);

//-------- engine attach/detach thandle funcs

// call updategamma when drivers change
GENESISAPI geBoolean geEngine_SetGamma(geEngine *Engine, geFloat Gamma);
GENESISAPI geBoolean geEngine_GetGamma(geEngine *Engine, geFloat *Gamma);
GENESISAPI geBoolean geEngine_SetFogEnable(geEngine *Engine, geBoolean Enable, geFloat r, geFloat g, geFloat b, geFloat Start, geFloat End);
GENESISAPI geBoolean geEngine_SetClearColor(geEngine *Engine, geFloat r, geFloat g, geFloat b);
void geEngine_UpdateGamma(geEngine *Engine);

geBoolean geEngine_BitmapListInit(geEngine *Engine);
geBoolean geEngine_BitmapListShutdown(geEngine *Engine);
geBoolean geEngine_DetachAllWorlds(geEngine *Engine);
geBoolean geEngine_CreateWorldLightmapTHandles(geEngine *Engine, geWorld *World);
geBoolean geEngine_DestroyWorldLightmapTHandles(geEngine *Engine, geWorld *World);
geBoolean geEngine_AttachAllWorlds(geEngine *Engine);
geBoolean geEngine_AttachAll(geEngine *Engine);
geBoolean geEngine_DetachAll(geEngine *Engine);

//-------- the splash screen
geBoolean geEngine_DoSplashScreen(geEngine *Engine, geDriver_Mode *DriverMode);

//-------- engine fonts
geBoolean geEngine_InitFonts(geEngine *Engine);
geBoolean geEngine_ShutdownFonts(geEngine *Engine);

//-------- engine drivers
HINSTANCE geEngine_LoadLibrary( const char * lpLibFileName, const char *DriverDirectory);
geBoolean geEngine_ResetDriver(geEngine *Engine);
GENESISAPI geDriver_System *geEngine_GetDriverSystem(geEngine *Engine);

GENESISAPI geBoolean geEngine_SetDriverAndMode(	geEngine *Engine,
												geDriver *Driver,
												geDriver_Mode *DriverMode);

GENESISAPI geBoolean geEngine_SetDriverAndModeNoSplash(	geEngine *Engine,
												geDriver *Driver,
												geDriver_Mode *DriverMode);

//-------- drawing with the engine (Decals & Misc Polys)

GENESISAPI geBoolean GENESISCC geEngine_DrawBitmap(const geEngine *Engine,
	const geBitmap *Bitmap,
	const geRect * Source, uint32 x, uint32 y);

GENESISAPI geBoolean GENESISCC geEngine_DrawAlphaBitmap(
		geEngine * Engine,
		geBitmap * pBitmap,
		geVec3d * VertUVArray,
		geCamera * ClipCamera,	// if null, uses full screen
		GE_Rect * PixelRect,		// pixels in the "camera" view
		GE_Rect * PercentRect,	// percent of the "camera" view
		geFloat   Alpha,
		GE_RGBA * RGBA_Array
		);

GENESISAPI void GENESISCC geEngine_RenderPoly(const geEngine *Engine, const GE_TLVertex *Points,
						int NumPoints, const geBitmap *Texture, uint32 Flags);

GENESISAPI void GENESISCC geEngine_RenderPolyArray(const geEngine *Engine, const GE_TLVertex ** pPoints, int * pNumPoints, int NumPolys,
								const geBitmap *Texture, uint32 Flags);

// changed QD Shadows
GENESISAPI geBoolean geEngine_SetStencilShadowsEnable(geEngine *Engine, geBoolean Enable, int NumLights, geFloat r, geFloat g, geFloat b, geFloat a);

GENESISAPI void GENESISCC geEngine_RenderPolyStencil(const geEngine *Engine, const geVec3d *Points,
						int NumPoints, uint32 Flags);

GENESISAPI void GENESISCC geEngine_DrawShadowPoly(geEngine *Engine, GE_RGBA ShadowColor);
// end change
//-------- temporary pre-geBitmap hacks
geBoolean Engine_UploadBitmap(geEngine *Engine, DRV_Bitmap *Bitmap, DRV_Bitmap *ABitmap, geFloat Gamma);
geBoolean Engine_SetupPixelFormats(geEngine *Engine);
geRDriver_THandle * Engine_CreateTHandle(geEngine *Engine,int Width,int Height,int Mips, int EngineTexType);
void Engine_DestroyTHandle(geEngine *Engine,geRDriver_THandle * THandle);

#ifdef __cplusplus
}
#endif

#endif // GE_ENGINE_H
