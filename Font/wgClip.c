/****************************************************************************************/
/*  WGCLIP.C                                                                            */
/*                                                                                      */
/*  Author: Thom Robertson                                                              */
/*  Description: 2D rectangular clip testing support                                    */
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
#define	WIN32_LEAN_AND_MEAN
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#include <windowsx.h>
#pragma warning(default : 4201 4214 4115)

#include <assert.h>
#include <string.h>

#include "genesis.h"
#include "basetype.h"
#include "extbox.h"

#include "wgClip.h"

#pragma warning (disable:4514)	// unreferenced inline function (caused by Windows)

//***************************************************************
// returns true if you need to draw at all.
GENESISAPI geBoolean GENESISCC CalculateClipping(
                           GE_Rect *artRect, int32 *resultX, int32 *resultY, 
                           int32 x, int32 y,
                           const GE_Rect bounds, int32 type)
{

	int32 localX, localY;
	int32 shiftX, shiftY;
	int32 baseX,  baseY;

   assert(artRect);
   assert(resultX);
   assert(resultY);

   baseX = artRect->Left;
   baseY = artRect->Top;

   // normalize the rect passed in.
   artRect->Left   = artRect->Left   - baseX;
   artRect->Right  = artRect->Right  - baseX;
   artRect->Top    = artRect->Top    - baseY;
   artRect->Bottom = artRect->Bottom - baseY;


	localX = x;
	localY = y;

	if (GE_CLIP_CENTER == type)
	{
		shiftX = artRect->Right  / 2;
		shiftY = artRect->Bottom / 2;
	}
	else
	{
		shiftX = 0;                                                     
		shiftY = 0;
	}
	
	if (artRect->Right + localX - shiftX > bounds.Right)
	{
		artRect->Right 	-= (artRect->Right + localX - shiftX) - bounds.Right;  // push Right edge Leftward
	}

	if (artRect->Bottom + localY - shiftY > bounds.Bottom)
	{
		artRect->Bottom 	-= (artRect->Bottom + localY - shiftY) - bounds.Bottom;  // push Bottom edge Leftward
	}

	if (artRect->Left + localX - shiftX < bounds.Left)
	{
		localX 			+= bounds.Left - (artRect->Left + localX - shiftX);
		artRect->Left 	+= localX - x;  // push Left edge Rightward
	}

	if (artRect->Top + localY - shiftY < bounds.Top)
	{
		localY 			+= bounds.Top - (artRect->Top + localY - shiftY);
		artRect->Top 	+= localY - y;
	}

   if (artRect->Left >= artRect->Right)
		return GE_FALSE;
	if (artRect->Top >= artRect->Bottom)
		return GE_FALSE;

	*resultX = localX - shiftX;
	*resultY = localY - shiftY;

   // un-normalize the rect passed in.
   artRect->Left   = artRect->Left   + baseX;
   artRect->Right  = artRect->Right  + baseX;
   artRect->Top    = artRect->Top    + baseY;
   artRect->Bottom = artRect->Bottom + baseY;


	return GE_TRUE;
}

