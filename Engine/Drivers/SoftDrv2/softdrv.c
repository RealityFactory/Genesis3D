/****************************************************************************************/
/*  SoftDrv.C                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  This is the API layer for the genesis software driver.                */
/*                                                                                      */
/*  Copyright (c)1999, WildTangent, Inc.                             */
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
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#pragma warning(default : 4201 4214 4115; disable : 4514 4244)
#include <stdio.h> 
#include <Assert.h>

#include "SoftDrv.h"
#include "CPUInfo.h"
#include "DCommon.h"
#include "SpanBuffer.h"		
#include "Span.h"
#include "ram.h"

#include "SWTHandle.h"
#include "Display.h"
#include "TRaster.h"
#include "DrawDecal.h"

#ifdef GENESIS_VERSION_2
#include "errorlog.h"
#else
#define geErrorLog_AddString(Error,xx,yy) 
#endif

//#define SOFTDRV_MAX_SPANS 10000
#define SOFTDRV_MAX_AVG_SPANS_PER_LINE (22)	

#define SOFTDRV_DESCRIPTION_LENGTH 256
typedef struct 
{
	char				Description[SOFTDRV_DESCRIPTION_LENGTH];
	Display_Type		DisplayType;
	DisplayModeInfo		*Info;
} SoftDrv_DisplayInfo;

typedef struct SoftDrv
{
	int						RefCount;
	int						DisplayCount;
	int						CurrentDisplayIndex;
	geRDriver_THandle		DrawBuffer;
	geRDriver_THandle		ZBuffer;
	SoftDrv_DisplayInfo		Display[DISPLAY_COUNT];
} SoftDrv;

static SoftDrv SoftDrv_Internals={GE_FALSE};


int32				 RenderMode = RENDER_NONE;
DRV_Window			 ClientWindow	={ 0 };
Display				*SD_Display = NULL;
geBoolean            SD_ProcessorHas3DNow;
geBoolean            SD_ProcessorHasMMX;
geBoolean			 SD_DIBDisplayMode = GE_FALSE;
geBoolean			 SD_Active = FALSE;
DRV_EngineSettings	 SD_EngineSettings=
							{   // to conform to DRV_Driver structure.
								/*CanSupportFlags*/ (DRV_SUPPORT_ALPHA|DRV_SUPPORT_COLORKEY),
								/*PreferenceFlags*/ (DRV_PREFERENCE_NO_MIRRORS | DRV_PREFERENCE_DRAW_WALPHA_IN_BSP)
							};

DRV_CacheInfo		SoftDrv_CacheInfo;

S32					LastError;
char				LastErrorStr[200];

void SoftDrv_LightMapSetupCallback(TRaster_Lightmap *LM);

void SoftDrv_ClearZBuffer(DRV_Window *Window)
{
	int32	ZBSize;

	ZBSize = (Window->Width*Window->Height)<<1;


#pragma message ("clear z buffer to biggest z.  was 0.")
	memset(SoftDrv_Internals.ZBuffer.BitPtr[0], 0xFF, ZBSize);

}


static void SoftDrv_DisplayInfoTable_Destroy( SoftDrv *S )
{
	int i;

	S->RefCount--;
	if (S->RefCount>0)
		return;
	
	for (i=0; i<S->DisplayCount; i++)
		{
			if (S->Display[i].Info != NULL)
				{
					DisplayModeInfo_Destroy( &(S->Display[i].Info) );
					S->Display[i].Info=NULL;
				}
		}
	S->DisplayCount = 0;
}
	
static geBoolean SoftDrv_DisplayInfoTable_Create( SoftDrv *S, geBoolean FillOutModes)
{
	char VersionString[SOFTDRV_DESCRIPTION_LENGTH]="v"DRV_VMAJS"."DRV_VMINS".";
	int i;
	FillOutModes;		// avoid unreference parameter warning
	
	assert( S != NULL );
	if (S->RefCount>0)
		{
			S->RefCount++;
			return GE_TRUE;
		}
	else
		S->RefCount=1;
	
			
	S->DisplayCount = 0;
	for (i=0; i<DISPLAY_COUNT; i++)
		{
			S->Display[i].Info = DisplayModeInfo_Create();
			if (S->Display[i].Info == NULL)
				{
					SoftDrv_DisplayInfoTable_Destroy( S );
					geErrorLog_AddString(GE_ERR_MEMORY_RESOURCE,"SoftDrv_DisplayInfoTableCreate: unable to create table",NULL);
					return GE_FALSE;
				}
			if (Display_GetDisplayInfo(		i,
											S->Display[i].Description, 
											SOFTDRV_DESCRIPTION_LENGTH-strlen(VersionString),
											S->Display[i].Info) == GE_FALSE)
				{
					DisplayModeInfo_Destroy( &(S->Display[i].Info) );
					geErrorLog_AddString(GE_ERR_MEMORY_RESOURCE,"SoftDrv_DisplayInfoTableCreate: problem filling table. (continuing)",NULL);
					S->Display[i].Info=NULL;
				}
			else
				{
					strcat(S->Display[i].Description,VersionString);
					S->Display[i].DisplayType = i;
					S->DisplayCount++;
				}
		}
	return GE_TRUE;
}

geBoolean DRIVERCC SoftDrv_SetActive(geBoolean Active)
{
	SD_Active = Active;
	Display_SetActive(SD_Display,Active);
	return TRUE;
}



geBoolean DRIVERCC SoftDrv_Init(DRV_DriverHook *Hook)
{
	// hook->width, hook->height are ignored...
	uint16	*ZBuffer=NULL;

	#pragma message ("fix to:") //geBoolean DRIVERCC SoftDrv_Init(Hwnd, char *DriverString, int Height, int Width, int BitsPerPixel, uint32 Flags)

	if (SoftDrv_DisplayInfoTable_Create(&(SoftDrv_Internals), GE_TRUE)==GE_FALSE)
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE,"SoftDrv_Init: failed to get mode info",NULL);
			return FALSE;
		}

	if ((Hook->Driver<0) || (Hook->Driver>=SoftDrv_Internals.DisplayCount))
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER,"SoftDrv_Init: bad driver index",NULL);
			SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
			return FALSE;
		}

	
//	VInfo		=&(SoftDrv->VideoModeInfo[Hook->Driver]);
	//VInfo->bpp	=16;

	SD_ProcessorHas3DNow = CPUInfo_TestFor3DNow();
	SD_ProcessorHasMMX   = CPUInfo_TestForMMX();
  
	{
		int Height, Width, BitsPerPixel;
		uint32 Flags;
		
		if (DisplayModeInfo_GetNth(	SoftDrv_Internals.Display[Hook->Driver].Info, 
									Hook->Mode,
									&Width,
									&Height,
									&BitsPerPixel,
									&Flags ) == GE_FALSE)
			{
				geErrorLog_AddString(GE_ERR_BAD_PARAMETER,"SoftDrv_Init: unable to get mode info: bad mode index",NULL);
				SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
				return FALSE;
			}

    SD_Display = Display_Create(	Hook->hWnd,
										SoftDrv_Internals.Display[Hook->Driver].DisplayType,
										Width,
										Height,
										BitsPerPixel,
										Flags);
		if (SD_Display == NULL)
			{
				geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE, "SoftDrv_Init: Could not initialize display",NULL);
				SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
				return	FALSE;
			}						
	
    if( 1 )
		{
			int32 BitsPerPixel;
			Display_Type DisplayType;
			uint32 Flags;			
		
			Display_GetDisplayFormat(	SD_Display, 
										&DisplayType, 
										&(ClientWindow.Width),
										&(ClientWindow.Height),
										&BitsPerPixel,
										&Flags);

		}

	}

	if (SpanBuffer_Create(ClientWindow.Width,ClientWindow.Height, ClientWindow.Height * SOFTDRV_MAX_AVG_SPANS_PER_LINE)==GE_FALSE)
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE, "SoftDrv_Init:  Could not create span buffer",NULL);
			Display_Destroy(&SD_Display);
			SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
			return	FALSE;
		}

	if (Display_GetPixelFormat(	SD_Display,
								//&ClientWindow.PixelPitch,
								&ClientWindow.BytesPerPixel,
								&ClientWindow.R_shift,
								&ClientWindow.R_mask,
								&ClientWindow.R_width,
								&ClientWindow.G_shift,
								&ClientWindow.G_mask,
								&ClientWindow.G_width,
								&ClientWindow.B_shift,
								&ClientWindow.B_mask,
								&ClientWindow.B_width)==GE_FALSE)
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE, "SoftDrv_Init:  Could not get display pixel format",NULL);
			Display_Destroy(&SD_Display);
			SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
			return	FALSE;
		}

  {	
  geSpan_DestinationFormat DestFormat;
  switch( ClientWindow.R_width + ClientWindow.G_width + ClientWindow.B_width )
    {
    case 15: DestFormat = GE_SPAN_DESTINATION_FORMAT_555;
             break;
    case 16: DestFormat = GE_SPAN_DESTINATION_FORMAT_565;
             break;
    default: 
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER,"SoftDrv_Init: unsupported destination format",NULL);
			SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
			return FALSE;
    }

	if (Span_SetOutputMode( DestFormat, GE_SPAN_HARDWARE_INTEL ) == GE_FALSE)
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE,"SoftDrv_Init: unable to set span drawing mode",NULL);
			SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
			return FALSE;
		}
  }


	{
		U32	OldFlags	=0;

		ZBuffer	=(U16 *)geRam_Allocate(ClientWindow.Width * ClientWindow.Height * 2);
		
		if(!ZBuffer)
		{
			geErrorLog_AddString(GE_ERR_MEMORY_RESOURCE,"SoftDrv_Init:  Not enought memory for ZBuffer",NULL);
			Display_Destroy(&SD_Display);
			SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
			return FALSE;
		}

		if(!VirtualProtect((U8 *)ZBuffer,
			(ClientWindow.Width * ClientWindow.Height)*2,
			PAGE_READWRITE | PAGE_NOCACHE,
			&OldFlags))
		{
			// nothing to do on failure...
		}
	}
	
#pragma message ("set up DRV_TLHandle for display and zbuffer")

	
	// assume the window is active:
	SoftDrv_SetActive(GE_TRUE);

	SoftDrv_Internals.DrawBuffer.Active=1;
	SoftDrv_Internals.DrawBuffer.Width  = ClientWindow.Width;
	SoftDrv_Internals.DrawBuffer.Height = ClientWindow.Height;
	SoftDrv_Internals.DrawBuffer.MipLevels = 1;
	SoftDrv_Internals.DrawBuffer.BitPtr[0] = NULL; // not locked yet. (U16 *)(ClientWindow.Buffer);
	SoftDrv_Internals.DrawBuffer.PalHandle = NULL;
	SoftDrv_Internals.DrawBuffer.AlphaHandle = NULL;
	SoftDrv_Internals.DrawBuffer.Flags = 0;

	SoftDrv_Internals.ZBuffer.Active=1;
	SoftDrv_Internals.ZBuffer.Width  = ClientWindow.Width;
	SoftDrv_Internals.ZBuffer.Height = ClientWindow.Height;
	SoftDrv_Internals.ZBuffer.MipLevels = 1;
	SoftDrv_Internals.ZBuffer.BitPtr[0] = (U16 *)(ZBuffer);
	SoftDrv_Internals.ZBuffer.PalHandle = NULL;
	SoftDrv_Internals.ZBuffer.AlphaHandle = NULL;
	SoftDrv_Internals.ZBuffer.Flags = 0;

	TRaster_Setup(32,&(SoftDrv_Internals.DrawBuffer),&(SoftDrv_Internals.ZBuffer),SoftDrv_LightMapSetupCallback);

	return	TRUE;
}

geBoolean DRIVERCC SoftDrv_Shutdown(void)
{
	SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
	
	if (SD_Display)
		Display_Destroy(&SD_Display);
	SD_Display = NULL;

	if(SoftDrv_Internals.ZBuffer.BitPtr[0]!=NULL)
		geRam_Free(SoftDrv_Internals.ZBuffer.BitPtr[0]);
	SoftDrv_Internals.ZBuffer.BitPtr[0]=NULL;
	SpanBuffer_Destroy();
	return	TRUE;
}

geBoolean DRIVERCC SoftDrv_SetGamma(float Gamma)
{
	Gamma;
	return TRUE;
}

geBoolean DRIVERCC SoftDrv_GetGamma(float *Gamma)
{
	assert(Gamma);

	*Gamma = 1.0f;

	return TRUE;
}


geBoolean	DRIVERCC SoftDrv_SetRenderWindowRect(void)
{
	return	GE_TRUE;
}


geBoolean DRIVERCC SoftDrv_BeginScene(geBoolean Clear, geBoolean ClearZ, RECT *pWorldRect)
{

	pWorldRect;		// unused.

	if (RenderMode != RENDER_NONE)
		{
			geErrorLog_AddString(GE_ERR_INTERNAL_RESOURCE,"SoftDrv_BeginScene: still in a render mode",geErrorLog_IntToString(RenderMode));
			return FALSE;
		}

	memset(&SoftDrv_CacheInfo, 0, sizeof(DRV_CacheInfo));


	if (ClearZ)
		{
			SoftDrv_ClearZBuffer(&ClientWindow);
		}
	
	
	if(!Display_Lock(SD_Display,&(ClientWindow.Buffer), &(ClientWindow.PixelPitch)))
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE ,"SoftDrv_BeginScene: failed to lock display buffer",NULL );
			return	FALSE;
		}

	if (Clear)
		{
			if (!Display_Wipe(SD_Display,0))
				{
					geErrorLog_AddString( GE_ERR_SUBSYSTEM_FAILURE,"SoftDrv_BeginScene: failed to wipe display buffer",NULL );
					return	FALSE;
				}
		}
	SoftDrv_Internals.DrawBuffer.BitPtr[0] = (U16 *)(ClientWindow.Buffer);
	return TRUE;
}


geBoolean DRIVERCC SoftDrv_EndScene(void)
{
	assert( RenderMode == RENDER_NONE );


	#if 0
		// useful to examine the zbuffer
		{
			int i;
			unsigned short *b,*z;
			b = (unsigned short *)ClientWindow.Buffer;
			z = (unsigned short *)ZBuffer;
			for (i=ClientWindow.Height * ClientWindow.Width; i>0; i--,b++,z++)
				{
					*b = *z;
				}
		}
	#endif

	if (!Display_Unlock(SD_Display))
		{
			geErrorLog_AddString( GE_ERR_SUBSYSTEM_FAILURE,"SoftDrv_EndScene: failed to unlock display buffer",NULL );
			return	FALSE;
		}
	if (!Display_Blit(SD_Display))
		{
			geErrorLog_AddString( GE_ERR_SUBSYSTEM_FAILURE,"SoftDrv_EndScene: failed to blit display buffer",NULL );
			return	FALSE;
		}
		
	//	SOFTDRV.NumWorldSpans = RegPixels;
	//	SOFTDRV.NumRenderedPolys = RGBPixels;

	return TRUE;
}


geBoolean DRIVERCC SoftDrv_BeginWorld(void)
{
	assert( RenderMode == RENDER_NONE );  // or RENDER_WORLD?
	SpanBuffer_Clear();
 	RenderMode = RENDER_WORLD;
	return TRUE;
}

geBoolean DRIVERCC SoftDrv_EndRenderMode(void)		// world,mesh,models
{
	assert( RenderMode != RENDER_NONE );
	RenderMode = RENDER_NONE;
	return TRUE;
}

geBoolean DRIVERCC SoftDrv_BeginMeshes(void)
{
	assert( RenderMode == RENDER_NONE);
	RenderMode = RENDER_MESHES;
	return TRUE;
}

geBoolean DRIVERCC SoftDrv_BeginModels(void)
{
	assert( RenderMode == RENDER_NONE);
	RenderMode = RENDER_MODELS;
	return TRUE;
}

// this is exported so that this driver source can double as a dll or statically linked.
DllExport geBoolean DriverHook(DRV_Driver **Driver)
{

	*Driver = &SOFTDRV;

	// Make sure the error string ptr is not null, or invalid!!!
	SOFTDRV.LastErrorStr = LastErrorStr;


	return TRUE;
}

GENESISAPI	void * geEngine_SoftwareDriver(void)
{
	return (void *)DriverHook;
}


geBoolean DRIVERCC SoftDrv_ScreenShot(const char *Name)
{
	Name;
	return FALSE;
}

geBoolean DRIVERCC SoftDrv_EnumSubDrivers(DRV_ENUM_DRV_CB *Cb, void *Context)
{
	int i;
	
	if (SoftDrv_DisplayInfoTable_Create(&(SoftDrv_Internals), GE_FALSE)==GE_FALSE)
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE,"SoftDrv_EnumSubDrivers: failed to get mode info",NULL);
			return FALSE;
		}

	for (i=0; i<SoftDrv_Internals.DisplayCount; i++)
		{
			if(!Cb(i, SoftDrv_Internals.Display[i].Description, Context))
			{
				break;
			}
		}

	SoftDrv_DisplayInfoTable_Destroy(&SoftDrv_Internals);
	return	TRUE;
}

	
geBoolean DRIVERCC SoftDrv_EnumModes(S32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, void *Context)
{
	int		i,Count;
	DriverName;		// avoid unused parameter;

	if (SoftDrv_DisplayInfoTable_Create(&(SoftDrv_Internals), GE_TRUE)==GE_FALSE)
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE,"SoftDrv_EnumModes: failed to get mode info",NULL);
			return FALSE;
		}
	
	if ((Driver < 0) || (Driver>=SoftDrv_Internals.DisplayCount))
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER,"SoftDrv_EnumModes: bad driver index",NULL);
			SoftDrv_DisplayInfoTable_Destroy(&(SoftDrv_Internals));
			return FALSE;
		}
	

	Count = DisplayModeInfo_GetModeCount( SoftDrv_Internals.Display[Driver].Info );

	for (i=0; i<Count; i++)
		{
			int Width,Height,BitsPerPixel;
			uint32 Flags;

			if (DisplayModeInfo_GetNth(SoftDrv_Internals.Display[Driver].Info,i,
									&Width,&Height,&BitsPerPixel,&Flags) != GE_FALSE)
				{
					char Description[256];
					if (Width<0 && Height<0)
						{
							sprintf(Description,"Window Mode");
						}
					else
						{
							sprintf(Description,"%dx%dx%d",Width, Height, BitsPerPixel);
						}
					if(Flags & MODEXMODE)
						strcat(Description," ModeX");
					if (SD_ProcessorHas3DNow)
						strcat(Description," 3DNow!");
		
					Cb(i,Description,Width,Height,Context);
				}
		}
	SoftDrv_DisplayInfoTable_Destroy(&SoftDrv_Internals);
	return TRUE;
}

geROP SoftDrv_GouraudFlagsToRop[16] = 
{
	GE_ROP_LSHADE_ZTESTSET,			//		ZCHK	ZWRITE
	GE_ROP_LSHADE_AFLAT_ZTESTSET,	//alpha	ZCHK	ZWRITE
	GE_ROP_LSHADE_ZTESTSET,			//		ZCHK	ZWRITE
	GE_ROP_LSHADE_AFLAT_ZTESTSET,	//alpha	ZCHK	ZWRITE
	GE_ROP_LSHADE_ZSET,				//		nozchk	ZWRITE
	GE_ROP_LSHADE_AFLAT_ZSET,		//alpha nozchk	ZWRITE
	GE_ROP_LSHADE_ZSET,				//		nozchk	ZWRITE
	GE_ROP_LSHADE_AFLAT_ZSET,		//alpha nozchk	ZWRITE
	GE_ROP_LSHADE_ZTEST,			//		ZCHK	nozwrite
	GE_ROP_LSHADE_AFLAT_ZTEST,		//alpha	ZCHK	nozwrite
	GE_ROP_LSHADE_ZTEST,			//		ZCHK	nozwrite
	GE_ROP_LSHADE_AFLAT_ZTEST,		//alpha	ZCHK	nozwrite
	GE_ROP_LSHADE,					//		nozchk  nozwrite
	GE_ROP_LSHADE_AFLAT,			//alpha nozchk  nozwrite
	GE_ROP_LSHADE,					//		nozchk	nozwrite
	GE_ROP_LSHADE_AFLAT,			//alpha	nozchk	nozwrite
};

geROP SoftDrv_MiscFlagsToRop[16][2] = 
{
	{GE_ROP_TMAP_LSHADE_ZTESTSET,			GE_ROP_TMAP_LSHADE_AMAP_ZTESTSET},		//		ZCHK	ZWRITE
	{GE_ROP_TMAP_LSHADE_AFLAT_ZTESTSET,		GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTESTSET},//alpha	ZCHK	ZWRITE
	{GE_ROP_TMAP_LSHADE_ZTESTSET,			GE_ROP_TMAP_LSHADE_AMAP_ZTESTSET},		//		ZCHK	ZWRITE
	{GE_ROP_TMAP_LSHADE_AFLAT_ZTESTSET,		GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTESTSET},//alpha	ZCHK	ZWRITE
	{GE_ROP_TMAP_LSHADE_ZSET,				GE_ROP_TMAP_LSHADE_AMAP_ZSET},			//		nozchk	ZWRITE
	{GE_ROP_TMAP_LSHADE_AFLAT_ZSET,			GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZSET},	//alpha nozchk	ZWRITE
	{GE_ROP_TMAP_LSHADE_ZSET,				GE_ROP_TMAP_LSHADE_AMAP_ZSET},			//		nozchk	ZWRITE
	{GE_ROP_TMAP_LSHADE_AFLAT_ZSET,			GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZSET},	//alpha nozchk	ZWRITE
	{GE_ROP_TMAP_LSHADE_ZTEST,				GE_ROP_TMAP_LSHADE_AMAP_ZTEST},			//		ZCHK	nozwrite
	{GE_ROP_TMAP_LSHADE_AFLAT_ZTEST,		GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTEST},	//alpha	ZCHK	nozwrite
	{GE_ROP_TMAP_LSHADE_ZTEST,				GE_ROP_TMAP_LSHADE_AMAP_ZTEST},			//		ZCHK	nozwrite
	{GE_ROP_TMAP_LSHADE_AFLAT_ZTEST,		GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTEST},	//alpha	ZCHK	nozwrite
	{GE_ROP_TMAP_LSHADE,					GE_ROP_TMAP_LSHADE_AMAP},				//		nozchk  nozwrite
	{GE_ROP_TMAP_LSHADE_AFLAT,				GE_ROP_TMAP_LSHADE_AMAP_AFLAT},			//alpha nozchk  nozwrite
	{GE_ROP_TMAP_LSHADE,					GE_ROP_TMAP_LSHADE_AMAP},				//		nozchk	nozwrite
	{GE_ROP_TMAP_LSHADE_AFLAT,				GE_ROP_TMAP_LSHADE_AMAP_AFLAT},			//alpha	nozchk	nozwrite
};

static int SoftDrv_ComputeMipLevel(
	const DRV_TLVertex 	*Pnts,	
	float ScaleU,float ScaleV,
	int MipCount,
	int NumPoints)
{
	float	du, dv, dx, dy, MipScale;
	int MipLevel;
	int i;

	
	MipScale= 999999.999f;
	for (i=0; i<NumPoints; i++)
	{
		float MipScaleT;
		int Nexti=  (i+1)%NumPoints;
		du	=Pnts[Nexti].u - Pnts[i].u;
		dv	=Pnts[Nexti].v - Pnts[i].v;
		dx	=Pnts[Nexti].x - Pnts[i].x;
		dy	=Pnts[Nexti].y - Pnts[i].y;

		du	/= ScaleU;
		dv	/= ScaleV;

		MipScaleT	=((du*du)+(dv*dv)) / ((dx*dx)+(dy*dy));
		if (MipScaleT < MipScale)
			MipScale = MipScaleT;

	}

	if(MipScale <= 3) //5)		// 2, 6, 12
		{
			MipLevel	=0;
		}
	else if(MipScale <= 15)   //20)
		{
			MipLevel	=1;
		}
	else if(MipScale <= 35)    // 45)
		{
			MipLevel	=2;
		}
	else
		{
			MipLevel	=3;
		}
	if(MipLevel >= MipCount)
		{
			MipLevel	=MipCount-1;
		}
	return MipLevel;
}
	

geBoolean DRIVERCC SoftDrv_RenderGouraudPoly(DRV_TLVertex *Pnts, S32 NumPoints, U32 Flags)
{
	int		i;
	geROP	ROP;
	
	if(!SD_Active)
	{
		return	GE_TRUE;
	}

	assert(Pnts != NULL);
	assert(NumPoints > 2);

	ROP = SoftDrv_GouraudFlagsToRop[Flags & 0xF];
	for(i=0;i < NumPoints-2;i++)
		{
			DRV_TLVertex Pnts2[3];
			
			if (  (((Pnts[i+1].x-Pnts[0].x) * (Pnts[i+2].y-Pnts[0].y)) - ((Pnts[i+1].y-Pnts[0].y)*(Pnts[i+2].x-Pnts[0].x)))<0.0f)
				{
					Pnts2[0] = Pnts[i+2];
					Pnts2[1] = Pnts[i+1];
					Pnts2[2] = Pnts[0];
				}
			else
				{
					Pnts2[0] = Pnts[0];
					Pnts2[1] = Pnts[i+1];
					Pnts2[2] = Pnts[i+2];
				}
			Pnts2[0].a = Pnts[0].a;
			#ifdef GENESIS_VERSION_2
				#pragma message ("temporary:")
				if( Pnts2[0].x < 0 )	Pnts2[0].x = 0 ;
				if( Pnts2[1].x < 0 )	Pnts2[1].x = 0 ;
				if( Pnts2[2].x < 0 )	Pnts2[2].x = 0 ;
				if( Pnts2[0].y < 0 )	Pnts2[0].y = 0 ;
				if( Pnts2[1].y < 0 )	Pnts2[1].y = 0 ;
				if( Pnts2[2].y < 0 )	Pnts2[2].y = 0 ;

				if( Pnts2[0].x >= ClientWindow.Width )	Pnts2[0].x = ClientWindow.Width-1 ;
				if( Pnts2[1].x >= ClientWindow.Width )	Pnts2[1].x = ClientWindow.Width-1 ;
				if( Pnts2[2].x >= ClientWindow.Width )	Pnts2[2].x = ClientWindow.Width-1 ;
				if( Pnts2[0].y >= ClientWindow.Height )	Pnts2[0].y = ClientWindow.Height-1 ;
				if( Pnts2[1].y >= ClientWindow.Height )	Pnts2[1].y = ClientWindow.Height-1 ;
				if( Pnts2[2].y >= ClientWindow.Height )	Pnts2[2].y = ClientWindow.Height-1 ;
			#else
				assert( Pnts2[0].x >= 0 ) ;
				assert( Pnts2[1].x >= 0 ) ;
				assert( Pnts2[2].x >= 0 ) ;
				assert( Pnts2[0].y >= 0 ) ;
				assert( Pnts2[1].y >= 0 ) ;
				assert( Pnts2[2].y >= 0 ) ;

				assert( Pnts2[0].x < ClientWindow.Width ) ;
				assert( Pnts2[1].x < ClientWindow.Width ) ;
				assert( Pnts2[2].x < ClientWindow.Width ) ;
				assert( Pnts2[0].y < ClientWindow.Height ) ;
				assert( Pnts2[1].y < ClientWindow.Height ) ;
				assert( Pnts2[2].y < ClientWindow.Height ) ;
			#endif


			TRaster_Rasterize( ROP ,NULL, 0, Pnts2 );
		}


 	return GE_TRUE;
}

	// Clean this up:
DRV_LInfo *SoftDrv_TempLInfo;
DRV_TexInfo *SoftDrv_TempTexInfo;

void SoftDrv_LightMapSetupCallback(TRaster_Lightmap *LM)
{
	geBoolean Dynamic;
	SOFTDRV.SetupLightmap(SoftDrv_TempLInfo, &Dynamic);
	#pragma message ("SetupLightmap callback: can it fail?")

	{
		float MipScale;
		float ShiftU,ShiftV;
		float ScaleU,ScaleV;
		float LightMapShiftU,LightMapShiftV;
	
		ShiftU = SoftDrv_TempTexInfo->ShiftU;
		ShiftV = SoftDrv_TempTexInfo->ShiftV;
		MipScale = (float)( 1<<LM->MipIndex);
		ScaleU = (1.0f/SoftDrv_TempTexInfo->DrawScaleU);
		ScaleV = (1.0f/SoftDrv_TempTexInfo->DrawScaleV);
		
		LightMapShiftU = ((float)(SoftDrv_TempLInfo->MinU));// - 8.0f?;
		LightMapShiftV = ((float)(SoftDrv_TempLInfo->MinV));// - 8.0f?;

		LM->LightMapShiftU = (ShiftU+LightMapShiftU*ScaleU)/MipScale;
		LM->LightMapScaleU = (1.0f/(16.0f * /*LogSize? */ ScaleU )) * MipScale;
		
		LM->LightMapShiftV = (ShiftV+LightMapShiftV*ScaleV)/MipScale;
		LM->LightMapScaleV = (1.0f/(16.0f * /*LogSize? */ ScaleV )) *MipScale;
		

		LM->BitPtr = (unsigned short *)SoftDrv_TempLInfo->RGBLight[0];
		LM->Height = SoftDrv_TempLInfo->Height;
		LM->Width  = SoftDrv_TempLInfo->Width;
	}
		

}



geBoolean DRIVERCC SoftDrv_RenderWorldPoly(	DRV_TLVertex		*Pnts, 
								S32					 NumPoints, 
								geRDriver_THandle	*THandle, 
								DRV_TexInfo			*TexInfo, 
								DRV_LInfo			*LInfo, 
								U32					 Flags)
{
	int32	i;
	geROP	ROP;
	int MipLevel;

	if(!SD_Active)
	{
		return	GE_TRUE;
	}

	// figure out which rop.  Based on lightmap, pixel format, alpha, and RenderMode.  Ick.
	if (RenderMode==RENDER_WORLD )//&& (THandle->PixelFormat.PixelFormat !=GE_PIXELFORMAT_16BIT_4444_ARGB))
		{
			// always z set but not test.
			assert( !(Flags & DRV_RENDER_ALPHA) );
			assert( !(THandle->PixelFormat.PixelFormat ==GE_PIXELFORMAT_16BIT_4444_ARGB));

			if (LInfo)
				{
					ROP = GE_ROP_TMAP_LMAP_ZSET_SBUF;
				}
			else
				{
					ROP = GE_ROP_TMAP_LSHADE_ZSET_SBUF;	
				}
		}
	else
		{
			if (LInfo)
				{
					if (THandle->PixelFormat.PixelFormat ==GE_PIXELFORMAT_16BIT_4444_ARGB)
						{
							ROP = GE_ROP_TMAP_LMAP_AMAP_ZTESTSET;
						}
					else if (Flags & DRV_RENDER_ALPHA)
						{
							ROP = GE_ROP_TMAP_LMAP_AFLAT_ZTESTSET;
						}
					else 
						{	
							// assert( Pnts.a == 255.0f );
							ROP = GE_ROP_TMAP_LMAP_ZTESTSET;
						}
				}
			else
				{
					if (THandle->PixelFormat.PixelFormat ==GE_PIXELFORMAT_16BIT_4444_ARGB)
						{
							ROP = GE_ROP_TMAP_LSHADE_AMAP_ZTESTSET;	
						}
					else if (Flags & DRV_RENDER_ALPHA)
						{
							ROP = GE_ROP_TMAP_LSHADE_AFLAT_ZTESTSET;
						}
					else 
						{	
							// assert( Pnts.a == 255.0f );
							ROP = GE_ROP_TMAP_LSHADE_ZTESTSET;
						}
				}
		}

	assert(THandle != NULL);
	assert(Pnts != NULL);
	assert(NumPoints > 2);
	{
		DRV_TLVertex Pnts2[3];
		float OOW,OOH;
		float ShiftU,ShiftV,ScaleU,ScaleV;
		// this scaling work can be done once at texture setup time
		ShiftU = TexInfo->ShiftU;
		ShiftV = TexInfo->ShiftV;
		ScaleU = 1.0f/TexInfo->DrawScaleU;
		ScaleV = 1.0f/TexInfo->DrawScaleV;
		OOW = 1.0f / (float)THandle->Width;
		OOH = 1.0f / (float)THandle->Height;
		
		SoftDrv_TempTexInfo = TexInfo;
		SoftDrv_TempLInfo = LInfo;

		Pnts2[0] = Pnts[0];
		Pnts2[0].u = (Pnts2[0].u*ScaleU+ShiftU)*OOW;
		Pnts2[0].v = (Pnts2[0].v*ScaleV+ShiftV)*OOH;

		MipLevel = SoftDrv_ComputeMipLevel(Pnts,TexInfo->DrawScaleU,TexInfo->DrawScaleV,THandle->MipLevels,NumPoints);

		for(i=0;i < NumPoints-2;i++)
			{
				// these are all wound the same way (clockwise)
				Pnts2[1] = Pnts[i+1];
				Pnts2[2] = Pnts[i+2];
				Pnts2[1].u = (Pnts2[1].u*ScaleU+ShiftU)*OOW;
				Pnts2[1].v = (Pnts2[1].v*ScaleV+ShiftV)*OOH;
				Pnts2[2].u = (Pnts2[2].u*ScaleU+ShiftU)*OOW;
				Pnts2[2].v = (Pnts2[2].v*ScaleV+ShiftV)*OOH;
				#ifdef GENESIS_VERSION_2
					#pragma message ("temporary:")
					if( Pnts2[0].x < 0 )	Pnts2[0].x = 0 ;
					if( Pnts2[1].x < 0 )	Pnts2[1].x = 0 ;
					if( Pnts2[2].x < 0 )	Pnts2[2].x = 0 ;
					if( Pnts2[0].y < 0 )	Pnts2[0].y = 0 ;
					if( Pnts2[1].y < 0 )	Pnts2[1].y = 0 ;
					if( Pnts2[2].y < 0 )	Pnts2[2].y = 0 ;

					if( Pnts2[0].x >= ClientWindow.Width )	Pnts2[0].x = ClientWindow.Width-1 ;
					if( Pnts2[1].x >= ClientWindow.Width )	Pnts2[1].x = ClientWindow.Width-1 ;
					if( Pnts2[2].x >= ClientWindow.Width )	Pnts2[2].x = ClientWindow.Width-1 ;
					if( Pnts2[0].y >= ClientWindow.Height )	Pnts2[0].y = ClientWindow.Height-1 ;
					if( Pnts2[1].y >= ClientWindow.Height )	Pnts2[1].y = ClientWindow.Height-1 ;
					if( Pnts2[2].y >= ClientWindow.Height )	Pnts2[2].y = ClientWindow.Height-1 ;
				#else
					assert( Pnts2[0].x >= 0 ) ;
					assert( Pnts2[1].x >= 0 ) ;
					assert( Pnts2[2].x >= 0 ) ;
					assert( Pnts2[0].y >= 0 ) ;
					assert( Pnts2[1].y >= 0 ) ;
					assert( Pnts2[2].y >= 0 ) ;

					assert( Pnts2[0].x < ClientWindow.Width ) ;
					assert( Pnts2[1].x < ClientWindow.Width ) ;
					assert( Pnts2[2].x < ClientWindow.Width ) ;
					assert( Pnts2[0].y < ClientWindow.Height ) ;
					assert( Pnts2[1].y < ClientWindow.Height ) ;
					assert( Pnts2[2].y < ClientWindow.Height ) ;
				#endif
				
				TRaster_Rasterize(  ROP,THandle, MipLevel, Pnts2 );
			}
	}
 	return GE_TRUE;
}

geBoolean DRIVERCC SoftDrv_RenderMiscTexturePoly(DRV_TLVertex *Pnts, S32 NumPoints, geRDriver_THandle *THandle, U32 Flags)
{
	int		i;
	geROP	ROP;
	int MipLevel;

	if(!SD_Active)
	{
		return	GE_TRUE;
	}

	assert(Pnts != NULL);
	assert(NumPoints > 2);
	ROP = SoftDrv_MiscFlagsToRop[Flags & 0xF][(THandle->PixelFormat.PixelFormat==GE_PIXELFORMAT_16BIT_4444_ARGB)?1:0];
	// message ("Automatically make a 4444 from other formats?")

	for(i=0;i < NumPoints-2;i++)
		{
			DRV_TLVertex Pnts2[3];

			if (  (((Pnts[i+1].x-Pnts[0].x) * (Pnts[i+2].y-Pnts[0].y)) - ((Pnts[i+1].y-Pnts[0].y)*(Pnts[i+2].x-Pnts[0].x)))<0.0f)
				{
					Pnts2[0] = Pnts[i+2];
					Pnts2[1] = Pnts[i+1];
					Pnts2[2] = Pnts[0];
				}
			else
				{
					Pnts2[0] = Pnts[0];
					Pnts2[1] = Pnts[i+1];
					Pnts2[2] = Pnts[i+2];
				}
			Pnts2[0].a = Pnts[0].a;
			#ifdef GENESIS_VERSION_2
				#pragma message ("temporary:")
				if( Pnts2[0].x < 0 )	Pnts2[0].x = 0 ;
				if( Pnts2[1].x < 0 )	Pnts2[1].x = 0 ;
				if( Pnts2[2].x < 0 )	Pnts2[2].x = 0 ;
				if( Pnts2[0].y < 0 )	Pnts2[0].y = 0 ;
				if( Pnts2[1].y < 0 )	Pnts2[1].y = 0 ;
				if( Pnts2[2].y < 0 )	Pnts2[2].y = 0 ;

				if( Pnts2[0].x >= ClientWindow.Width )	Pnts2[0].x = ClientWindow.Width-1 ;
				if( Pnts2[1].x >= ClientWindow.Width )	Pnts2[1].x = ClientWindow.Width-1 ;
				if( Pnts2[2].x >= ClientWindow.Width )	Pnts2[2].x = ClientWindow.Width-1 ;
				if( Pnts2[0].y >= ClientWindow.Height )	Pnts2[0].y = ClientWindow.Height-1 ;
				if( Pnts2[1].y >= ClientWindow.Height )	Pnts2[1].y = ClientWindow.Height-1 ;
				if( Pnts2[2].y >= ClientWindow.Height )	Pnts2[2].y = ClientWindow.Height-1 ;
			#else
				assert( Pnts2[0].x >= 0 ) ;
				assert( Pnts2[1].x >= 0 ) ;
				assert( Pnts2[2].x >= 0 ) ;
				assert( Pnts2[0].y >= 0 ) ;
				assert( Pnts2[1].y >= 0 ) ;
				assert( Pnts2[2].y >= 0 ) ;

				assert( Pnts2[0].x < ClientWindow.Width ) ;
				assert( Pnts2[1].x < ClientWindow.Width ) ;
				assert( Pnts2[2].x < ClientWindow.Width ) ;
				assert( Pnts2[0].y < ClientWindow.Height ) ;
				assert( Pnts2[1].y < ClientWindow.Height ) ;
				assert( Pnts2[2].y < ClientWindow.Height ) ;
			#endif

			
			if (THandle->MipLevels>1)
				MipLevel = SoftDrv_ComputeMipLevel(Pnts,1.0f,1.0f,THandle->MipLevels,NumPoints);
			else
				MipLevel =0;
			TRaster_Rasterize(  ROP,THandle,MipLevel, Pnts2 );
		}


 	return GE_TRUE;
}


geBoolean	DRIVERCC	SoftDrv_ResetAll(void)
{
	return SWTHandle_FreeAllTextureHandles();
}

geBoolean DRIVERCC SoftDrv_SetFogEnable(geBoolean Enable, float r, float g, float b, float Start, float End)
{
	Enable,r,g,b,Start,End;
	return GE_FALSE;
}


DRV_Driver SOFTDRV = 
{
	"Software driver. v"DRV_VMAJS"."DRV_VMINS". Copyright 1999, WildTangent, Inc.; All Rights Reserved.",
	DRV_VERSION_MAJOR,
	DRV_VERSION_MINOR,

	DRV_ERROR_NONE,
	NULL,
	
	SoftDrv_EnumSubDrivers,
	SoftDrv_EnumModes,
	SWTHandle_EnumPixelFormats,

	SoftDrv_Init,
	SoftDrv_Shutdown,
	SoftDrv_ResetAll,
	SoftDrv_SetRenderWindowRect,
	SoftDrv_SetActive,

	SWTHandle_CreateTexture,
	SWTHandle_DestroyTexture,

	SWTHandle_LockTextureHandle,
	SWTHandle_UnLockTextureHandle,

	SWTHandle_SetPalette,
	SWTHandle_GetPalette,
	SWTHandle_SetAlpha,
	SWTHandle_GetAlpha,

	SWTHandle_GetInfo,

	SoftDrv_BeginScene,
	SoftDrv_EndScene,

	SoftDrv_BeginWorld,
	SoftDrv_EndRenderMode,
	SoftDrv_BeginMeshes,
	SoftDrv_EndRenderMode,
	SoftDrv_BeginModels,
	SoftDrv_EndRenderMode,

	SoftDrv_RenderGouraudPoly,
	SoftDrv_RenderWorldPoly,
	SoftDrv_RenderMiscTexturePoly,

	DrawDecal,

	0,0,0,
	
	&SoftDrv_CacheInfo,

	SoftDrv_ScreenShot,

	SoftDrv_SetGamma,
	SoftDrv_GetGamma,
	SoftDrv_SetFogEnable,

	&SD_EngineSettings,
	NULL,								// Init to NULL, engine SHOULD set this (SetupLightmap)
	NULL
};



