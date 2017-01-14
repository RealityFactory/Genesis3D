/****************************************************************************************/
/*  TRaster.C                                                                           */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <assert.h>

#include "TRaster.h"
#include "Triangle.h"
#include "span.h"
#include "spanbuffer.h"

Span_DrawFunction TRaster_DrawSpan;		// used to draw for current triangle
void GENESISCC TRaster_LightMapSetup(void);

// Construct different edge-walkers depending on the various rops:

//SPANEDGES OPTIONS:  TMAP  LSHADE  ZBUF  SBUF

#define SPANEDGES LSHADE
static void GENESISCC TRaster_SpanEdges_LSHADE(int Height)
	{
		#include "SpanEdges_Factory.h"
	}

#define SPANEDGES LSHADE + ZBUF
static void GENESISCC TRaster_SpanEdges_LSHADE_ZBUF(int Height)
	{
		#include "SpanEdges_Factory.h"
	}

#define SPANEDGES TMAP + LSHADE 
static void GENESISCC TRaster_SpanEdges_TMAP_LSHADE(int Height)
	{
		#include "SpanEdges_Factory.h"
	}

#define SPANEDGES TMAP + LSHADE + ZBUF
static void GENESISCC TRaster_SpanEdges_TMAP_LSHADE_ZBUF(int Height)
	{
		#include "SpanEdges_Factory.h"
	}

#define SPANEDGES TMAP + LMAP + ZBUF + SBUF
static void GENESISCC TRaster_SpanEdges_TMAP_LMAP_ZBUF_SBUF(int Height)
	{
		#include "SpanEdges_Factory.h"
	}

#define SPANEDGES TMAP + LSHADE + ZBUF + SBUF
static void GENESISCC TRaster_SpanEdges_TMAP_LSHADE_ZBUF_SBUF(int Height)
	{
		#include "SpanEdges_Factory.h"
	}

#define SPANEDGES TMAP + LMAP
static void GENESISCC TRaster_SpanEdges_TMAP_LMAP(int Height)
	{
		#include "SpanEdges_Factory.h"
	}

#define SPANEDGES TMAP + LMAP + ZBUF
static void GENESISCC TRaster_SpanEdges_TMAP_LMAP_ZBUF(int Height)
	{
		#include "SpanEdges_Factory.h"
	}


typedef void  ( GENESISCC *TRaster_SpanEdgesFunction)(int Height);

typedef struct
{
	geROP ROP;
	int Flags;
	TRaster_SpanEdgesFunction SpanEdges;
} TRaster_RopInfo;


TRaster_RopInfo TRaster_RopTable[GE_ROP_END] =
//								
{//ROP ID						
{GE_ROP_LSHADE,	  					LSHADE,											TRaster_SpanEdges_LSHADE },		
{GE_ROP_LSHADE_ZSET,  				LSHADE | ZBUF | ZSET,							TRaster_SpanEdges_LSHADE_ZBUF },	
{GE_ROP_LSHADE_ZTEST,  				LSHADE | ZBUF | ZTEST,							TRaster_SpanEdges_LSHADE_ZBUF },	
{GE_ROP_LSHADE_ZTESTSET,  			LSHADE | ZBUF | ZTEST | ZSET,					TRaster_SpanEdges_LSHADE_ZBUF },	
{GE_ROP_LSHADE_AFLAT,				LSHADE | AFLAT,									TRaster_SpanEdges_LSHADE_ZBUF },	
{GE_ROP_LSHADE_AFLAT_ZSET,			LSHADE | AFLAT | ZBUF | ZSET,					TRaster_SpanEdges_LSHADE_ZBUF },	
{GE_ROP_LSHADE_AFLAT_ZTEST,			LSHADE | AFLAT | ZBUF | ZTEST,					TRaster_SpanEdges_LSHADE_ZBUF },	
{GE_ROP_LSHADE_AFLAT_ZTESTSET,		LSHADE | AFLAT | ZBUF | ZTEST | ZSET,			TRaster_SpanEdges_LSHADE_ZBUF },	
{GE_ROP_TMAP_LSHADE,  				TMAP | LSHADE,									TRaster_SpanEdges_TMAP_LSHADE },	
{GE_ROP_TMAP_LSHADE_ZSET,			TMAP | LSHADE | ZBUF | ZSET,					TRaster_SpanEdges_TMAP_LSHADE_ZBUF},
{GE_ROP_TMAP_LSHADE_ZTEST,  		TMAP | LSHADE | ZBUF | ZTEST,					TRaster_SpanEdges_TMAP_LSHADE_ZBUF},
{GE_ROP_TMAP_LSHADE_ZTESTSET,		TMAP | LSHADE | ZBUF | ZTEST | ZSET,			TRaster_SpanEdges_TMAP_LSHADE_ZBUF},
{GE_ROP_TMAP_LMAP_ZSET_SBUF,  		TMAP | LMAP | ZBUF | ZSET | SBUF,				TRaster_SpanEdges_TMAP_LMAP_ZBUF_SBUF },
{GE_ROP_TMAP_LSHADE_ZSET_SBUF,		TMAP | LSHADE | ZBUF | ZSET | SBUF,				TRaster_SpanEdges_TMAP_LSHADE_ZBUF_SBUF },
{GE_ROP_TMAP_LMAP_ZTESTSET,			TMAP | LMAP | ZBUF | ZTEST | ZSET,				TRaster_SpanEdges_TMAP_LMAP_ZBUF },
{GE_ROP_TMAP_LSHADE_AFLAT,			TMAP | LSHADE | AFLAT ,							TRaster_SpanEdges_TMAP_LSHADE },		
{GE_ROP_TMAP_LSHADE_AFLAT_ZSET,		TMAP | LSHADE | AFLAT | ZBUF | ZSET,			TRaster_SpanEdges_TMAP_LSHADE_ZBUF },	
{GE_ROP_TMAP_LSHADE_AFLAT_ZTEST,	TMAP | LSHADE | AFLAT | ZBUF | ZTEST,			TRaster_SpanEdges_TMAP_LSHADE_ZBUF },	
{GE_ROP_TMAP_LSHADE_AFLAT_ZTESTSET,	TMAP | LSHADE | AFLAT | ZBUF | ZTEST | ZSET,	TRaster_SpanEdges_TMAP_LSHADE_ZBUF },	
{GE_ROP_TMAP_LSHADE_AMAP,			TMAP | LSHADE | AMAP,							TRaster_SpanEdges_TMAP_LSHADE },	
{GE_ROP_TMAP_LSHADE_AMAP_ZSET,		TMAP | LSHADE | AMAP | ZBUF | ZSET,				TRaster_SpanEdges_TMAP_LSHADE_ZBUF },	
{GE_ROP_TMAP_LSHADE_AMAP_ZTEST,		TMAP | LSHADE | AMAP | ZBUF | ZTEST,			TRaster_SpanEdges_TMAP_LSHADE_ZBUF },	
{GE_ROP_TMAP_LSHADE_AMAP_ZTESTSET,	TMAP | LSHADE | AMAP | ZBUF | ZTEST | ZSET,		TRaster_SpanEdges_TMAP_LSHADE_ZBUF },	
{GE_ROP_TMAP_LMAP_AMAP,				TMAP | LMAP | AMAP,								TRaster_SpanEdges_TMAP_LMAP },		
{GE_ROP_TMAP_LMAP_AMAP_ZSET,		TMAP | LMAP | AMAP | ZBUF | ZSET,				TRaster_SpanEdges_TMAP_LMAP_ZBUF },	
{GE_ROP_TMAP_LMAP_AMAP_ZTEST,		TMAP | LMAP | AMAP | ZBUF | ZTEST,				TRaster_SpanEdges_TMAP_LMAP_ZBUF },	
{GE_ROP_TMAP_LMAP_AMAP_ZTESTSET,	TMAP | LMAP | AMAP | ZBUF | ZTEST | ZSET,		TRaster_SpanEdges_TMAP_LMAP_ZBUF },
{GE_ROP_TMAP_LMAP_AFLAT_ZTESTSET,	TMAP | LMAP | AFLAT | ZBUF |  ZTEST | ZSET,		TRaster_SpanEdges_TMAP_LMAP_ZBUF },
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT,			TMAP | LSHADE | AMAP | AFLAT, 							TRaster_SpanEdges_TMAP_LSHADE },
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZSET,	TMAP | LSHADE | AMAP | AFLAT | ZBUF |  ZSET,			TRaster_SpanEdges_TMAP_LSHADE_ZBUF },
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTEST,	TMAP | LSHADE | AMAP | AFLAT | ZBUF |  ZTEST,			TRaster_SpanEdges_TMAP_LSHADE_ZBUF },
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTESTSET,TMAP | LSHADE | AMAP | AFLAT | ZBUF |  ZTEST | ZSET,	TRaster_SpanEdges_TMAP_LSHADE_ZBUF },

};




void GENESISCC TRaster_LightMapSetup(void)
{
	TRaster_Lightmap LM;
	Triangle.IsLightMapSetup=GE_TRUE;
	LM.MipIndex = Triangle.MipIndex;

	Triangle.LightMapSetup(&LM);

	Triangle.LightMapBits = (LIGHTMAPPIXEL *)LM.BitPtr; 
	Triangle.LightMapWidth  = LM.Width;
	Triangle.LightMapHeight = LM.Height;

	Triangle.LightMapShiftU = (int)(65536.0f * LM.LightMapShiftU);
	Triangle.LightMapScaleU = (int)(256.0f   * LM.LightMapScaleU);

	Triangle.LightMapShiftV = (int)(65536.0f * LM.LightMapShiftV);
	Triangle.LightMapScaleV = (int)(256.0f   * LM.LightMapScaleV);

	Triangle.LightMapStride = Triangle.LightMapWidth * 3;
	Triangle.LightMapMaxU   = (Triangle.LightMapWidth-1)<<16;
	Triangle.LightMapMaxV   = (Triangle.LightMapHeight-1)<<16;
}


void GENESISCC TRaster_Setup(int MaxAffineSize,geRDriver_THandle *Dest, geRDriver_THandle *ZBuffer, void (*LightMapSetup)(TRaster_Lightmap *LM))
{
	int i;
	
	assert( MaxAffineSize > 0);
	assert( MaxAffineSize < TRASTER_SMALL_DIVIDE_TABLESIZE);
	assert( Dest != NULL );
	assert( LightMapSetup != NULL );
	
	Triangle.MaxAffineSize = (float)MaxAffineSize;


	for (i=1; i<TRASTER_SMALL_DIVIDE_TABLESIZE; i++)
		{
			Triangle.SmallDivideTable[i] = 0x10000/i;
		}
	#ifdef 	NOISE_FILTER
		for (i=0; i<256; i++)
			{
				Triangle.RandomTable[i] = (rand() & 0x03)<<24;
			}
	#endif

	Triangle.ZMap    = ZBuffer;
	Triangle.DestMap = Dest;
	Triangle.LightMapSetup = LightMapSetup;
}


		
		// expected ranges for pVertices elements:
		//   x,y  (pretty much anything)  but these are in screen space...
		//   z  (0..65536)  
		//   r,g,b:  0..255
		//   a: 0..255
		//   pVertices expected in clockwise winding order.  Counter clockwise will not be rasterized.
		// future: for larger polys, add parameter that allows previous gradient to be reused. (if it was computed)
void GENESISCC TRaster_Rasterize( 
		geROP ROP,
		geRDriver_THandle *Texture,
		int MipIndex,
		const DRV_TLVertex 	*pVertices)
{
	uint32 DestPtr;
	int32  DestWidth;
	int Top,Middle,Bottom;
	float Y0 = pVertices[0].y; 
	float Y1 = pVertices[1].y;
	float Y2 = pVertices[2].y;


	
	Triangle_Edge TopToBottom, TopToSplit, SplitToBottom;
	int SplitRight;		// set if triangle has two right-side edges (and one left-side edge)

	#ifdef NOISE_FILTER
	Triangle.RandomIndex=0;
	#endif

	assert( ROP >=0 ) ;
	assert( ROP <= GE_ROP_END );

	Triangle.ROPFlags = TRaster_RopTable[ROP].Flags;

	assert( TRaster_RopTable[ROP].SpanEdges != NULL );
	assert( ((Triangle.ROPFlags & AFLAT)  && (pVertices[0].a>=0.0f && pVertices[0].a<=255.1f)) || !(Triangle.ROPFlags & AFLAT));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[0].r>=0.0f && pVertices[0].r<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[1].r>=0.0f && pVertices[1].r<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[2].r>=0.0f && pVertices[2].r<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[0].g>=0.0f && pVertices[0].g<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[1].g>=0.0f && pVertices[1].g<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[2].g>=0.0f && pVertices[2].g<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[0].b>=0.0f && pVertices[0].b<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[1].b>=0.0f && pVertices[1].b<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	assert( ((Triangle.ROPFlags & LSHADE) && (pVertices[2].b>=0.0f && pVertices[2].b<=255.1f)) || !(Triangle.ROPFlags & LSHADE));
	
	if (Triangle.ROPFlags & TMAP)
		{
			int W,H;
			assert( Texture != NULL );
			W = Texture->Width >> MipIndex;
			H = Texture->Height >> MipIndex;
			if (Triangle_GradientsCompute(&(Triangle.Gradients),
						pVertices,(float)(W),(float)(H))==GE_FALSE)
				return;  // poly has no area.

			for(Triangle.StrideShift=1;((1<<Triangle.StrideShift) < W); Triangle.StrideShift++);
			assert( (1<<Triangle.StrideShift) == W );
			if (Triangle.ROPFlags & LMAP)
				{
					Triangle.IsLightMapSetup = GE_FALSE;
				}
			
			Triangle.UMask = W-1;
			Triangle.VMask = H-1;
			assert( MipIndex >= 0 );
			assert( MipIndex <  Texture->MipLevels );
			Triangle.MipIndex = MipIndex;
			Triangle.TextureBits  = (TEXTUREPIXEL *)Texture->BitPtr[MipIndex]; 
			if (Texture->PalHandle)
				Triangle.Palette  = (Triangle_PaletteEntry *)Texture->PalHandle->BitPtr[0];
		}
	else
		{
			if (Triangle_GradientsCompute(&(Triangle.Gradients),pVertices,1.0f,1.0f)==GE_FALSE)
				return;  // poly has no area.
		}
	if (Triangle.ROPFlags & ZBUF)
		{
			assert(Triangle.ZMap != NULL );
			assert(Triangle.ZMap->Width == Triangle.DestMap->Width);

			Triangle.ZBufferAddressDelta = ((int)(Triangle.ZMap->BitPtr[0])) - ((int)(Triangle.DestMap->BitPtr[0]));
		}
	TRaster_DrawSpan = Span_GetDrawFunction(ROP);
	A = (int32)(pVertices[0].a / (255.0f/16.0f) );
	OneMinusA = 16 - A;
		
	// sort vertices in y
	if(Y0 < Y1) 
		{
			if(Y2 < Y0) 
				{			Top = 2; Middle = 0; Bottom = 1; SplitRight = 1; } 
			else 
				{
					if(Y1 < Y2) 
						{	Top = 0; Middle = 1; Bottom = 2; SplitRight = 1; } 
					else 
						{	Top = 0; Middle = 2; Bottom = 1; SplitRight = 0; }
				}
		} 
	else 
		{
			if(Y2 < Y1) 
				{			Top = 2; Middle = 1; Bottom = 0; SplitRight = 0; } 
			else 
				{
					if(Y0 < Y2) 
						{	Top = 1; Middle = 0; Bottom = 2; SplitRight = 0; } 
					else 
						{	Top = 1; Middle = 2; Bottom = 0; SplitRight = 1; }
				}
		}

	
	Triangle_EdgeCompute(&TopToBottom,   &(Triangle.Gradients),pVertices,Top,   Bottom, SplitRight);
	Triangle_EdgeCompute(&TopToSplit,    &(Triangle.Gradients),pVertices,Top,   Middle, !SplitRight);
	Triangle_EdgeCompute(&SplitToBottom,&(Triangle.Gradients),pVertices,Middle,Bottom, !SplitRight);

	// to maximize mmx optimization, there is no floating point from this point on
	if(SplitRight) 
		{
			Triangle.Left  = TopToBottom;
			Triangle.Right = TopToSplit;
		} 
	else 
		{
			Triangle.Left  = TopToSplit;
			Triangle.Right = TopToBottom;
		}

	DestPtr   = ( uint32 )(Triangle.DestMap->BitPtr[0]);
	DestWidth = TOPDOWN_OR_BOTTOMUP(Triangle.DestMap->Width);
	Triangle.Left.Dest		  = DestPtr + ((Triangle.Left.X  + Triangle.Left.Y * DestWidth)<<DESTPIXEL_SHIFTER);
	Triangle.Left.DestStep    = (Triangle.Left.XStep + DestWidth)<<DESTPIXEL_SHIFTER;


	TRaster_RopTable[ROP].SpanEdges(TopToSplit.Height);

	if(SplitRight) 
		{
			Triangle.Right = SplitToBottom;
		}
	else
		{
			Triangle.Left = SplitToBottom;
			Triangle.Left.Dest		  = DestPtr + ((Triangle.Left.X  + Triangle.Left.Y * DestWidth)<<DESTPIXEL_SHIFTER);
			Triangle.Left.DestStep    = (Triangle.Left.XStep + DestWidth)<<DESTPIXEL_SHIFTER;
		}

	TRaster_RopTable[ROP].SpanEdges(SplitToBottom.Height);
}
