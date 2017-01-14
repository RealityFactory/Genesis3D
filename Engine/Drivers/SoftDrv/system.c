/****************************************************************************************/
/*  system.c                                                                            */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  misc init stuff                                                       */
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
#include <Windows.h>
#include <stdio.h>

//#define USE_CACHE

#include "System.h"
#include "SoftDrv.h"
#include "DCommon.h"
#include "Render.h"
#include "Sal.h"
#include "dmodes.h"


BOOL SysInit(void)
{
	if(bWindowed)
	{
		SAL_get_pixel_format(&ClientWindow.PixelPitch,
							&ClientWindow.BytesPerPixel,
							&ClientWindow.R_shift,
							&ClientWindow.R_mask,
							&ClientWindow.R_width,
							&ClientWindow.G_shift,
							&ClientWindow.G_mask,
							&ClientWindow.G_width,
							&ClientWindow.B_shift,
							&ClientWindow.B_mask,
							&ClientWindow.B_width);
	}
	else
	{
		GetDDrawPixelFormat(&ClientWindow);
	}

	if (!RenderInit(&ClientWindow))
		return FALSE;
	
	return TRUE;
}

BOOL SysShutdown(void)
{
	RenderShutdown();

	return TRUE;
}