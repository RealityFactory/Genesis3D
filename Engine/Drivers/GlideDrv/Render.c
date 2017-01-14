/****************************************************************************************/
/*  Render.c                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Code to render polys in glide                                          */
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
#include <assert.h>
#include <Math.h>

#include "Render.h"
#include "GMain.h"
#include "GlideDrv.h"
#include "GSpan.h"
#include "GTHandle.h"

#define ENABLE_WIREFRAME	

#ifdef ENABLE_WIREFRAME
static int DoWireFrame = 0;
#endif

#define MAX_LMAP_SIZE		32

#define SNAP_VERT(v)  ( ( float )( ( long )( ( v ) * 16 ) ) * 0.0625f /* 1/16 */ )

#define RENDER_MAX_PNTS (64)

typedef enum
{
	ColorCombine_Undefined,
	ColorCombine_Gouraud,
	ColorCombine_Texture,
	ColorCombine_TextureGouraud,
	ColorCombine_TextureGouraudWithFog,
} Render_ColorCombine;

typedef enum
{
	TexCombine_Undefined,
	TexCombine_SinglePassGouraud,
	TexCombine_SinglePassTexture,
	TexCombine_SimultaneousPass,
	TexCombine_PassThrough
} Render_TexCombine;

DRV_RENDER_MODE		RenderMode = RENDER_NONE;
Render_ColorCombine	Render_OldColorCombine = ColorCombine_Undefined;
Render_TexCombine	Render_OldTexCombine = TexCombine_Undefined;
int32				Render_HardwareMode = RENDER_UNKNOWN_MODE;
uint32				Render_HardwareFlags = 0;

uint32				PolyMode = DRV_POLYMODE_NORMAL;

DRV_CacheInfo		CacheInfo;

uint32				CurrentLRU;

extern				g_FogEnable;

static FxU32 LastTextureAddr[2] = {(FxU32)-1, (FxU32)-1};

//==========================================================================================
//	TextureSource
//==========================================================================================
void TextureSource(GrChipID_t Tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo  *info )
{
	if (LastTextureAddr[Tmu] == startAddress)
		return;

	grTexSource(Tmu, startAddress, evenOdd, info);

	LastTextureAddr[Tmu] = startAddress;
}

//==========================================================================================
//	Render_SetColorCombine
//==========================================================================================
void Render_SetColorCombine(Render_ColorCombine ColorCombine)
{
	if (Render_OldColorCombine == ColorCombine)
		return;		// Nothing to change
	
	switch(ColorCombine)
	{
		case ColorCombine_Gouraud:
		{
			guColorCombineFunction( GR_COLORCOMBINE_ITRGB);
			break;
		}
		case ColorCombine_Texture:
		{
			guColorCombineFunction( GR_COLORCOMBINE_DECAL_TEXTURE);
			break;
		}
		case ColorCombine_TextureGouraud:
		{
			guColorCombineFunction( GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
			break;
		}
		case ColorCombine_TextureGouraudWithFog:
		{
			guColorCombineFunction( GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_ADD_ALPHA);
			break;
		}

		default:
			assert(0);
	}
	
	Render_OldColorCombine = ColorCombine;
}

//==========================================================================================
//	Render_SetTexCombine
//==========================================================================================
void Render_SetTexCombine(Render_TexCombine TexCombine)
{
	if (Render_OldTexCombine == TexCombine)
		return;		// Nothing to change
	
	switch(TexCombine)
	{
		case TexCombine_SinglePassGouraud:
		{
			guTexCombineFunction( TMU[0], GR_TEXTURECOMBINE_ONE);
			
			if (g_BoardInfo.NumTMU >= 2)
				guTexCombineFunction( TMU[1], GR_TEXTURECOMBINE_ZERO);
			break;
		}

		case TexCombine_SinglePassTexture:
		{
			guTexCombineFunction( TMU[0], GR_TEXTURECOMBINE_DECAL);
			
			if (g_BoardInfo.NumTMU >= 2)
				guTexCombineFunction( TMU[1], GR_TEXTURECOMBINE_ZERO);
			break;
		}

		case TexCombine_SimultaneousPass:
		{
			assert(g_BoardInfo.NumTMU >= 2);

			guTexCombineFunction( TMU[0], GR_TEXTURECOMBINE_MULTIPLY);
			guTexCombineFunction( TMU[1], GR_TEXTURECOMBINE_DECAL);
		
			break;
		}
	
		case TexCombine_PassThrough:
		{
			assert(g_BoardInfo.NumTMU >= 2);
	
			guTexCombineFunction( TMU[0], GR_TEXTURECOMBINE_OTHER);
			guTexCombineFunction( TMU[1], GR_TEXTURECOMBINE_DECAL);
		
			break;
		}

		default:
			assert(0);
	}

	Render_OldTexCombine = TexCombine;
}

#if 1
//==========================================================================================
//	Render_SetHardwareMode
//==========================================================================================
void Render_SetHardwareMode(int32 NewMode, uint32 NewFlags)
{
	if (NewFlags != Render_HardwareFlags)		// See if the flags gave changed,,,
	{
		if (NewFlags & DRV_RENDER_NO_ZMASK)
			grDepthBufferFunction(GR_CMP_ALWAYS);
		else if (Render_HardwareFlags & DRV_RENDER_NO_ZMASK)
			grDepthBufferFunction( GR_CMP_GEQUAL );	

		if (NewFlags & DRV_RENDER_NO_ZWRITE)
			grDepthMask(FXFALSE);
		else if (Render_HardwareFlags & DRV_RENDER_NO_ZWRITE)
			grDepthMask(FXTRUE);
		
		if (NewFlags & DRV_RENDER_CLAMP_UV)
			grTexClampMode(TMU[0], GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
		else if (Render_HardwareFlags & DRV_RENDER_CLAMP_UV)
			grTexClampMode(TMU[0], GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
	}

	// Make these flags recent
	Render_HardwareFlags = NewFlags;

	if (NewMode == Render_HardwareMode)// && NewFlags == Render_HardwareFlags) 
		return;		// Nothing to change

	if (Render_HardwareMode == RENDER_DECAL_MODE)
		grChromakeyMode(GR_CHROMAKEY_DISABLE);		// Restore chroma key mode

	// sets up hardware mode 
	switch (NewMode)
	{
		case (RENDER_MISC_TEX_POLY_MODE):
		{
			grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);		// Bug fix thanks to Bobtree

			Render_SetColorCombine(ColorCombine_TextureGouraud);
			Render_SetTexCombine(TexCombine_SinglePassTexture);

			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,  GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_SRC_ALPHA,  GR_BLEND_ONE_MINUS_SRC_ALPHA);

			if (g_FogEnable)
				//grFogMode(GR_FOG_WITH_ITERATED_ALPHA); 
				grFogMode(GR_FOG_WITH_TABLE); 

			break;
		}

		case (RENDER_MISC_GOURAD_POLY_MODE):
		{
			Render_SetColorCombine(ColorCombine_Gouraud);
			Render_SetTexCombine(TexCombine_SinglePassGouraud);

			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_ONE, GR_BLEND_ONE);

			if (g_FogEnable)
				//grFogMode(GR_FOG_WITH_ITERATED_ALPHA); 
				grFogMode(GR_FOG_WITH_TABLE); 

			break;
		}

		case (RENDER_LINES_POLY_MODE):
		{	
			Render_SetColorCombine(ColorCombine_Gouraud);
			Render_SetTexCombine(TexCombine_SinglePassGouraud);
			
			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_ONE, GR_BLEND_ONE);

			if (g_FogEnable)
				//grFogMode(GR_FOG_WITH_ITERATED_ALPHA); 
				grFogMode(GR_FOG_WITH_TABLE); 

			break;
		}

		case (RENDER_WORLD_POLY_MODE_NO_LIGHTMAP):
		{
			Render_SetColorCombine(ColorCombine_TextureGouraud);
			Render_SetTexCombine(TexCombine_SinglePassTexture);
			
			grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);

			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
								 GR_BLEND_ONE, GR_BLEND_ZERO);

			if (g_FogEnable)
				//grFogMode(GR_FOG_WITH_ITERATED_ALPHA); 
				grFogMode(GR_FOG_WITH_TABLE); 

			break;
		}

		case (RENDER_WORLD_POLY_MODE):
		{
			Render_SetColorCombine(ColorCombine_TextureGouraud);
			Render_SetTexCombine(TexCombine_SinglePassTexture);
			
			grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);

			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
								 GR_BLEND_ONE, GR_BLEND_ZERO);

			if (g_FogEnable)
				//grFogMode(GR_FOG_WITH_ITERATED_ALPHA | GR_FOG_ADD2); 
				grFogMode(GR_FOG_WITH_TABLE | GR_FOG_ADD2); 

			break;
		}

		case (RENDER_WORLD_TRANSPARENT_POLY_MODE):
		{
			Render_SetColorCombine(ColorCombine_TextureGouraud);
			Render_SetTexCombine(TexCombine_SinglePassTexture);

			grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);		// Bug fix thanks to Bobtree

			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_SRC_COLOR, GR_BLEND_DST_COLOR);

			if (g_FogEnable)
				//grFogMode(GR_FOG_WITH_ITERATED_ALPHA);
				grFogMode(GR_FOG_WITH_TABLE); 

			break;
		}

		// NOTE - IF this card has 2 TMU's, world polys AND lightmap polys will be piped through here
		//	Notice how Simultaneous mode is turned on when 2 TMU's are detected... -JP
		case(RENDER_LIGHTMAP_POLY_MODE):
		{
			grTexMipMapMode( TMU[1], GR_MIPMAP_DISABLE, FXFALSE );

		
			if (g_BoardInfo.NumTMU >= 2)
			{
				Render_SetColorCombine(ColorCombine_TextureGouraud);

				grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
									 GR_BLEND_SRC_COLOR, GR_BLEND_DST_COLOR);

				if (g_FogEnable)
					//grFogMode(GR_FOG_WITH_ITERATED_ALPHA);
					grFogMode(GR_FOG_WITH_TABLE); 

				Render_SetTexCombine(TexCombine_SimultaneousPass);
			
				grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);
			}
			else
			{
				Render_SetColorCombine(ColorCombine_TextureGouraud);

				// Singlepass mode if there is onle 1 TMU
				Render_SetTexCombine(TexCombine_SinglePassTexture);
			
				// Modulate the texture with the framebuffer
				if (g_FogEnable)
				{
					//grFogMode(GR_FOG_WITH_ITERATED_ALPHA | GR_FOG_MULT2);
					grFogMode(GR_FOG_WITH_TABLE | GR_FOG_MULT2); 

					grAlphaBlendFunction(	GR_BLEND_ONE, GR_BLEND_PREFOG_COLOR,
											GR_BLEND_ONE, GR_BLEND_ZERO);
				}
				else
				{
					grAlphaBlendFunction(	GR_BLEND_DST_COLOR, GR_BLEND_ZERO,
											GR_BLEND_ONE, GR_BLEND_ZERO);
				}

				// Force clamping to be on if this card only has one TMU
				grTexClampMode(TMU[0], GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
				Render_HardwareFlags |= DRV_RENDER_CLAMP_UV;
			}

			break;
		}

		case(RENDER_LIGHTMAP_FOG_POLY_MODE):
		{
			grTexMipMapMode( TMU[1], GR_MIPMAP_DISABLE, FXFALSE );

			Render_SetColorCombine(ColorCombine_TextureGouraud);

			if (g_BoardInfo.NumTMU >= 2)
			{
				Render_SetTexCombine(TexCombine_PassThrough);
				grTexMipMapMode(TMU[0], GR_MIPMAP_NEAREST, FXFALSE);
			}
			else
			{
				Render_SetTexCombine(TexCombine_SinglePassTexture);
			}

			if (g_FogEnable)
				//grFogMode(GR_FOG_WITH_ITERATED_ALPHA); 
				grFogMode(GR_FOG_DISABLE); 

			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ONE,
								 GR_BLEND_ONE, GR_BLEND_ONE);

			break;
		}

		case (RENDER_DECAL_MODE):
		{
			grLfbConstantDepth(0xffff);
			grLfbConstantAlpha(0xff);
			grChromakeyMode(GR_CHROMAKEY_ENABLE);
			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
								 GR_BLEND_ONE, GR_BLEND_ZERO);

			break;
		}

		default:
		{
			assert(0);
		}
	}

	Render_HardwareMode = NewMode;
}
#else
//==========================================================================================
//	Render_SetHardwareMode
//==========================================================================================
void Render_SetHardwareMode(int32 NewMode, uint32 NewFlags)
{
	if (NewFlags != Render_HardwareFlags)		// See if the flags gave changed,,,
	{
		if (NewFlags & DRV_RENDER_NO_ZMASK)
			grDepthBufferFunction(GR_CMP_ALWAYS);
		else if (Render_HardwareFlags & DRV_RENDER_NO_ZMASK)
			grDepthBufferFunction( GR_CMP_GEQUAL );	

		if (NewFlags & DRV_RENDER_NO_ZWRITE)
			grDepthMask(FXFALSE);
		else if (Render_HardwareFlags & DRV_RENDER_NO_ZWRITE)
			grDepthMask(FXTRUE);
		
		if (NewFlags & DRV_RENDER_CLAMP_UV)
			grTexClampMode(TMU[0], GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
		else if (Render_HardwareFlags & DRV_RENDER_CLAMP_UV)
			grTexClampMode(TMU[0], GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
	}

	// Make these flags recent
	Render_HardwareFlags = NewFlags;

	if (NewMode == Render_HardwareMode)// && NewFlags == Render_HardwareFlags) 
		return;		// Nothing to change

	if (Render_HardwareMode == RENDER_DECAL_MODE)
		grChromakeyMode(GR_CHROMAKEY_DISABLE);		// Restore chroma key mode

	// sets up all hardware mode 
	switch (NewMode)
	{
		case (RENDER_MISC_TEX_POLY_MODE):
		{
			grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);		//Bug fix thanks to Bobtree

			guColorCombineFunction( GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);

			if (g_BoardInfo.NumTMU >= 2)
			{
				grTexCombine( TMU[0],
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  FXFALSE, FXFALSE );
				grTexCombine(	TMU[1], 
								GR_COMBINE_FUNCTION_NONE, 
								GR_COMBINE_FACTOR_NONE,
								GR_COMBINE_FUNCTION_NONE, 
								GR_COMBINE_FACTOR_NONE,
								FXFALSE, FXFALSE );
			}
			else
			{
				grTexCombine( TMU[0],
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  FXFALSE, FXFALSE );
			}

			//grChromakeyMode(GR_CHROMAKEY_ENABLE);			

			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,  GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_SRC_ALPHA,  GR_BLEND_ONE_MINUS_SRC_ALPHA);

			if (Render_HardwareMode == RENDER_MISC_GOURAD_POLY_MODE)
			{
				grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
					GR_COMBINE_FACTOR_LOCAL,
					GR_COMBINE_LOCAL_ITERATED,
					GR_COMBINE_OTHER_TEXTURE,
					FXFALSE );
			}
			
			break;
		}
		case (RENDER_MISC_GOURAD_POLY_MODE):
		{
			guColorCombineFunction( GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
			guTexCombineFunction( TMU[0], GR_TEXTURECOMBINE_ONE );
			
			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_ONE, GR_BLEND_ONE);

			//grChromakeyMode(GR_CHROMAKEY_DISABLE);

			break;
		}

		case (RENDER_LINES_POLY_MODE):
		{	
			guColorCombineFunction( GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
			guTexCombineFunction( TMU[0], GR_TEXTURECOMBINE_ONE );
			
			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_ONE, GR_BLEND_ONE);

			//grChromakeyMode(GR_CHROMAKEY_DISABLE);
			break;
		}

		case (RENDER_WORLD_POLY_MODE_NO_LIGHTMAP):
		case (RENDER_WORLD_POLY_MODE):
		{
			grTexCombine( TMU[0],
						  GR_COMBINE_FUNCTION_LOCAL,
						  GR_COMBINE_FACTOR_NONE,
						  GR_COMBINE_FUNCTION_LOCAL,
						  GR_COMBINE_FACTOR_NONE,
						  FXFALSE, FXFALSE );

			grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);

			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
								 GR_BLEND_ONE, GR_BLEND_ZERO);

			if (Render_HardwareMode == RENDER_MISC_GOURAD_POLY_MODE)
			{
				grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
					GR_COMBINE_FACTOR_LOCAL,
					GR_COMBINE_LOCAL_ITERATED,
					GR_COMBINE_OTHER_TEXTURE,
					FXFALSE );
			}
		
			break;
		}

		case (RENDER_WORLD_TRANSPARENT_POLY_MODE):
		{
			//grChromakeyMode(GR_CHROMAKEY_ENABLE);			

			if (g_BoardInfo.NumTMU >= 2)
			{
				grTexCombine( TMU[0],
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  FXFALSE, FXFALSE );

				grTexCombine(	TMU[1], 
								GR_COMBINE_FUNCTION_NONE, 
								GR_COMBINE_FACTOR_NONE,
								GR_COMBINE_FUNCTION_NONE, 
								GR_COMBINE_FACTOR_NONE,
								FXFALSE, FXFALSE ); 
			}
			else
			{
				grTexCombine( TMU[0],
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  FXFALSE, FXFALSE );
				
			}

			grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);		//Bug fix thanks to Bobtree

			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
								 GR_BLEND_SRC_COLOR, GR_BLEND_DST_COLOR);

			if (Render_HardwareMode == RENDER_MISC_GOURAD_POLY_MODE)
			{
				grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
					GR_COMBINE_FACTOR_LOCAL,
					GR_COMBINE_LOCAL_ITERATED,
					GR_COMBINE_OTHER_TEXTURE,
					FXFALSE );
			}
		
			break;
		}

		case(RENDER_LIGHTMAP_POLY_MODE):
		{
			grTexMipMapMode( TMU[1], GR_MIPMAP_DISABLE, FXFALSE );

			//grChromakeyMode(GR_CHROMAKEY_DISABLE);

			if (g_BoardInfo.NumTMU >= 2)
			{
				grTexCombine(TMU[0], 
							 GR_COMBINE_FUNCTION_SCALE_OTHER, 
							 GR_COMBINE_FACTOR_LOCAL,
							 GR_COMBINE_FUNCTION_SCALE_OTHER, 
							 GR_COMBINE_FACTOR_LOCAL,
							 FXFALSE, FXFALSE ); 
			
				// The lightmap textures (TMU[1]) will be blended locally
				grTexCombine( TMU[1],
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_NONE,
							  FXFALSE, FXFALSE );
			
				grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
									 GR_BLEND_SRC_COLOR, GR_BLEND_DST_COLOR);
			
				grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);
			}
			else
			{
				grAlphaBlendFunction(GR_BLEND_SRC_COLOR, GR_BLEND_DST_COLOR,
									 GR_BLEND_SRC_COLOR, GR_BLEND_DST_COLOR);
				
				// Force clamping to be on TMU0 if this is a 1 TMU board
				grTexClampMode(TMU[0], GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
				Render_HardwareFlags |= DRV_RENDER_CLAMP_UV;
			}

			if (Render_HardwareMode == RENDER_MISC_GOURAD_POLY_MODE)
			{
				grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
					GR_COMBINE_FACTOR_LOCAL,
					GR_COMBINE_LOCAL_ITERATED,
					GR_COMBINE_OTHER_TEXTURE,
					FXFALSE );
			}
			
			break;
		}

		case(RENDER_LIGHTMAP_FOG_POLY_MODE):
		{
			grTexMipMapMode( TMU[1], GR_MIPMAP_DISABLE, FXFALSE );

			//grChromakeyMode(GR_CHROMAKEY_DISABLE);

			if (g_BoardInfo.NumTMU >= 2)
			{
			
				grTexCombine(TMU[0], 
							 GR_COMBINE_FUNCTION_SCALE_OTHER, 
							 GR_COMBINE_FACTOR_ONE,
							 GR_COMBINE_FUNCTION_SCALE_OTHER, 
							 GR_COMBINE_FACTOR_ONE,
							 FXFALSE, FXFALSE ); 
			
				// The lightmap textures (TMU[1]) will be blended locally
				grTexCombine( TMU[1],
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_ONE,
							  GR_COMBINE_FUNCTION_LOCAL,
							  GR_COMBINE_FACTOR_ONE,
							  FXFALSE, FXFALSE );

				grTexMipMapMode( TMU[0], GR_MIPMAP_NEAREST, FXFALSE);
			}

			if (Render_HardwareMode == RENDER_MISC_GOURAD_POLY_MODE)
			{
				grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
					GR_COMBINE_FACTOR_LOCAL,
					GR_COMBINE_LOCAL_ITERATED,
					GR_COMBINE_OTHER_TEXTURE,
					FXFALSE );
			}

			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ONE,
								 GR_BLEND_ONE, GR_BLEND_ONE);

			break;
		}

		case (RENDER_DECAL_MODE):
		{
			grLfbConstantDepth(0xffff);
			grLfbConstantAlpha(0xff);
			grChromakeyMode(GR_CHROMAKEY_ENABLE);
			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
								 GR_BLEND_ONE, GR_BLEND_ZERO);

			break;
		}

		default:
		{
			assert(0);
		}
	}

	Render_HardwareMode = NewMode;
}
#endif

//****************************************************************************
//	Render a regualar gouraud shaded poly
//****************************************************************************
geBoolean DRIVERCC Render_GouraudPoly(DRV_TLVertex *Pnts, int32 NumPoints, uint32 Flags)
{
	int32 i;
	GrVertex	vrtx[RENDER_MAX_PNTS];
	float		Alpha = Pnts->a;

	for (i = 0; i< NumPoints; i++)
	{
		float ZRecip;

		vrtx[i].r = Pnts->r;
		vrtx[i].g = Pnts->g;
		vrtx[i].b = Pnts->b;
		vrtx[i].a = Alpha;
		
		// set the SOW, TOW, and OOW values for a 1 TMU configuration TMU0 
		ZRecip = (1.0f/(Pnts->z));
		vrtx[i].ooz = (65535.0f) * ZRecip;
		vrtx[i].oow = ZRecip;
		vrtx[i].tmuvtx[0].oow = ZRecip;

		vrtx[i].x = SNAP_VERT(Pnts->x);
		vrtx[i].y = SNAP_VERT(Pnts->y);

		Pnts++;
	}

	Render_SetHardwareMode(RENDER_MISC_GOURAD_POLY_MODE, Flags);

	grDrawPolygonVertexList( NumPoints, vrtx); 

	GLIDEDRV.NumRenderedPolys++;

	return TRUE;
}

//==========================================================================================
//	Render_LinesPoly
//==========================================================================================
geBoolean DRIVERCC Render_LinesPoly(DRV_TLVertex *Pnts, int32 NumPoints)
{
	int32 i;
	GrVertex	vrtx[RENDER_MAX_PNTS];

	for (i = 0; i< NumPoints; i++)
	{
		float	ZRecip;

		vrtx[i].r = 255.0f;//Pnts->r;
		vrtx[i].g = 255.0f;//Pnts->g;
		vrtx[i].b = 255.0f;//Pnts->b;
		vrtx[i].a = 255.0f;
		
		// set the SOW, TOW, and OOW values for a 1 TMU configuration TMU0 
		ZRecip = (1/(Pnts->z));
		vrtx[i].ooz = (65535.0f) / Pnts->z;
		vrtx[i].oow = ZRecip;
		vrtx[i].tmuvtx[0].oow = ZRecip;

		vrtx[i].x = SNAP_VERT(Pnts->x);
		vrtx[i].y = SNAP_VERT(Pnts->y);

		Pnts++;
	}

	Render_SetHardwareMode(RENDER_LINES_POLY_MODE, 0);

	for (i=0; i< NumPoints; i++)
	{
		int32 i2 = ((i+1) < NumPoints) ? (i+1) : 0;

		grDrawLine(&vrtx[i], &vrtx[i2]);
	}

	GLIDEDRV.NumRenderedPolys++;

	return TRUE;
}

//****************************************************************************
//	Render a world texture / lightmap 
//****************************************************************************
geBoolean DRIVERCC Render_WorldPoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags)
{
	GrVertex		Vrtx[RENDER_MAX_PNTS], *pVrtx;
	float			OneOverSize_255;
	float			ShiftU, ShiftV, ScaleU, ScaleV;
	DRV_TLVertex	*pPnts;
	int32			i;
	float			Alpha;

	assert(Pnts);
	assert(TexInfo);

#ifdef ENABLE_WIREFRAME	
	if ( DoWireFrame )
		return (Render_LinesPoly(Pnts, NumPoints));
#endif

#if 0
	switch (PolyMode)
	{
		case DRV_POLYMODE_NORMAL:
			break;							// Use this function
		case DRV_POLYMODE_GOURAUD:
			return (Render_GouraudPoly(Pnts, NumPoints, 0));
		case DRV_POLYMODE_LINES:
			return (Render_LinesPoly(Pnts, NumPoints));
	}
#endif
	
	GLIDEDRV.NumRenderedPolys++;

	OneOverSize_255 = THandle->OneOverLogSize_255;

	// Get how much to shift U, and V for the Texture
	ShiftU = TexInfo->ShiftU;
	ShiftV = TexInfo->ShiftV;
	ScaleU = 1.0f/TexInfo->DrawScaleU;
	ScaleV = 1.0f/TexInfo->DrawScaleV;

	pPnts = Pnts;

	pVrtx = Vrtx;

#if 0
	// Fix the uv's to be as close to the origin as possible, without affecting their appearance...
	//if (pPnts->u > 1000.0f || pPnts->v > 1000.0f)
	{
		float		OneOverLogSize;

 		OneOverLogSize = 1.0f / (float)THandle->LogSize;
		
		ShiftU -= (float)(((int32)(pPnts->u*ScaleU/THandle->Width))*THandle->Width);
		ShiftV -= (float)(((int32)(pPnts->v*ScaleV/THandle->Height))*THandle->Height);
	}
#endif

	Alpha = Pnts->a;

	for (i = 0; i< NumPoints; i++)
	{
		float	ZRecip;
		
		pVrtx->a = Alpha;

		pVrtx->r = pPnts->r;
		pVrtx->g = pPnts->g;
		pVrtx->b = pPnts->b;
		
		ZRecip = (1.0f/(pPnts->z));
		pVrtx->ooz = 65535.0f * ZRecip;
		pVrtx->oow = ZRecip;

		pVrtx->tmuvtx[0].oow = ZRecip;
		pVrtx->tmuvtx[1].oow = ZRecip;
	
		ZRecip *= OneOverSize_255;

		pVrtx->tmuvtx[TMU[0]].sow = (pPnts->u*ScaleU + ShiftU) * ZRecip;
		pVrtx->tmuvtx[TMU[0]].tow = (pPnts->v*ScaleV + ShiftV) * ZRecip;

		pVrtx->x = SNAP_VERT(pPnts->x);
		pVrtx->y = SNAP_VERT(pPnts->y);

		pPnts++;
		pVrtx++;
	}
	

	// Set the source texture for TMU 0
	SetupTexture(THandle);
	
	// If only 1 TMU (or no lightmap), then draw first pass poly now. 
	if (g_BoardInfo.NumTMU == 1 || !LInfo)		
	{								// The lightmap will blend over it
		if (Flags & DRV_RENDER_ALPHA)
			Render_SetHardwareMode(RENDER_WORLD_TRANSPARENT_POLY_MODE, Flags);
		else
		{
			if (LInfo)
				Render_SetHardwareMode(RENDER_WORLD_POLY_MODE, Flags);
			else
				Render_SetHardwareMode(RENDER_WORLD_POLY_MODE_NO_LIGHTMAP, Flags);
		}

		grDrawPolygonVertexList( NumPoints, Vrtx); 
	}

	if (LInfo)					// If there is a lightmap, render it now, on top of the first pass poly
	{
		geBoolean  Dynamic;

		// How much to shift u'vs back into lightmap space
		ShiftU = (float)LInfo->MinU-8.0f;
		ShiftV = (float)LInfo->MinV-8.0f;
		
		pPnts = Pnts;

		// Call the engine to set this sucker up, because it's visible...
		GLIDEDRV.SetupLightmap(LInfo, &Dynamic);
		
		OneOverSize_255 = LInfo->THandle->OneOverLogSize_255;

		pVrtx = Vrtx;

		for (i = 0; i< NumPoints; i++)
		{
			float u = pPnts->u-ShiftU;
			float v = pPnts->v-ShiftV;
			float ZRecip = pVrtx->oow * OneOverSize_255;

			pVrtx->tmuvtx[TMU[1]].sow = u*ZRecip;
			pVrtx->tmuvtx[TMU[1]].tow = v*ZRecip;
			pPnts++;
			pVrtx++;
		}
		
		RenderLightmapPoly(Vrtx, NumPoints, LInfo, (geBoolean)Dynamic, Flags);
	}

	return TRUE;
}

void RenderLightmapPoly(GrVertex *vrtx, int32 NumPoints, DRV_LInfo *LInfo, geBoolean Dynamic, uint32 Flags)
{
	geRDriver_THandle	*THandle;
	int32				l;
	GCache_Slot			*Slot;

	THandle = LInfo->THandle;

	Slot = SetupLMapTexture(THandle, LInfo, Dynamic, 0);
	
	GCache_SlotSetLRU(Slot, CurrentLRU);
	TextureSource(TMU[1], GCache_SlotGetMemAddress(Slot), GR_MIPMAPLEVELMASK_BOTH, GCache_SlotGetInfo(Slot));

	Render_SetHardwareMode(RENDER_LIGHTMAP_POLY_MODE, Flags);

	grDrawPolygonVertexList( NumPoints, vrtx); 

	// Render special maps
	for (l=1; l< 2; l++)
	{
		if (!LInfo->RGBLight[l])
			continue;
		
		switch(l)
		{
			case LMAP_TYPE_LIGHT:
				DownloadLightmap(LInfo, THandle->LogSize, Slot, 0);
				TextureSource(TMU[1], GCache_SlotGetMemAddress(Slot), GR_MIPMAPLEVELMASK_BOTH, GCache_SlotGetInfo(Slot));
				Render_SetHardwareMode(RENDER_LIGHTMAP_POLY_MODE, Flags);
				break;

			case LMAP_TYPE_FOG:
				DownloadLightmap(LInfo, THandle->LogSize, Slot, l);
				TextureSource(TMU[1], GCache_SlotGetMemAddress(Slot), GR_MIPMAPLEVELMASK_BOTH, GCache_SlotGetInfo(Slot));
				Render_SetHardwareMode(RENDER_LIGHTMAP_FOG_POLY_MODE, Flags);
				break;
		}
		
		grDrawPolygonVertexList( NumPoints, vrtx); 
	}
}

//**********************************************************************************
//	Downloads a lightmap to the card
//**********************************************************************************
void DownloadLightmap(DRV_LInfo *LInfo, int32 Wh, GCache_Slot *Slot, int32 LMapNum)
{
	uint16			TempL[MAX_LMAP_SIZE*MAX_LMAP_SIZE];			// Temp to hold converted 565 lightmap
	int32			w,h;
	uint16			*pTempP = TempL;
	uint8			r,g,b;
	uint8			*Bits;
	int32			W = LInfo->Width;
	int32			H = LInfo->Height;
	GrTexInfo		*Info;

	//memset(TempL, 0, sizeof(uint6)*32*32);
	Bits = (uint8*)LInfo->RGBLight[LMapNum];

	for (h = 0; h< H; h++)
	{
		for (w = 0; w< W; w++)
		{
			r = *(Bits++);
			g = *(Bits++);
			b = *(Bits++);

			r >>= 3;
			g >>= 2;
			b >>= 3;

			*pTempP++ = (uint16)((r<<(11)) + (g << 5) + b);
		}
		pTempP += (Wh - w);
	}
	
 	Info = GCache_SlotGetInfo(Slot);
	Info->data = TempL;
	
	GCache_UpdateSlot(LMapCache, Slot, Info);
}


//************************************************************************************
//	Render a misc texture poly....
//************************************************************************************
geBoolean DRIVERCC Render_MiscTexturePoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, uint32 Flags)
{
	int32				i;
	GrVertex			vrtx[RENDER_MAX_PNTS];
	DRV_TLVertex		*pPnt = Pnts;
	float				Alpha, Width_255, Height_255;
	
#ifdef ENABLE_WIREFRAME
	if ( DoWireFrame )
		return (Render_LinesPoly(Pnts, NumPoints));
#endif

	assert( Pnts != NULL );
	assert( NumPoints < RENDER_MAX_PNTS );
	assert( THandle != NULL );

	{
		float OneOverLogSize;
		
		OneOverLogSize = THandle->OneOverLogSize_255;

		Width_255  = (float)THandle->Width  * OneOverLogSize;
		Height_255 = (float)THandle->Height * OneOverLogSize;
	}

	Alpha = Pnts->a;

	for (i = 0; i< NumPoints; i++)
	{
		float ZRecip;

		vrtx[i].a = Alpha;
		vrtx[i].r = pPnt->r;
		vrtx[i].g = pPnt->g;
		vrtx[i].b = pPnt->b;
		// set the SOW, TOW, and OOW values for a 1 TMU configuration TMU0 
		ZRecip = (1.0f/(pPnt->z));		// 1/2 frame in this divide.
		vrtx[i].ooz = (65535.0f) * ZRecip; 
		vrtx[i].oow = ZRecip;
		vrtx[i].tmuvtx[0].oow = ZRecip;

		vrtx[i].tmuvtx[0].sow = pPnt->u * Width_255 * ZRecip;
		vrtx[i].tmuvtx[0].tow = pPnt->v * Height_255 * ZRecip;

		vrtx[i].x = SNAP_VERT(pPnt->x);
		vrtx[i].y = SNAP_VERT(pPnt->y);

		pPnt++;
	}

	SetupTexture(THandle);

	Render_SetHardwareMode(RENDER_MISC_TEX_POLY_MODE, Flags);

	grDrawPolygonVertexList( NumPoints, vrtx); 
	//grAADrawPolygonVertexList( NumPoints, vrtx); 

	//GLIDEDRV.NumRenderedPolys++;

	return TRUE;
}

geRDriver_THandle		*OldPalHandle;

//============================================================================================
//	SetupTexture
//============================================================================================
void SetupTexture(geRDriver_THandle *THandle)
{
	GTHandle_CheckTextures();

	// Setup the palette
	if (THandle->PixelFormat.PixelFormat == GE_PIXELFORMAT_8BIT)
	{
		assert(THandle->PalHandle);
		assert(THandle->PalHandle->Data);

		// CB <> one shared palette in glide; added _UPDATE check
		if ((OldPalHandle != THandle->PalHandle) || (THandle->PalHandle->Flags & THANDLE_UPDATE))
		{
			grTexDownloadTable(TMU[0], GR_TEXTABLE_PALETTE, THandle->PalHandle->Data);
			OldPalHandle = THandle->PalHandle;
			THandle->PalHandle->Flags &= ~THANDLE_UPDATE;
		}
	}

	if (!THandle->Slot || GCache_SlotGetUserData(THandle->Slot) != THandle)
	{
		THandle->Slot = GCache_TypeFindSlot(THandle->CacheType);
		assert(THandle->Slot);

		GCache_SlotSetUserData(THandle->Slot, THandle);
		THandle->Flags |= THANDLE_UPDATE;
		
		CacheInfo.TexMisses++;
	}
	
	if (THandle->Flags & THANDLE_UPDATE)
	{
		GrTexInfo		*Info;

 		Info = GCache_SlotGetInfo(THandle->Slot);
	
		// Set the data to the correct bits
		Info->data = THandle->Data;
	
		// We must make sure the textures formats and the caches format match (formats can change on the fly)
		GlideFormatFromGenesisFormat(THandle->PixelFormat.PixelFormat, &Info->format);

		GCache_UpdateSlot(TextureCache, THandle->Slot, Info);

		THandle->Flags &= ~THANDLE_UPDATE;
	}

	GCache_SlotSetLRU(THandle->Slot, CurrentLRU);
	TextureSource(TMU[0], GCache_SlotGetMemAddress(THandle->Slot), GR_MIPMAPLEVELMASK_BOTH, GCache_SlotGetInfo(THandle->Slot));
}

//============================================================================================
//	SetupLMapTexture
//============================================================================================
GCache_Slot *SetupLMapTexture(geRDriver_THandle *THandle, DRV_LInfo *LInfo, geBoolean Dynamic, int32 LMapNum)
{
	GTHandle_CheckTextures();

	if (Dynamic)
		THandle->Flags |= THANDLE_UPDATE;

	if (!THandle->Slot || GCache_SlotGetUserData(THandle->Slot) != THandle)
	{
		THandle->Slot = GCache_TypeFindSlot(THandle->CacheType);
		assert(THandle->Slot);

		GCache_SlotSetUserData(THandle->Slot, THandle);
		THandle->Flags |= THANDLE_UPDATE;

		CacheInfo.LMapMisses++;
	}

	if (THandle->Flags & THANDLE_UPDATE)
	{
		DownloadLightmap(LInfo, THandle->LogSize, THandle->Slot, LMapNum);
	}
	
	if (Dynamic)
		THandle->Flags |= THANDLE_UPDATE;
	else
		THandle->Flags &= ~THANDLE_UPDATE;

	GCache_SlotSetLRU(THandle->Slot, CurrentLRU);

	return THandle->Slot;
}

//==================================================================================
//	Render_DrawDecal
//==================================================================================
geBoolean DRIVERCC Render_DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y)
{
	int32		Width, Height, Stride, OriginalWidth, w, h, Add1, Add2;
	uint16		*Data;
	GrLfbInfo_t	Info;
	uint16		*BackBuffer;
	
	if (x >= ClientWindow.Width)
		return TRUE;
	if (y >= ClientWindow.Height)
		return TRUE;

	Width = THandle->Width;
	OriginalWidth = Width;
	Height = THandle->Height;
	Stride = Width<<1;
	Data = (uint16*)THandle->Data;

	if (SRect)
	{
		Data += SRect->top*Width + SRect->left;
		Height = SRect->bottom - SRect->top;
		Width = SRect->right - SRect->left;
	}

	if (x < 0)
	{
		if (x+Width <= 0)
			return TRUE;
		Data += -x;
		Width -= -x;
		x=0;
	}

	if (y < 0)
	{
		if (y+Height <= 0)
			return TRUE;
		Data += (-y)*OriginalWidth;
		Height -= -y;
		y=0;
	}
	
	if (x + Width >= ClientWindow.Width)
		Width -= (x+Width) - ClientWindow.Width;

	if (y + Height >= ClientWindow.Height)
		Height -=  (y+Height)- ClientWindow.Height;

	/*
	if (!grLfbWriteRegion(GR_BUFFER_BACKBUFFER , 
						x, y, 
						GR_LFB_SRC_FMT_1555, 
						Width, Height, Stride, 
						(void*)Data))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_BlitDecal:  The GLIDE decal blit operation could not be performed.");
		return FALSE;
	}
	*/

	Info.size = sizeof(Info);

	Render_SetHardwareMode(RENDER_DECAL_MODE, 0);
	
	if (!grLfbLock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_565, 0, FXFALSE, &Info))
	//if (!grLfbLock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_1555, 0, FXTRUE, &Info))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_BlitDecal:  Could not lock the back buffer.");
		return FALSE;
	}

	BackBuffer = (uint16*)Info.lfbPtr;

	BackBuffer += (y * (Info.strideInBytes>>1)) + x;

	Add1 = OriginalWidth - Width;
	Add2 = (Info.strideInBytes>>1) - Width;

	for (h=0; h<Height; h++)
	{
		for (w=0; w<Width; w++)
		{
			if (*Data != 1)
				*BackBuffer = *Data;

			Data++;
			BackBuffer++;
		}
		Data += Add1;
		BackBuffer += Add2;
	}

	if (!grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER))
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "GLIDE_BlitDecal:  Could not unlock the back buffer.");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	Scene managment functions
//=====================================================================================
geBoolean DRIVERCC BeginScene(geBoolean Clear, geBoolean ClearZ, RECT *WorldRect)
{
	memset(&CacheInfo, 0, sizeof(DRV_CacheInfo));

#ifdef ENABLE_WIREFRAME	
	{
		uint32 KeyState1, KeyState2;
		
		#pragma message("Glide : WireFrame enabled!")
		KeyState1 = GetAsyncKeyState(VK_CONTROL) & 0x8001;
		KeyState2 = GetAsyncKeyState(VK_F8) & 0x8001;
		if (KeyState1 && KeyState2)
		{
			DoWireFrame ^= 1;
		}
	}

	if (DoWireFrame)
	{
		Clear = GE_TRUE;
		ClearZ = GE_TRUE;
	}
#endif
	
	if (!GTHandle_CheckTextures())
		return GE_FALSE;

	// FIXME:	Make clear zbuffer and frame buffer seperate
	if (Clear)
		grColorMask(FXTRUE, FXFALSE);
	else
		grColorMask(FXFALSE, FXFALSE);

	
	if (g_FogEnable)
		grBufferClear(((uint32)g_FogB<<16)|((uint32)g_FogG<<8)|(uint32)g_FogR, 0, GR_ZDEPTHVALUE_FARTHEST);
	else
		grBufferClear(0, 0, GR_ZDEPTHVALUE_FARTHEST);

	grColorMask(FXTRUE, FXTRUE);

	GLIDEDRV.NumRenderedPolys = 0;
	
	CurrentLRU++;

	return TRUE;
}

geBoolean DRIVERCC EndScene(void)
{

// Mike: force z buffer on
	grDepthMask(FXTRUE);
	Render_HardwareFlags &= ~DRV_RENDER_NO_ZWRITE;

	grDepthBufferFunction( GR_CMP_GEQUAL );	
	Render_HardwareFlags &= ~DRV_RENDER_NO_ZMASK;

	// swapping the front and back buffer on the next vertical retrace
#if 1
	grBufferSwap( 1 );
#else
	grBufferSwap( 0 );
#endif

	return TRUE;
}

geBoolean DRIVERCC BeginWorld(void)
{

	ResetSpans(ClientWindow.Height);
	NumWorldPixels = 0;

	GLIDEDRV.NumWorldPixels = 0;
	GLIDEDRV.NumWorldSpans = 0;

	RenderMode = RENDER_WORLD;

	return TRUE;
}

geBoolean DRIVERCC EndWorld(void)
{
	
	GLIDEDRV.NumWorldPixels = NumWorldPixels;
	GLIDEDRV.NumWorldSpans = NumSpans;

	RenderMode = RENDER_NONE;

	return TRUE;
}

geBoolean DRIVERCC BeginMeshes(void)
{
	
	RenderMode = RENDER_MESHES;

	return TRUE;
}

geBoolean DRIVERCC EndMeshes(void)
{
	RenderMode = RENDER_NONE;

	return TRUE;
}

geBoolean DRIVERCC BeginModels(void)
{

	RenderMode = RENDER_MODELS;

	return TRUE;
}

geBoolean DRIVERCC EndModels(void)
{

	RenderMode = RENDER_NONE;

	return TRUE;
}