
/****************************************************************************************/
/*  PalOptimize                                                                         */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Palette Perfecting code                                               */
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

/*********

our colors are referred to as "uint8" triples, but are actually typically YUV

--------

The basics :

	find the average color to which each palette entry is mapped.
	set the palette entry to that color.
	repeat.

The problems :

	1. this does NOT converge ; there's some wierd histerysis;
		it also does NOT monotonically improve MSE !
		we simply stop when MSE hits a local min.

		<*> I think this is because our "ClosestPal" algorithm is imperfect
			since it works within an oct-tree structure

	2. we can fall into "unstable local minimum" traps, like :
		(this is a plot in color space)

				X
			   + +
			  X + X

		here X indicates a blob of colors used, and + indicates a palette color;
		this configuration is stable under our algorithm, though of
		course the optimal is

				*
		
			  *   *

		(where * = X and +)
		(note that these configurations cannot occur in 1d finite graphs; the
		1d infinite graph for an unstable minimum is ...X+X+X+X+X+... )

		this could be solved with like a Monte-Carlo walk,
			adding some rare random component.
		it doesn't work to just add a random wiggle whenever we stall out;
			perhaps a random componenet in the direction of the deltas, like

			diffR = Guassian_Rand( center = diffR , variance = abs(diffR) /2 );

		tried it:
			seems to help sometimes, but isn't a clear win

**********/

#include "yuv.h"
#include "palettize.h"
#include "utility.h"
#include "tsc.h"
#include "log.h"

#include "bitmap.h"
#include "pixelformat.h"

/*******/

typedef struct 
{
	int totR,totG,totB,count;
} palOptInfo;

int stepTable[] = { 1009 , 757, 499, 401, 307, 239, 197, 157, 131, 103, 67, 41, 29, 17, 13, 7, 4, 1 };

void paletteOptimize(const geBitmap_Info * BmInfo,const void * Bits,uint8 *palette,int palEntries,int maxSamples)
{
palInfo *palInfo;
int pal,R,G,B,A;
uint32 mse,last_mse;
uint8 *palPtr;
uint8 savePalette[768];
int extraStepIndex,extraStepSize,extraStepSizeBytes,samples,totSamples;
gePixelFormat_ColorGetter GetColor;
const gePixelFormat_Operations * PixelOps;
uint8 *ptr,*ptrEnd;
int d;
palOptInfo optInfo[256];

	assert(palEntries <= 256);

	pushTSC();

	// palette is 768 bytes

	R = palette[(palEntries-1)*3 + 0];
	G = palette[(palEntries-1)*3 + 1];
	B = palette[(palEntries-1)*3 + 2];
	for(pal=palEntries;pal<256;pal++)
	{
		palette[pal*3 + 0] = R;
		palette[pal*3 + 1] = G;
		palette[pal*3 + 2] = B;
	}

	PixelOps = gePixelFormat_GetOperations(BmInfo->Format);
	GetColor = PixelOps->GetColor;
	ptrEnd = (uint8 *)Bits + BmInfo->Stride * BmInfo->Height * PixelOps->BytesPerPel;

	mse = ~(uint32)0;
	extraStepIndex = 0;
	extraStepSize = -1;
	totSamples = 0;
	if ( maxSamples <= 0 ) maxSamples = 0x0FFFFFFF;
	for(;;)
	{
		if ( extraStepSize != 0 )
		{
			extraStepSize = ( stepTable[ extraStepIndex ] - 1 );
			extraStepSizeBytes = extraStepSize * PixelOps->BytesPerPel;
			extraStepIndex++;
		}

		last_mse = mse;

		// <> this 'closestPal' is not great for this application
		//		it's approximate & has a large initialization overhead
		//	 we should use the methods from the "Local K-Means" paper

		palInfo = closestPalInit(palette);
		if ( ! palInfo ) return;

		memclear(optInfo,sizeof(palOptInfo)*palEntries);

		mse = 0;
		samples =0;
		ptr = (uint8 *)Bits;
		while( ptr < ptrEnd )
		{
			GetColor(&ptr,&R,&G,&B,&A);

			pal = closestPal(R,G,B,palInfo);

			if ( pal >= palEntries ) pal = palEntries-1;			
		
			palPtr = palette + pal*3;
			d = R - (*palPtr++);	mse += d*d;
			d = G - (*palPtr++);	mse += d*d;
			d = B - (*palPtr  );	mse += d*d;

			optInfo[pal].totR += R;
			optInfo[pal].totG += G;
			optInfo[pal].totB += B;
			optInfo[pal].count ++;

			ptr += extraStepSizeBytes;
			samples ++;
		}

		closestPalFree(palInfo);

		if ( samples == 0 ) continue;
		mse = (int)(((double)mse*256.0)/samples);
		totSamples += samples;

		if ( mse >= last_mse && extraStepSize == 0 )
		{
			memcpy(palette,savePalette,768);
			mse = last_mse;
			break;
		}
		else if ( mse > last_mse )
		{
#if 0
			// seems to slow convergence (!?)
			memcpy(palette,savePalette,768);
			mse = last_mse;
#endif
		}
		else
		{
			memcpy(savePalette,palette,768);
		}
	
		Log_Printf("mse*256 = %7d , extrastep = %4d, samples = %7d\n",mse,extraStepSize,samples);

		if ( totSamples >= maxSamples )
			break;

		for(pal=0,palPtr = palette; pal<palEntries ; pal++,palPtr += 3)
		{
		double fd;
		int diffR,diffG,diffB;

			if ( optInfo[pal].count == 0 ) continue;

			fd = 1.0 / optInfo[pal].count;

			diffR = (int)(optInfo[pal].totR * fd) - palPtr[0];
			diffG = (int)(optInfo[pal].totG * fd) - palPtr[1];
			diffB = (int)(optInfo[pal].totB * fd) - palPtr[2];

#if 1
			#define DIV(diff)	if ( diff < 0 ) diff = - ((-diff + 1)>>1); else if ( diff > 0 ) diff = ((diff + 1)>>1)
			DIV(diffR);
			DIV(diffG);
			DIV(diffB);
#endif

#if 0
			// this helps sometimes, hurts sometimes
			diffR = GaussianRand(diffR,abs(diffR));
			diffG = GaussianRand(diffG,abs(diffG));
			diffB = GaussianRand(diffB,abs(diffB));
#endif

			palPtr[0] = minmax( palPtr[0]+diffR , 0,255);
			palPtr[1] = minmax( palPtr[1]+diffG , 0,255);
			palPtr[2] = minmax( palPtr[2]+diffB , 0,255);
		}
		
		if ( abs(mse - last_mse) < 50 && extraStepSize == 0 )
		{
			break;
		}
	}

	showPopTSC("palOptimize");
}
