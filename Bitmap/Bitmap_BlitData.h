#ifndef BITMAP_BLITDATA_H
#define BITMAP_BLITDATA_H

#ifndef BITMAP_PRIVATE_H
Intentional Error : bitmap_blidata only allowed in bitmap internals!
#endif
/****************************************************************************************/
/*  Bitmap_BlitData.h                                                                   */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  The Bitmap_BlitData function                                          */
/*					Does all format conversions											*/
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


extern geBoolean geBitmap_BlitData(
								const geBitmap_Info * SrcInfo,const void *SrcData,const geBitmap *SrcBmp,
									  geBitmap_Info * DstInfo,		void *DstData,const geBitmap *DstBmp,
								int SizeX,
								int SizeY);

#endif //BITMAP_BLITDATA_H
