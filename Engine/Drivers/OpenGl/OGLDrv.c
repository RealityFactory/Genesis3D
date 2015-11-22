/****************************************************************************************/
/*  OglDrv.c                                                                            */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Exposed interface for OpenGL version of Genesis Driver                 */
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
/*                                                                                      */
/****************************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <gl/gl.h>

#include "OglDrv.h"
#include "OglMisc.h"
#include "THandle.h"
#include "Render.h"
#include "Win32.h"


int32 LastError;
char LastErrorStr[255];		

GLfloat	CurrentGamma = 1.0f;
DRV_Window	ClientWindow;
static DRV_CacheInfo	CacheInfo;

GLint maxTextureSize = 0;
geBoolean FogEnabled = GE_TRUE;
geBoolean RenderingIsOK = GE_TRUE;

// Toggle and function pointers for OpenGL multitexture extention
GLboolean multitexture = GE_FALSE;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;



DRV_Driver OGLDRV = 
{

	"OpenGL driver. v"DRV_VMAJS"."DRV_VMINS". Copyright 1999, Eclipse Inc.; All Rights Reserved.",

	DRV_VERSION_MAJOR,
	DRV_VERSION_MINOR,

	DRV_ERROR_NONE,
	NULL,
	
	EnumSubDrivers,
	EnumModes,
	
	EnumPixelFormats,

	DrvInit,
	DrvShutdown,
	DrvResetAll,
	DrvUpdateWindow,
	DrvSetActive,

	THandle_Create,
	THandle_Destroy,

	THandle_Lock,
	THandle_UnLock,

	NULL, 
	NULL, 

	NULL, 
	NULL, 

	THandle_GetInfo,

	BeginScene,
	EndScene,
	BeginWorld,
	EndWorld,
	BeginMeshes,
	EndMeshes,
	BeginModels,
	EndModels,
// changed QD Shadows
	BeginShadowVolumes,
	EndShadowVolumes,
	1,
// end change

	Render_GouraudPoly,
	Render_WorldPoly,
	Render_MiscTexturePoly,
// changed QD Shadows
	Render_StencilPoly,
	DrawShadowPoly,
// end change

	DrawDecal,

	0,0,0,

	&CacheInfo,

	ScreenShot,

	SetGamma, 
	GetGamma,

	SetFogEnable, 

	NULL,
	NULL,								// Init to NULL, engine SHOULD set this (SetupLightmap)
	NULL,
};

// Not implemented, but you noticed that already huh?
geBoolean DRIVERCC SetFogEnable(geBoolean Enable, float r, float g, float b, float Start, float End)
{
	GLfloat fogColor[4];

	if(Enable==GE_TRUE)
	{
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_LINEAR);
		fogColor[0] = (GLfloat)r/255.0f;
		fogColor[1] = (GLfloat)g/255.0f;
		fogColor[2] = (GLfloat)b/255.0f;
		fogColor[3] = 1.0f;
		glFogfv(GL_FOG_COLOR, fogColor);
		//glFogf(GL_FOG_DENSITY, 1.0f); // changed QD Fog: only used for exponential fogmode
		glHint(GL_FOG_HINT, GL_FASTEST);
		
		// changed QD Fog
		if(Start < 1.0f)
			Start = 1.0f;
		if(End < 1.0f)
			End = 1.0f;
		glFogf(GL_FOG_START, 1.0f- 1.0f/Start); 
		glFogf(GL_FOG_END, 1.0f - 1.0f/End);
		FogEnabled = GE_TRUE;
		glClearColor(fogColor[0], fogColor[1], fogColor[2], 1.0); /* fog color */
		// end change
	}
	else
	{
		glDisable(GL_FOG);
		FogEnabled = GE_FALSE;
		// changed QD Fog
		glClearColor(0.0, 0.0, 0.0, 1.0);
	}
	return GE_TRUE;
}


geBoolean DRIVERCC DrvInit(DRV_DriverHook *Hook)
{
	RECT		WRect;
	FILE *stream;

	COLOR_DEPTH = 16;
	ZBUFFER_DEPTH = 16;

	stream = fopen("D3D24.ini","r");
	if(stream)
	{
		fscanf(stream,"%d",&COLOR_DEPTH);
		fscanf(stream,"%d",&ZBUFFER_DEPTH);
		fclose(stream);
	}

	WindowSetup(Hook);
	
	if(Hook->Width == -1 && Hook->Height == -1)
	{
		GetClientRect(Hook->hWnd, &WRect);
		
		Hook->Width = (WRect.right - WRect.left);
		Hook->Height = (WRect.bottom - WRect.top);
	}
	else if(!SetFullscreen(Hook))
	{
		return GE_FALSE;
	} 

	SetGLPixelFormat(Hook);

	ClientWindow.Width = Hook->Width;
	ClientWindow.Height = Hook->Height;
	ClientWindow.hWnd = Hook->hWnd;
	
#ifdef USE_LIGHTMAPS
	if(ExtensionExists("GL_ARB_multitexture"))
	{
		glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
		glMultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC)wglGetProcAddress("glMultiTexCoord4fARB");

		if(glActiveTextureARB != NULL && glMultiTexCoord4fARB != NULL)
		{
			multitexture = GL_TRUE;
		}
	}
	else 
#endif
	{
		multitexture = GL_FALSE;
	} 


	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);    
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	if(multitexture)
	{
		glActiveTextureARB(GL_TEXTURE1_ARB);

		glDisable(GL_TEXTURE_1D);
		glDisable(GL_TEXTURE_2D);		

		glActiveTextureARB(GL_TEXTURE0_ARB);
	} 

	SetFogEnable(GE_FALSE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

	InitMatrices(ClientWindow.Width, ClientWindow.Height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (!THandle_Startup())
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "OGL_DrvInit:  THandle_Startup failed...\n");
		return GE_FALSE;
	}

	RenderingIsOK = GE_TRUE;

	return GE_TRUE;
}


geBoolean DRIVERCC DrvShutdown(void)
{
	// Tell OpenGL to finish whatever is in the pipe, because we're closing up shop.
	glFinish();

	WindowCleanup();

	RenderingIsOK = GE_FALSE;

	return GE_TRUE;
}


// Should handle a resize and re-InitMatrices here...
geBoolean DRIVERCC DrvUpdateWindow(void)
{

	return	GE_TRUE;
}


geBoolean	DRIVERCC DrvSetActive(geBoolean Active)
{
	RenderingIsOK = Active;
	return	GE_TRUE;
}


geBoolean DRIVERCC SetGamma(float Gamma)
{	
 	GLfloat lut[256];
	GLint	i;
	
	CurrentGamma = Gamma;
	
 	for(i = 0; i < 256; i++)
	{
		lut[i] = (GLfloat)pow(i / 255.0, 1.0 / CurrentGamma);
	}

	glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
	glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 256, lut);
	glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 256, lut);
	glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 256, lut); 

	return GE_TRUE;
}


geBoolean DRIVERCC GetGamma(float *Gamma)
{
	*Gamma = CurrentGamma;
	
	return TRUE;
}


DRV_EngineSettings EngineSettings;

DllExport BOOL DriverHook(DRV_Driver **Driver)
{

	EngineSettings.CanSupportFlags = (DRV_SUPPORT_ALPHA | DRV_SUPPORT_COLORKEY);
	EngineSettings.PreferenceFlags = 0;

	OGLDRV.EngineSettings = &EngineSettings;
    
	*Driver = &OGLDRV;

	// Make sure the error string ptr is not null, or invalid!!!
    OGLDRV.LastErrorStr = LastErrorStr;

	SetLastDrvError(DRV_ERROR_NONE, "OGL:  No error.");

	return GE_TRUE;
}


geBoolean DRIVERCC EnumModes(int32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, 
							 void *Context)
{
	GLint modeCount = 0;

	modeCount = EnumNativeModes(Cb, Context);

	return GE_TRUE;
}


geBoolean DRIVERCC EnumSubDrivers(DRV_ENUM_DRV_CB *Cb, void *Context)
{

	if(!Cb(0, "OpenGL Driver v"DRV_VMAJS"."DRV_VMINS".", Context))
	{
		return GE_TRUE;
	}

	return GE_TRUE;
}


// For now, we keep it simple. In the future, we may want to added paletted support 
// (or maybe not)...and also check for OpenGL extentions, like GL_EXT_abgr, GL_EXT_bgra, etc.
// Note: OpenGL is traditionally RGBA based.  ABGR is used here because of endian-oddness in 
// the naming of Genesis's pixel formats.
// See: (Genesis Engine's) Bitmap/pixelformat.h for more information. 
geRDriver_PixelFormat	PixelFormats[] =
{
	{GE_PIXELFORMAT_32BIT_ABGR,		RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP},
	{GE_PIXELFORMAT_24BIT_RGB,		RDRIVER_PF_2D | RDRIVER_PF_CAN_DO_COLORKEY},
	{GE_PIXELFORMAT_24BIT_RGB,		RDRIVER_PF_LIGHTMAP},
};

#define NUM_PIXEL_FORMATS	(sizeof(PixelFormats)/sizeof(geRDriver_PixelFormat))


geBoolean DRIVERCC EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context)
{
	GLint i;

	for(i = 0; i < NUM_PIXEL_FORMATS; i++)
	{
		if(!Cb(&PixelFormats[i], Context))
		{
			return GE_TRUE;
		}
	}

	return GE_TRUE;
}


void SetLastDrvError(int32 Error, char *ErrorStr)
{
	LastError = Error;
	
	if(ErrorStr)
	{
		strcpy(LastErrorStr, ErrorStr);
	}
	else
	{
		LastErrorStr[0] = 0;
	}

	OGLDRV.LastErrorStr = LastErrorStr;
	OGLDRV.LastError = LastError;
}


// I break the rules here.  There's an implied assumption that screenshot will produce
// a BMP, I use TGA instead.  BMP is so...Windows.  This screws up GTest a bit, as GTest looks
// for files ending in .bmp before writing, and will thus keep overwriting one screenshot.
geBoolean DRIVERCC ScreenShot(const char *Name)
{
	unsigned char tgaHeader[18];
	GLubyte *buffer;
	FILE *fp;
	char *newName;
	int nameLen;

	buffer = (GLubyte *)malloc(sizeof(GLubyte) * ClientWindow.Width * ClientWindow.Height * 3);	

	glFinish();

	glReadPixels(0, 0, ClientWindow.Width, ClientWindow.Height, 
		GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer);
 
	memset(tgaHeader, 0, sizeof(tgaHeader));
	tgaHeader[2] = 2;
	tgaHeader[12] = (unsigned char)ClientWindow.Width;
	tgaHeader[13] = (unsigned char)((unsigned long)ClientWindow.Width >> 8);
	tgaHeader[14] = (unsigned char)ClientWindow.Height;
	tgaHeader[15] = (unsigned char)((unsigned long)ClientWindow.Height >> 8);
	tgaHeader[16] = 24;
 
	// Convert the extention (if one exists) to .tga.  They probably expect a .bmp.
	newName = strdup(Name);

	nameLen = strlen(newName);

	if(nameLen > 3)
	{
		if(newName[nameLen - 4] == '.')
		{
			strcpy(newName + nameLen - 3, "tga");
		}
	}

    fp = fopen(newName, "wb");
    
	free(newName);

	if(fp == NULL) 
	{
		free(buffer);
        return GE_FALSE;
    }
 
    fwrite(tgaHeader, 1, 18, fp);
    fwrite(buffer, 3, ClientWindow.Width * ClientWindow.Height, fp);
    fclose(fp);
 
	free(buffer);
	
	return GE_TRUE;
}

