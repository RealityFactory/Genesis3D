/****************************************************************************************/
/*  Scene.cpp                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Begin/EndScene code, etc                                               */
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

#include "D3DDrv.h"
#include "DCommon.h"
#include "Scene.h"
#include "Render.h"
#include "GSpan.h"
#include "D3DCache.h"
#include "D3D_Fx.h"
#include "D3D_Main.h"
#include "PCache.h"
#include "D3D_Err.h"
#include "THandle.h"

//#define D3D_MANAGE_TEXTURES
#define SUPER_FLUSH

int32 RenderMode;

uint32 CurrentLRU;

BOOL DRIVERCC BeginScene(BOOL Clear, BOOL ClearZ, RECT *WorldRect)
{
	HRESULT	Result;

	CurrentLRU++;

	if (!AppInfo.lpD3DDevice)
	{
		D3DMain_Log("BeginScene:  No D3D Device!.");
		return FALSE;
	}

	// Make sure we clear the cache info structure...
	memset(&CacheInfo, 0, sizeof(DRV_CacheInfo));

	if (!THandle_CheckCache())
		return GE_FALSE;

	//	Watch for inactive app or minimize
	if(AppInfo.RenderingIsOK)
	{
		if (!Main_ClearBackBuffer(Clear, ClearZ))
		{
			D3DMain_Log("D3DClearBuffers failed.");
			return FALSE;
		}
		
		D3DDRV.NumRenderedPolys = 0;
		
		Result = AppInfo.lpD3DDevice->BeginScene();

		if (Result != D3D_OK)
		{
			D3DMain_Log("BeginScene:  D3D BeginScene Failed.\n%s.", D3DErrorToString(Result));
			return FALSE;
		}

		D3DBilinearFilter(D3DFILTER_LINEAR, D3DFILTER_LINEAR);
		D3DPolygonMode (D3DFILL_SOLID);
		
		D3DZWriteEnable (TRUE);
		D3DZEnable(TRUE);
		D3DZFunc(D3DCMP_LESSEQUAL);

		D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		D3DBlendEnable(TRUE);

		if (AppInfo.FogEnable)
		{
			AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE , TRUE);
			AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR , ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB);
		}
		else
		{
			AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE , FALSE);
		}
	}

	return TRUE;
}

BOOL DRIVERCC EndScene(void)
{
	HRESULT		Result;

	if (!AppInfo.lpD3DDevice)
		return FALSE;

	if(AppInfo.RenderingIsOK)
	{
		// Flush everything one last time...
		PCache_FlushWorldPolys();
		PCache_FlushMiscPolys();

		Result = AppInfo.lpD3DDevice->EndScene();

		if (Result != D3D_OK)
		{
			D3DMain_Log("EndScene:  D3D EndScene Failed.\n%s", D3DErrorToString(Result));
			return FALSE;
		}

		if (!Main_ShowBackBuffer())
			return FALSE;
	}
	return TRUE;
}

BOOL DRIVERCC BeginWorld(void)
{
	#ifdef USE_SPANS
		if(AppInfo.RenderingIsOK)
		{
			ResetSpans(ClientWindow.Height);
		}
	#endif

	RenderMode = RENDER_WORLD;
	return TRUE;
}

BOOL DRIVERCC EndWorld(void)
{
	RenderMode = RENDER_NONE;

	if(AppInfo.RenderingIsOK)
	{
		PCache_FlushWorldPolys();
		PCache_FlushMiscPolys();

		assert(AppInfo.lpD3DDevice);
	#ifdef SUPER_FLUSH
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);
		AppInfo.lpD3DDevice->EndScene();
		AppInfo.lpD3DDevice->BeginScene();
	#endif

	}
	return TRUE;
}

BOOL GlobalMeshHack;

BOOL DRIVERCC BeginMeshes(void)
{
	GlobalMeshHack = TRUE;
	RenderMode = RENDER_MESHES;
	return TRUE;
}

BOOL DRIVERCC EndMeshes(void)
{
	RenderMode = RENDER_NONE;

	if(AppInfo.RenderingIsOK)
	{
		PCache_FlushMiscPolys();
		PCache_FlushWorldPolys();

	#ifdef SUPER_FLUSH
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);
		AppInfo.lpD3DDevice->EndScene();
		AppInfo.lpD3DDevice->BeginScene();
	#endif

		GlobalMeshHack = FALSE;
	}
	return TRUE;
}

BOOL DRIVERCC BeginModels(void)
{
	RenderMode = RENDER_MODELS;
	return TRUE;
}

BOOL DRIVERCC EndModels(void)
{
	RenderMode = RENDER_NONE;

	if(AppInfo.RenderingIsOK)
	{
		PCache_FlushWorldPolys();
		PCache_FlushMiscPolys();

	#ifdef SUPER_FLUSH
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);
		AppInfo.lpD3DDevice->EndScene();
		AppInfo.lpD3DDevice->BeginScene();
	#endif

	}
	return TRUE;
}
