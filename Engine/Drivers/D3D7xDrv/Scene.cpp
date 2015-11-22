/****************************************************************************************/
/*  Scene.cpp                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Begin/EndScene code, etc                                               */
/*                                                                                      */
/*  Edit History:                                                                       */
/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
/*  02/28/2001 Wendell Buckner
/*   These render states are unsupported d3d 7.0
/*  07/16/2000 Wendell Buckner
/*   Convert to Directx7...    */
/*
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

/*  07/16/2000 Wendell Buckner
/*   Convert to Directx7...    
#include "D3DDrv.h"             */
#include "D3DDrv7x.h"

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

/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
BOOL InScene = FALSE;

__inline DWORD F2DW(float f)
{
   DWORD            retval = 0;

   _asm {
      fld            f
      lea            eax, [retval]
      fistp         dword ptr[eax]
   }

   return retval;
}

// changed QD Shadows
//BOOL DRIVERCC BeginScene(BOOL Clear, BOOL ClearZ, RECT *WorldRect)
BOOL DRIVERCC BeginScene(BOOL Clear, BOOL ClearZ, BOOL ClearStencil, RECT *WorldRect)
// end change
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
// changed QD Shadows
//		if (!Main_ClearBackBuffer(Clear, ClearZ))
		if (!Main_ClearBackBuffer(Clear, ClearZ, ClearStencil))

		{
			D3DMain_Log("D3DClearBuffers failed.");
			return FALSE;
		}

		D3DDRV.StencilTestMode = 1; // zpass=1, zfail=0
// end change		
		D3DDRV.NumRenderedPolys = 0;
		
		Result = AppInfo.lpD3DDevice->BeginScene();
//      D3DMain_Log("Begin Scene\n");

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
//	changed QD shadows
		D3DStencilEnable(FALSE);
		D3DStencilFunc(D3DCMP_ALWAYS);
// end change

		D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		D3DBlendEnable(TRUE);

/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. 
		if (AppInfo.FogEnable)
		{
			AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE , TRUE);
			AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR , ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB);
		}
		else
		{
			AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE , FALSE);
		} */
		if (AppInfo.FogEnable)
		{
			D3DFogEnable ( AppInfo.FogEnable, (F2DW(AppInfo.FogR)<<16)|(F2DW(AppInfo.FogG)<<8)|F2DW(AppInfo.FogB) );
		}
		else
		{
			D3DFogEnable ( AppInfo.FogEnable, (F2DW(AppInfo.ClearR)<<16)|(F2DW(AppInfo.ClearG)<<8)|F2DW(AppInfo.ClearB) );
		} 

        
	}

/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
    InScene = TRUE;
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
//      D3DMain_Log("End Scene\n");

		if (Result != D3D_OK)
		{
			D3DMain_Log("EndScene:  D3D EndScene Failed.\n%s", D3DErrorToString(Result));
			return FALSE;
		}

/* 01/28/2003 Wendell Buckner
     Cache decals so that they can be drawn after all the 3d stuff... */
		DCache_FlushDecalRects();

		if (!Main_ShowBackBuffer())
			return FALSE;
	}

/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
    InScene = FALSE;
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
/* 02/28/2001 Wendell Buckner
   These render states are unsupported d3d 7.0
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);*/
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
/* 02/28/2001 Wendell Buckner
   These render states are unsupported d3d 7.0
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);*/
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
/* 02/28/2001 Wendell Buckner
   These render states are unsupported d3d 7.0
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);*/
		AppInfo.lpD3DDevice->EndScene();
		AppInfo.lpD3DDevice->BeginScene();
	#endif

	}
	return TRUE;
}

// changed QD Shadows
BOOL DRIVERCC BeginShadowVolumes(void)
{
	RenderMode = RENDER_SHADOWVOLUMES;
	D3DSetTexture(0, NULL);
	D3DFogEnable (FALSE, 0 );
	return TRUE;
}

BOOL DRIVERCC EndShadowVolumes(void)
{
	RenderMode = RENDER_NONE;

	if(AppInfo.RenderingIsOK)
	{
		//PCache_FlushWorldPolys();
		//PCache_FlushMiscPolys();
		//PCache_FlushStencilPolys();
		//D3DDrawShadow(0.0f, 0.0f, 0.0f, 94.0f);
	#ifdef SUPER_FLUSH
		AppInfo.lpD3DDevice->EndScene();
		AppInfo.lpD3DDevice->BeginScene();
	#endif

	}
	return TRUE;
}
// end change