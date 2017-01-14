/****************************************************************************************/
/*  TClip                                                                               */
/*                                                                                      */
/*  Author: Mike Sandige & Charles Bloom                                                */
/*  Description: Triangle Clipping to the screen rectangle                              */
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

//#define DO_TIMER

/*********

Cbloom Jan 18
TClip gained 2-3 fps
(not counting the gains already from _SetTexture)

I reorganized the TClip_Triangle function flow to early-out 
for triangles all-in or all-out.  The old code considered this
case, but was not as lean as possible for these most-common cases.

To solve these problems, flow was changed to :
	1. do all compares and accumulated the 3 out bits for each of the five faces
		(so we have 15 bit-flags)
	2. then do clips while more clips remain.
	3. as a free benefit, the new structure means that Rasterize is only called once
			in TClip_Triangle, so it was inlined.

Step two results in very fast exiting when no clipping remains.


If/When we get the Intel compiler that can optimize ?: to CMOV, speed will improve
even more!

-----------------------------------

Timer profiling shows:
(with D3DDrv, in ActView, viewing dema.act)
times are seconds per frame

default pose : 58.6 fps
TClip_New            : 0.006749
TClip_Rasterize      : 0.006149

default pose : 52.9 fps
TClip_Old            : 0.007230 : 1.$ %
TClip_Rasterize      : 0.006183 : 1.$ %

***********/

// TClip.c
//  Fast Triangle Clipping
/*}{***********************/

#include <assert.h>
#include <string.h>

#include "TClip.h"
#include "engine.h"
#include "bitmap._h"

#include "list.h"
#include "ram.h"  

#include "timer.h"

TIMER_VARS(TClip_Triangle);

//#define ONE_OVER_Z_PIPELINE	// this has more accuracy, but the slowness of doing 1/ divides

typedef enum 
{	
	BACK_CLIPPING_PLANE = 0,
	LEFT_CLIPPING_PLANE,
	RIGHT_CLIPPING_PLANE,
	TOP_CLIPPING_PLANE,
	BOTTOM_CLIPPING_PLANE,
	NUM_CLIPPING_PLANES
} geTClip_ClippingPlane;

// 3 bits for V_IN/OUT flags
#define V_ALL_IN (0)
#define V0_OUT	(1)
#define V1_OUT	(2)
#define V2_OUT	(4)

	// at a=0, result is l;  at a=1, result is h
#define LINEAR_INTERPOLATE(a,l,h)     ((l)+(((h)-(l))*(a)))
 
#define RASTERIZECC 
typedef void (RASTERIZECC *geTClip_Rasterize_FuncPtr) (const GE_LVertex * TriVtx);

typedef struct geTClip_StaticsType
{
	geFloat LeftEdge;
	geFloat RightEdge;
	geFloat TopEdge;
	geFloat BottomEdge;
	geFloat BackEdge;

	DRV_Driver * Driver;
	geEngine	*Engine;
	const geBitmap *Bitmap;
	geRDriver_THandle * THandle;

	geTClip_Rasterize_FuncPtr RasterizeFunc;
} geTClip_StaticsType;

/*}{************ Protos ***********/

static void RASTERIZECC geTClip_Rasterize_Tex(const GE_LVertex * TriVtx);
static void RASTERIZECC geTClip_Rasterize_Gou(const GE_LVertex * TriVtx);
static void GENESISCC geTClip_Split(GE_LVertex *NewVertex,const GE_LVertex *V1,const GE_LVertex *V2,int ClippingPlane);
static void GENESISCC geTClip_TrianglePlane(const GE_LVertex * zTriVertex,geTClip_ClippingPlane ClippingPlane);
static void GENESISCC geTClip_TrianglePlane_Old(const GE_LVertex * zTriVertex,geTClip_ClippingPlane ClippingPlane);

/*}{************ The State Statics ***********/

static Link * geTClip_Link = NULL;
static geTClip_StaticsType geTClip_Statics;

/*}{************ Functions ***********/

geBoolean GENESISCC geTClip_Push(void)
{
geTClip_StaticsType * TCI;

	if ( ! geTClip_Link )
	{
		List_Start();
		geTClip_Link = Link_Create();
		if ( ! geTClip_Link ) 
			return GE_FALSE;
	}

	TCI = geRam_Allocate(sizeof(geTClip_StaticsType));
	if ( ! TCI )
		return GE_FALSE;
	memcpy(TCI,&geTClip_Statics,sizeof(geTClip_StaticsType));

	Link_Push( geTClip_Link , TCI );

	return GE_TRUE;
}

geBoolean GENESISCC geTClip_Pop(void)
{
geTClip_StaticsType * TCI;
	if ( ! geTClip_Link )
		return GE_FALSE;
	TCI = Link_Pop( geTClip_Link );
	if ( ! TCI )
		return GE_FALSE;
	memcpy(&geTClip_Statics,TCI,sizeof(geTClip_StaticsType));
	geRam_Free(TCI);

	if ( ! Link_Peek(geTClip_Link) )
	{
		Link_Destroy(geTClip_Link);
		geTClip_Link = NULL;
		List_Stop();
	}
	return GE_TRUE;
}

geBoolean GENESISCC geTClip_SetTexture(const geBitmap * Bitmap)
{
	geTClip_Statics.Bitmap = Bitmap;
	if ( Bitmap )
	{
		geTClip_Statics.THandle = geBitmap_GetTHandle(Bitmap);
		assert(geTClip_Statics.THandle);
		geTClip_Statics.RasterizeFunc = geTClip_Rasterize_Tex;
	}
	else
	{
		geTClip_Statics.THandle = NULL;
		geTClip_Statics.RasterizeFunc = geTClip_Rasterize_Gou;
	}
return GE_TRUE;
}

void GENESISCC geTClip_SetupEdges(
	geEngine *Engine,
	geFloat LeftEdge, 
	geFloat RightEdge,
	geFloat TopEdge ,
	geFloat BottomEdge,
	geFloat BackEdge)
{ 
	assert(Engine);
	memset(&geTClip_Statics,0,sizeof(geTClip_Statics));
	geTClip_Statics.Engine		= Engine;
	geTClip_Statics.Driver		= Engine->DriverInfo.RDriver; //Engine_GetRDriver(Engine);
	geTClip_Statics.LeftEdge	= LeftEdge;
	geTClip_Statics.RightEdge	= RightEdge;
	geTClip_Statics.TopEdge		= TopEdge;
	geTClip_Statics.BottomEdge	= BottomEdge;
	geTClip_Statics.BackEdge	= BackEdge;
}

void geTClip_Done(void)
{
	TIMER_REPORT(TClip_Triangle);
}

void GENESISCC geTClip_Triangle(const GE_LVertex TriVertex[3])
{

	TIMER_P(TClip_Triangle);

#if 1
	geTClip_TrianglePlane(TriVertex,BACK_CLIPPING_PLANE);
#else
	geTClip_TrianglePlane_Old(TriVertex,BACK_CLIPPING_PLANE);
#endif

	TIMER_Q(TClip_Triangle);
}

/*}{************ TClip_Rasterize ***********/

static void RASTERIZECC geTClip_Rasterize_Tex(const GE_LVertex * TriVtx)
{
	geTClip_Statics.Driver->RenderMiscTexturePoly((DRV_TLVertex *)TriVtx,
		3,geTClip_Statics.THandle,0);
}

static void RASTERIZECC geTClip_Rasterize_Gou(const GE_LVertex * TriVtx)
{
	geTClip_Statics.Driver->RenderGouraudPoly((DRV_TLVertex *)TriVtx,3,0);
}

static void GENESISCC geTClip_Rasterize(const GE_LVertex * TriVtx)
{

// we require GE_LVertex == DRV_TLVertex == GE_TLVertex
//	this is a silly point because the TClip inputs should really be GE_TLVertex anyway !!!!

	if ( geTClip_Statics.THandle )
	{
		geTClip_Statics.Driver->RenderMiscTexturePoly((DRV_TLVertex *)TriVtx,
			3,geTClip_Statics.THandle,0);
	}
	else
	{
		geTClip_Statics.Driver->RenderGouraudPoly((DRV_TLVertex *)TriVtx,
			3,0);
	}
}

/*}{************ TClip_Split ***********/

static void GENESISCC geTClip_Split(GE_LVertex *NewVertex,const GE_LVertex *V1,const GE_LVertex *V2,int ClippingPlane)
{
	geFloat Ratio=0.0f;
	geFloat OneOverZ1,OneOverZ2;
	
	#ifdef ONE_OVER_Z_PIPELINE
		// in here ->Z is really (one over z)
	OneOverZ1 = V1->Z;
	OneOverZ2 = V2->Z;
	#else
	OneOverZ1 = 1.0f/V1->Z;
	OneOverZ2 = 1.0f/V2->Z;
	#endif

	switch (ClippingPlane)
	{
		case (BACK_CLIPPING_PLANE):
			assert((V2->Z - V1->Z)!=0.0f);
			Ratio = ((1.0f/geTClip_Statics.BackEdge) - OneOverZ2)/( OneOverZ1 - OneOverZ2 );

			NewVertex->X = LINEAR_INTERPOLATE(Ratio,(V2->X),(V1->X));
			NewVertex->Y = LINEAR_INTERPOLATE(Ratio,(V2->Y),(V1->Y));
			#ifdef ONE_OVER_Z_PIPELINE
			NewVertex->Z = 1.0f/ geTClip_Statics.BackEdge;
			#else
			NewVertex->Z = geTClip_Statics.BackEdge;
			#endif
		
			break;
		case (LEFT_CLIPPING_PLANE):
			assert((V2->X - V1->X)!=0.0f);
			Ratio = (geTClip_Statics.LeftEdge - V2->X)/( V1->X - V2->X);

			NewVertex->X = geTClip_Statics.LeftEdge;
			NewVertex->Y = LINEAR_INTERPOLATE(Ratio,(V2->Y),(V1->Y));
			#ifdef ONE_OVER_Z_PIPELINE
			NewVertex->Z = LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#else
			NewVertex->Z = 1.0f/LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#endif
	
			break;
		case (RIGHT_CLIPPING_PLANE):
			assert((V2->X - V1->X)!=0.0f);
			Ratio = (geTClip_Statics.RightEdge - V2->X)/( V1->X - V2->X);

			NewVertex->X = geTClip_Statics.RightEdge;
			NewVertex->Y = LINEAR_INTERPOLATE(Ratio,(V2->Y),(V1->Y));
			#ifdef ONE_OVER_Z_PIPELINE
			NewVertex->Z = LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#else
			NewVertex->Z = 1.0f/LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#endif

			break;
		case (TOP_CLIPPING_PLANE):
			assert((V2->Y - V1->Y)!=0.0f);
			Ratio = (geTClip_Statics.TopEdge - V2->Y)/( V1->Y - V2->Y);

			NewVertex->X = LINEAR_INTERPOLATE(Ratio,(V2->X),(V1->X));
			NewVertex->Y = geTClip_Statics.TopEdge;
			#ifdef ONE_OVER_Z_PIPELINE
			NewVertex->Z = LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#else
			NewVertex->Z = 1.0f/LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#endif
			
			break;
		case (BOTTOM_CLIPPING_PLANE):
			assert((V2->Y - V1->Y)!=0.0f);
			Ratio = (geTClip_Statics.BottomEdge - V2->Y)/( V1->Y - V2->Y);

			NewVertex->X = LINEAR_INTERPOLATE(Ratio,(V2->X),(V1->X));
			NewVertex->Y = geTClip_Statics.BottomEdge;
			#ifdef ONE_OVER_Z_PIPELINE
			NewVertex->Z = LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#else
			NewVertex->Z = 1.0f/LINEAR_INTERPOLATE(Ratio,OneOverZ2,OneOverZ1);
			#endif

			break;
	}

	
	{
	geFloat OneOverZ1_Ratio;
	geFloat OneOverZ2_Ratio;
		#ifdef ONE_OVER_Z_PIPELINE
		OneOverZ1 *= 1.0f / NewVertex->Z;
		OneOverZ2 *= 1.0f / NewVertex->Z;
		#else
		OneOverZ1 *= NewVertex->Z;
		OneOverZ2 *= NewVertex->Z;
		#endif
		OneOverZ1_Ratio = OneOverZ1 * Ratio;
		OneOverZ2_Ratio = OneOverZ2 * Ratio;

		//  the following is optimized to get rid of a handfull of multiplies. Read:
		//	NewVertex->r = LINEAR_INTERPOLATE(Ratio,(V2->r * OneOverZ2),(V1->r * OneOverZ1));

		NewVertex->r =(V2->r * OneOverZ2) + (V1->r * OneOverZ1_Ratio) - (V2->r * OneOverZ2_Ratio);
		NewVertex->g =(V2->g * OneOverZ2) + (V1->g * OneOverZ1_Ratio) - (V2->g * OneOverZ2_Ratio);
		NewVertex->b =(V2->b * OneOverZ2) + (V1->b * OneOverZ1_Ratio) - (V2->b * OneOverZ2_Ratio);
		NewVertex->a =(V2->a * OneOverZ2) + (V1->a * OneOverZ1_Ratio) - (V2->a * OneOverZ2_Ratio);
		NewVertex->u =(V2->u * OneOverZ2) + (V1->u * OneOverZ1_Ratio) - (V2->u * OneOverZ2_Ratio);
		NewVertex->v =(V2->v * OneOverZ2) + (V1->v * OneOverZ1_Ratio) - (V2->v * OneOverZ2_Ratio);
	}

}


/*}{************ TClip_TrianglePlane ***********/

static void GENESISCC geTClip_TrianglePlane(const GE_LVertex * TriVertex,
											geTClip_ClippingPlane ClippingPlane)
{
uint32 OutBits = 0;

	switch(ClippingPlane)
	{
	case BACK_CLIPPING_PLANE:

		OutBits |= (TriVertex[0].Z < geTClip_Statics.BackEdge) ? V0_OUT : 0;
		OutBits |= (TriVertex[1].Z < geTClip_Statics.BackEdge) ? V1_OUT : 0;
		OutBits |= (TriVertex[2].Z < geTClip_Statics.BackEdge) ? V2_OUT : 0;

	case LEFT_CLIPPING_PLANE:

		OutBits |= (TriVertex[0].X < geTClip_Statics.LeftEdge)  ? (V0_OUT<<3) : 0;
		OutBits |= (TriVertex[1].X < geTClip_Statics.LeftEdge)  ? (V1_OUT<<3) : 0;
		OutBits |= (TriVertex[2].X < geTClip_Statics.LeftEdge)  ? (V2_OUT<<3) : 0;

	case RIGHT_CLIPPING_PLANE:

		OutBits |= (TriVertex[0].X > geTClip_Statics.RightEdge) ? (V0_OUT<<6) : 0;
		OutBits |= (TriVertex[1].X > geTClip_Statics.RightEdge) ? (V1_OUT<<6) : 0;
		OutBits |= (TriVertex[2].X > geTClip_Statics.RightEdge) ? (V2_OUT<<6) : 0;

	case TOP_CLIPPING_PLANE:

		OutBits |= (TriVertex[0].Y < geTClip_Statics.TopEdge) ? (V0_OUT<<9) : 0;
		OutBits |= (TriVertex[1].Y < geTClip_Statics.TopEdge) ? (V1_OUT<<9) : 0;
		OutBits |= (TriVertex[2].Y < geTClip_Statics.TopEdge) ? (V2_OUT<<9) : 0;

	case BOTTOM_CLIPPING_PLANE:

		OutBits |= (TriVertex[0].Y > geTClip_Statics.BottomEdge) ?  (V0_OUT<<12) : 0;
		OutBits |= (TriVertex[1].Y > geTClip_Statics.BottomEdge) ?  (V1_OUT<<12) : 0;
		OutBits |= (TriVertex[2].Y > geTClip_Statics.BottomEdge) ?  (V2_OUT<<12) : 0;

	case NUM_CLIPPING_PLANES:
		break;
	}

	if ( OutBits )
	{
	GE_LVertex NewTriVertex[3];
		ClippingPlane = 0;
		for(;;)
		{
			assert(ClippingPlane < NUM_CLIPPING_PLANES);

			switch ( OutBits & 7 )
			{
				case (V_ALL_IN):  //NOT CLIPPED
					OutBits >>= 3;
					ClippingPlane ++;
					continue;

				// these all return:

				case (V0_OUT):
					NewTriVertex[0] = TriVertex[2];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+0,TriVertex+2,ClippingPlane);
					NewTriVertex[2] = TriVertex[1];

					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1);

					NewTriVertex[0] = NewTriVertex[1];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+0,TriVertex+1,ClippingPlane);

					//<> could gain a little speed like this, but who cares?
					//	if ( ! (OutBits>>3) )
					//		goto Rasterize
					//	else
					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1); 
					return;

				case (V1_OUT):
					NewTriVertex[0] = TriVertex[0];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+0,TriVertex+1,ClippingPlane);
					NewTriVertex[2] = TriVertex[2];

					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1);

					NewTriVertex[0] = NewTriVertex[1];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+1,TriVertex+2,ClippingPlane);
					
					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1); 
					return;

				case (V0_OUT + V1_OUT):
					NewTriVertex[0] = TriVertex[2];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+0,TriVertex+2,ClippingPlane);
					geTClip_Split(&(NewTriVertex[2]),TriVertex+1,TriVertex+2,ClippingPlane);
				
					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1); 
					return;

				case (V2_OUT):
					NewTriVertex[0] = TriVertex[1];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+1,TriVertex+2,ClippingPlane);
					NewTriVertex[2] = TriVertex[0];

					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1);

					NewTriVertex[0] = NewTriVertex[1];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+0,TriVertex+2,ClippingPlane);

					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1);
					return;

				case (V2_OUT + V0_OUT):
					NewTriVertex[0] = TriVertex[1];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+1,TriVertex+2,ClippingPlane);
					geTClip_Split(&(NewTriVertex[2]),TriVertex+0,TriVertex+1,ClippingPlane);

					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1);
					return;

				case (V2_OUT + V1_OUT):
					NewTriVertex[0] = TriVertex[0];
					geTClip_Split(&(NewTriVertex[1]),TriVertex+0,TriVertex+1,ClippingPlane);
					geTClip_Split(&(NewTriVertex[2]),TriVertex+0,TriVertex+2,ClippingPlane);

					geTClip_TrianglePlane(NewTriVertex,ClippingPlane+1);
					return;

				case (V2_OUT + V1_OUT + V0_OUT):
					/* TOTALLY CLIPPED */
					return;
			}
		}
	}

#if 0 // {

	// this eliminates an 'if' , but doesn't seem to help :^(
	// presumably because it's a predictable branch
	geTClip_Statics.RasterizeFunc(TriVertex);

#else //}{

	if ( geTClip_Statics.THandle )
	{
		geTClip_Statics.Driver->RenderMiscTexturePoly((DRV_TLVertex *)TriVertex,
			3,geTClip_Statics.THandle,0);
	}
	else
	{
		geTClip_Statics.Driver->RenderGouraudPoly((DRV_TLVertex *)TriVertex,3,0);
	}

#endif //}

}

/*}{*********** EOF ************/
