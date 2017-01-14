/****************************************************************************************/
/*  List                                                                                */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description: List/Link/Node Primitives                                              */
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

//#define SAFE_HASH_DEBUG

/*****

List_Ram can still be as high as 30% of the time!

*******/

// #define DO_TIMER

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "mempool.h"
#include "ram.h"
#include "crc32.h"

/**********************************/
// Timer Stuff

#ifdef DO_TIMER
#include "timer.h"

TIMER_VARS(List_Ram);
TIMER_VARS(List_RadixWalk);
TIMER_VARS(List_RadixInit);

#else

#define TIMER_P(x)
#define TIMER_Q(X)

#endif // DO_TIMER

#ifdef _DEBUG
#define Debug(func)	func;
#define DebugAlert()	do { __asm { int 03h } } while(0)
#else
#define Debug(func)
#define DebugAlert()
#endif

#define MemAlloc(size)	geRam_AllocateClear(size)
#define MemFree(mem)	geRam_Free(mem)

#ifdef DO_TIMER // {
#undef new
#define new(type)		mymalloc(sizeof(type))
#undef destroy
#define destroy(mem)	if ( mem ) myfree(mem)

void * mymalloc(int size)
{
void *ret;
TIMER_P(List_Ram);
ret = MemAlloc(size);
TIMER_Q(List_Ram);
return ret;
}
void myfree(void *mem)
{
TIMER_P(List_Ram);
MemFree(mem);
TIMER_Q(List_Ram);
}

#undef MemAlloc
#define MemAlloc mymalloc
#undef MemFree
#define MemFree myfree

#else // }{ DO_TIMER

#undef  new
#define new(type)		MemAlloc(sizeof(type))
#undef  destroy
#define destroy(mem)	if ( mem ) MemFree(mem)

#endif // } DO_TIMER

/**********************************/

static MemPool *ListPool_g = NULL, *LinkPool_g = NULL, *HashNodePool_g = NULL;
static int UsageCount = 0;

/**********************************/

int LN_ListLen(LinkNode *pList)
{
LinkNode *pNode;
int Len=0;
	if ( ! pList )
		return 0;
	LN_Walk(pNode,pList) {
		Len++;
	}
return Len;
}

LinkNode *	LISTCALL LN_CutHead(LinkNode *pList)
{
LinkNode * LN;
	assert(pList);
	LN = pList->Next;
	if ( LN == pList ) return NULL;
	LN_Cut(LN);
return LN;
}

LinkNode *	LISTCALL LN_CutTail(LinkNode *pList)
{
LinkNode * LN;
	assert(pList);
	LN = pList->Prev;
	if ( LN == pList ) return NULL;
	LN_Cut(LN);
return LN;
}

/**********************************/

struct List
{
	List *Next,*Prev;
	void * Data;
};

void	LISTCALL List_Destroy(List * pList)
{
List * pNode;
	assert(pList);
	pNode = pList->Next;
	while( pNode != pList )
	{
	List *Next;
		Next = pNode->Next;
		MemPool_FreeHunk(ListPool_g,pNode);
		pNode = Next;
	}
	MemPool_FreeHunk(ListPool_g,pList);
}

List *	LISTCALL List_Create(void)
{
List * pNew;

	pNew = MemPool_GetHunk(ListPool_g);
	assert(pNew);
	pNew->Data = NULL;
	pNew->Next = pNew->Prev = pNew;
return pNew;
}

List *	LISTCALL List_AddTail(List *pList,void * Data)
{
List * pNew;

	pNew = MemPool_GetHunk(ListPool_g);
	assert(pNew);

	pNew->Next = pList;
	pNew->Prev = pList->Prev;
	pNew->Next->Prev = pNew;
	pNew->Prev->Next = pNew;
	
	pNew->Data= Data;
return pNew;
}

List *	LISTCALL List_AddHead(List *pList,void * Data)
{
List * pNew;

	pNew = MemPool_GetHunk(ListPool_g);
	assert(pNew);
	pNew->Data= Data;

	pNew->Prev = pList;
	pNew->Next = pList->Next;
	pNew->Next->Prev = pNew;
	pNew->Prev->Next = pNew;
return pNew;
}

void *	LISTCALL List_CutHead(List *pList)
{
List * pNode;
void * pData;
	pNode = pList->Next;
	if ( pNode == pList ) return NULL;
	pData = pNode->Data;

	pList->Next = pNode->Next;
	pList->Next->Prev = pList;

	MemPool_FreeHunk(ListPool_g,pNode);

return pData;
}

void *	LISTCALL List_CutTail(List *pList)
{
List * pNode;
void * pData;
	pNode = pList->Prev;
	if ( pNode == pList ) return NULL;
	pData = pNode->Data;

	pList->Prev = pNode->Prev;
	pList->Prev->Next = pList;

	MemPool_FreeHunk(ListPool_g,pNode);

return pData;
}


void *	LISTCALL List_PeekHead(List *pList)
{
List * pNode;
	pNode = pList->Next;
	if ( pNode == pList ) return NULL;
return pNode->Data;
}

void *	LISTCALL List_PeekTail(List *pList)
{
List * pNode;
	pNode = pList->Prev;
	if ( pNode == pList ) return NULL;
return pNode->Data;
}

void	LISTCALL List_CutNode(List *pNode)
{
	pNode->Prev->Next = pNode->Next;
	pNode->Next->Prev = pNode->Prev;

	pNode->Next = pNode->Prev = pNode;
}

void	LISTCALL List_DeleteNode(List *pNode)
{
	List_CutNode(pNode);
	List_FreeNode(pNode);
}

void	LISTCALL List_FreeNode(List *pNode)
{
	assert(pNode);
	MemPool_FreeHunk(ListPool_g,pNode);
}

void *	LISTCALL List_NodeData(List *pNode)
{
	assert(pNode);
	return pNode->Data;
}

List *	LISTCALL List_Next(List *pNode)
{
	return pNode->Next;
}
List *	LISTCALL List_Prev(List *pNode)
{
	return pNode->Prev;
}

List *	List_Find(List *pList,void *Data)
{
List *pNode;
	assert(pList);
	for(pNode = pList->Next; pNode != pList; pNode = pNode->Next )
	{
		if ( pNode->Data == Data )
			return pNode;
	}
	return NULL;
}

/***********************************************************/
// Stack by linked list

struct Link
{
	Link * Next;
	void * Data;
};

void	LISTCALL Link_Destroy(Link * pLink)
{
	while( pLink )
	{
	Link *Next;
		Next = pLink->Next;
		MemPool_FreeHunk(LinkPool_g,pLink);
		pLink = Next;
	}
}

Link *	LISTCALL Link_Create(void)
{
Link * pLink;
	pLink = MemPool_GetHunk(LinkPool_g);
	assert(pLink);
	pLink->Next = NULL;
	pLink->Data = NULL;
return pLink;
}

void	LISTCALL Link_Push(Link *pLink,void * Data)
{
Link * pNode;
	assert(pLink);
	pNode = MemPool_GetHunk(LinkPool_g);
	assert(pNode);
//	DebugWarn(geRam_IsValidPtr(pLink));
//	DebugWarn(geRam_IsValidPtr(pNode));
	pNode->Data = Data;

	pNode->Next = pLink->Next;
	pLink->Next = pNode;
}

void *	LISTCALL Link_Pop(Link *pLink)
{
void *pData;
Link * pNode;
	if ( ! pLink ) return NULL;
//	DebugWarn(geRam_IsValidPtr(pLink));
	pNode = pLink->Next;
	if ( ! pNode ) return NULL;
//	DebugWarn(geRam_IsValidPtr(pNode));
	pData = pNode->Data;
	pLink->Next = pNode->Next;
	MemPool_FreeHunk(LinkPool_g,pNode);
return pData;
}

void *	LISTCALL Link_Peek(Link *pLink)
{
void *pData;
Link * pNode;
	if ( ! pLink ) return NULL;
//	DebugWarn(geRam_IsValidPtr(pLink));
	pNode = pLink->Next;
	if ( ! pNode ) return NULL;
//	DebugWarn(geRam_IsValidPtr(pNode));
	pData = pNode->Data;
return pData;
}

/*************************************************************/

#ifdef _DEBUG	// else it's in list.h
struct Stack
{
	void ** Buffer, **End;
	void ** Head;
	int members;
};
#endif

void	LISTCALL Stack_Destroy(Stack * pStack)
{
	if ( pStack )
	{
		destroy(pStack->Buffer);
		destroy(pStack);
	}
}

Stack *	LISTCALL Stack_Create(void)
{
Stack * pStack;
	pStack = new(Stack);
	assert(pStack);
	pStack->members = 4096;
	pStack->Buffer = MemAlloc(sizeof(void *)*(pStack->members));
	assert(pStack->Buffer);
	pStack->End = pStack->Buffer + (pStack->members);
	pStack->Head = pStack->Buffer;
return pStack;
}

int	LISTCALL Stack_Extend(Stack *pStack)
{
void ** NewBuffer;
int newmembers;
	// realloc
	newmembers = pStack->members + 4096;
	NewBuffer = MemAlloc(sizeof(void *)*newmembers);
	assert(NewBuffer);
	memcpy(NewBuffer,pStack->Buffer, sizeof(void *)*(pStack->members));
	pStack->Head = NewBuffer + pStack->members;
	pStack->members = newmembers;
	MemFree(pStack->Buffer);
	pStack->Buffer = NewBuffer;

	pStack->End = pStack->Buffer + (pStack->members);
return newmembers;
}

void	LISTCALL Stack_Push_Func(Stack *pStack,void * Data)
{
	assert(pStack);
	*(pStack->Head)++ = Data;
	if ( pStack->Head == pStack->End )
		Stack_Extend(pStack);
}

void *	LISTCALL Stack_Pop_Func(Stack *pStack)
{
	assert(pStack);
	if ( pStack->Head == pStack->Buffer )
		return NULL;
	pStack->Head --;
return *(pStack->Head);
}

/*************************************************************/

struct RadixList
{
	int NumLists;
	int Min,Max;
	List ** Lists;
};

RadixList * RadixList_Create(int RadixListMax)
{
RadixList * pRadixList;
int r;
	pRadixList = new(RadixList);
	assert(pRadixList);
	pRadixList->NumLists = RadixListMax+1;
	pRadixList->Lists = MemAlloc(sizeof(List *)*(pRadixList->NumLists));
	assert(pRadixList->Lists);
	for(r=0;r<(pRadixList->NumLists);r++)
	{
		pRadixList->Lists[r] = List_Create();
		assert(pRadixList->Lists[r]);
	}
	pRadixList->Min = pRadixList->NumLists;
	pRadixList->Max = - 1;
return pRadixList;
}

void RadixList_Destroy(RadixList * pRadixList)
{
int r;
	assert(pRadixList);
	for(r=0;r<(pRadixList->NumLists);r++)
	{
		List_Destroy(pRadixList->Lists[r]);
	}
	MemFree(pRadixList->Lists);
	MemFree(pRadixList);
}

List * RadixList_Add(RadixList *pRadixList,void * Data,int Key)
{
List * l;
	assert(pRadixList);
	assert( Key < pRadixList->NumLists && Key >= 0 );
	l = List_AddTail(pRadixList->Lists[Key],Data);
	if ( Key < pRadixList->Min ) pRadixList->Min = Key;
	if ( Key > pRadixList->Max ) pRadixList->Max = Key;
return l;
}

void * RadixList_CutMax(RadixList *pRadixList,int *pKey)
{
	assert(pRadixList);

	while ( pRadixList->Max >= 0 )
	{
	void * Data;

		if ( Data = List_CutHead(pRadixList->Lists[pRadixList->Max]) )
		{
			if ( pKey ) *pKey = pRadixList->Max;
			return Data;	
		}
		pRadixList->Max --;

	}

return NULL;
}

void * RadixList_CutMin(RadixList *pRadixList,int *pKey)
{
	assert(pRadixList);

	while ( pRadixList->Min < pRadixList->NumLists )
	{
	void * Data;

		if ( Data = List_CutHead(pRadixList->Lists[pRadixList->Min]) )
		{
			if ( pKey ) *pKey = pRadixList->Min;
			return Data;	
		}
		pRadixList->Min ++;

	}

return NULL;
}

void * RadixList_CutKey(RadixList *pRadixList,int Key)
{
	assert(pRadixList);
return List_CutHead(pRadixList->Lists[Key]);
}

/*************************************************/

struct RadixLN
{
	int NumLNs;
	int Min,Max;
	LinkNode * LNs;
};

RadixLN * RadixLN_Create(int RadixLNMax)
{
RadixLN * pRadixLN;
LinkNode * LNs;
int r;
	pRadixLN = new(RadixLN);
	assert(pRadixLN);
	pRadixLN->NumLNs = RadixLNMax+1;
	pRadixLN->LNs = MemAlloc(sizeof(LinkNode)*(pRadixLN->NumLNs));
	assert(pRadixLN->LNs);

TIMER_P(List_RadixInit); // not counting the allocs, tracking in List_Ram

	LNs = pRadixLN->LNs;
	for(r=(pRadixLN->NumLNs);r--;LNs++)
	{
		// LN_InitList( LNs );
		LNs->Next = LNs->Prev = LNs;
	}
	pRadixLN->Min = RadixLNMax;
	pRadixLN->Max = 0;
	
TIMER_Q(List_RadixInit);

return pRadixLN;
}

void RadixLN_Destroy(RadixLN * pRadixLN)
{
	assert(pRadixLN);
	MemFree(pRadixLN->LNs);
	MemFree(pRadixLN);
}

void RadixLN_AddHead(RadixLN *pRadixLN,LinkNode *LN,int Key)
{
	assert(pRadixLN);
	assert( Key < pRadixLN->NumLNs && Key >= 0 );
	LN_AddHead(&(pRadixLN->LNs[Key]),LN);
	if ( Key < pRadixLN->Min ) pRadixLN->Min = Key;
	if ( Key > pRadixLN->Max ) pRadixLN->Max = Key;
}

void RadixLN_AddTail(RadixLN *pRadixLN,LinkNode *LN,int Key)
{
	assert(pRadixLN);
	assert( Key < pRadixLN->NumLNs && Key >= 0 );
	LN_AddTail(&(pRadixLN->LNs[Key]),LN);
	if ( Key < pRadixLN->Min ) pRadixLN->Min = Key;
	if ( Key > pRadixLN->Max ) pRadixLN->Max = Key;
}

LinkNode * RadixLN_CutMax(RadixLN *pRadixLN,int *pKey)
{
	assert(pRadixLN);

TIMER_P(List_RadixWalk);
	while ( pRadixLN->Max >= 0 )
	{
	LinkNode * LN;

		if ( LN = LN_CutHead(&(pRadixLN->LNs[pRadixLN->Max])) )
		{
			if ( pKey ) *pKey = pRadixLN->Max;
TIMER_Q(List_RadixWalk);
			return LN;
		}
		pRadixLN->Max --;
	}
TIMER_Q(List_RadixWalk);

return NULL;
}

LinkNode * RadixLN_CutMin(RadixLN *pRadixLN,int *pKey)
{
	assert(pRadixLN);

TIMER_P(List_RadixWalk);
	while ( pRadixLN->Min < pRadixLN->NumLNs )
	{
	LinkNode * LN;

		if ( LN = LN_CutHead(&(pRadixLN->LNs[pRadixLN->Min])) )
		{
			if ( pKey ) *pKey = pRadixLN->Max;
TIMER_Q(List_RadixWalk);
			return LN;
		}
		pRadixLN->Min ++;
	}
TIMER_Q(List_RadixWalk);

return NULL;
}

LinkNode * RadixLN_CutKey(RadixLN *pRadixLN,int Key)
{
	assert(pRadixLN);
return LN_CutHead(&(pRadixLN->LNs[Key]));
}


LinkNode * RadixLN_PeekMax(RadixLN *pRadixLN,int *pKey)
{
LinkNode * LNs;
	assert(pRadixLN);

TIMER_P(List_RadixWalk);
	LNs = & pRadixLN->LNs[pRadixLN->Max];
	while ( pRadixLN->Max >= 0 )
	{
	LinkNode * LN;

		LN = LNs->Next;
		if ( LN != LNs )
		{
			if ( pKey ) *pKey = pRadixLN->Max;
TIMER_Q(List_RadixWalk);
			return LN;
		}
		pRadixLN->Max --;
		LNs --;
	}
TIMER_Q(List_RadixWalk);

return NULL;
}

LinkNode * RadixLN_PeekMin(RadixLN *pRadixLN,int *pKey)
{
LinkNode * LNlist;
	assert(pRadixLN);

TIMER_P(List_RadixWalk);
	LNlist = & pRadixLN->LNs[pRadixLN->Min];
	while ( pRadixLN->Min < pRadixLN->NumLNs )
	{
	LinkNode * LN;

		LN = LNlist->Next;
		if ( LN != LNlist )
		{
			if ( pKey ) *pKey = pRadixLN->Min;
TIMER_Q(List_RadixWalk);
			return LN;
		}
		pRadixLN->Min ++;
		LNlist ++;
	}
TIMER_Q(List_RadixWalk);

return NULL;
}

/*************************************************/

struct RadixLink
{
	int NumLinks;
	int Min,Max;
	Link * Links;
};

RadixLink * RadixLink_Create(int RadixLinkMax)
{
RadixLink * pRadixLink;
	pRadixLink = new(RadixLink);
	assert(pRadixLink);
	pRadixLink->NumLinks = RadixLinkMax+1;
	pRadixLink->Links = MemAlloc(sizeof(Link)*(pRadixLink->NumLinks));
	assert(pRadixLink->Links);
	//memset(pRadixLink->Links,0,(sizeof(Link)*(pRadixLink->NumLinks)));
	pRadixLink->Min = pRadixLink->NumLinks;
	pRadixLink->Max = - 1;
return pRadixLink;
}

void RadixLink_Grow(RadixLink *pRadixLink,int NewMax)
{
Link * OldLinks;
int OldNumLinks;

	OldNumLinks = pRadixLink->NumLinks;
	OldLinks = pRadixLink->Links;

	pRadixLink->NumLinks = NewMax + 1;
	pRadixLink->Links = MemAlloc(sizeof(Link)*(pRadixLink->NumLinks));

	assert(pRadixLink->Links);

	memcpy(pRadixLink->Links, OldLinks, (sizeof(Link)*OldNumLinks));
	//memset(pRadixLink->Links + OldNumLinks,0,(sizeof(Link)*(pRadixLink->NumLinks - OldNumLinks)));

	MemFree(OldLinks);
}

void RadixLink_Destroy(RadixLink * pRadixLink)
{
int r;
	assert(pRadixLink);
	for(r=0;r<(pRadixLink->NumLinks);r++)
	{
		if ( pRadixLink->Links[r].Next )
			Link_Destroy(pRadixLink->Links[r].Next);
	}
	MemFree(pRadixLink->Links);
	MemFree(pRadixLink);
}

void RadixLink_Add(RadixLink *pRadixLink,void * Data,int Key)
{
	assert(pRadixLink);
	assert( Key >= 0 );

	if ( Key >= pRadixLink->NumLinks )
		RadixLink_Grow(pRadixLink, Key + (Key>>2) );

	Link_Push(&(pRadixLink->Links[Key]),Data);

	if ( Key < pRadixLink->Min ) pRadixLink->Min = Key;
	if ( Key > pRadixLink->Max ) pRadixLink->Max = Key;
}

void * RadixLink_CutMax(RadixLink *pRadixLink,int *pKey)
{
Link * pLink;
	pLink = (pRadixLink->Links) + (pRadixLink->Max);
	while ( pRadixLink->Max >= 0 )
	{
		if ( pLink->Next )
		{
			if ( pKey ) *pKey = pRadixLink->Max;
			return Link_Pop(pLink);
		}
		pRadixLink->Max --;
		pLink --;
	}
return NULL;
}

void * RadixLink_CutMin(RadixLink *pRadixLink,int *pKey)
{
Link * pLink;
	pLink = (pRadixLink->Links) + (pRadixLink->Min);
	while ( pRadixLink->Min < pRadixLink->NumLinks )
	{
		if ( pLink->Next )
		{
			if ( pKey ) *pKey = pRadixLink->Min;
			return Link_Pop(pLink);
		}
		pRadixLink->Min ++;
		pLink ++;
	}
return NULL;
}

void * RadixLink_CutKey(RadixLink *pRadixLink,int Key)
{
	assert(pRadixLink);
return Link_Pop(&(pRadixLink->Links[Key]));
}


/***** debug only : **********************/

void List_TimerReport(void)
{
#ifdef DO_TIMER
	TIMER_REPORT(List_Ram);
	TIMER_REPORT(List_RadixWalk);
	TIMER_REPORT(List_RadixInit);
#endif
}

/*************************************************/

/**************************************************

Hash has separate Keys & Data

We use a single Circular list of all the nodes.  We have
cappers of Hash = 0 and HASH_SIZE at the head and tail.
So a general hash looks like : (with HASH_SIZE == 7)


(Head)-->A--x->C--x->E--x-->(Tail)
[0]		[1][2][3][4][5][6]

the brackets are hash buckets; little x's mean that hash bucket
has seen no nodes of its own hash, so it points to the next
larger hash's list.

to do a Get :
	simply look at your current pointer and walk while hash == my hash
	example : Get(Hash == 3)
		we get node C : ok, test it
		next is node E : not my hash, so I'm done

to do a delete :
	simply cut your node, and point yourself to the next node.
	example : Delete( Node C )

(Head)-->A--x--x--x->E--x-->(Tail)
[0]		[1][2][3][4][5][6]

		now hash bucket 3 points to node E

to do an add :

	first you must take your current pointer and walk backwards until
	the preceding node has a lower hash than you (see example later).

	then simply add the new node before the current one and make yourself
	point to the new one.

	Example 1: Add an A:
		bucket 1 already is in the right place
		add before the old node :

       +-A1
       | |
(Head)-+ A2-x--x--x->E--x-->(Tail)
[0]		[1][2][3][4][5][6]

		now make the hash point to the new add:

         A2-+
         |  |
(Head)---A1 x--x--x->E--x-->(Tail)
[0]		[1][2][3][4][5][6]

	Example 2 : we need the extra seek in each _Add()
		add a node C at hash 3


         A--+--+
         |     |
(Head)---A (E) C--x->E--x-->(Tail)
[0]		[1][2][3][4][5][6]

		the funny notation is to indicate that bucket 2 still
		points to node E.

		now add a node B at hash 2
		we currently point to node (E)
		the previous node is (C), which is greater than B, so
		we step back; now the current is (C) and the previous is (A) :

         A--+
         |  |   
(Head)---A  +->C--x->E--x-->(Tail)
[0]		[1][2][3][4][5][6]

		which is a good list, so we do the normal add:

         A--+
         |  |   
(Head)---A  B->C--x->E--x-->(Tail)
[0]		[1][2][3][4][5][6]

**************

the whole goal of this system is that the WalkNext() function is
blazing fast.  The only thing that hurts in the _Add function, but
the cost of adding N nodes to hash of size M is only O(N + M*M)
(because the first M adds are O(M) and the later adds are O(1))

*****************************/

#ifdef SAFE_HASH_DEBUG //{

#pragma message("using safe debug hash implementation")

struct Hash
{
	HashNode ** NodeArray;
	int			NodeArrayLen;
	int			NodeCount;
};


struct HashNode
{
	uint32	Key;
	uint32	Data;
};

Hash *	Hash_Create(void)
{
Hash * H;
	H = new(Hash);
	assert(H);
	//memset(H,0,sizeof(*H));
return H;
}

void Hash_Destroy(Hash *H)
{
	assert(H);
	if ( H->NodeArray )
	{
	int n;
		for(n=0;n<H->NodeCount;n++)
		{
			MemPool_FreeHunk(HashNodePool_g,H->NodeArray[n]);
		}
		destroy(H->NodeArray);
	}

	destroy(H);
}	

HashNode *	LISTCALL Hash_Add(Hash *H,uint32 Key,uint32 Data)
{
HashNode * hn;
	hn = MemPool_GetHunk(HashNodePool_g);
	assert(hn);
	hn->Key = Key;
	hn->Data = Data;

	if ( H->NodeCount == H->NodeArrayLen )
	{
		H->NodeArrayLen = H->NodeCount + 100;
		if ( H->NodeArray )
		{
			H->NodeArray = geRam_Realloc(H->NodeArray,H->NodeArrayLen*sizeof(HashNode *));
		}
		else
		{
			H->NodeArray = geRam_Allocate(H->NodeArrayLen*sizeof(HashNode *));
		}
		assert(H->NodeArray);
	}

	H->NodeArray[H->NodeCount] = hn;
	H->NodeCount ++;

return hn;
}

void		LISTCALL Hash_DeleteNode(Hash *pHash,HashNode *pNode)
{
int n;

	n = 0;
	while( pHash->NodeArray[n] != pNode )
	{
		n++;
		assert( n < pHash->NodeCount );
	}

	pHash->NodeArray[n] = pHash->NodeArray[ pHash->NodeCount - 1];
	assert(pHash->NodeArray[n]);
	pHash->NodeArray[ pHash->NodeCount - 1] = NULL;
	pHash->NodeCount --;

	MemPool_FreeHunk(HashNodePool_g,pNode);
}

HashNode *	LISTCALL Hash_Get(Hash *pHash,uint32 Key,uint32 *pData)
{
int n;
	for(n=0;n<pHash->NodeCount;n++)
	{
		if ( pHash->NodeArray[n]->Key == Key )
		{
			if ( pData )
				*pData = pHash->NodeArray[n]->Data;
			return pHash->NodeArray[n];
		}
	}
	return NULL;
}

HashNode *	LISTCALL Hash_WalkNext(Hash *pHash,HashNode *pCur)
{
int n;

	if ( pCur )
	{
		n = 0;
		while( pHash->NodeArray[n] != pCur )
		{
			n++;
			assert( n < pHash->NodeCount );
		}
		n ++;
	}
	else
	{
		n = 0;
	}
	if ( n == pHash->NodeCount )
		return NULL;
	else
		return pHash->NodeArray[n];
}

uint32		LISTCALL Hash_NumMembers(Hash *pHash)
{
	return pHash->NodeCount;
}


#else //}{ the real thing

#define HASH_MOD	(1009) // or 1013  , a nice prime
#define HASH_SIZE	(HASH_MOD + 1) 
#define HASH(Key)	((( ((Key)>>15) ^ (Key) )%HASH_MOD)+1)

struct Hash
{

	Debug(Hash * MySelf1)

	HashNode *	HashTable[HASH_SIZE];
	HashNode *	NodeList;

	Debug(int	Members)
	Debug(Hash * MySelf2)
};

struct HashNode
{
	LinkNode LN;
	uint32	Key;
	uint32	Data;
	uint32	Hash;
};

Hash * Hash_Create(void)
{
Hash * pHash;
HashNode *pHead,*pTail;

	pHash = new(Hash);
	if ( ! pHash ) 
		return NULL;
	
	//memset(pHash,0,sizeof(Hash));

	Debug( pHash->MySelf1 = pHash )
	Debug( pHash->MySelf2 = pHash )

	pTail = MemPool_GetHunk(HashNodePool_g);
	assert(pTail);
	LN_Null(pTail);
	pTail->Key = pTail->Data = 0;
	pTail->Hash = HASH_SIZE;

	pHead = MemPool_GetHunk(HashNodePool_g);
	assert(pHead);
	LN_Null(pHead);
	pHead->Key = pHead->Data = 0;
	pHead->Hash = 0;

	LN_AddBefore(pHead,pTail);

	pHash->NodeList = pHead;

	Debug(pHash->Members = 0)

return pHash;
}

void Hash_Destroy(Hash *pHash)
{
	if ( pHash )
	{
	HashNode *pList,*pNode,*pNext;
	
		assert( pHash->MySelf1 == pHash && pHash->MySelf2 == pHash );

		Debug(pHash->Members += 2) // count Head & Tail

		pList = pHash->NodeList;
		LN_Walk_Editting(pNode,pList,pNext) {
			MemPool_FreeHunk(HashNodePool_g,pNode);
			assert(pHash->Members > 1);
			Debug(pHash->Members --)
		}
		assert(pHash->Members == 1);
		MemPool_FreeHunk(HashNodePool_g,pList);
		destroy(pHash);
	}
}

#ifdef _DEBUG
int Hash_ListLen(Hash *pHash,uint32 H)
{
HashNode *hn;
int Len = 0;
	hn = pHash->HashTable[H];
	if ( ! hn )
		return 0;
	while( hn->Hash == H )
	{
		Len ++;
		assert(pHash->Members >= Len);
		hn = LN_Next(hn);
	}
return Len;
}
#endif

HashNode * LISTCALL Hash_Add(Hash *pHash,uint32 Key,uint32 Data)
{
uint32 H;
HashNode *hn,**pTable,*Node,*Prev;
Debug( int ListLen1; int ListLen2; int HashLen1; int HashLen2; int WalkLen)

	assert(pHash);
	assert( pHash->MySelf1 == pHash && pHash->MySelf2 == pHash );

	hn = MemPool_GetHunk(HashNodePool_g);
	assert(hn);
	Debug( LN_Null(hn) )

	hn->Key = Key;
	hn->Data = Data;
	hn->Hash = H = HASH(Key);

	pTable = &(pHash->HashTable[H]);
	Node = *pTable;

	assert( H > 0 );
	assert( pHash->NodeList->Hash == 0 );
	assert( ((HashNode *)LN_Prev(pHash->NodeList))->Hash == HASH_SIZE );

	Debug(HashLen1 = Hash_NumMembers(pHash))
	Debug(ListLen1 = Hash_ListLen(pHash,H))

	if ( ! Node )
	{
		Prev = pHash->NodeList;
		Node = LN_Next(Prev);

		Debug(WalkLen = 0)

		assert(Prev->Hash < H);
		while( Node->Hash < H )
		{
			Prev = Node;
			Node = LN_Next(Prev);
		
			assert(WalkLen < pHash->Members );
			Debug( WalkLen ++)
			
			assert(Prev->Hash <= Node->Hash);
			assert(Prev->Hash < HASH_SIZE );
		}

		assert(Prev->Hash < H);
		assert(Node->Hash >= H);
	}

	LN_AddBefore(hn,Node);
	*pTable = hn;

	Debug(pHash->Members ++)

	Debug(ListLen2 = Hash_ListLen(pHash,H))
	assert( ListLen2 == (ListLen1 + 1) );
	Debug(HashLen2 = Hash_NumMembers(pHash))
	assert( HashLen2 == (HashLen1 + 1) );

return hn;
}

void LISTCALL Hash_DeleteNode(Hash *pHash,HashNode *pNode)
{
HashNode ** pHead;
uint32 H;
Debug( int ListLen1; int ListLen2;int HashLen1; int HashLen2)

	assert(pNode );
	assert( pHash );
	assert( pHash->MySelf1 == pHash && pHash->MySelf2 == pHash );

	assert( pNode->Hash > 0 && pNode->Hash < HASH_SIZE );

	H = pNode->Hash;
	pHead = & ( pHash->HashTable[H] );

	Debug( HashLen1 = Hash_NumMembers(pHash) )
	Debug( ListLen1 = Hash_ListLen(pHash,H) )
	assert(pHash->Members > 0);
	Debug(pHash->Members --)

	if ( *pHead == pNode )
	{
		*pHead = LN_Next(pNode);
		if ( (*pHead)->Hash != H )
		{
			// pNode was the head of the list
			assert( ((HashNode *)LN_Prev(pNode))->Hash < H );
			*pHead = NULL;
		}
	}
	assert( *pHead == NULL || (*pHead)->Hash == H );

	LN_Cut(pNode);

	MemPool_FreeHunk(HashNodePool_g,pNode);
	
	Debug( HashLen2 = Hash_NumMembers(pHash) )
	Debug( ListLen2 = Hash_ListLen(pHash,H) )
	assert( HashLen2 == (HashLen1 - 1) );

}

HashNode * LISTCALL Hash_Get(Hash *pHash,uint32 Key,uint32 *pData)
{
uint32 H;
HashNode *pNode;

	assert(pHash );
	assert( pHash->MySelf1 == pHash && pHash->MySelf2 == pHash );

	H = HASH(Key);
	assert( H > 0 && H < HASH_SIZE );

	pNode = pHash->HashTable[H];
	if ( ! pNode )
		return NULL;

	assert( pNode->Hash == H );

	while( pNode->Hash == H )
	{
		if ( pNode->Key == Key )
		{
			if ( pData )
				*pData = pNode->Data;
			return pNode;
		}
		pNode = LN_Next(pNode);
	}

return NULL;
}

HashNode * LISTCALL Hash_WalkNext(Hash *pHash,HashNode *pNode)
{
HashNode *pNext;

	assert(pHash);
	assert( pHash->MySelf1 == pHash && pHash->MySelf2 == pHash );

	if ( ! pNode )
	{
		pNode = pHash->NodeList;
		assert( pNode->Hash == 0 );
	}

	pNext = LN_Next(pNode);
	assert( pNext->Hash > 0 );
	assert( pNext->Hash >= pNode->Hash );

	assert( pNext != pNode );
	assert( LN_Prev(pNext) == pNode );

	if ( pNext->Hash == HASH_SIZE )
	{
		assert( ((HashNode *)LN_Next(pNext)) == pHash->NodeList );
		return NULL;
	}

	assert(pNext->Hash < HASH_SIZE);

return pNext;
}

uint32		LISTCALL Hash_NumMembers(Hash *pHash)
{
uint32 N;
HashNode *pNode;

	assert(pHash);
	assert( pHash->MySelf1 == pHash && pHash->MySelf2 == pHash );

	pNode = pHash->NodeList;
	assert( pNode->Hash == 0 );
	pNode = LN_Next(pNode);
	assert( pNode->Hash > 0 );

	N = 0;
	while( pNode->Hash < HASH_SIZE )
	{
		assert( N < (uint32)pHash->Members );
		pNode = LN_Next(pNode);
		N ++;
	}

	assert( N == (uint32)pHash->Members );

return N;
}

#endif //}{ hashes

uint32	Hash_StringToKey(const char * String)
{
return CRC32_Array(String,strlen(String));
}

void	HashNode_SetData(HashNode *pNode,uint32 Data)
{
	assert(pNode);
	pNode->Data = Data;
}

void	HashNode_GetData(HashNode *pNode,uint32 *pKey,uint32 *pData)
{
	assert(pNode);
	*pKey	= pNode->Key;
	*pData	= pNode->Data;
}

uint32	HashNode_Data(HashNode *pNode)
{
	assert(pNode);
return pNode->Data;
}

uint32	HashNode_Key(HashNode *pNode)
{
	assert(pNode);
return pNode->Key;
}

/***************************/

geBoolean List_Start(void)
{
	assert(UsageCount >= 0 );
	if ( UsageCount == 0 )
	{
		assert(ListPool_g == NULL && LinkPool_g == NULL);
		if ( ! (ListPool_g = MemPool_Create(sizeof(List),1024,1024) ) )
			return GE_FALSE;
		if ( ! (LinkPool_g = MemPool_Create(sizeof(Link),128,128) ) )
			return GE_FALSE;
		if ( ! (HashNodePool_g = MemPool_Create(sizeof(HashNode),128,128) ) )
			return GE_FALSE;
		// could latch ListFuncs_Stop() on atexit() , but no need, really..
	}
	UsageCount ++;
	return GE_TRUE;
}

geBoolean List_Stop(void)
{
	assert(UsageCount > 0 );
	UsageCount --;
	if ( UsageCount == 0 )
	{
		assert(ListPool_g && LinkPool_g );
		MemPool_Destroy(&ListPool_g); ListPool_g = NULL;
		MemPool_Destroy(&LinkPool_g); LinkPool_g = NULL;
		MemPool_Destroy(&HashNodePool_g); HashNodePool_g = NULL;
	}
	return GE_TRUE;
}
