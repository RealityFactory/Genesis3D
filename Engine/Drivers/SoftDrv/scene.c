/****************************************************************************************/
/*  scene.c                                                                             */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  Init and state related code                                           */
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
#include <assert.h>

#include "SoftDrv.h"
#include "DCommon.h"
#include "Render.h"
#include "Scene.h"
#include "Span.h"
#include "System.h"
#include "Sal.h"
#include "dmodes.h"

S32	RenderMode = RENDER_NONE;

VidModeList *cmode=NULL;

//global rect!!!!!  Will be dangerous future
RECT	WorldRect;
RECT	FrontRect;
U16		FPUCW, OldFPUCW;

extern 		CPUInfo			ProcessorInfo;

void SetFPU24(void)
{
	_asm
	{
		fstcw	[OldFPUCW]		; store copy of CW
		mov		ax, OldFPUCW	; get it in ax
		and		eax,0xFFFFFCFF
//		or		eax,0x0000600
		mov		[FPUCW],ax		; store it
		fldcw	[FPUCW]			; load the FPU
	}
}

void RestoreFPU(void)
{
	_asm	fldcw	[OldFPUCW]	; restore the FPU
}

BOOL DRIVERCC BeginScene(BOOL Clear, BOOL ClearZ, RECT *pWorldRect)
{
	if(ProcessorInfo.Has3DNow)
	{
		SetFPU24();
	}

	if (RenderMode != RENDER_NONE)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "Still in some other render mode.");
		return FALSE;
	}

	if(0 && pWorldRect)	//offsets still relative to the top of the screen
	{
		WorldRect	=*pWorldRect;
		WorldRect.right;
		WorldRect.bottom;
	}
	else	//use full screen
	{
		WorldRect.left	=WorldRect.top		=0;
		WorldRect.right	=ClientWindow.Width - 1;
		WorldRect.bottom=ClientWindow.Height - 1;
	}

	if(!bWindowed)
	{
		if(cmode->flags & STRETCHMODE)
		{
			float	xstrch, ystrch;

			xstrch			=640.0f / ((float)ClientWindow.Width);
			ystrch			=480.0f / ((float)ClientWindow.Height);
			FrontRect.left	=(int)(xstrch * (float)WorldRect.left);
			FrontRect.right	=(int)(xstrch * (float)WorldRect.right);
			FrontRect.bottom=(int)(ystrch * (float)WorldRect.bottom);
			FrontRect.top	=(int)(ystrch * (float)WorldRect.top);
		}
	}

	NumPixels = 0;

	if(ClearZ)
	{
		ClearZBuffer(&ClientWindow);
	}
	if(Clear)
	{
		if(bWindowed)
		{
			SAL_wipe_surface(SAL_BACK_SURFACE, 0);
		}
		else
		{
			ClearBackBuffer(&ClientWindow);
		}
	}

	if(bWindowed)
	{
		SAL_lock_surface(SAL_BACK_SURFACE, &ClientWindow.Buffer, &ClientWindow.PixelPitch);
	}
	else
	{
		if(!LockDDrawBackBuffer(&ClientWindow, &WorldRect))
		{
			SetLastDrvError(DRV_ERROR_INVALID_PARMS, "World rect invalid.");
			DumpErrorLogToFile("softdrv.log");
			return	FALSE;
		}
	}
	assert(ClientWindow.Buffer);

	return TRUE;
}

BOOL DRIVERCC EndScene(void)
{
	if (RenderMode != RENDER_NONE)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "Still in some other render mode.");
		return FALSE;
	}
	if(bWindowed)
	{
		SAL_release_surface(SAL_BACK_SURFACE, FALSE);
//		SAL_flip_surface();
		SAL_blit_surface();
		SAL_serve_message_queue();
	}
	else
	{
		UnlockDDrawBackBuffer(&ClientWindow, &WorldRect);
		RefreshDDraw(&ClientWindow, cmode, &WorldRect, &FrontRect);
	}

//	SOFTDRV.NumWorldSpans = RegPixels;
//	SOFTDRV.NumRenderedPolys = RGBPixels;

	if(ProcessorInfo.Has3DNow)
	{
		RestoreFPU();
	}

	return TRUE;
}

static	U32	OldFlags	=0;

BOOL DRIVERCC BeginWorld(void)
{
	if (RenderMode != RENDER_NONE && RenderMode != RENDER_WORLD)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "Still in some other render mode.");
		return FALSE;
	}

	ResetSpans(ClientWindow.Height);
	
 	RenderMode = RENDER_WORLD;

//	if(!VirtualProtect((U8 *)ZBuffer,
//		(ClientWindow.Width * ClientWindow.Height)*2,
//		PAGE_READWRITE | PAGE_NOCACHE,
//		&OldFlags))
//	{
//		ErrorPrintf("Failed to set zbuffer page uncacheable\n");
//	}

	return TRUE;
}

BOOL DRIVERCC EndWorld(void)
{
	RenderMode = RENDER_NONE;

//	if(!VirtualProtect((U8 *)ZBuffer,
//		(ClientWindow.Width * ClientWindow.Height)*2,
//		OldFlags,	&OldFlags))
//	{
//		ErrorPrintf("Failed to set zbuffer page cacheable\n");
//	}
	return TRUE;
}

BOOL DRIVERCC BeginMeshes(void)
{
	if (RenderMode != RENDER_NONE)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "Still in some other render mode.");
		return FALSE;
	}

	ResetSpans(ClientWindow.Height);
	NumPixels = 0;
	RenderMode = RENDER_MESHES;

	return TRUE;
}

BOOL DRIVERCC EndMeshes(void)
{
	RenderMode = RENDER_NONE;

	return TRUE;
}

BOOL DRIVERCC BeginModels(void)
{
	if (RenderMode != RENDER_NONE)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "Still in some other render mode.");
		return FALSE;
	}
	ResetSpans(ClientWindow.Height);
	NumPixels = 0;

	RenderMode = RENDER_MODELS;

	return TRUE;
}

BOOL DRIVERCC EndModels(void)
{
	RenderMode = RENDER_NONE;
	return TRUE;
}
