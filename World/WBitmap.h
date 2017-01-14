/****************************************************************************************/
/*  WBitmap.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Creates geBitmaps from the data in the BSP, that are used to render    */
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
#ifndef WBITMAP_H
#define WBITMAP_H

#include <Assert.h>

#include "BaseType.h"
#include "GBSPFile.h"
#include "Bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

//==
// This stuff should really go in GBSPFile.h...
	#define		TEXTURE_SKYBOX				(1<<0)				// This is a skybox texture

	//	TexInfo Flags (Global shared info about each surface)
	#define		TEXINFO_MIRROR				(1<<0)
	#define		TEXINFO_FULLBRIGHT			(1<<1)				// No lightmap/fullbright
	#define		TEXINFO_SKY					(1<<2)				// Sky face
	#define		TEXINFO_LIGHT				(1<<3)				// Face produces light
	#define		TEXINFO_TRANS				(1<<4)				// Face has transparency
	#define		TEXINFO_GOURAUD				(1<<5)
	#define		TEXINFO_FLAT				(1<<6)
	#define		TEXINFO_NO_LIGHTMAP			(1<<15)
//==

//================================================================================
//	#defs
//================================================================================
#define		WBITMAP_SKYBOX				(1<<0)				// This is a skybox wbitmap

//================================================================================
//	Structure defines
//================================================================================
typedef struct geWBitmap			geWBitmap;
typedef struct geWBitmap_Pool		geWBitmap_Pool;

//================================================================================
//	Function defines
//================================================================================
geWBitmap_Pool *geWBitmap_Pool_Create(GBSP_BSPData *BSPData);
void geWBitmap_Pool_Destroy(geWBitmap_Pool *Pool);
int32 geWBitmap_Pool_GetWBitmapCount(geWBitmap_Pool *Pool);
geWBitmap *geWBitmap_Pool_GetWBitmapByBitmap(geWBitmap_Pool *Pool, const geBitmap *Bitmap);
geWBitmap *geWBitmap_Pool_GetWBitmapByIndex(geWBitmap_Pool *Pool, int32 Index);
geBitmap *geWBitmap_Pool_GetBitmapByIndex(geWBitmap_Pool *Pool, int32 Index);
geBitmap *geWBitmap_Pool_GetBitmapByName(geWBitmap_Pool *Pool, const char *BitmapName);
geBoolean geWBitmap_Pool_CreateAllWBitmaps(geWBitmap_Pool *Pool, GBSP_BSPData *BSPData);
void geWBitmap_Pool_DestroyAllWBitmaps(geWBitmap_Pool *Pool);
uint32 geWBitmap_GetFlags(geWBitmap *WBitmap);
geBitmap *geWBitmap_GetBitmap(geWBitmap *WBitmap);
int32 geWBitmap_GetVisFrame(geWBitmap *WBitmap);
geBoolean geWBitmap_SetVisFrame(geWBitmap *WBitmap, int32 VisFrame);

#ifdef __cplusplus
}
#endif

#endif
