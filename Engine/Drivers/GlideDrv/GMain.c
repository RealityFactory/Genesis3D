/****************************************************************************************/
/*  GMain.c                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Glide initialization code, etc                                         */
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
#include <StdIO.h>
#include <Assert.h>

#include "GlideDrv.h"
#include "GMain.h"
#include "GCache.h"
#include "Glide.h"
#include "GTHandle.h"

int WriteBMP(unsigned short *ScreenBuffer, const char *Name);

GrChipID_t			TMU[3];					// TMU map number table

DRV_Window			ClientWindow;

static RECT			OldWindow;

GMain_BoardInfo		g_BoardInfo;			// Global board info for current hardware

geBoolean			g_FogEnable = GE_TRUE;
float				g_FogR;
float				g_FogG;
float				g_FogB;

//==============================================================================
//==============================================================================
geBoolean GMain_Startup(DRV_DriverHook *Hook)
{
 
	int32			VidMode;

	//SetEnvironmentVariable("SST_GAMMA", "1.0");

	switch(Hook->Mode)
	{
		case 0:
		{
			ClientWindow.Width = 512;
			ClientWindow.Height = 384;
			VidMode = GR_RESOLUTION_512x384;
			break;
		}
		case 1:
		{
			ClientWindow.Width = 640;
			ClientWindow.Height = 480;
			VidMode = GR_RESOLUTION_640x480;
			break;
		}
		case 2:
		{
			ClientWindow.Width = 800;
			ClientWindow.Height = 600;
			VidMode = GR_RESOLUTION_800x600;
			break;
		}
		case 3:
		{
			ClientWindow.Width = 1024;
			ClientWindow.Height = 768;
			VidMode = GR_RESOLUTION_1024x768;
			break;
		}
		default:
		{
			SetLastDrvError(DRV_ERROR_NULL_WINDOW, "GLIDE_DrvInit:  Invalid display mode.");
			return FALSE;
		}
	}

	ClientWindow.hWnd = Hook->hWnd;

	// Go full-screen so we won't lose the mouse
	{
		RECT	DeskTop;

		// Save the old window size
		GetWindowRect(ClientWindow.hWnd, &OldWindow);

		// Get the size of the desktop
		GetWindowRect(GetDesktopWindow(), &DeskTop);

		// Resize the window to the size of the desktop
		MoveWindow(ClientWindow.hWnd, DeskTop.left-4, DeskTop.top-40, DeskTop.right+20, DeskTop.bottom+20, TRUE);

		// Center the mouse
		SetCursorPos(ClientWindow.Width / 2, ClientWindow.Height / 2);
	}

	// initialize the Glide library 
	grGlideInit();

	// Get the info about this board
	if (!GMain_GetBoardInfo(&g_BoardInfo))
		return FALSE;

	if (g_BoardInfo.NumTMU <= 0)
	{
		SetLastDrvError(DRV_ERROR_INIT_ERROR, "GLIDE_DrvInit:  Not enough texture mapping units.");
		return GE_FALSE;
	}

	// select the current graphics system 
	grSstSelect(0);

	// initialize and open the graphics system 
	if (!grSstWinOpen( (U32)ClientWindow.hWnd,
              VidMode,
              GR_REFRESH_60Hz,
              GR_COLORFORMAT_ABGR,
              GR_ORIGIN_UPPER_LEFT,
              2, 1 ))
	{
		SetLastDrvError(DRV_ERROR_INIT_ERROR, "GLIDE_DrvInit:  grSstWinOpen failed.");
		return GE_FALSE;
	}

	// We know that GLIDE will be in 5-6-5 mode...
	ClientWindow.R_shift = 5+6;
	ClientWindow.G_shift = 5;
	ClientWindow.B_shift = 0;

	ClientWindow.R_mask = 0xf800;
	ClientWindow.G_mask = 0x07e0;
	ClientWindow.B_mask = 0x001f;

	ClientWindow.R_width = 5;
	ClientWindow.G_width = 6;
	ClientWindow.B_width = 5;

	SetLastDrvError(DRV_ERROR_NONE, "GMain_Startup:  No error.");

	if (!GTHandle_Startup())
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GMain_Startup:  GTHandle_Startup failed...\n");
		return GE_FALSE;
	}
	
	if (!GMain_InitGlideRegisters())
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GMain_Startup:  GMain_InitGlideRegisters failed...\n");
		return GE_FALSE;
	}

	// Init the 3d display
	//grSstControl(GR_CONTROL_ACTIVATE);
	grGammaCorrectionValue(1.0f);

	return GE_TRUE;
}

//==================================================================================
//	GMain_Shutdown
//==================================================================================
void GMain_Shutdown(void)
{
	GTHandle_Shutdown();

	// Resize the window to the size of the original size
	MoveWindow(ClientWindow.hWnd, OldWindow.left, OldWindow.top, OldWindow.right, OldWindow.bottom, TRUE);

	grGlideShutdown();
}

//==================================================================================
//	GMain_GetBoardInfo
//	Glide is assumed to be initialized before this function is called...
//==================================================================================
geBoolean GMain_GetBoardInfo(GMain_BoardInfo *Info)
{
	GrHwConfiguration	GlideHwConfig;

	// detect the Voodoo Graphics hardware in the host system
	if (!grSstQueryHardware(&GlideHwConfig))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GMain_GetBoardInfo:  grSstQueryHardware failed.");
		return GE_FALSE;
	}

	memset(Info, 0, sizeof(*Info));

	switch (GlideHwConfig.SSTs[0].type)
	{
		case GR_SSTTYPE_VOODOO:

			Info->MainRam = GlideHwConfig.SSTs[0].sstBoard.VoodooConfig.fbRam;
			Info->NumTMU = GlideHwConfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;
			break;

		case GR_SSTTYPE_SST96:
			Info->MainRam = GlideHwConfig.SSTs[0].sstBoard.SST96Config.fbRam;
			Info->NumTMU = GlideHwConfig.SSTs[0].sstBoard.SST96Config.nTexelfx;
			break;

		case GR_SSTTYPE_AT3D:
			SetLastDrvError(DRV_ERROR_GENERIC, "GMain_GetBoardInfo:  GR_SSTTYPE_AT3D not supported.");
			return GE_FALSE;

		case GR_SSTTYPE_Voodoo2:
			Info->MainRam = GlideHwConfig.SSTs[0].sstBoard.Voodoo2Config.fbRam;
			Info->NumTMU = GlideHwConfig.SSTs[0].sstBoard.Voodoo2Config.nTexelfx;
			break;
	}

#if 0
	Info->NumTMU = 1;
#endif

	return GE_TRUE;
}

static GrFog_t FogTable[GR_FOG_TABLE_SIZE];

//==================================================================================
//	GMain_InitGlideRegisters
//==================================================================================
geBoolean GMain_InitGlideRegisters(void)
{
	//
	// Setup card register states
	//

	// fix up the z-buffer
	grDepthBufferMode( GR_DEPTHBUFFER_ZBUFFER );
	grDepthBufferFunction( GR_CMP_GEQUAL );
	grDepthMask( FXTRUE );
	
	// Fixup the transparent color
	grChromakeyMode(GR_CHROMAKEY_DISABLE);			// Off by default, on for decals though...
	grChromakeyValue(1);

	// Fixup the Mipmapping 
	//	TMU
	//	Bias			-32...+31
	//	Detail scale	0...7
	//	Detail Max		0...1.0
	grTexDetailControl(TMU[0], 0, 1, 1.0f);

	// -8... 7.75
	grTexLodBiasValue(TMU[0], -1.0f);
	//grTexLodBiasValue(TMU[0], 7.0f);

	// Tell it how we like it...
	guColorCombineFunction( GR_COLORCOMBINE_DECAL_TEXTURE );

	guTexCombineFunction( TMU[0], GR_TEXTURECOMBINE_DECAL );
	
	//rTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);
	//grTexMipMapMode( TMU[0], GR_MIPMAP_DISABLE, FXFALSE );

	//grTexFilterMode( TMU[0], GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_BILINEAR);
	//grTexFilterMode( TMU[0], GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_POINT_SAMPLED);
	grTexFilterMode( TMU[0], GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	grTexClampMode( TMU[0], GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);

    grTexCombine( TMU[0],
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  FXFALSE, FXFALSE );
	
	
	// turn on gouraud shading
	grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
					GR_COMBINE_FACTOR_LOCAL,
					GR_COMBINE_LOCAL_ITERATED,
					GR_COMBINE_OTHER_TEXTURE,
					FXFALSE );
	
	
	grAlphaCombine( GR_COMBINE_FUNCTION_BLEND_OTHER,
					GR_COMBINE_FACTOR_LOCAL_ALPHA,
					GR_COMBINE_LOCAL_ITERATED,
					GR_COMBINE_OTHER_TEXTURE,
					FXFALSE );
	
	/*
	grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL,
                    GR_COMBINE_FACTOR_LOCAL,
                    GR_COMBINE_LOCAL_ITERATED,
                    GR_COMBINE_OTHER_TEXTURE,
                    FXFALSE );
	*/

	//grAlphaControlsITRGBLighting(FXTRUE);
	//grGlideShamelessPlug(FXTRUE);

	if (g_BoardInfo.NumTMU >= 2)
	{
		grTexDetailControl(TMU[1], 0, 1, 0.3f);
		grTexLodBiasValue(TMU[1], 0.3f);

		guTexCombineFunction( TMU[1], GR_TEXTURECOMBINE_DECAL );
	
		grTexFilterMode( TMU[1], GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
		grTexClampMode( TMU[1], GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);

		grTexCombine(	TMU[1], 
						GR_COMBINE_FUNCTION_SCALE_OTHER, 
						GR_COMBINE_FACTOR_LOCAL,
						GR_COMBINE_FUNCTION_SCALE_OTHER, 
						GR_COMBINE_FACTOR_LOCAL,
						FXFALSE, FXFALSE ); 

		// Always texture clamp on second TMU (it only uses lightmaps for now)
		grTexClampMode(TMU[1], GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);

		grHints(GR_HINT_STWHINT, GR_STWHINT_ST_DIFF_TMU0 | GR_STWHINT_ST_DIFF_TMU1);
	}

	//GMain_SetFogEnable(GE_TRUE, 0.0f, 255.0f, 0.0f, 500.0f, 1500.0f);
	GMain_SetFogEnable(GE_FALSE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

	return GE_TRUE;
}

extern uint32				CurrentLRU;		// Shame!!!

//==================================================================================
//	Reset the 3dfx, and free all allocated ram
//==================================================================================
geBoolean GMain_ResetAll(void)
{
	GTHandle_Shutdown();

	if (!GTHandle_Startup())
		return GE_FALSE;
	
	if (!GMain_InitGlideRegisters())
		return GE_FALSE;

	CurrentLRU = 0;

	return GE_TRUE;
}	

//==================================================================================
//==================================================================================
geBoolean DRIVERCC GMain_ScreenShot(const char *Name)
{
	uint16		*Buffer;
	
	Buffer = (uint16*)malloc(sizeof(uint16*)*640*480);
	
	if (!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,640*2, (void*)Buffer))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE: Could not save BMP.");
		return FALSE;
	}
	
	WriteBMP(Buffer, Name);

	free(Buffer);

	return GE_TRUE;
}

//==================================================================================
//	GMain_SetFogEnable
//==================================================================================
geBoolean DRIVERCC GMain_SetFogEnable(geBoolean Enable, float r, float g, float b, float Start, float End)
{
	g_FogEnable = Enable;
	g_FogR = r;
	g_FogG = g;
	g_FogB = b;

	if (g_FogEnable)
	{
		grFogMode(GR_FOG_WITH_TABLE); 
		grFogColorValue(((uint32)b<<16)|((uint32)g<<8)|(uint32)r);
		guFogGenerateLinear(FogTable, Start, End); 
		grFogTable(FogTable);
	}
	else
	{
		grFogMode(GR_FOG_DISABLE);
	}

	return GE_TRUE;
}
