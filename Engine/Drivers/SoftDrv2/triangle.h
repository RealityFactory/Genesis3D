/****************************************************************************************/
/*  Triangle.h                                                                          */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description:  Edge and Gradient calculations for triangle rasterizater              */
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


#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "basetype.h"
#include "swthandle.h"			// geRDriver_THandle

#ifdef __cplusplus
extern "C" {
#endif



//#define NOISE_FILTER

#define USE_FIXED_POINT 



#define OOZ_NUMERATOR_SHIFTER  (32)
#define OOZ_NUMERATOR     0xFFFFFFFF
/* oz 20 ooz 30 divprec 16 oozmulprec 8 ozmulprec 8  good. */
/* oz 20 ooz 26 divprec 16 oozmulprec 10 ozmulprec 6  good. */
/* oz 20 ooz 26 divprec 18 oozmulprec 12 ozmulprec 4  good. (small textures suffer a little)*/

//							 32
#define OZ_FXP_SHIFTER /*________________   *     */  (20)								
#define OOZ_FXP_SHIFTER       (26)											

#define OOZ_DIV_PRECISION_BITS (18)

#define OOZ_MULTIPLY_PRECISION_BITS   (12)
#define OZ_MULTIPLY_PRECISION_BITS    (4)				// affects the accuracy to the nearest texel 

#define OOZ_DIV_PREP_RSHIFT    (OOZ_FXP_SHIFTER - OOZ_DIV_PRECISION_BITS)				

#define RGB_FXP_SHIFTER (15)	
#define Z_FXP_SHIFTER (8)

#define Z_FXP_MULTIPLIER   ((float)( 1 <<   Z_FXP_SHIFTER ))	
#define OOZ_FXP_MULTIPLIER ((float)( 1 << OOZ_FXP_SHIFTER ))	
#define OZ_FXP_MULTIPLIER  ((float)( 1 <<  OZ_FXP_SHIFTER )) 
#define RGB_FXP_MULTIPLIER ((float)( 1 << RGB_FXP_SHIFTER ))

#define OOZ_MUL_PREP_RSHIFT  (OOZ_NUMERATOR_SHIFTER - OOZ_FXP_SHIFTER + OOZ_DIV_PREP_RSHIFT - OOZ_MULTIPLY_PRECISION_BITS)		
#define  OZ_MUL_PREP_RSHIFT  (OZ_FXP_SHIFTER - OZ_MULTIPLY_PRECISION_BITS)									
/* fixed point is used for the edge and span iterators.   

		The 1/z (OOZ) iterator is stored with 6.26 precision.
		The other 1/U iterators are stored with 12.20 precision.
		(These were experimentially determined by minimizing the visible errors.)

		(OOZ_NUMERATOR is ~= 0.32)

		so, to compute a 16.16 fixed point U from 1/z and U/z

		 U  ={[(2^OZ_FXP_SHIFTER)/((1/z * 2^OOZ_FXP_SHIFTER)>>OOZ_DIV_PRE_RSHIFT)]>>OOZ_MUL_PREP_RSHIFT} * {[U/z * 2^OZ_FXP_SHIFTER]>>OZ_MUL_PRE_PRSHIFT}

		 U  ={[(2^32)/((1/z * 2^26)>>8)]>>2} * {[U/z * 2^20]>>16}
		    ={[(2^32)/(   1/z * 2^18   ]>>2} * {   U/z * 2^4    }
		    ={[       z * 2^14         ]>>2} * {   U/z * 2^4    }
			={        z * 2^12             } * {   U/z * 2^4    }
			=                        (z*U/z) * 2^16
			=                              U * 2^16



*/


#if OOZ_DIV_PREP_RSHIFT > 0
	#define OOZ_DIV_PREP(OOZ)   ((OOZ)>>OOZ_DIV_PREP_RSHIFT)
#else
	#define OOZ_DIV_PREP(OOZ)   ((OOZ)<<(-OOZ_DIV_PREP_RSHIFT))
#endif

#if OZ_MUL_PREP_RSHIFT > 0
	#define OZ_MUL_PREP(OZ)		((OZ)>>OZ_MUL_PREP_RSHIFT)   
#else
	#define OZ_MUL_PREP(OZ)		((OZ)<<(-OZ_MUL_PREP_RSHIFT))   
#endif

#if OOZ_MUL_PREP_RSHIFT > 0
	#define OOZ_MUL_PREP(OZ)	((OZ)>>OOZ_MUL_PREP_RSHIFT)   
#else
	#define OOZ_MUL_PREP(OZ)	((OZ)<<(-OOZ_MUL_PREP_RSHIFT))   
#endif

#if (OOZ_MULTIPLY_PRECISION_BITS > 8)
	#define OOZ_MUL_Z(OOZ,Z)  ( ((OOZ)>>(OOZ_MULTIPLY_PRECISION_BITS-8)) * (Z) )
#else
	#define OOZ_MUL_Z(OOZ,Z)  ( ((OOZ)<<(8-OOZ_MULTIPLY_PRECISION_BITS)) * (Z) )
#endif

#if (OOZ_MULTIPLY_PRECISION_BITS > 8)
	#define GRADIENT_OOZ_MUL_Z(OOZ,Z)  ( ((OOZ)>>(OOZ_FXPMULTIPLY_PRECISION_BITS-8)) * (Z) )
#else
	#define GRADIENT_OOZ_MUL_Z(OOZ,Z)  ( ((OOZ)<<(8-OOZ_MULTIPLY_PRECISION_BITS)) * (Z) )
#endif

#if (OZ_MULTIPLY_SHIFTER <0) || (OOZ_MULTIPLY_SHIFTER<0)
	error.  
#endif

#define USE_FIXED_POINT 
#define DEST16BIT

#ifdef USE_FIXED_POINT
#define FXFL int32
#define FXFL_OOZ(XXX)   ((int32)((XXX) * OOZ_FXP_MULTIPLIER))
#define FXFL_OZ(XXX)    ((int32)((XXX) *  OZ_FXP_MULTIPLIER))
#define FXFL_RGB(XXX)   ((int32)((XXX) * RGB_FXP_MULTIPLIER))
#define FXFL_Z(XXX)     ((int32)((XXX) *   Z_FXP_MULTIPLIER))
#define OOZ_FXP_TO_16_16(XXX)  ( (XXX)>>(OOZ_FXP_SHIFTER-16) )
#define  OZ_FXP_TO_16_16(XXX)  ( (XXX)>>(OZ_FXP_SHIFTER-16)  )
#define     Z_FXP_TO_INT(XXX)  ( (XXX)>>Z_FXP_SHIFTER        )
#define RGB_FXP_TO_16_16(XXX)  (XXX)
#else
	error.
#endif


#define TEXTUREPIXEL	unsigned char 
#define LIGHTMAPPIXEL   unsigned char
#ifdef DEST16BIT
	#define DESTPIXEL	unsigned short
#else
	#define DESTPIXEL	unsigned char
#endif
#define ZMAPPIXEL		unsigned short
#define ALPHAMAPPIXEL	unsigned short

#define DESTPIXEL_SHIFTER (sizeof(DESTPIXEL)/2)
#ifdef USE_DIBS
#define TOPDOWN_OR_BOTTOMUP(XXX) (-(XXX))  // + for TOPDOWN, - for BOTTOMUP.
#else
#define TOPDOWN_OR_BOTTOMUP(XXX) ((XXX))  // + for TOPDOWN, - for BOTTOMUP.
#endif

// ROP FLAGS
#define TMAP		0x1
#define LSHADE		0x2
#define LFLAT		0x4
#define LMAP        0x8
#define AFLAT		0x10
#define AMAP		0x20
#define ZSET		0x40
#define ZTEST		0x80
#define ZBUF		0x100		// any zbuffering
#define SBUF		0x200
#define D565		0x200		// not really a rop flag, but used in generating spans



#define TRASTER_SMALL_DIVIDE_TABLESIZE 129



typedef struct Triangle_Gradients 
{
	float OneOverZ[3];			// 1/Z per vtx	(if Affine Z per vtx)  Normalized!
								// all Z's stored here are normalized to [0..1]  see FZScale below
	float UOverZ[3];			// U/Z per vtx	(if Affine U per vtx)  
	float VOverZ[3];			// V/Z per vtx	(if Affine V per vtx)  
	float FdOneOverZdX;			// d(1/Z)/dX	(if Affine dZ/dX )
	float dOneOverZdY;			// d(1/Z)/dY	(if Affine dZ/dY )
	float FdUOverZdX;			// d(U/Z)/dX	(if Affine dU/dX )
	float dUOverZdY;			// d(U/Z)/dY	(if Affine dU/dY )
	float FdVOverZdX;			// d(V/Z)/dX	(if Affine dV/dX )
	float dVOverZdY;			// d(V/Z)/dY	(if Affine dV/dY )

	int SubSpanWidth;			// maximum affine subdivision width for this poly (power of 2)
	int SubSpanShift;			//   shift to divide by SubSpanLength   1<<SubSpanShift == SubSpanWidth
	int Affine;					// flag:  if true, then all gradients are NOT 1/Z, just Z

	// lighting interpolation is always affine
	float FdRdX;				// dR/dX (the F means float, since there is a fixed point version of this also)
	float  dRdY;				// dR/dY  
	float FdGdX;				// dG/dX (the F means float, since there is a fixed point version of this also)
	float  dGdY;				// dG/dY  
	float FdBdX;				// dB/dX (the F means float, since there is a fixed point version of this also)
	float  dBdY;				// dB/dY  

	FXFL dOneOverZdX;			// fixed point FdOneOverZdX (FXFL_OOZ)		 see precision comments
	FXFL dUOverZdX;				// fixed point FdUOverZdX   (FXFL_OZ)
	FXFL dVOverZdX;				// fixed point FdVOverZdX   (FXFL_OZ)
	FXFL dRdX;					// fixed point FdRdX        (FXFL_RGB)
	FXFL dGdX;					// fixed point FdGdX        (FXFL_RGB)
	FXFL dBdX;					// fixed point FdBdX        (FXFL_RGB)

	float FZScale;				// Z is normalized to a max Z of 1.0.  so Normalized_Z = Z/FZScale;
	FXFL  ZScale;				// fixed point FZScale      (FXFL_Z)
} Triangle_Gradients;


typedef struct Triangle_Edge
{
	int32 X;					// current X of edge pixel 
	int32 XStep;				// X + XStep = X for next edge point
	uint32 Dest;				// current address into destination bits for edge pixel
	int32 DestStep;				// Dest + DestStep = Dest for next edge point
	int32 Numerator, Denominator;// DDA fraction
	int32 ErrorTerm;				// DDA error

	FXFL OneOverZ;				// current 1/Z 	 (if Gradients.Affine: Z)
	FXFL OneOverZStep;			// OneOverZ + OneOverZStep = 1/Z for next edge point
	FXFL UOverZ;				// current U/Z   (if Gradients.Affine: U)
	FXFL UOverZStep;			// UOneOverZ + UOverZStep = U/Z for next edge point
	FXFL VOverZ;				// V/Z and step     (if Gradients.Affine: V )
	FXFL VOverZStep;			// VOneOverZ + VOverZStep = V/Z for next edge point
	FXFL R;						// current R
	FXFL RStep;					// R + RStep = R for next edge point
	FXFL G;						// current G
	FXFL GStep;					// G + GStep = G for next edge point
	FXFL B;						// B
	FXFL BStep;					// B + BStep = B for next edge point
	
	// (these are copied from Gradients) 
	FXFL dOneOverZdX;	
	FXFL dUOverZdX; 	
	FXFL dVOverZdX; 	
	FXFL dRdX;			
	FXFL dGdX;			
	FXFL dBdX;			
	//--------

	int Y;						// current Y of edge pixel
	int Height;					// number of vertical pixels in this edge

} Triangle_Edge;


#define Triangle_PaletteEntry uint32

typedef struct Triangle_Triangle
{
	int ROPFlags;						// bit flags for rop.
	Triangle_Gradients Gradients;		// Changes across the triangle with respect to the screen
	Triangle_Edge Left;					// current left edge of currently drawing triangle
	Triangle_Edge Right;				// current right edge of currently drawing triangle
	
	DESTPIXEL *DestBits;				// pointer into destination bits at left edge of span to draw
	TEXTUREPIXEL *TextureBits;			// pointer to first scan line of texture bits
	Triangle_PaletteEntry *Palette;		// pointer to texture palette
	int MipIndex;						// mip level; index 0 is highest detail
	int StrideShift;					// Texture is always a power of two width.  This is the power.

	int   SpanWidth;					// Width in pixels of current span

	int UMask;							// Mask U by this for tiling
	int VMask;							// Mask V by this for tiling
	
	LIGHTMAPPIXEL *LightMapBits;		// pointer to first scan line of light map bits as supplied by engine
										// scale a U or V down into the lightmap
	int LightMapWidth;					// in lightmap pixels (luxels)
	int LightMapStride;					// in bytes
	int LightMapHeight;					// in lightmap pixels (luxels)
	int LightMapMaxU;					// maximum lightmap U (in 16:16 fixed point)
	int LightMapMaxV;					// maximum lightmap V (in 16:16 fixed point)
	int32 LightMapShiftU;						// 16:16 shift such that LMU = (u-LightMapShiftU)*LightMapScaleU
	int32 LightMapScaleU;						// 8:8 multiplication such that LMU = (u-LightMapShiftU)*LightMapScaleU
	int32 LightMapShiftV;						// 16:16 shift such that LMV = (v-LightMapShiftV)*LightMapScaleV
	int32 LightMapScaleV;						// 8:8 multiplication such that LMV = (v-LightMapShiftV)*LightMapScaleV

	ZMAPPIXEL *ZMapBits;				// pointer into zmap bits at left edge of span to draw.
	geRDriver_THandle *ZMap;			// reference to currently selected zmap
	geRDriver_THandle *DestMap;			// reference to currently selected destination bitmap
	int ZBufferAddressDelta;			// Destination bitmap bits + ZBufferAddressDelta = Zbuffer bits

	int SmallDivideTable[TRASTER_SMALL_DIVIDE_TABLESIZE];	// for quick divides by 1..TRASTER_SMALL_DIVIDE_TABLESIZE

	float MaxAffineSize;				// if triangle is smaller than this, the rasterizer reverts to affine.

	geBoolean IsLightMapSetup;			// GE_TRUE if light map is already set up. 
	void (*LightMapSetup)();			// called to set up lightmap 

	#ifdef NOISE_FILTER		
	int RandomIndex;					// experimental: to reduce 16bit banding			
	int RandomTable[256];
	unsigned char RandomTableIndex=0;
	#endif
} Triangle_Triangle;

// Global used by span routines.  Not in Triangle for simplified asm addressing.

int32 OneOverZ,UOverZ,VOverZ;			// Current 1/Z, U/Z, V/Z for left edge of span (and subspans)
int32 R,G,B;							// Current R,G,B,A for left edge of span (and subspans) 
										//  R = Red Channel, G = Green Channel, B = Blue Channel
int32 A,OneMinusA;						//  A = Alpha Channel   A is 0..16    OneMinusA is 16..0

Triangle_Triangle Triangle;

	// computes gradients for triangle.  
	// Doesn't set any global variables (Triangle), but may reference them for mode info (sorry)
geBoolean GENESISCC Triangle_GradientsCompute( 
					Triangle_Gradients *G,			// Gradients to compute (yeah, this is also global)
					const DRV_TLVertex *pVertices,	// vertex corners of triangle (U,V,R,G,B,etc are [0..1])
					float TextureWidth,				// Width of texture in pixels  (scale U up to [0..Width])
					float TextureHeight);			// Height of texture in pixels (scale V up to [0..Height])


	//	computes gradients for an edge of the triangle.
	//  Doesn't set or reference any global variables (Triangle).
void GENESISCC Triangle_EdgeCompute( 
		Triangle_Edge *E,							// Edge to compute
		const Triangle_Gradients *Gradients,		// Gradients to use (yeah, this is also global)
		const DRV_TLVertex *pVertices,				// vertex corners of triangle (U,V,R,G,B,etc are [0..1])
		int Top,									// Index into pVertices for 'top' (smallest y) vertex 
		int Bottom,									// Index into pVertices for 'bottom' (greatest y) vertex
		int IsLeftEdge);							// Flag:  is this on the left side of the triangle
													//   only x is computed for the right side

#ifdef __cplusplus
}
#endif



#endif