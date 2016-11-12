/****************************************************************************************/
/*  Palettize                                                                           */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Palettize-ing code                                                    */
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

our colors are referred to as "RGB" triples, but are actually typically YUV

------

can palettize a 256x256 bitmap in less than 0.1 seconds

------

we palettize ("inverse colormap") using an octree lookup system

<> do we need to be able to palettize to RGBA ??

**********/

#include "palettize.h"
#include <stdlib.h>
#include <assert.h>
#include "ram.h"
#include "mempool.h"

#ifdef _TSC
#pragma message("palettize using TSC")
#include "tsc.h"
#endif

/*******/

#define new(type)		geRam_AllocateClear(sizeof(type))
#define allocate(ptr)	ptr = geRam_AllocateClear(sizeof(*ptr))
#define clear(ptr)		memset(ptr,0,sizeof(*ptr))
#define destroy(ptr)	if ( ptr ) { geRam_Free(ptr); (ptr) = NULL; } else

/*******/

typedef struct palInfo palInfo;

int __inline	closestPalInlineRGB(int R,int G,int B,palInfo *pi);
int				closestPal(int R,int G,int B,palInfo *pi);
palInfo *		closestPalInit(uint8 * palette);
void			closestPalFree(palInfo *info);

/******/

geBoolean palettizePlane(const	geBitmap_Info * SrcInfo,const	void * SrcBits,
								geBitmap_Info * DstInfo,		void * DstBits,
								int SizeX,int SizeY)
{
palInfo *palInfo;
int x,y,xtra,bpp;
gePixelFormat Format;
int R,G,B,A;
uint8 palette[768],*pSrc,*pDst;

	assert( SrcInfo && SrcBits );
	assert( DstInfo && DstBits );

	assert( DstInfo->Format == GE_PIXELFORMAT_8BIT_PAL );
	assert( gePixelFormat_IsRaw(SrcInfo->Format) );

	if ( ! DstInfo->Palette )
		return GE_FALSE;

	if ( ! geBitmap_Palette_GetData(DstInfo->Palette,palette,GE_PIXELFORMAT_24BIT_RGB,256) )
		return GE_FALSE;

#ifdef _TSC
	pushTSC();
#endif

	// rgbPlane is (planeLen*3) bytes
	// palette is 768 bytes

	palInfo = closestPalInit(palette);
	if ( ! palInfo ) return GE_FALSE;

	Format = SrcInfo->Format;
	bpp = gePixelFormat_BytesPerPel(Format);
	xtra = (SrcInfo->Stride - SizeX) * bpp;
	pSrc = (uint8 *)SrcBits;
	pDst = DstBits;

	if ( DstInfo->HasColorKey )
	{
	int DstCK;
	const gePixelFormat_Operations * ops;
		ops = gePixelFormat_GetOperations(Format);
		assert(ops);
		DstCK = DstInfo->ColorKey;

		if ( gePixelFormat_HasAlpha(Format) )	
		{
		gePixelFormat_ColorGetter GetColor;
			GetColor = ops->GetColor;
			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					GetColor(&pSrc,&R,&G,&B,&A);
					if ( A < 128 )
					{
						*pDst++ = DstCK;
					}
					else
					{
						*pDst = closestPalInlineRGB(R,G,B,palInfo);
						if ( *pDst == DstCK ) // {} this is really poor color-key avoidance!
							*pDst ^= 1;
						pDst++;
					}
				}
				pSrc += xtra;
				pDst += DstInfo->Stride - SizeX;
			}
		}
		else if ( SrcInfo->HasColorKey )
		{
		uint32 SrcCK,Pixel;
		gePixelFormat_PixelGetter GetPixel;
		gePixelFormat_Decomposer DecomposePixel;
			DecomposePixel = ops->DecomposePixel;
			GetPixel = ops->GetPixel;

			SrcCK = SrcInfo->ColorKey;

			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					Pixel = GetPixel(&pSrc);
					if ( Pixel == SrcCK )
					{
						*pDst++ = DstCK;
					}
					else
					{
						DecomposePixel(Pixel,&R,&G,&B,&A);
						*pDst = closestPalInlineRGB(R,G,B,palInfo);
						if ( *pDst == DstCK ) // {} this is really poor color-key avoidance!
							*pDst ^= 1;
						pDst++;
					}
				}
				pSrc += xtra;
				pDst += DstInfo->Stride - SizeX;
			}
		}
		else
		{
		gePixelFormat_ColorGetter GetColor;
			GetColor = ops->GetColor;

			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					GetColor(&pSrc,&R,&G,&B,&A);
					*pDst = closestPalInlineRGB(R,G,B,palInfo);
					if ( *pDst == DstCK ) // {} this is really poor color-key avoidance!
						*pDst ^= 1;
					pDst++;
				}
				pSrc += xtra;
				pDst += DstInfo->Stride - SizeX;
			}
		}
	}
	else
	{
		// dst does not have CK, and can't have alpha in this universe, so ignore src colorkey
	#if 0 // these special cases just avoid a functional-call overhead
		if ( Format == GE_PIXELFORMAT_24BIT_RGB )
		{
			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					R = *pSrc++; G = *pSrc++; B = *pSrc++;
					*pDst++ = closestPalInlineRGB(R,G,B,palInfo);
				}
				pSrc += xtra;
				pDst += DstInfo->Stride - SizeX;
			}
		}
		else if ( Format == GE_PIXELFORMAT_24BIT_BGR )
		{
			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					B = *pSrc++; G = *pSrc++; R = *pSrc++;
					*pDst++ = closestPalInlineRGB(R,G,B,palInfo);
				}
				pSrc += xtra;
				pDst += DstInfo->Stride - SizeX;
			}
		}
		else
	#endif
		{
		const gePixelFormat_Operations * ops;
		gePixelFormat_ColorGetter GetColor;
			ops = gePixelFormat_GetOperations(Format);
			assert(ops);
			GetColor = ops->GetColor;
			assert(GetColor);
			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					GetColor(&pSrc,&R,&G,&B,&A);
					*pDst++ = closestPalInlineRGB(R,G,B,palInfo);
				}
				pSrc += xtra;
				pDst += DstInfo->Stride - SizeX;
			}
		}
	}

#ifdef _TSC
	showPopTSC("palettize");
#endif

	closestPalFree(palInfo);
	
return GE_TRUE;
}

/***************
**
*

Build an OctTree containing all the palette entries; the RGB value
is the index into the tree, the value at the leaf is a palette index.  All null children are then set
to point to their closest neighbor.  It has a maximum depth of 8.

To find a palette entry, you take your RGB and just keep stepping in; in fact, it's quite trivial.
on an image of B bytes and a palette of P entries, this method is O(B+P)

Find the right neighbor for null children is a very difficult algorithm.  I punt and
leave them null; when we find a null in descent, we do a hash-assisted search to find
the right pal entry, then add this color & pal entry to the octree for future use.

we store palette entries in the octree as (palVal+1) so that we can use 0 to mean "not assigned"

per-pixel time : 5e-7	(found in octree lookup)
per-color time : 7e-6	(not in octree time)

total_seconds = (5e-7)*(num_pels + palettize_size) + 
	(3e-8)*(num_actual_colors - palettize_size)*(palettize_size)

	(coder=bitplane,transform=L97)

stop-rate 4 , PSNR on :
brute-force 
	pal1 ; 33.29
	pal2 : 37.69
	pal3 : 33.69
	pal4 : 44.69
OctTree without "expandNulls"	("fast")
	pal1 ; 25.73
	pal2 : 32.50
	pal3 : 27.84
	pal4 : 28.07
OctTree with brute-force "expandNulls"
	pal1 ; 32.53
	pal2 : 37.09
	pal3 : 33.27
	pal4 : 33.50
OctTree with brute-force on null
	pal1 ; 33.15
	pal2 : 37.71
	pal3 : 33.65
	pal4 : 35.05
*
**
 *****************/

#define QUANT_BITS	(4)
#define QUANT_SHIFT	(8-QUANT_BITS)
#define QUANT_ROUND	(1<<(QUANT_SHIFT-1))
#define HASH_BITS	(QUANT_BITS*3)
#define HASH_SIZE	(1<<HASH_BITS)
#define HASH(R,G,B)	( (((R)>>QUANT_SHIFT)<<(QUANT_BITS+QUANT_BITS)) + (((G)>>QUANT_SHIFT)<<(QUANT_BITS)) + (((B)>>QUANT_SHIFT)))
#define HASHROUNDED(R,G,B)	( (((R+QUANT_ROUND)>>QUANT_SHIFT)<<(QUANT_BITS+QUANT_BITS)) + (((G+QUANT_ROUND)>>QUANT_SHIFT)<<(QUANT_BITS)) + (((B+QUANT_ROUND)>>QUANT_SHIFT)))

typedef struct octNode octNode;
struct octNode 
{
	octNode * kids[8];
	octNode * parent;
};

typedef struct hashNode 
{
	struct hashNode *next;
	int R,G,B,pal;
} hashNode;

struct palInfo 
{
	uint8 *palette;
	octNode *root;
	hashNode * hash[HASH_SIZE+1];
};

// internal protos:

int colorDistance(uint8 *ca,uint8 *cb);
int findClosestPalBrute(int R,int G,int B,palInfo *pi);
void addOctNode(octNode *root,int R,int G,int B,int palVal);
void addHash(palInfo *pi,int R,int G,int B,int palVal,int hash);

#define RGBbits(R,G,B,bits) (((((R)>>(bits))&1)<<2) + ((((G)>>(bits))&1)<<1) + (((B)>>((bits)))&1))

static MemPool * octNodePool = NULL;
static MemPool * hashNodePool = NULL;
static int PoolRefs = 0;

void Palettize_Start(void)
{
	if ( PoolRefs == 0 )
	{
		// we init with 256 octnodes, then add one for each unique color
		octNodePool = MemPool_Create(sizeof(octNode),1024,1024);
		assert(octNodePool);
		hashNodePool = MemPool_Create(sizeof(hashNode),1024,1024);
		assert(hashNodePool);
	}
	PoolRefs ++;
}

void Palettize_Stop(void)
{
	PoolRefs --;
	if ( PoolRefs == 0 )
	{
		MemPool_Destroy(&octNodePool);
		MemPool_Destroy(&hashNodePool);
	}
}

/********************/

palInfo * closestPalInit(uint8 * palette)
{
palInfo *pi;
int i;

	i = HASH_SIZE;

	if ( (pi = new(palInfo)) == NULL )
		return NULL;

	pi->palette = palette;

	pi->root = MemPool_GetHunk(octNodePool);
	assert(pi->root);

	for(i=0;i<256;i++) 
	{
		int R,G,B;
		R = palette[3*i]; G = palette[3*i+1]; B = palette[3*i+2];
		addOctNode(pi->root,R,G,B,i);
		addHash(pi,R,G,B,i,HASH(R,G,B));
	}

return pi;
}

int findClosestPal(int R,int G,int B,palInfo *pi)
{
hashNode *node;
int hash,d,bestD,bestP;

	hash = HASHROUNDED(R,G,B);
	if ( hash > HASH_SIZE ) hash = HASH_SIZE;

	node = pi->hash[ hash ];
	if ( ! node ) 
	{
		bestP = findClosestPalBrute(R,G,B,pi);
#if 1
		// helps speed a little; depends on how common individual RGB values are
		// (makes it so that if we see this exact RGB again we return bestP right away)
		addOctNode(pi->root,R,G,B,bestP);
#endif
#if 0
		//this could help speed, but actually makes this method approximate
		node = MemPool_GetHunk(hashNodePool);
		assert(node);
		node->next = pi->hash[hash];
		pi->hash[hash] = node;
		node->R = R;
		node->G = G;
		node->B = B;
		node->pal = bestP;
#endif
		return bestP;
	}

	bestD = 99999999;	bestP = node->pal;
	while(node) 
	{
		d = (R - node->R)*(R - node->R) + (G - node->G)*(G - node->G) + (B - node->B)*(B - node->B);
		if ( d < bestD ) 
		{
			bestD = d;
			bestP = node->pal;
		}
		node = node->next;
	}

#if 1
	// <> ?
	// helps speed a little; depends on how common individual RGB values are
	// (makes it so that if we see this exact RGB again we return bestP right away)
	addOctNode(pi->root,R,G,B,bestP);
#endif

	return bestP;
}

#define doStep(bits)	do { kid = (node)->kids[ RGBbits(R,G,B,bits) ]; \
				if ( kid ) node = kid; else return findClosestPal(R,G,B,pi); } while(0)

#define doSteps()	do { node = pi->root; doStep(7); doStep(6); doStep(5); doStep(4); doStep(3); doStep(2); doStep(1); doStep(0); } while(0)

int __inline closestPalInlineRGB(int R,int G,int B,palInfo *pi)
{
octNode *node,*kid;

	doSteps();

return ((int)node)-1;
}

int closestPal(int R,int G,int B,palInfo *pi)
{
octNode *node,*kid;

	doSteps();

	assert( ((int)node) <= 256 && ((int)node) > 0 );

return ((int)node)-1;
}

void closestPalFree(palInfo *pi)
{

	assert(pi);
	
	MemPool_Reset(octNodePool);
	MemPool_Reset(hashNodePool);

	destroy(pi);
}


int findClosestPalBrute(int R,int G,int B,palInfo *pi)
{
int d,p;
int bestD,bestP;
uint8 * pal;
uint8 color[3];

	// now do a brute-force best-pal search to find the best pal entry

	color[0] = R;	color[1] = G;	color[2] = B;
	pal = pi->palette;
	bestD = colorDistance(color,pal);
	bestP = 0;
	for(p=1;p<256;p++)
	{
		pal += 3;
		d = colorDistance(color,pal);
		if ( d < bestD ) 
		{
			bestD = d;
			bestP = p;
		}
	}
return bestP;
}

int colorDistance(uint8 *ca,uint8 *cb)
{
int d,x;
	d = 0;
	x = ca[0] - cb[0];
	d += x*x;
	x = ca[1] - cb[1];
	d += x*x;
	x = ca[2] - cb[2];
	d += x*x;
return d;
}

void addOctNode(octNode *root,int R,int G,int B,int palVal)
{
int idx;
int bits;
octNode *node = root;

	for(bits=7;bits>0;bits--) 
	{
		idx = RGBbits(R,G,B,bits);
		if ( ! node->kids[idx] ) 
		{
			node->kids[idx] = MemPool_GetHunk(octNodePool);
			node->kids[idx]->parent = node;
		}
		node = node->kids[idx];
	}
	idx = RGBbits(R,G,B,0);
	node->kids[idx] = (octNode *)(palVal+1);
}

void addHash(palInfo *pi,int R,int G,int B,int palVal,int hash)
{
hashNode *node;
int i,h;

	for(i=0;i<8;i++) 
	{
		h = hash + (i&1) + (((i>>1)&1)<<QUANT_BITS) + ((i>>2)<<(QUANT_BITS+QUANT_BITS));
		if ( h <= HASH_SIZE ) 
		{
			node = MemPool_GetHunk(hashNodePool);
			assert(node);
			node->next = pi->hash[h];
			pi->hash[h] = node;
			node->R = R;
			node->G = G;
			node->B = B;
			node->pal = palVal;
		}
	}
}
