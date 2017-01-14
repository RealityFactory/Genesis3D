/****************************************************************************************/
/*  TRaster.H                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description:  API layer for Triangle Rasterizer                                     */
/*                                                                                      */
/*  Code fragments from Chris Hecker's texture mapping articles used with               */
/*  permission.  http://www.d6.com/users/checker                                        */
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
#ifndef TRASTER_H
#define TRASTER_H
//	 TRaster
//   Triangle Rasterizer
//

#include "basetype.h"
#include "rop.h"
#include "swthandle.h"			// geRDriver_THandle

#ifdef __cplusplus
extern "C" {
#endif

#define TRASTER_DEFAULT_MAX_AFFINE_SIZE (32)

typedef struct
{
	unsigned short *BitPtr;						// pointer to lightmap bits
	int Height;									// Lightmap Height
	int Width;									// Lightmap Width
	float LightMapShiftU,LightMapShiftV;		// Lightmap coordinates u (LMU) = (TextureU-LightMapShiftU)*LightMapScaleU
	float LightMapScaleU,LightMapScaleV;		// Lightmap coordinates v (LMV) = (TextureV-LightMapShiftV)*LightMapScaleV
	int MipIndex;								// Texture's mipping level
} TRaster_Lightmap;


		// Call this before calling _Rasterize
void GENESISCC TRaster_Setup(
		int MaxAffineSize,						// maximum width or height for a non-perspective corrected poly
		geRDriver_THandle *Dest,				// destination bitmap
		geRDriver_THandle *ZBuffer,				// zbuffer bitmap
		void (*Callback)(TRaster_Lightmap *LM));// initialize lightmap callback 

		// expected ranges for pVertices elements:
		//   x,y  (pretty much anything)  but these are in screen space...
		//   z  (0..65536)  
		//   r,g,b:  0..255
		//   a: 0..255
		//   pVertices expected in clockwise winding order.  Counter clockwise will not be rasterized.
void GENESISCC TRaster_Rasterize( 
		geROP ROP,								// ROP (raster operation to use for this triangle)
		geRDriver_THandle *Texture,				// Texture to use (can be NULL if ROP doesn't use it)
		int MipIndex,							// index of MIP level to use. 0 is highest detail
		const DRV_TLVertex 	*pVertices);		// corners of the triangle (there must be 3 of these!)


#ifdef __cplusplus
}
#endif

#endif
