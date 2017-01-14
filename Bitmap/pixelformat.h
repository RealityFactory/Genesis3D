#ifndef	PIXELFORMAT_H
#define	PIXELFORMAT_H

/****************************************************************************************/
/*  PixelFormat.h                                                                       */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  The abstract Pixel primitives                                         */
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

#include "basetype.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum		// all supported formats (including shifts)
{
	GE_PIXELFORMAT_NO_DATA = 0,
	GE_PIXELFORMAT_8BIT,				// PAL
	GE_PIXELFORMAT_8BIT_GRAY,		// no palette (intensity from bit value)
	GE_PIXELFORMAT_16BIT_555_RGB,
	GE_PIXELFORMAT_16BIT_555_BGR,
	GE_PIXELFORMAT_16BIT_565_RGB,	// #5
	GE_PIXELFORMAT_16BIT_565_BGR, 
	GE_PIXELFORMAT_16BIT_4444_ARGB, // #7
	GE_PIXELFORMAT_16BIT_1555_ARGB, 
	GE_PIXELFORMAT_24BIT_RGB,		// #9
	GE_PIXELFORMAT_24BIT_BGR,
	GE_PIXELFORMAT_24BIT_YUV,		// * see note below
	GE_PIXELFORMAT_32BIT_RGBX, 
	GE_PIXELFORMAT_32BIT_XRGB, 
	GE_PIXELFORMAT_32BIT_BGRX, 
	GE_PIXELFORMAT_32BIT_XBGR,
	GE_PIXELFORMAT_32BIT_RGBA, 
	GE_PIXELFORMAT_32BIT_ARGB,		// #17
	GE_PIXELFORMAT_32BIT_BGRA, 
	GE_PIXELFORMAT_32BIT_ABGR,
	
	GE_PIXELFORMAT_WAVELET,			// #20 , Wavelet Compression

	GE_PIXELFORMAT_COUNT
} gePixelFormat;
	
/******

there's something wacked out about these format names :

	for 16 bit & 32 bit , the _RGB or _BGR refers to their order
		*in the word or dword* ; since we're on intel, this means
		the bytes in the data file have the *opposite* order !!
		(for example the 32 bit _ARGB is actually B,G,R,A in raw bytes)
	for 24 bit , the _RGB or _BGR refers to their order in the
		actual bytes, so that windows bitmaps actually have
		_RGB order in a dword !!

* YUV : the pixelformat ops here are identical to those of 24bit_RGB ;
		this is just a place-keeper to notify you that you should to a YUV_to_RGB conversion

*********/

#define GE_PIXELFORMAT_8BIT_PAL GE_PIXELFORMAT_8BIT

typedef uint32	(*gePixelFormat_Composer   )(int R,int G,int B,int A);
typedef void	(*gePixelFormat_Decomposer )(uint32 Pixel,int *R,int *G,int *B,int *A);

typedef void	(*gePixelFormat_ColorGetter)(uint8 **ppData,int *R,int *G,int *B,int *A);
typedef void	(*gePixelFormat_ColorPutter)(uint8 **ppData,int  R,int  G,int  B,int  A);

typedef uint32	(*gePixelFormat_PixelGetter)(uint8 **ppData);
typedef void	(*gePixelFormat_PixelPutter)(uint8 **ppData,uint32 Pixel);

typedef struct gePixelFormat_Operations
{
	uint32	RMask;
	uint32	GMask;
	uint32	BMask;
	uint32	AMask;

	int		RShift;
	int		GShift;
	int		BShift;
	int		AShift;

	int		RAdd;
	int		GAdd;
	int		BAdd;
	int		AAdd;

	int			BytesPerPel;
	geBoolean	HasPalette;
	char *		Description;
	
	gePixelFormat_Composer		ComposePixel;
	gePixelFormat_Decomposer	DecomposePixel;

	gePixelFormat_ColorGetter	GetColor;
	gePixelFormat_ColorPutter	PutColor;

	gePixelFormat_PixelGetter	GetPixel;
	gePixelFormat_PixelPutter	PutPixel;
} gePixelFormat_Operations;

	// the Masks double as boolean "HaveAlpha" .. etc..

GENESISAPI const gePixelFormat_Operations * GENESISCC gePixelFormat_GetOperations( gePixelFormat Format );

	// quick accessors to _GetOps
GENESISAPI geBoolean	GENESISCC gePixelFormat_IsValid(		gePixelFormat Format);
GENESISAPI unsigned int GENESISCC gePixelFormat_BytesPerPel(	gePixelFormat Format );
GENESISAPI geBoolean	GENESISCC gePixelFormat_HasPalette(		gePixelFormat Format );
GENESISAPI geBoolean	GENESISCC gePixelFormat_HasAlpha(		gePixelFormat Format );
GENESISAPI geBoolean	GENESISCC gePixelFormat_HasGoodAlpha(	gePixelFormat Format ); // more than 1 bit of alpha
GENESISAPI const char * GENESISCC gePixelFormat_Description(	gePixelFormat Format );
GENESISAPI geBoolean	GENESISCC gePixelFormat_IsRaw(			gePixelFormat Format );
									// 'Raw' means pixels can be made with the Compose operations

GENESISAPI uint32		GENESISCC gePixelFormat_ComposePixel(	gePixelFormat Format,int R,int G,int B,int A);
GENESISAPI void			GENESISCC gePixelFormat_DecomposePixel(	gePixelFormat Format,uint32 Pixel,int *R,int *G,int *B,int *A);
			
															// these four functions move ppData to the next pixel

GENESISAPI void			GENESISCC gePixelFormat_GetColor(gePixelFormat Format,uint8 **ppData,int *R,int *G,int *B,int *A);
GENESISAPI void			GENESISCC gePixelFormat_PutColor(gePixelFormat Format,uint8 **ppData,int R,int G,int B,int A);

GENESISAPI uint32		GENESISCC gePixelFormat_GetPixel(gePixelFormat Format,uint8 **ppData);
GENESISAPI void			GENESISCC gePixelFormat_PutPixel(gePixelFormat Format,uint8 **ppData,uint32 Pixel);
	
GENESISAPI uint32		GENESISCC gePixelFormat_ConvertPixel(gePixelFormat Format,uint32 Pixel,gePixelFormat ToFormat);


#ifdef __cplusplus
}
#endif

#endif

