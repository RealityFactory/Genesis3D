#ifndef LIST_H
#define LIST_H

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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

#include "basetype.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LISTCALL	__fastcall

/*******************************************/
/** you must wrap any calls to this module with these: **/

geBoolean List_Start(void);
geBoolean List_Stop(void);

/*******************************************/
/** basic list types ****/

typedef struct List List;

extern List *	LISTCALL List_Create(void);
extern void		LISTCALL List_Destroy(List * pList);
extern List *	LISTCALL List_AddTail(List *pList,void * Data);
extern List *	LISTCALL List_AddHead(List *pList,void * Data);
	// returns a pointer to the node created
extern void *	LISTCALL List_CutHead(List *pList);
extern void *	LISTCALL List_CutTail(List *pList);
extern void *	LISTCALL List_PeekHead(List *pList);
extern void *	LISTCALL List_PeekTail(List *pList);

extern List *	LISTCALL List_Next(List *pNode);
extern List *	LISTCALL List_Prev(List *pNode);

extern void		LISTCALL List_CutNode(List *pNode);
extern void		LISTCALL List_DeleteNode(List *pNode);
extern void		LISTCALL List_FreeNode(List *pNode);
extern void *	LISTCALL List_NodeData(List *pNode);

extern List *	List_Find(List *pList,void *Data);

/****

	Iterate on a list via :

	List *pNode,*pList;
	for( pNode = List_Next(pList); pNode != pList; pNode = List_Next(pNode) )
	{
		//do stuff to pNode
	}

****/

typedef struct Stack Stack;

extern Stack * 	LISTCALL Stack_Create(void);
extern void 	LISTCALL Stack_Destroy(Stack * pStack);
extern void 	LISTCALL Stack_Push_Func(Stack *pStack,void * Data);
extern void * 	LISTCALL Stack_Pop_Func(Stack *pStack);
extern int 		LISTCALL Stack_Extend(Stack *pStack);	// returns new length

#ifdef _DEBUG

#define Stack_Push	Stack_Push_Func
#define Stack_Pop	Stack_Pop_Func

#else

// this struct is exposed only in release mode!
// don't look into it!

struct Stack
{
	void ** Buffer, **End;
	void ** Head;
	int members;
};

//#define Stack_Push(pStack,Data)	do { *((pStack)->Head)++ = (void *)(Data); if ( (pStack)->Head == (pStack)->End ) Stack_Extend(pStack); } while(0)
#define Stack_Push(pStack,Data)	*((pStack)->Head)++ = (void *)(Data), ( (pStack)->Head != (pStack)->End ) || Stack_Extend(pStack)
#define Stack_Pop(pStack)		( ((pStack)->Head == (pStack)->Buffer) ? NULL : *( -- ((pStack)->Head) ) )

#endif

typedef struct Link Link;

extern Link * 	LISTCALL Link_Create(void);
extern void		LISTCALL Link_Destroy(Link * pLink);
extern void		LISTCALL Link_Push(Link *pLink,void * Data);
extern void *	LISTCALL Link_Pop(Link *pLink);
extern void *	LISTCALL Link_Peek(Link *pLink);

typedef struct LinkNode LinkNode;

/************************************/
/*** a radix of each type ****/

typedef struct RadixList RadixList;

extern RadixList * RadixList_Create(int RadixListMax);
extern void RadixList_Destroy(RadixList * pRadixList);
extern List * RadixList_Add(RadixList *pRadixList,void * Data,int Key);
	// returns a pointer to the node created
extern void * RadixList_CutMax(RadixList *pRadixList,int * pMaxKey);
extern void * RadixList_CutMin(RadixList *pRadixList,int * pMinKey);
extern void * RadixList_CutKey(RadixList *pRadixList,int Key);

typedef struct RadixLN RadixLN;

extern RadixLN * RadixLN_Create(int RadixLNMax);
extern void RadixLN_Destroy(RadixLN * pRadixLN);
extern void RadixLN_AddTail(RadixLN *pRadixLN,LinkNode * LN,int Key);
extern void RadixLN_AddHead(RadixLN *pRadixLN,LinkNode * LN,int Key);
extern LinkNode * RadixLN_CutMax(RadixLN *pRadixLN,int * pMaxKey);
extern LinkNode * RadixLN_CutMin(RadixLN *pRadixLN,int * pMinKey);
extern LinkNode * RadixLN_CutKey(RadixLN *pRadixLN,int Key);
extern LinkNode * RadixLN_PeekMax(RadixLN *pRadixLN,int * pMaxKey);
extern LinkNode * RadixLN_PeekMin(RadixLN *pRadixLN,int * pMinKey);

typedef struct RadixLink RadixLink;

extern			RadixLink * RadixLink_Create(int RadixLinkMax);
extern void		RadixLink_Destroy(RadixLink * pRadixLink);
extern void		RadixLink_Add(RadixLink *pRadixLink,void * Data,int Key);
extern void *	RadixLink_CutMax(RadixLink *pRadixLink,int * pMaxKey);
extern void *	RadixLink_CutMin(RadixLink *pRadixLink,int * pMinKey);
extern void *	RadixLink_CutKey(RadixLink *pRadixLink,int Key);
extern void		RadixLink_Grow(RadixLink *pRadixLink,int NewMax);

/******************************/

typedef struct Hash Hash;
typedef struct HashNode HashNode;

extern Hash *	Hash_Create(void);
extern void		Hash_Destroy(Hash *pHash);
HashNode *	LISTCALL Hash_Add(Hash *pHash,uint32 Key,uint32 Data);
void		LISTCALL Hash_DeleteNode(Hash *pHash,HashNode *pNode);
HashNode *	LISTCALL Hash_Get(Hash *pHash,uint32 Key,uint32 *pData);
							// pdata is optional
HashNode *	LISTCALL Hash_WalkNext(Hash *pHash,HashNode *pCur);
							//use pCur == NULL to start walking

uint32		LISTCALL Hash_NumMembers(Hash *pHash);

void	HashNode_SetData(HashNode *pNode,uint32 Data);
void	HashNode_GetData(HashNode *pNode,uint32 *pKey,uint32 *pData);
uint32	HashNode_Key(HashNode *pNode);
uint32	HashNode_Data(HashNode *pNode);

uint32	Hash_StringToKey(const char * String);

/******************************/

struct LinkNode 
{
	LinkNode *Next,*Prev;
}; 

#define zLN_InitList(List)			do { (List)->Next = List; (List)->Prev = List; } while(0)
#define zLN_Cut(Node)				do { (Node)->Prev->Next = (Node)->Next; (Node)->Next->Prev = (Node)->Prev; zLN_InitList(Node); } while(0)
#define zLN_Fix(Node)				do { (Node)->Prev->Next = Node; (Node)->Next->Prev = Node; } while(0)
#define zLN_AddAfter(Node,List)		do { (Node)->Prev = List; (Node)->Next = (List)->Next; LN_Fix(Node); } while(0)
#define zLN_AddBefore(Node,List)	do { (Node)->Next = List; (Node)->Prev = (List)->Prev; LN_Fix(Node); } while(0)
#define zLN_Walk_Editting(Node,List,Holder)	for( Node = (List)->Next; (Node) != (List) && ((Holder) = (Node)->Next) != NULL ; Node = Holder )
#define zLN_Walk(Node,List)			for( Node = (List)->Next; (Node) != (List) ; Node = (Node)->Next )
#define zLN_EmptyList(List)			( (List)->Next == (List) )

#define LN_InitList(List)			zLN_InitList((LinkNode *)List)
#define LN_Cut(Node)				zLN_Cut((LinkNode *)Node)
#define LN_Fix(Node)				zLN_Fix((LinkNode *)Node)
#define LN_AddAfter(Node,List)		zLN_AddAfter((LinkNode *)Node,(LinkNode *)List)
#define LN_AddBefore(Node,List)		zLN_AddBefore((LinkNode *)Node,(LinkNode *)List)
#define LN_Walk(Node,List)			zLN_Walk((LinkNode *)Node,(LinkNode *)List)
#define LN_Walk_Editting(Node,List,Holder)			zLN_Walk_Editting((LinkNode *)Node,(LinkNode *)List,((LinkNode *)Holder))
#define LN_EmptyList(List)			zLN_EmptyList((LinkNode *)List)
#define LN_Prev(Node)				(void *)(((LinkNode *)Node)->Prev)
#define LN_Next(Node)				(void *)(((LinkNode *)Node)->Next)

#define LN_Null(node)	LN_InitList(node)

LinkNode *	LISTCALL LN_CutHead(LinkNode *pList);
LinkNode *	LISTCALL LN_CutTail(LinkNode *pList);

int LN_ListLen(LinkNode *pList);

#define LN_AddHead(list,node)	LN_AddAfter(node,list)
#define LN_AddTail(list,node)	LN_AddBefore(node,list)
#define LN_IsEmpty	LN_EmptyList


	/* use LN_Walk as :
	*

		void doStuffOnAllNodes(LinkNode *pList)
		{
			LinkNode *pNode;
			LN_Walk(pNode,pList) {
				doStuff(pNode);
			}
		}

	*
	*/

#ifdef __cplusplus
}
#endif

#endif  // LIST_H

