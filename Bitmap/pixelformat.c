/****************************************************************************************/
/*  PixelFormat.c                                                                       */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

/*}{****************************************************************************************/

/***

this code only work on Intel-Endian CPU's

	{
	uint32 x = (('A'<<24) + ('B'<<16) + ('C'<<8) + 'D');

		assert( memcmp(&x,"DCBA",4) == 0 );
	}

***/

#include <stdlib.h>
#include <assert.h>
#include "pixelformat.h"

#define isinrange(x,lo,hi)	( (x)>=(lo) && (x)<=(hi) )

#define SHIFTL(val,shift)	( (shift) >= 0 ? ((val)<<(shift)) : ((val)>>(-(shift))) )
#define SHIFTR(val,shift)	( (shift) >= 0 ? ((val)>>(shift)) : ((val)<<(-(shift))) )

// internal protos

extern const gePixelFormat_Operations * gePixelFormat_Operations_Array;

/*}{****************************************************************************************/

GENESISAPI uint32 GENESISCC gePixelFormat_ComposePixel(gePixelFormat Format,int R,int G,int B,int A)
{
const gePixelFormat_Operations * ops;
	ops = &gePixelFormat_Operations_Array[Format];
	assert(ops);
	assert(ops->ComposePixel);
	return ops->ComposePixel(R,G,B,A);
}

GENESISAPI void GENESISCC gePixelFormat_DecomposePixel(gePixelFormat Format,uint32 Pixel,int *R,int *G,int *B,int *A)
{
const gePixelFormat_Operations * ops;
	ops = &gePixelFormat_Operations_Array[Format];
	assert(ops);
	assert(ops->DecomposePixel);
	ops->DecomposePixel(Pixel,R,G,B,A);
}

GENESISAPI uint32 GENESISCC gePixelFormat_GetPixel(gePixelFormat Format,uint8 **ppData)
{
const gePixelFormat_Operations * ops;
	ops = &gePixelFormat_Operations_Array[Format];
	assert(ops);
	assert(ops->GetPixel);
	return ops->GetPixel(ppData);
}

GENESISAPI void GENESISCC gePixelFormat_PutPixel(gePixelFormat Format,uint8 **ppData,uint32 Pixel)
{
const gePixelFormat_Operations * ops;
	ops = &gePixelFormat_Operations_Array[Format];
	assert(ops);
	assert(ops->PutPixel);
	ops->PutPixel(ppData,Pixel);
}

GENESISAPI void GENESISCC gePixelFormat_GetColor(gePixelFormat Format,uint8 **ppData,int *R,int *G,int *B,int *A)
{
const gePixelFormat_Operations * ops;
	ops = &gePixelFormat_Operations_Array[Format];
	assert(ops);
	assert(ops->GetColor);
	ops->GetColor(ppData,R,G,B,A);
}
GENESISAPI void GENESISCC gePixelFormat_PutColor(gePixelFormat Format,uint8 **ppData,int R,int G,int B,int A)
{
const gePixelFormat_Operations * ops;
	ops = &gePixelFormat_Operations_Array[Format];
	assert(ops);
	assert(ops->PutColor);
	ops->PutColor(ppData,R,G,B,A);
}

/*}{****************************************************************************************/

GENESISAPI uint32 GENESISCC gePixelFormat_ConvertPixel(gePixelFormat Format,uint32 Pixel,gePixelFormat ToFormat)
{
int R,G,B,A;
		gePixelFormat_DecomposePixel(Format,Pixel,&R,&G,&B,&A);
return	gePixelFormat_ComposePixel(ToFormat,R,G,B,A);
}

GENESISAPI const gePixelFormat_Operations * GENESISCC gePixelFormat_GetOperations( gePixelFormat Format )
{
	if ( ! gePixelFormat_IsValid(Format) )
		return NULL;
	else
		return & gePixelFormat_Operations_Array[Format];
}

/*}{****************************************************************************************/

GENESISAPI unsigned int GENESISCC gePixelFormat_BytesPerPel( gePixelFormat Format )
{
	assert( gePixelFormat_IsValid(Format) );
return gePixelFormat_Operations_Array[Format].BytesPerPel;
}

GENESISAPI geBoolean GENESISCC gePixelFormat_HasPalette(  gePixelFormat Format )
{
	assert( gePixelFormat_IsValid(Format) );
return gePixelFormat_Operations_Array[Format].HasPalette;
}

GENESISAPI geBoolean GENESISCC gePixelFormat_HasAlpha(  gePixelFormat Format )
{
	assert( gePixelFormat_IsValid(Format) );
//	if ( Format == GE_PIXELFORMAT_16BIT_1555_ARGB ) @@
//		return 0;
return gePixelFormat_Operations_Array[Format].AMask;
}

static int NumBitsOn(uint32 val)
{
uint32 count = 0;
	while(val)
	{
		count += val&1;
		val >>= 1;
	}
return count;
}

GENESISAPI geBoolean GENESISCC gePixelFormat_HasGoodAlpha(  gePixelFormat Format )
{
	assert( gePixelFormat_IsValid(Format) );
	
	if ( NumBitsOn(gePixelFormat_Operations_Array[Format].AMask) > 1 )
		return GE_TRUE;
	else
		return GE_FALSE;
}

GENESISAPI geBoolean GENESISCC gePixelFormat_IsRaw(  gePixelFormat Format )
{
	assert( gePixelFormat_IsValid(Format) );
	if ( gePixelFormat_Operations_Array[Format].ComposePixel )
		return GE_TRUE;
	else
		return GE_FALSE;
}

GENESISAPI const char * GENESISCC gePixelFormat_Description(  gePixelFormat Format )
{
	assert( gePixelFormat_IsValid(Format) );
return gePixelFormat_Operations_Array[Format].Description;
}

GENESISAPI geBoolean GENESISCC gePixelFormat_IsValid(gePixelFormat Format)
{
	if ( (int)Format < 0 || (int)Format >= GE_PIXELFORMAT_COUNT )
		return GE_FALSE;
	return GE_TRUE;
}

/*}{****************************************************************************************/

/*}{****************************************************************************************/

uint32	GetPixel_8bit(uint8 **ppData)
{
uint32 pel;
	pel = *((uint8 *)(*ppData));
	(*ppData) += 1;
return pel;
}

void	PutPixel_8bit(uint8 **ppData,uint32 Pixel)
{
	*((uint8 *)(*ppData)) = (uint8)Pixel;
	(*ppData) += 1;
}

uint32	GetPixel_16bit(uint8 **ppData)
{
uint32 pel;
	pel = *((uint16 *)(*ppData));
	(*ppData) += 2;
return pel;
}

void	PutPixel_16bit(uint8 **ppData,uint32 Pixel)
{
	*((uint16 *)(*ppData)) = (uint16)Pixel;
	(*ppData) += 2;
}

uint32	GetPixel_24bit(uint8 **ppData)
{
uint32 pel;
	pel  = (*ppData)[0] <<16;
	pel += (*ppData)[1] << 8;
	pel += (*ppData)[2];
	(*ppData) += 3;
return pel;
}

void	PutPixel_24bit(uint8 **ppData,uint32 Pixel)
{
	(*ppData)[0] = (uint8)(Pixel>>16);
	(*ppData)[1] = (uint8)(Pixel>>8);
	(*ppData)[2] = (uint8)(Pixel);
	(*ppData) += 3;
}

uint32	GetPixel_32bit(uint8 **ppData)
{
uint32 pel;
	pel = *((uint32 *)(*ppData));
	(*ppData) += 4;
return pel;
}

void	PutPixel_32bit(uint8 **ppData,uint32 Pixel)
{
	*((uint32 *)(*ppData)) = (uint32)Pixel;
	(*ppData) += 4;
}

/*}{****************************************************************************************/

//#define RGB_to_Gray(R,G,B)	((R + G + B)/3)
#define RGB_to_Gray(R,G,B)	max(max(R,G),B)

uint32	Compose_8bitGray   (int R,int G,int B,int A)
{
return (uint32)RGB_to_Gray(R,G,B);
}

void	Decompose_8bitGray (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*R = *G = *B = (int)Pixel;
	*A = 255;
}

void	Get_8bitGray(uint8 **ppData,int *R,int *G,int *B,int *A)
{
int V;
	V = **ppData;
	(*ppData) += 1;
	*R = *G = *B = V; *A = 255;
}

void	Put_8bitGray(uint8 **ppData,int  R,int  G,int  B,int  A)
{
int V;
	V = RGB_to_Gray(R,G,B);
	**ppData = V;
	(*ppData) += 1;
}

/*}{****************************************************************************************/

uint32	Compose_nada   (int R,int G,int B,int A)
{
return 0;
}

void	Decompose_nada (uint32 Pixel,int *R,int *G,int *B,int *A)
{
}

void	Get_nada(uint8 **ppData,int *R,int *G,int *B,int *A)
{
}

void	Put_nada(uint8 **ppData,int  R,int  G,int  B,int  A)
{
}

/*}{****************************************************************************************/

uint32	Compose_555rgb   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	( ((R>>3)<<10) + ((G>>3)<<5) + ((B>>3)) );
}

void	Decompose_555rgb (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*R = ((Pixel & 0x7C00)>>7) + 4;
	*G = ((Pixel & 0x03E0)>>2) + 4;
	*B = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Get_555rgb(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint16 Pixel;
	Pixel = *((uint16 *)*ppData);
	(*ppData) += 2;
	*R = ((Pixel & 0x7C00)>>7) + 4;
	*G = ((Pixel & 0x03E0)>>2) + 4;
	*B = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Put_555rgb(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint16 Pixel;
	Pixel = ( ((R>>3)<<10) + ((G>>3)<<5) + ((B>>3)) );
	*((uint16 *)*ppData) = Pixel;
	(*ppData) += 2;
}

uint32	Compose_555bgr   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	( ((B>>3)<<10) + ((G>>3)<<5) + ((R>>3)) );
}

void	Decompose_555bgr (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*B = ((Pixel & 0x7C00)>>7) + 4;
	*G = ((Pixel & 0x03E0)>>2) + 4;
	*R = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Get_555bgr(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint16 Pixel;
	Pixel = *((uint16 *)*ppData);
	(*ppData) += 2;
	*B = ((Pixel & 0x7C00)>>7) + 4;
	*G = ((Pixel & 0x03E0)>>2) + 4;
	*R = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Put_555bgr(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint16 Pixel;
	Pixel = ( ((B>>3)<<10) + ((G>>3)<<5) + ((R>>3)) );
	*((uint16 *)*ppData) = Pixel;
	(*ppData) += 2;
}


/*}{****************************************************************************************/

uint32	Compose_565rgb   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	( ((R>>3)<<11) + ((G>>2)<<5) + ((B>>3)) );
}

void	Decompose_565rgb (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*R = ((Pixel & 0xF800)>>8) + 4;
	*G = ((Pixel & 0x07E0)>>3) + 2;
	*B = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Get_565rgb(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint16 Pixel;
	Pixel = *((uint16 *)*ppData);
	(*ppData) += 2;
	*R = ((Pixel & 0xF800)>>8) + 4;
	*G = ((Pixel & 0x07E0)>>3) + 2;
	*B = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Put_565rgb(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint16 Pixel;
	Pixel = ( ((R>>3)<<11) + ((G>>2)<<5) + ((B>>3)) );
	*((uint16 *)*ppData) = Pixel;
	(*ppData) += 2;
}

uint32	Compose_565bgr   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	( ((B>>3)<<11) + ((G>>2)<<5) + ((R>>3)) );
}

void	Decompose_565bgr (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*B = ((Pixel & 0xF800)>>8) + 4;
	*G = ((Pixel & 0x07E0)>>3) + 2;
	*R = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Get_565bgr(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint16 Pixel;
	Pixel = *((uint16 *)*ppData);
	(*ppData) += 2;
	*B = ((Pixel & 0xF800)>>8) + 4;
	*G = ((Pixel & 0x07E0)>>3) + 2;
	*R = ((Pixel & 0x001F)<<3) + 4;
	*A = 255;
}

void	Put_565bgr(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint16 Pixel;
	Pixel = ( ((B>>3)<<11) + ((G>>2)<<5) + ((R>>3)) );
	*((uint16 *)*ppData) = Pixel;
	(*ppData) += 2;
}

/*}{****************************************************************************************/

uint32	Compose_4444   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B && (A&0xFF) == A );
	return	((A>>4)<<12) + ((R>>4)<<8) + ((G>>4)<<4) + (B>>4);
}

void	Decompose_4444 (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*A = ((Pixel & 0xF000)>>8);
	*R = ((Pixel & 0x0F00)>>4) + 8;
	*G = ((Pixel & 0x00F0)   ) + 8;
	*B = ((Pixel & 0x000F)<<4) + 8;
}

void	Get_4444(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint16 Pixel;
	Pixel = *((uint16 *)*ppData);
	(*ppData) += 2;
	*A = ((Pixel & 0xF000)>>8);
	*R = ((Pixel & 0x0F00)>>4) + 8;
	*G = ((Pixel & 0x00F0)   ) + 8;
	*B = ((Pixel & 0x000F)<<4) + 8;
}

void	Put_4444(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint16 Pixel;
	Pixel = ((A>>4)<<12) + ((R>>4)<<8) + ((G>>4)<<4) + (B>>4);
	*((uint16 *)*ppData) = Pixel;
	(*ppData) += 2;
}

uint32	Compose_1555   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B && (A&0xFF) == A );
	return	((A>>7)<<15) + ((R>>3)<<10) + ((G>>3)<<5) + ((B>>3)) ;
}

void	Decompose_1555 (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*R = ((Pixel & 0x7C00)>>7) + 4;
	*G = ((Pixel & 0x03E0)>>2) + 4;
	*B = ((Pixel & 0x001F)<<3) + 4;
	*A = (Pixel>>15)<<7;
}

void	Get_1555(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint16 Pixel;
	Pixel = *((uint16 *)*ppData);
	(*ppData) += 2;
	*R = ((Pixel & 0x7C00)>>7) + 4;
	*G = ((Pixel & 0x03E0)>>2) + 4;
	*B = ((Pixel & 0x001F)<<3) + 4;
	*A = (Pixel>>15)<<7;
}

void	Put_1555(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint16 Pixel;
	Pixel = ((A>>7)<<15) + ( ((R>>3)<<10) + ((G>>3)<<5) + ((B>>3)) );
	*((uint16 *)*ppData) = Pixel;
	(*ppData) += 2;
}


/*}{****************************************************************************************/

uint32	Compose_24rgb   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	(R<<16) + (G<<8) + B;
}

void	Decompose_24rgb (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*A = 255;
	*R = (Pixel>>16)&0xFF;
	*G = (Pixel>>8)&0xFF;
	*B = (Pixel)&0xFF;
}

void	Get_24rgb(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint8 * ptr = *ppData;
	*R = ptr[0];
	*G = ptr[1];
	*B = ptr[2];
	*A = 255;
	*ppData = ptr + 3;
}

void	Put_24rgb(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint8 * ptr = *ppData;
	ptr[0] = R;
	ptr[1] = G;
	ptr[2] = B;
	*ppData = ptr + 3;
}

uint32	Compose_24bgr   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	(B<<16) + (G<<8) + R;
}

void	Decompose_24bgr (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*A = 255;
	*B = (Pixel>>16)&0xFF;
	*G = (Pixel>>8)&0xFF;
	*R = (Pixel)&0xFF;
}

void	Get_24bgr(uint8 **ppData,int *R,int *G,int *B,int *A)
{
uint8 * ptr = *ppData;
	*B = ptr[0];
	*G = ptr[1];
	*R = ptr[2];
	*A = 255;
	*ppData = ptr + 3;
}

void	Put_24bgr(uint8 **ppData,int  R,int  G,int  B,int  A)
{
uint8 * ptr = *ppData;
	ptr[0] = B;
	ptr[1] = G;
	ptr[2] = R;
	*ppData = ptr + 3;
}

/*}{****************************************************************************************/

// RGBX in 32 bit is XBGR in bytes !!!

uint32	Compose_32rgbx   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	(R<<24) + (G<<16) + (B<<8);
}

void	Decompose_32rgbx (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*A = 255;
	*R = (Pixel>>24)&0xFF;
	*G = (Pixel>>16)&0xFF;
	*B = (Pixel>> 8)&0xFF;
}

void	Get_32rgbx(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	(*ppData) += 1;
	*B = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*R = **ppData; (*ppData) += 1;
	*A = 255;
}

void	Put_32rgbx(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	(*ppData) += 1;
	**ppData = B; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = R; (*ppData) += 1;
}

uint32	Compose_32xrgb   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	(R<<16) + (G<<8) + (B);
}

void	Decompose_32xrgb (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*A = 255;
	*R = (Pixel>>16)&0xFF;
	*G = (Pixel>> 8)&0xFF;
	*B = (Pixel   )&0xFF;
}

void	Get_32xrgb(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	*B = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*R = **ppData; (*ppData) += 1;
	(*ppData) += 1;
	*A = 255;
}

void	Put_32xrgb(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	**ppData = B; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = R; (*ppData) += 1;
	(*ppData) += 1;
}

uint32	Compose_32bgrx   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	(B<<24) + (G<<16) + (R<<8);
}

void	Decompose_32bgrx (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*A = 255;
	*B = (Pixel>>24)&0xFF;
	*G = (Pixel>>16)&0xFF;
	*R = (Pixel>> 8)&0xFF;
}

void	Get_32bgrx(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	(*ppData) += 1;
	*R = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*B = **ppData; (*ppData) += 1;
	*A = 255;
}

void	Put_32bgrx(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	(*ppData) += 1;
	**ppData = R; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = B; (*ppData) += 1;
}

uint32	Compose_32xbgr   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B );
	return	(B<<16) + (G<<8) + (R);
}

void	Decompose_32xbgr (uint32 Pixel,int *R,int *G,int *B,int *A)
{
	*A = 255;
	*B = (Pixel>>16)&0xFF;
	*G = (Pixel>> 8)&0xFF;
	*R = (Pixel   )&0xFF;
}

void	Get_32xbgr(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	*R = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*B = **ppData; (*ppData) += 1;
	(*ppData) += 1;
	*A = 255;
}

void	Put_32xbgr(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	**ppData = R; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = B; (*ppData) += 1;
	(*ppData) += 1;
}

/*}{****************************************************************************************/

uint32	Compose_32rgba   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B && (A&0xFF) == A );
	return	(R<<24) + (G<<16) + (B<<8) + A;
}

void	Decompose_32rgba (uint32 pixel,int *R,int *G,int *B,int *A)
{
	*R = (pixel>>24)&0xFF;
	*G = (pixel>>16)&0xFF;
	*B = (pixel>> 8)&0xFF;
	*A = (pixel    )&0xFF;
}

void	Get_32rgba(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	*A = **ppData; (*ppData) += 1;
	*B = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*R = **ppData; (*ppData) += 1;
}

void	Put_32rgba(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	**ppData = A; (*ppData) += 1;
	**ppData = B; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = R; (*ppData) += 1;
}

uint32	Compose_32argb   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B && (A&0xFF) == A );
	return	(A<<24) + (R<<16) + (G<<8) + (B);
}

void	Decompose_32argb (uint32 pixel,int *R,int *G,int *B,int *A)
{
	*A = (pixel>>24)&0xFF;
	*R = (pixel>>16)&0xFF;
	*G = (pixel>> 8)&0xFF;
	*B = (pixel    )&0xFF;
}

void	Get_32argb(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	*B = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*R = **ppData; (*ppData) += 1;
	*A = **ppData; (*ppData) += 1;
}

void	Put_32argb(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	**ppData = B; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = R; (*ppData) += 1;
	**ppData = A; (*ppData) += 1;
}

uint32	Compose_32bgra   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B && (A&0xFF) == A );
	return	(B<<24) + (G<<16) + (R<<8) + A;
}

void	Decompose_32bgra (uint32 pixel,int *R,int *G,int *B,int *A)
{
	*B = (pixel>>24)&0xFF;
	*G = (pixel>>16)&0xFF;
	*R = (pixel>> 8)&0xFF;
	*A = (pixel    )&0xFF;
}

void	Get_32bgra(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	*A = **ppData; (*ppData) += 1;
	*R = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*B = **ppData; (*ppData) += 1;
}

void	Put_32bgra(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	**ppData = A; (*ppData) += 1;
	**ppData = R; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = B; (*ppData) += 1;
}

uint32	Compose_32abgr   (int R,int G,int B,int A)
{
	assert( (R&0xFF) == R && (G&0xFF) == G && (B&0xFF) == B && (A&0xFF) == A );
	return	(A<<24) + (B<<16) + (G<<8) + (R);
}

void	Decompose_32abgr (uint32 pixel,int *R,int *G,int *B,int *A)
{
	*A = (pixel>>24)&0xFF;
	*B = (pixel>>16)&0xFF;
	*G = (pixel>> 8)&0xFF;
	*R = (pixel    )&0xFF;
}

void	Get_32abgr(uint8 **ppData,int *R,int *G,int *B,int *A)
{
	*R = **ppData; (*ppData) += 1;
	*G = **ppData; (*ppData) += 1;
	*B = **ppData; (*ppData) += 1;
	*A = **ppData; (*ppData) += 1;
}

void	Put_32abgr(uint8 **ppData,int  R,int  G,int  B,int  A)
{
	**ppData = R; (*ppData) += 1;
	**ppData = G; (*ppData) += 1;
	**ppData = B; (*ppData) += 1;
	**ppData = A; (*ppData) += 1;
}


/*}{********* the giant format-ops definition ****************/

static const gePixelFormat_Operations gePixelFormat_Operations_Array_Def[] = 
{
	{0,0,0,0,			0,0,0,0,	0,0,0,0,	0 ,0,		"invalid"},	

	{0,0,0,0,			0,0,0,0,	0,0,0,0,	1 ,1,		"8bit pal",		NULL,NULL,	NULL,NULL,	GetPixel_8bit, PutPixel_8bit},
	//	Gray = (R>>2) + (G>>1) + (B>>2)
	{0x3F,0x7F,0x3F,0,	-2,-1,-2,0, 3,0,3,0xFF,	1 ,0,		"8bit gray",	Compose_8bitGray,Decompose_8bitGray, Get_8bitGray,Put_8bitGray, GetPixel_8bit, PutPixel_8bit},

	// 16 bit (in uwords)
	{0x7C00	,0x03E0	,0x001F	,0,			 7, 2, -3, 0,	4,4,4,0xFF,		2,0,	"555 RGB",		Compose_555rgb,Decompose_555rgb, Get_555rgb,Put_555rgb,	GetPixel_16bit,PutPixel_16bit},	
	{0x001F	,0x03E0	,0x7C00	,0,			-3, 2,  7, 0,	4,4,4,0xFF,		2,0,	"555 BGR",		Compose_555bgr,Decompose_555bgr, Get_555bgr,Put_555bgr,	GetPixel_16bit,PutPixel_16bit},
	{0xF800	,0x07E0	,0x001F	,0,			 8, 3, -3, 0,	4,2,4,0xFF,		2,0,	"565 RGB",		Compose_565rgb,Decompose_565rgb, Get_565rgb,Put_565rgb,	GetPixel_16bit,PutPixel_16bit},
	{0x001F	,0x07E0	,0xF800	,0,			-3, 3,  8, 0,	4,2,4,0xFF,		2,0,	"565 BGR",		Compose_565bgr,Decompose_565bgr, Get_565bgr,Put_565bgr,	GetPixel_16bit,PutPixel_16bit},
	{0x0F00	,0x00F0	,0x000F	,0xF000, 	 4, 0, -4, 8,	8,8,8,8,		2,0,	"4444 ARGB",	Compose_4444,Decompose_4444, Get_4444,Put_4444,			GetPixel_16bit,PutPixel_16bit},
	{0x7C00	,0x03E0	,0x001F	,0x8000, 	 7, 2, -3, 8,	4,4,4,0x40,		2,0,	"1555 ARGB",	Compose_1555,Decompose_1555, Get_1555,Put_1555,			GetPixel_16bit,PutPixel_16bit},

	// 24 bit (in bytes!)
	{0x00FF0000,0x0000FF00,0x000000FF,0, 	16, 8, 0,0,		0,0,0,0xFF,	3,0,	"24bit RGB",	Compose_24rgb,Decompose_24rgb, Get_24rgb,Put_24rgb,	GetPixel_24bit,PutPixel_24bit},
	{0x000000FF,0x0000FF00,0x00FF0000,0, 	0 , 8,16,0,		0,0,0,0xFF,	3,0,	"24bit BGR",	Compose_24bgr,Decompose_24bgr, Get_24bgr,Put_24bgr,	GetPixel_24bit,PutPixel_24bit},
	{0x00FF0000,0x0000FF00,0x000000FF,0, 	16, 8, 0,0,		0,0,0,0xFF,	3,0,	"24bit YUV",	Compose_24rgb,Decompose_24rgb, Get_24rgb,Put_24rgb,	GetPixel_24bit,PutPixel_24bit},

	// 32 bit (in ulongs)
	{0xFF000000,0x00FF0000,0x0000FF00,0, 	24,16, 8,0,		0,0,0,0xFF,	4,0,	"32 bit RGBX",	Compose_32rgbx,Decompose_32rgbx, Get_32rgbx,Put_32rgbx,	GetPixel_32bit,PutPixel_32bit},
	{0x00FF0000,0x0000FF00,0x000000FF,0, 	16, 8, 0,0,		0,0,0,0xFF,	4,0,	"32 bit XRGB",	Compose_32xrgb,Decompose_32xrgb, Get_32xrgb,Put_32xrgb,	GetPixel_32bit,PutPixel_32bit},
	{0x0000FF00,0x00FF0000,0xFF000000,0, 	8 ,16,24,0,		0,0,0,0xFF,	4,0,	"32 bit BGRX",	Compose_32bgrx,Decompose_32bgrx, Get_32bgrx,Put_32bgrx,	GetPixel_32bit,PutPixel_32bit},
	{0x000000FF,0x0000FF00,0x00FF0000,0, 	0 , 8,16,0,		0,0,0,0xFF,	4,0,	"32 bit XBGR",	Compose_32xbgr,Decompose_32xbgr, Get_32xbgr,Put_32xbgr,	GetPixel_32bit,PutPixel_32bit},

	{0xFF000000,0x00FF0000,0x0000FF00,0x000000FF, 	24,16, 8, 0,	0,0,0,0,4,0,"32 bit RGBA",	Compose_32rgba,Decompose_32rgba, Get_32rgba,Put_32rgba,	GetPixel_32bit,PutPixel_32bit},
	{0x00FF0000,0x0000FF00,0x000000FF,0xFF000000,	16, 8, 0,24,	0,0,0,0,4,0,"32 bit ARGB",	Compose_32argb,Decompose_32argb, Get_32argb,Put_32argb,	GetPixel_32bit,PutPixel_32bit},
	{0x0000FF00,0x00FF0000,0xFF000000,0x000000FF, 	8 ,16,24, 0,	0,0,0,0,4,0,"32 bit BGRA",	Compose_32bgra,Decompose_32bgra, Get_32bgra,Put_32bgra,	GetPixel_32bit,PutPixel_32bit},
	{0x000000FF,0x0000FF00,0x00FF0000,0xFF000000,	0 , 8,16,24,	0,0,0,0,4,0,"32 bit ABGR",	Compose_32abgr,Decompose_32abgr, Get_32abgr,Put_32abgr,	GetPixel_32bit,PutPixel_32bit},

	{0,0,0,0, 			0,0,0,0,	0,0,0,0,	0 ,0,	"wavelet"},

	{0,0,0,0,			0,0,0,0,	0,0,0,0,	0 ,0,	"invalid"}
};

const gePixelFormat_Operations * gePixelFormat_Operations_Array = gePixelFormat_Operations_Array_Def;


/*}{************************************************/

/*
uint32	Compose_   (int R,int G,int B,int A)
{
}

void	Decompose_ (uint32 Pixel,int *R,int *G,int *B,int *A)
{
}

void	Get_(uint8 **ppData,int *R,int *G,int *B,int *A)
{
}

void	Put_(uint8 **ppData,int  R,int  G,int  B,int  A)
{
}
*/

/*}{****************************************************************************************/

