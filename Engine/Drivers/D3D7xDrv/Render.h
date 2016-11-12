/****************************************************************************************/
/*  Render.h                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Code to render polys under D3D                                         */
/*                                                                                      */
/*  Edit History:                                                                       */
/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
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
#ifndef RENDER_H
#define RENDER_H

#include <Windows.h>

#include "DCommon.h"

geBoolean DRIVERCC RenderGouraudPoly(DRV_TLVertex *Pnts, int32 NumPoints, uint32 Flags);
geBoolean DRIVERCC RenderWorldPoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags);
geBoolean DRIVERCC RenderMiscTexturePoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, uint32 Flags);
// changed QD Shadows
geBoolean DRIVERCC RenderStencilPoly(DRV_XYZVertex *Pnts, int32 NumPoints, uint32 Flags);
geBoolean DRIVERCC DrawShadowPoly(geFloat r, geFloat g, geFloat b, geFloat a);
// end change
geBoolean DRIVERCC DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y);

/* 01/28/2003 Wendell Buckner
     Cache decals so that they can be drawn after all the 3d stuff... */
geBoolean DRIVERCC DrawDecalRect(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y);
#endif
