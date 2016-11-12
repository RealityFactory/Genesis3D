/****************************************************************************************/
/*  MEMPOOL.C                                                                           */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description: Fixed size block memory allocator implementation                       */
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
#include <string.h>
#include <assert.h>

#include "mempool.h"
#include "ram.h"

/*
 *	MemPool is a 'root' level object (eg. 'list' uses us).  We sit only above 'Ram'
 *		in the heirarchy
 *
 *  MemPool
 *    system for fast Resetting & Freeing of nodes; for use in tree structures
 *     requires no traversing for freeing
 *    does auto-extending of memory space in case you use more space than
 *     expected or if you don't know how much you will need
 *	HunkLength is forced to be a multiple of 4
 *
 */

#define RamCalloc(size)			geRam_AllocateClear(size)
#define RamFree(mem)			geRam_Free(mem)
#define RamRealloc(mem,size)	geRam_Realloc(mem,size)

#ifndef memclear
#define memclear(mem,size)	memset(mem,0,size);
#endif

/**********************/

/*structs:*/
typedef struct MemBlock MemBlock;
struct MemBlock
{
	MemBlock * Next;
	char * MemBase;
	char * MemPtr;
	int MemLength;
	int MemFree;	// MemPtr + MemFree == MemBase + MemLength
};


struct MemPool
{
	int HunkLength;
	MemBlock * CurMemBlock; // jump into the list
	MemBlock * MemList; // list of memblocks
	int AutoExtendNumItems;
	int NumFreedHunks;
	int MaxNumFreedHunks;
	void ** FreedHunks;
	int NumItemsActive;
};

int MemBlock_IsValid(MemBlock *mb)
{
	if ( ! mb ) return 0;
	if ( mb->Next )
	{
		if ( ! MemBlock_IsValid(mb->Next) )
			return 0;
	}

	assert( geRam_IsValidPtr(mb->MemBase) );

	if ( mb->MemFree < 0 || mb->MemFree > mb->MemLength )
		return 0;
	if ( mb->MemPtr != mb->MemBase + (mb->MemLength - mb->MemFree) )
		return 0;
return 1;
}

int MemPool_IsValid(MemPool * Pool)
{
	if ( ! Pool ) return 0;
	if ( Pool->NumFreedHunks < 0 || Pool->AutoExtendNumItems < 0
		|| Pool->MaxNumFreedHunks < 0 || Pool->NumItemsActive || Pool->HunkLength )
		return 0;
	if ( Pool->NumFreedHunks >= Pool->MaxNumFreedHunks ) return 0;

	// make sure curmemblock is in the memblock list
	// <> make sure the freedhunks are in the memblocks (this is the most likely error)

	return MemBlock_IsValid(Pool->MemList);
}

MemPool * MemPool_Create (int HunkLength,int NumHunks,int AutoExtendNumItems)
{
MemPool * Ret;

if ( (Ret = RamCalloc(sizeof(MemPool))) == NULL )
  return(NULL);

Ret->HunkLength = (HunkLength+3)&(~3);
Ret->CurMemBlock = NULL;
Ret->MemList = NULL;
Ret->NumFreedHunks = 0;
Ret->MaxNumFreedHunks = 16;
Ret->AutoExtendNumItems = AutoExtendNumItems;
Ret->NumItemsActive = 0;

if ( (Ret->FreedHunks = RamCalloc(Ret->MaxNumFreedHunks*sizeof(void *))) == NULL )
{
	RamFree(Ret);
	return(NULL);
}

if ( ! MemPool_Extend(Ret,NumHunks) )
{
	RamFree(Ret->FreedHunks);
	RamFree(Ret);
	return(NULL);
}

return(Ret);
}

void MemPool_Destroy(MemPool ** pPool)
{
MemBlock * CurMemBlock;
MemBlock * NextMemBlock;
MemPool * Pool;

	assert(pPool);

	Pool = *pPool;

	if ( Pool == NULL ) return;

	CurMemBlock = Pool->MemList;
	while(CurMemBlock)
	{
		RamFree(CurMemBlock->MemBase);
		NextMemBlock = CurMemBlock->Next;
		RamFree(CurMemBlock);
		CurMemBlock = NextMemBlock;
	}

	RamFree(Pool->FreedHunks);
	RamFree(Pool);

	*pPool = NULL;
}

void * MemPool_GetHunk (MemPool * Pool)
{
void * Ret;
MemBlock * CurMemBlock;

	if ( Pool->NumFreedHunks > 0 )
	{
		Pool->NumFreedHunks--;
		Ret = Pool->FreedHunks[Pool->NumFreedHunks];

		goto GotHunk;
	}

	if ( (CurMemBlock = Pool->CurMemBlock) == NULL )
		return(NULL);

	if ( CurMemBlock->MemFree < Pool->HunkLength )
	{
		if ( ! MemPool_Extend(Pool,Pool->AutoExtendNumItems) )
				return(NULL);
		CurMemBlock = Pool->CurMemBlock;
	}

	Ret = (void *) CurMemBlock->MemPtr;

	CurMemBlock->MemFree -= Pool->HunkLength;
	CurMemBlock->MemPtr	+= Pool->HunkLength;

GotHunk:

// clear at alloc & reset & Free, not in Get
//	memclear(Ret,Pool->HunkLength);

	Pool->NumItemsActive++;

return(Ret);
}

void MemPool_Reset(MemPool * Pool)
{
MemBlock * CurMemBlock;

	Pool->NumFreedHunks = 0;
	Pool->NumItemsActive = 0;
	Pool->CurMemBlock = Pool->MemList;

	for( CurMemBlock = Pool->MemList; CurMemBlock ; CurMemBlock = CurMemBlock->Next)
	{
		CurMemBlock->MemFree = CurMemBlock->MemLength;
		CurMemBlock->MemPtr = CurMemBlock->MemBase;

		memclear(CurMemBlock->MemBase,CurMemBlock->MemLength);
	}
}

int MemPool_Extend(MemPool * Pool, int NumHunks)
{
MemBlock * NewMemBlock;

	NewMemBlock = Pool->MemList;
	while ( NewMemBlock )
	{
		if ( NewMemBlock->MemFree > Pool->HunkLength )
		{
			Pool->CurMemBlock = NewMemBlock;
			return(1);
		}
		NewMemBlock = NewMemBlock->Next;
	}

	if ( (NewMemBlock = RamCalloc(sizeof(MemBlock))) == NULL )
		return(0);

	NewMemBlock->MemLength = NumHunks * Pool->HunkLength;
	NewMemBlock->MemFree = NewMemBlock->MemLength;

	if ( (NewMemBlock->MemBase = RamCalloc(NewMemBlock->MemLength)) == NULL )
	{
		RamFree(NewMemBlock);
		return(0);
	}

	NewMemBlock->MemPtr = NewMemBlock->MemBase;

	NewMemBlock->Next = Pool->MemList;
	Pool->MemList = NewMemBlock;

	Pool->CurMemBlock = NewMemBlock;

return(1);
}

int MemPool_FreeHunk(MemPool * Pool,void * Hunk)
{
	assert( Pool );
	// <> assert Hunk is in Pool !

	// we must use this growing array for Freed Hunks, because
	//  to use a linked list of freed hunks, we'd need to use MemPool !!!

	if ( Pool->NumFreedHunks >= Pool->MaxNumFreedHunks )
	{
		Pool->MaxNumFreedHunks <<= 1;

		Pool->FreedHunks = RamRealloc(Pool->FreedHunks,Pool->MaxNumFreedHunks*sizeof(void *));
		assert(Pool->FreedHunks);
		if ( ! Pool->FreedHunks )
			return(0);
	}

	Pool->FreedHunks[Pool->NumFreedHunks] = Hunk;
	Pool->NumFreedHunks++;

	Pool->NumItemsActive--;

	memclear(Hunk,Pool->HunkLength);

return(1);
}

/*************************/
static MemBlock * WalkMemBlock;
static char *WalkPtr,*WalkPtrEnd;
static MemPool * LastPool;

void * MemPool_WalkNext(MemPool *Pool,void *Hunk)
{
	if ( ! Pool ) return NULL;
	if ( ! Hunk )
	{
		LastPool = Pool;
		WalkMemBlock = Pool->MemList;
		if ( ! WalkMemBlock ) return NULL;
		WalkPtr = WalkMemBlock->MemBase;
		WalkPtrEnd = WalkPtr + (WalkMemBlock->MemLength - WalkMemBlock->MemFree);
	}
	else
	{
		assert( Pool == LastPool );
		assert( WalkPtr == Hunk );
		rewalk:
		WalkPtr += Pool->HunkLength;
		if ( WalkPtr >= WalkPtrEnd )
		{
			WalkMemBlock = WalkMemBlock->Next;
			if ( ! WalkMemBlock ) return NULL;
			WalkPtr = WalkMemBlock->MemBase;
			WalkPtrEnd = WalkPtr + (WalkMemBlock->MemLength - WalkMemBlock->MemFree);
		}
	}

	// must check to see if WalkPtr is in the FreedHunks ! 
	// this is so slow it makes this function worthless!
	{
	int i;
		for(i=0;i<Pool->NumFreedHunks;i++)
		{
			if ( WalkPtr == Pool->FreedHunks[i] )
			{
				Hunk = WalkPtr;
				goto rewalk;
			}
		}
	}

return WalkPtr;
}
