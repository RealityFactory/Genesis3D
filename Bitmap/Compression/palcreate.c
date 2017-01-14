/*}{*************************************************/

/****************************************************************************************/
/*  PalCreate                                                                           */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Palette Creation code                                                 */
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

/**********

createPalGood goes in about 0.8 seconds
with about 0.5 of those in the "CreatePalOctTree" function

// <> could use Optimize

---------------

createPaletteFast :
	trivial kludge:
		gather colors in an octree
		sorts colors on popularity
		adds them to the palette, trying to avoiding adding extremely similar colors
		has some speed-ups (like croppping low-count leaves)

createPaletteGood :
	this is the "optimal" octree color quantizer. (see below, note on non-optimality)
	it is *VERY FAST* !
	gather all colors in an octree
	prune isolated strands so all nodes have > 1 kids
	the primary action is a "collapse" move:
		a leaf is cut, so that its color will be mapped to a sibling
		if it has no siblings, the color gets mapped to its parent
	each leaf keeps track of the "cost" (= increase in MSE) of cutting it
	each node has color which is the weighted average of its children
	the "cost" of a node, is the cost of all its children, plus the cost to
		move its new centroid.  This is exact, it's kind of subtle. see later
	we keep removing the node which has the lowest cost to cut
		(we use a radix sort to sort on cutCost ; this gives us the speed win)

my fast (incremental) way to compute the GE_TRUE node cost :
	GE_TRUE_cost = Sum[kids] kid_count * (kid_color - new_color)^2
	my_cost = Sum[kids] kid_count * (kid_color - node_color)^2
					+ node_count * (node_color - new_color)^2

	GE_TRUE_cost = Sum[kids] kid_count * (kid_color - new_color)^2
			  = Sum[kids] kid_count * ((kid_color - node_color) + (node_color - new_color))^2
			  = Sum[kids] kid_count * ((kid_color - node_color)^2 + (node_color - new_color)^2
										+ 2 * (kid_color - node_color) * (node_color - new_color))
			= approx_cost + 2 *  (node_color - new_color) * { Sum[kids] kid_count * (kid_color - node_color) }
					  
	the correction here is exactly zero! why :
		Sum[kids] kid_count * (kid_color - node_color) = (Sum[kids] kid_count * kid_color) - node_count * node_color = 0 !
	since that's the definition of node_color !

why this isn't exactly optimal:
	because octree children without the same parent are never grouped.
	the classic example	is in the binary tree, values 128 and 127 should have a cost of 1 to be
		merged together, but 128 will be merged with all values > 128 first.
	that is, the square boundaries of the tree are unnatural cuts in color space.
	this means that the "cutCost" is not accurate; there could be an actual lower MSE cost
		than our cutCost.
	furthermore, cutCost should be relative to all other leaves, not to their parent nodes,
		so that when I cut one leaf it changes the cutCosts of all other leaves.

***********/
/*}{*************************************************/

#include "palcreate.h"
#include "tsc.h"
#include "paloptimize.h"
#include "ram.h"
#include "yuv.h"
#include "mempool.h"
#include "utility.h"		// delete macro
#include <stdlib.h>
#include <assert.h>

/*******/

#define allocate(ptr)	ptr = geRam_AllocateClear(sizeof(*ptr))
#define clear(ptr)		memset(ptr,0,sizeof(*ptr))

/*}{*************************************************/

geBitmap_Palette * createPaletteGood(const geBitmap_Info * Info,const void * Bits);
geBitmap_Palette * createPaletteFast(const geBitmap_Info * Info,const void * Bits);

paletteCreater myPaletteCreater = createPaletteGood;

void setCreatePaletteFunc(paletteCreater func)
{
	assert( func == createPaletteGood || func == createPaletteFast );
	myPaletteCreater = func;
}

geBitmap_Palette * createPalette(const geBitmap_Info * Info,const void * Bits)
{
	assert(Info && Bits);
	switch(Info->Format)
	{
		case GE_PIXELFORMAT_8BIT_PAL :
			return Info->Palette;
		case GE_PIXELFORMAT_8BIT_GRAY :
		{
		geBitmap_Palette * Pal;
		uint8 GrayPal[256];
		int i;
			Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_8BIT_GRAY,256);
			if ( ! Pal ) return NULL;
			for(i=0;i<256;i++) GrayPal[i] = i;
			geBitmap_Palette_SetData(Pal,GrayPal,GE_PIXELFORMAT_8BIT_GRAY,256);
		return Pal;
		}
		default:
			return myPaletteCreater(Info,Bits);
	}
}

geBitmap_Palette * createPaletteFromBitmap(const geBitmap * Bitmap,geBoolean Optimize)
{
geBitmap * Lock;
geBitmap_Info Info;
const void * Bits;
geBitmap_Palette * Pal;

	if ( ! geBitmap_GetInfo(Bitmap,&Info,NULL) )
		return NULL;

	if ( ! geBitmap_LockForRead((geBitmap *)Bitmap,&Lock,0,0,GE_PIXELFORMAT_24BIT_RGB,GE_FALSE,0) )
		return NULL;
	
	if ( ! geBitmap_GetInfo(Lock,&Info,NULL) )
		return NULL;

	Bits = (const void *) geBitmap_GetBits(Lock);

	Pal = createPalette(&Info,Bits);

	if ( Pal && Optimize )
	{
	uint8 paldata[768];

		if ( ! geBitmap_Palette_GetData(Pal,paldata,GE_PIXELFORMAT_24BIT_RGB,256) )
			assert(0);
		
		paletteOptimize(&Info,Bits,paldata,256,0);
		
		if ( ! geBitmap_Palette_SetData(Pal,paldata,GE_PIXELFORMAT_24BIT_RGB,256) )
			assert(0);
	}
		
	geBitmap_UnLock(Lock);

return Pal;
}

/*}{*************************************************/

typedef struct octNode octNode;
struct octNode 
{
	octNode * kids[8];
	octNode * parent;
	int count,nKids;
	int R,G,B;

	// for the pruner:
	uint32 cutCost;	// this could overflow in the upper root nodes
	octNode *prev,*next; // sorted linked list of leaves
};

#define square(x)	((x)*(x))

#define RGBbits(R,G,B,bits) (((((R)>>(bits))&1)<<2) + ((((G)>>(bits))&1)<<1) + (((B)>>((bits)))&1))

#define RADIX_SIZE	1024

int createOctTree(octNode * root,const geBitmap_Info * Info,const void * Bits,geBoolean doYUV);
geBitmap_Palette * createPaletteGoodSub(const geBitmap_Info * Info,const void * Bits);
void addOctNode(octNode *root,int R,int G,int B,int *nLeavesPtr);
void gatherLeaves(octNode *node,octNode *** leavesPtrPtr,int minCount);
void gatherLeavesCutting(octNode *node,octNode *** leavesPtrPtr);
int leafCompareCount(const void *a,const void *b);
int leafCompareCost(const void *a,const void *b);
int findClosest(int R,int G,int B,uint8 *palette,int palEntries,int *foundPalPtr);
void computeOctRGBs(octNode *node);
void computeCutCosts(octNode *node);
void readLeavesToPal(octNode **leaves,int gotLeaves,uint8 *palette,int palEntries);
void insertRadix(octNode * radix,octNode *leaf);

/*}{*************************************************/

static MemPool * octNodePool = NULL;
static int PoolRefs = 0;

void PalCreate_Start(void)
{
	if ( PoolRefs == 0 )
	{
	int num;
		// we do addOctNode, one for each unique color
		// make the poolhunks 64k
		num = (1<<16) / sizeof(octNode);
		octNodePool = MemPool_Create(sizeof(octNode),num,num);
		assert(octNodePool);
	}
	PoolRefs ++;
}

void PalCreate_Stop(void)
{
	PoolRefs --;
	if ( PoolRefs == 0 )
	{
		MemPool_Destroy(&octNodePool);
	}
}

/*}{*************************************************/

geBitmap_Palette * createPaletteFast(const geBitmap_Info * Info,const void * Bits)
{
octNode * root;
int nLeaves,minCount,gotLeaves;
octNode ** leaves,**leavesPtr;
uint8 palette[768];
int palEntries = 256;
geBitmap_Palette * Pal;

	pushTSC();

	// read the whole image into an octree
	//	this is the only pass over the input plane

	MemPool_Reset(octNodePool);
	root = MemPool_GetHunk(octNodePool);
	assert(root);
	nLeaves = createOctTree(root,Info,Bits,GE_FALSE);

	leaves = geRam_AllocateClear(sizeof(octNode *)*nLeaves);
	assert(leaves);
	
	// gather leaves into a linear array
	//	for speed we ignore leaves with a count lower than [x]

	gotLeaves = 0;
	for( minCount = 3; minCount >= 0 && gotLeaves < palEntries ; minCount-- )
	{
		leavesPtr = leaves;
		gatherLeaves(root,&leavesPtr,minCount);
		gotLeaves = ((uint32)leavesPtr - (uint32)leaves)/sizeof(octNode *);
	}

	// sort the leaves by count

	qsort(leaves,gotLeaves,sizeof(octNode *),leafCompareCount);

	// read the sorted leaves in by count; we try to only read in leaves
	//	that are farther than 'minDistance' from nodes already in the palette

	readLeavesToPal(leaves,gotLeaves,palette,palEntries);

	destroy(leaves);

	showPopTSC("createPalFast");

	Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_24BIT_RGB,palEntries);
	if ( ! Pal )
		return NULL;
	if ( ! geBitmap_Palette_SetData(Pal,palette,GE_PIXELFORMAT_24BIT_RGB,palEntries) )
		assert(0);
return Pal;
}

/*}{*************************************************/

geBitmap_Palette * createPaletteGood(const geBitmap_Info * Info,const void * Bits)
{
	return createPaletteGoodSub(Info,Bits);
}

geBitmap_Palette * createPaletteGoodSub(const geBitmap_Info * Info,const void * Bits)
{
octNode * root;
int nLeaves,i,gotLeaves,radixN;
octNode ** leaves,**leavesPtr;
octNode *leaf,*node;
octNode *radix;
uint8 palette[768],*palPtr;
int palEntries = 256;
geBitmap_Palette * Pal;

	pushTSC();

	// read the whole image into an octree
	//	this is the only pass over the input plane

	MemPool_Reset(octNodePool);
	root = MemPool_GetHunk(octNodePool);
	assert(root);

	nLeaves = createOctTree(root,Info,Bits,GE_TRUE);

	leaves = geRam_AllocateClear(sizeof(octNode *)*nLeaves);
	assert(leaves);

	computeOctRGBs(root);
	root->parent = root;
	computeCutCosts(root);
	root->parent = NULL;
	
	// gather leaves into a linear array
	//	for speed we ignore leaves with a count lower than [x]

	leavesPtr = leaves;
	gatherLeavesCutting(root,&leavesPtr);
	gotLeaves = ((uint32)leavesPtr - (uint32)leaves)/sizeof(octNode *);

	// if gotLeaves < palEntries, just exit asap
	if ( gotLeaves < palEntries )
	{
		readLeavesToPal(leaves,gotLeaves,palette,palEntries);
		goto done;
	}

	// sort the leaves by cutCost
	// radix sort instead of qsort

	radix = geRam_AllocateClear(sizeof(octNode)*RADIX_SIZE);
	assert(radix);

	for(i=0;i<RADIX_SIZE;i++)
		radix[i].next = radix[i].prev = &radix[i];

	for(i=0;i<gotLeaves;i++)
		insertRadix(radix,leaves[i]);

	// keep cutting the tail
	radixN = 0;
	while(gotLeaves > palEntries)
	{
		while( (leaf = radix[radixN].next) == &(radix[radixN]) )
		{
			radixN++;
			assert( radixN < RADIX_SIZE );
		}
		// cut it
		leaf->prev->next = leaf->next;
		leaf->next->prev = leaf->prev;

		node = leaf->parent;
		assert(node);
		node->nKids --;

		// might turn its parent into a leaf;
		// if so, add it to the list
			// nKids no longer corresponds to the actual number of active kids

		if ( node->nKids == 0 )
			insertRadix(radix,node);
		else
			gotLeaves--;
	}

	// read the sorted leaves in by count; we try to only read in leaves
	//	that are farther than 'minDistance' from nodes already in the palette

	palPtr = palette;
	radixN = RADIX_SIZE-1;
	leaf = radix[radixN].prev;	
	for(i=0;i<palEntries && radixN>0;i++)
	{
		*palPtr++ = leaf->R;
		*palPtr++ = leaf->G;
		*palPtr++ = leaf->B;
		leaf = leaf->prev;
		while ( leaf == &(radix[radixN]) )
		{
			radixN --;
			if ( ! radixN )
				break;
			leaf = radix[radixN].prev;
		}
	}

	destroy(radix);

done:

	destroy(leaves);

	showPopTSC("createPalGood");

	YUVb_to_RGBb_line(palette,palette,palEntries);

	Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_24BIT_RGB,palEntries);
	if ( ! Pal )
		return NULL;

	if ( ! geBitmap_Palette_SetData(Pal,palette,GE_PIXELFORMAT_24BIT_RGB,palEntries) )
		assert(0);

return Pal;
}

/*}{*************************************************/

void insertRadix(octNode * radix,octNode *leaf)
{
octNode *insertAt;

	if ( leaf->cutCost >= RADIX_SIZE ) 
	{
		octNode * head;
		insertAt = head = & radix[RADIX_SIZE-1];
		while(insertAt->cutCost < leaf->cutCost && insertAt->next != head )
			insertAt = insertAt->next;
	}
	else
		insertAt = & radix[leaf->cutCost];

	leaf->next = insertAt->next;
	leaf->next->prev = leaf;
	insertAt->next = leaf;
	insertAt->next->prev = insertAt;
}

int findClosest(int R,int G,int B,uint8 *palette,int palEntries,int *foundPalPtr)
{
int i,d,bestD,bestP;
	bestD = 99999999; bestP = -1;
	for(i=palEntries;i--;)
	{
		d = square(R - palette[0]) + square(G - palette[1]) + square(B - palette[2]);
		palette += 3;
		if ( d < bestD ) 
		{
			bestD = d;
			bestP = i;
		}
	}
	if ( foundPalPtr ) *foundPalPtr = bestP;
return bestD;
}

static void addOctNode(octNode *root,int R,int G,int B,int *nLeavesPtr)
{
int idx;
int bits;
octNode *node;

	node = root;
	for(bits=7;bits>=0;bits--) 
	{
		idx = RGBbits(R,G,B,bits);
		if ( ! node->kids[idx] ) 
		{
			node->nKids ++;
			node->kids[idx] = MemPool_GetHunk(octNodePool);
			node->kids[idx]->parent = node;
		}
		node->count ++;
		node = node->kids[idx];
	}
	if ( node->count == 0 ) (*nLeavesPtr)++;
	node->count ++;
	node->R = R;
	node->G = G;
	node->B = B;
}

static void gatherLeaves(octNode *node,octNode *** leavesPtrPtr,int minCount)
{
	if ( node->count <= minCount ) return;
	if ( node->nKids == 0 ) 
	{
		*(*leavesPtrPtr)++ = node;	
	}
	else
	{
		int i;
		for(i=0;i<8;i++)
		{
			if ( node->kids[i] ) gatherLeaves(node->kids[i],leavesPtrPtr,minCount);
		}
	}
}

static void gatherLeavesCutting(octNode *node,octNode *** leavesPtrPtr)
{
	if ( node->nKids > 0 ) 
	{
		int i;
		for(i=0;i<8;i++)
		{
			if ( node->kids[i] )
			{
				if ( node->kids[i]->count <= 1 || node->kids[i]->cutCost <= 1 )
				{
					//freeOctNodes(node->kids[i]);
					node->kids[i] = NULL;
					node->nKids--;
				}
				else
				{
					gatherLeavesCutting(node->kids[i],leavesPtrPtr);
					
					if ( node->kids[i]->nKids == 1 )
					{
						octNode *kid;
						int j;
						kid = node->kids[i];
						for(j=0;j<8;j++)
							if ( kid->kids[j] ) 
								node->kids[i] = kid->kids[j];
						assert( node->kids[i] != kid );
						node->kids[i]->cutCost = kid->cutCost;
						//destroy(kid);
						node->kids[i]->parent = node;
					}
				}
			}
		}
	}

	if ( node->nKids == 0 ) 
	{
		*(*leavesPtrPtr)++ = node;	
	}
}

static int leafCompareCount(const void *a,const void *b)
{
octNode *na,*nb;
	na = *((octNode **)a);
	nb = *((octNode **)b);
return (nb->count) - (na->count);
}
static int leafCompareCost(const void *a,const void *b)
{
octNode *na,*nb;
	na = *((octNode **)a);
	nb = *((octNode **)b);
return (nb->cutCost) - (na->cutCost);
}

void computeCutCosts(octNode *node)
{
	assert(node->parent);
	node->cutCost = square(node->R - node->parent->R)
					+ square(node->G - node->parent->G)
					+ square(node->B - node->parent->B);
	node->cutCost *= node->count;
	
	if ( node->nKids > 0 )
	{
	int i;
		for(i=0;i<8;i++)
			if ( node->kids[i] )
			{
				computeCutCosts(node->kids[i]);
				node->cutCost += node->kids[i]->cutCost;
			}
	}
}

void computeOctRGBs(octNode *node)
{
	if ( node->nKids > 0 )
	{
	int R,G,B;
	int i;
	octNode *kid;
		R = G = B = 0;
		for(i=0;i<8;i++)
			if ( node->kids[i] )
				computeOctRGBs(node->kids[i]);
		for(i=0;i<8;i++)
		{
			if ( kid = node->kids[i] )
			{
				R += kid->count * kid->R;
				G += kid->count * kid->G;
				B += kid->count * kid->B;
			}
		}
		node->R = R / (node->count);
		node->G = G / (node->count);
		node->B = B / (node->count);
	}
}

void readLeavesToPal(octNode **leaves,int gotLeaves,uint8 *palette,int palEntries)
{
octNode **leavesPtr;
uint8 *palPtr;
int R,G,B;
int i,palGot;
int distance,minDistance;

	palPtr = palette; palGot = 0;
	for(minDistance=256;minDistance>=0 && palGot < palEntries;minDistance>>=1)
	{
		leavesPtr = leaves;
		for(i=0;i<gotLeaves;i++)
		{
			R = (*leavesPtr)->R;
			G = (*leavesPtr)->G;
			B = (*leavesPtr)->B;
			leavesPtr++;
			distance = findClosest(R,G,B,palette,palGot,NULL);
			if ( distance >= minDistance )
			{
				*palPtr++ = R;
				*palPtr++ = G;
				*palPtr++ = B;
				palGot ++;
				if ( palGot == palEntries )
					break;
			}
		}
	}
}

/*}{*************************************************/

int createOctTree(octNode * root,const geBitmap_Info * Info,const void * Bits,geBoolean doYUV)
{
int nLeaves;
int w,h,xtra,bpp,x,y;
gePixelFormat Format;
const gePixelFormat_Operations * ops;
int R,G,B,A;
gePixelFormat_Decomposer Decompose;

	assert(Bits);

	nLeaves = 0;

	Format = Info->Format;
	w = Info->Width;
	h = Info->Height;
	xtra = Info->Stride - Info->Width;
	bpp = gePixelFormat_BytesPerPel(Format);

	ops = gePixelFormat_GetOperations(Format);
	assert(ops);
	Decompose = ops->DecomposePixel;
	assert(Decompose);

//	pushTSC();

	if ( doYUV )
	{
		switch(bpp)
		{
			default:
			case 0:
				return GE_FALSE;
			case 1:
			{
			const uint8 *ptr;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr++,&R,&G,&B,&A);
						RGBi_to_YUVi(R,G,B,&R,&G,&B);
						addOctNode(root,R,G,B,&nLeaves);
					}
					ptr += xtra;
				}
				break;
			}
			case 2:
			{
			const uint16 *ptr;
			uint32 R,G,B,A;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr++,&R,&G,&B,&A);
						RGBi_to_YUVi(R,G,B,&R,&G,&B);
						addOctNode(root,R,G,B,&nLeaves);
					}
					ptr += xtra;
				}
				break;
			}

			case 3:
			{
			const uint8 *ptr;
			uint32 R,G,B,A,Pixel;

				ptr = Bits;
				xtra *= 3;

				switch(Format)
				{
				case GE_PIXELFORMAT_24BIT_RGB :
					for(y=h;y--;)
					{
						for(x=w;x--;)
						{
							RGBb_to_YUVi(ptr,&R,&G,&B);
							ptr += 3;
							addOctNode(root,R,G,B,&nLeaves);
						}
						ptr += xtra;
					}
					break;
				case GE_PIXELFORMAT_24BIT_BGR :
					for(y=h;y--;)
					{
						for(x=w;x--;)
						{
							B = *ptr++;
							G = *ptr++;
							R = *ptr++;
							RGBi_to_YUVi(R,G,B,&R,&G,&B);
							addOctNode(root,R,G,B,&nLeaves);
						}
						ptr += xtra;
					}
					break;
				default:
					// can't get here now
					for(y=h;y--;)
					{
						for(x=w;x--;)
						{
							Pixel = (ptr[0]<<16) + (ptr[1]<<8) + ptr[2]; ptr += 3;
							Decompose(Pixel,&R,&G,&B,&A);
							RGBi_to_YUVi(R,G,B,&R,&G,&B);
							addOctNode(root,R,G,B,&nLeaves);
						}
						ptr += xtra;
					}
					break;
				}
				break;  // Thanks Bobtree
			}

			case 4:
			{
			const uint32 *ptr;
			uint32 R,G,B,A;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr++,&R,&G,&B,&A);
						RGBi_to_YUVi(R,G,B,&R,&G,&B);
						addOctNode(root,R,G,B,&nLeaves);
					}
					ptr += xtra;
				}
				break;
			}
		}
	}
	else
	{
		switch(bpp)
		{
			default:
			case 0:
				return GE_FALSE;
			case 1:
			{
			const uint8 *ptr;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr++,&R,&G,&B,&A);
						addOctNode(root,R,G,B,&nLeaves);
					}
					ptr += xtra;
				}
				break;
			}
			case 2:
			{
			const uint16 *ptr;
			uint32 R,G,B,A;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr++,&R,&G,&B,&A);
						addOctNode(root,R,G,B,&nLeaves);
					}
					ptr += xtra;
				}
				break;
			}

			case 3:
			{
			const uint8 *ptr;
			uint32 R,G,B,A,Pixel;

				ptr = Bits;
				xtra *= 3;

				switch(Format)
				{
				case GE_PIXELFORMAT_24BIT_RGB :
					for(y=h;y--;)
					{
						for(x=w;x--;)
						{
							R = *ptr++;
							G = *ptr++;
							B = *ptr++;
							addOctNode(root,R,G,B,&nLeaves);
						}
						ptr += xtra;
					}
					break;
				case GE_PIXELFORMAT_24BIT_BGR :
					for(y=h;y--;)
					{
						for(x=w;x--;)
						{
							B = *ptr++;
							G = *ptr++;
							R = *ptr++;
							addOctNode(root,R,G,B,&nLeaves);
						}
						ptr += xtra;
					}
					break;
				default:
					// can't get here now
					for(y=h;y--;)
					{
						for(x=w;x--;)
						{
							Pixel = (ptr[0]<<16) + (ptr[1]<<8) + ptr[2]; ptr += 3;
							Decompose(Pixel,&R,&G,&B,&A);
							addOctNode(root,R,G,B,&nLeaves);
						}
						ptr += xtra;
					}
					break;
				}
				break;		// Thanks Bobtree
			}

			case 4:
			{
			const uint32 *ptr;
			uint32 R,G,B,A;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr++,&R,&G,&B,&A);
						addOctNode(root,R,G,B,&nLeaves);
					}
					ptr += xtra;
				}
				break;
			}
		}
	}

//	showPopTSC("create Pal OctTree");

return nLeaves;
}

/*}{*************************************************/
