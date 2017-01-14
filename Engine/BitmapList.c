/****************************************************************************************/
/*  BitmapList.c                                                                        */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description: Maintains a pool of bitmap pointers.                                   */
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
#include <Assert.h>

#ifdef _DEBUG
#include <windows.h>
#include <stdio.h>
#endif

#include "BitmapList.h"
#include "DCommon.h"
#include "Bitmap.h"
#include "Bitmap._h"
#include "list.h"
#include "mempool.h"
#include "errorlog.h"
#include "ram.h"
//#include "tsc.h"

struct BitmapList
{
#ifdef _DEBUG
	BitmapList * MySelf;
#endif
	Hash * Hash;
	int Members,Adds;
};


geBoolean BitmapList_IsValid(BitmapList *pList);

//================================================================================
//	BitmapList_Create
//================================================================================
BitmapList *BitmapList_Create(void)
{
BitmapList * pList;
	pList = geRam_Allocate(sizeof(*pList));
	if (! pList )
		return NULL;
	memset(pList,0,sizeof(*pList));
	pList->Hash = Hash_Create();
	if ( ! pList->Hash )
	{
		geRam_Free(pList);
		return NULL;
	}
	#ifdef _DEBUG
	pList->MySelf = pList;
	#endif
return pList;
}

//================================================================================
//	BitmapList_Destroy
//================================================================================
geBoolean BitmapList_Destroy(BitmapList *pList)
{
geBoolean	Ret = GE_TRUE;

	if ( ! pList )
		return GE_TRUE;

	if ( pList->Hash )
	{
	HashNode	*pNode;
		pNode = NULL;
		
		while( (pNode = Hash_WalkNext(pList->Hash,pNode)) != NULL )
		{
		geBitmap *Bmp;
		uint32 TimesAdded;

			HashNode_GetData(pNode,(uint32 *)&Bmp,&TimesAdded);

			if (!geBitmap_DetachDriver(Bmp, GE_TRUE))
				Ret = GE_FALSE;

			assert( pList->Members >= 1 && pList->Adds >= (int)TimesAdded );

			pList->Members --;

			assert( TimesAdded >= 1 );

			while(TimesAdded --)
			{
				assert(Bmp);
				geBitmap_Destroy(&Bmp);
				pList->Adds --;
			}
		}

		// Finally, destroy the entire hash table
		Hash_Destroy(pList->Hash);
	}

	geRam_Free(pList);

	return Ret;
}

//================================================================================
//	BitmapList_SetGamma
//================================================================================
geBoolean BitmapList_SetGamma(BitmapList *pList, geFloat Gamma)
{
HashNode *pNode;

	assert(BitmapList_IsValid(pList));

#ifdef _DEBUG
	//pushTSC();
#endif

	pNode = NULL;
	while( (pNode = Hash_WalkNext(pList->Hash,pNode)) != NULL )
	{
	geBitmap *Bmp;
		Bmp = (geBitmap *)HashNode_Key(pNode);

		if (!geBitmap_SetGammaCorrection(Bmp, Gamma, GE_TRUE) )
		{
			geErrorLog_AddString(-1,"BitmapList_SetGamma : SetGamma failed.", NULL);
			return GE_FALSE;
		}
	}
	
#ifdef _DEBUG
	//showPopTSCper("BitmapList_SetGamma",pList->MembersAttached,"bitmap");
#endif

return GE_TRUE;
}

//================================================================================
//	BitmapList_AttachAll
//================================================================================
geBoolean BitmapList_AttachAll(BitmapList *pList, DRV_Driver *Driver, geFloat Gamma)
{
HashNode *pNode;
int MembersAttached;

	assert(BitmapList_IsValid(pList));

	//pushTSC();

	pNode = NULL;
	MembersAttached = 0;
	while( (pNode = Hash_WalkNext(pList->Hash,pNode)) != NULL )
	{
	geBitmap *Bmp;

		Bmp = (geBitmap *)HashNode_Key(pNode);

		if (!geBitmap_SetGammaCorrection_DontChange(Bmp, Gamma) )
		{
			geErrorLog_AddString(-1,"BitmapList_AttachAll : SetGamma failed", NULL);
			return GE_FALSE;
		}

		if (!geBitmap_AttachToDriver(Bmp, Driver, 0) )
		{
			geErrorLog_AddString(-1,"BitmapList_AttachAll : AttachToDriver failed", NULL);
			return GE_FALSE;
		}

		MembersAttached ++;
	}

	//showPopTSC("BitmapList_AttachAll");

	assert( MembersAttached == pList->Members );

	return GE_TRUE;
}

//================================================================================
//	BitmapList_DetachAll
//================================================================================
geBoolean BitmapList_DetachAll(BitmapList *pList)
{
HashNode	*pNode;
geBoolean	Ret = GE_TRUE;
int MembersAttached;

	assert(BitmapList_IsValid(pList));

	pNode = NULL;
	while( (pNode = Hash_WalkNext(pList->Hash,pNode)) != NULL )
	{
	geBitmap *Bmp;
	uint32 TimesAdded;

		HashNode_GetData(pNode,(uint32 *)&Bmp,&TimesAdded);

		if (!geBitmap_DetachDriver(Bmp, GE_TRUE))
			Ret = GE_FALSE;
	}

	MembersAttached = 0;

	return Ret;
}

//================================================================================
//	BitmapList_CountMembers
//================================================================================
int BitmapList_CountMembers(BitmapList *pList)
{
#ifdef NDEBUG
	return pList->Members;
#else
HashNode *pNode;
int Count;

	Count = 0;
	pNode = NULL;
	while( (pNode = Hash_WalkNext(pList->Hash,pNode)) != NULL )
	{
		Count ++;
	}

	assert( Count == pList->Members );
	assert( pList->Adds >= pList->Members );

return Count;
#endif
}
int BitmapList_CountMembersAttached(BitmapList *pList)
{
HashNode *pNode;
int Count;

	Count = 0;
	pNode = NULL;
	while( (pNode = Hash_WalkNext(pList->Hash,pNode)) != NULL )
	{
	geBitmap *Bmp;
	uint32 TimesAdded;

		HashNode_GetData(pNode,(uint32 *)&Bmp,&TimesAdded);

		if ( geBitmap_GetTHandle(Bmp) )
			Count ++;
	}

	assert( pList->Adds >= pList->Members && pList->Members >= Count );

return Count;
}

//================================================================================
//	BitmapList_Has
//================================================================================
geBoolean BitmapList_Has(BitmapList *pList, geBitmap *Bitmap)
{
HashNode *pNode;
uint32 TimesAdded;

	assert(pList && Bitmap);

	pNode = Hash_Get(pList->Hash,(uint32)Bitmap,&TimesAdded);

	assert( pList->Adds >= (int)TimesAdded );

return (pNode && TimesAdded) ? GE_TRUE : GE_FALSE;
}

//================================================================================
//	BitmapList_Add
//================================================================================
geBoolean BitmapList_Add(BitmapList *pList, geBitmap *Bitmap)
{	
HashNode *pNode;
uint32 TimesAdded;

	assert(BitmapList_IsValid(pList));
	assert(Bitmap);

	// Increase reference count on this Bitmap
	geBitmap_CreateRef(Bitmap);

	pList->Adds ++;

	if ( (pNode = Hash_Get(pList->Hash, (uint32)Bitmap, &TimesAdded)) != NULL )
	{
		HashNode_SetData(pNode,TimesAdded+1);
		return GE_FALSE;
	}
	else
	{
		pList->Members ++;
		Hash_Add(pList->Hash,(uint32)Bitmap,1);
		return GE_TRUE;
	}
}

//================================================================================
//	BitmapList_Remove
//================================================================================
geBoolean BitmapList_Remove(BitmapList *pList,geBitmap *Bitmap)
{
HashNode *pNode;
uint32 TimesAdded;
uint32 Key;

	assert(BitmapList_IsValid(pList));
	assert(Bitmap);

	Key = (uint32) Bitmap;
	pNode = Hash_Get(pList->Hash,Key,&TimesAdded);

	assert(pNode);

	pList->Adds --;
	TimesAdded --;

	if ( TimesAdded <= 0 )
	{
		if ( ! geBitmap_DetachDriver(Bitmap, GE_TRUE) )
		{
			geErrorLog_AddString(-1, "BitmapList_Remove:  geBitmap_DetachDriver failed.", NULL);
			return GE_FALSE;
		}
	}

	geBitmap_Destroy(&Bitmap);

	if ( TimesAdded <= 0 )
	{
		pList->Members --;
		Hash_DeleteNode(pList->Hash,pNode);
		return GE_TRUE;
	}
	else
	{
		HashNode_SetData(pNode,TimesAdded);
		return GE_FALSE;
	}
}


geBoolean BitmapList_IsValid(BitmapList *pList)
{
	if ( ! pList ) 
		return GE_FALSE;
		
	if ( pList->Adds < pList->Members )
		return GE_FALSE;

#ifdef _DEBUG
	if ( pList->MySelf != pList )
		return GE_FALSE;
#endif

	if ( pList->Members != BitmapList_CountMembers(pList) )
		return GE_FALSE;

return GE_TRUE;
}
