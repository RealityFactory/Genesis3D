/****************************************************************************************/
/*  BitmapList.h                                                                        */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef BITMAPLIST_H
#define BITMAPLIST_H

#include "GeTypes.h"
#include "DCommon.h"
#include "Bitmap.h"

typedef struct BitmapList		BitmapList;

#ifdef __cplusplus
extern "C" {
#endif

BitmapList *BitmapList_Create(void);
geBoolean BitmapList_Destroy(BitmapList *pList);

geBoolean BitmapList_SetGamma(BitmapList *pList, geFloat Gamma);

geBoolean BitmapList_AttachAll(BitmapList *pList, DRV_Driver *Drivera, geFloat Gamma);
geBoolean BitmapList_DetachAll(BitmapList *pList);

	// _Add & _Remove do NOT return Ok/NOk	
geBoolean BitmapList_Add(BitmapList *pList, geBitmap *Bitmap);	// returns Was It New ?
geBoolean BitmapList_Remove(BitmapList *pList,geBitmap *Bitmap);// returns Was It Removed ?
	// _Add & _Remove also do not do any Attach or Detach

geBoolean BitmapList_Has(BitmapList *pList, geBitmap *Bitmap);

#ifndef NDEBUG
int			BitmapList_CountMembers(BitmapList *pList);
int			BitmapList_CountMembersAttached(BitmapList *pList);
#endif

#ifdef __cplusplus
}
#endif
#endif
