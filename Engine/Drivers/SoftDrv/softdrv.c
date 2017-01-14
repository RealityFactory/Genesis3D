/****************************************************************************************/
/*  softdrv.c                                                                           */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  Init, mode, and cpu identification code                               */
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
#include <Assert.h>
#include <stdio.h>
#include <mmsystem.h>
#include <math.h>

#include "SoftDrv.h"
#include "System.h"
#include "DCommon.h"
#include "Sal.h"

#include "Register.h"
#include "Scene.h"
#include "Render.h"
#include "dmodes.h"

static	DRV_CacheInfo	CacheInfo;
static	VidEnumInfo		VidInfo[16];
static	VidEnumInfo		*VInfo;

extern	VidModeList		*cmode;
extern	DRV_Window		ClientWindow	={ 0 };
extern	CPUInfo			ProcessorInfo	={ 0 };
extern	int				NumDevices		=0;

S32					LastError;
char				LastErrorStr[200];

BOOL DRIVERCC EnumSubDrivers(DRV_ENUM_DRV_CB *Cb, void *Context);
BOOL DRIVERCC EnumModes(S32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, void *Context);
BOOL DRIVERCC SetDrvRenderState(S32 State, U32 Flag);
BOOL DRIVERCC GetDrvRenderState(S32 State, U32 *Flag);
BOOL DRIVERCC EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context);

typedef unsigned __int64 QWORD;
//#define RDTSC _asm _emit 0fh _asm _emit 031h
#define CPUID _asm _emit 0fh _asm _emit 0a2h

BOOL DRIVERCC	ScreenShot(const char *Name);
static U32		GetCPUIDEAX(U32 funcNum);
static U32		GetCPUIDEDX(U32 funcNum);
static U32		GetCPUIDString(U32 funcNum, char *szId);
static void		GetCPUIDStringAMD(U32 funcNum, char *szId);
//static QWORD	GetRDTSC(void);
//static U32		GetRDTSCOverHead(void);
//static U32		GetProcessorSpeed(void);
static void		GetCPUInfo(void);

extern	__int64	QZBufferPrec	=0;
extern	__int64	QFixedScale		=0;
extern	__int64	QFixedScale16	=0;
extern	__int64	Q128			=0;
extern	double	Magic, MipMagic, MipMagic2;

void	SetupMiscConstants(void)
{
	*(((float *)((&QZBufferPrec)))+1)	=-ZBUFFER_PREC;
	*((float *)(&QZBufferPrec))			=-ZBUFFER_PREC;

	*(((float *)((&QFixedScale16)))+1)	=4096.0f;
	*((float *)(&QFixedScale16))		=4096.0f;

	*(((float *)((&QFixedScale)))+1)	=65536.0f;
	*((float *)(&QFixedScale))			=65536.0f;

	*((float *)&Q128)					=128.0f;
	*(((float *)((&Q128)))+1)			=128.0f;

	Magic		=ldexp(1.5, 60);
	MipMagic	=ldexp(1.5, 52-16);
	MipMagic2	=ldexp(1.5, 52-12);
}


//big chunk of duplicated code to resetup stuff that
//gets nuked when the driver is freed
BOOL DRIVERCC DrvInit(DRV_DriverHook *Hook)
{
	RECT	window_rect;	//for resetting (thanks mike!)
	BOOL	result;

	VInfo		=&VidInfo[Hook->Driver];
	VInfo->bpp	=16;

	GetCPUInfo();

	window_rect.left = -1;
	window_rect.top = -1;
	window_rect.right = -1;
	window_rect.bottom = -1;
		
	SetLastError( ERROR_SUCCESS );

	DoEnumeration(VInfo);
	DoModeEnumeration(VInfo);

	//check for windowed mode
	if(Hook->Mode==VInfo->NumVidModes)
	{
		bWindowed	=TRUE;

		if (!SAL_startup(FALSE))
		{
			SetLastDrvError(DRV_ERROR_INIT_ERROR, "SOFT_DrvInit:  Could not initialize SAL.");
			DumpErrorLogToFile("softdrv.log");
			return FALSE;
		}

		result = GetClientRect(Hook->hWnd, &window_rect);
		if( !result || window_rect.right == -1 || window_rect.bottom == -1 )
		{
			// This means you probably passed in an illegal hwnd.
			int err = GetLastError();
			assert( result );
			DumpErrorLogToFile("softdrv.log");
			return FALSE;
		}

		SAL_set_main_window(Hook->hWnd);
		ClientWindow.Width  = (window_rect.right  - window_rect.left);
		ClientWindow.Height = (window_rect.bottom - window_rect.top);

		assert( ClientWindow.Width > 0 );
		assert( ClientWindow.Height > 0 );

		if(ProcessorInfo.Has3DNow)
		{
			if(!SAL_set_display_mode(ClientWindow.Width,
									ClientWindow.Height,
									32,
									SAL_WINDOW,
									TRUE))
			{
				SetLastDrvError(DRV_ERROR_INIT_ERROR, "SOFT_DrvInit:  Could not set display mode.");
				DumpErrorLogToFile("softdrv.log");
				return	FALSE;
			}
		}
		else
		{
			if(!SAL_set_display_mode(ClientWindow.Width,
									ClientWindow.Height,
									16,
									SAL_WINDOW,
									TRUE))
			{
				SetLastDrvError(DRV_ERROR_INIT_ERROR, "SOFT_DrvInit:  Could not set display mode.");
				DumpErrorLogToFile("softdrv.log");
				return	FALSE;
			}
		}
	}
	else
	{
		if(!DoDDrawInit(Hook->hWnd, VInfo))
		{
			DumpErrorLogToFile("softdrv.log");
			return	FALSE;
		}
		ClientWindow.Width = Hook->Width;
		ClientWindow.Height = Hook->Height;
		if(VInfo->VidModes[Hook->Mode].flags & STRETCHMODE)
		{
			if(VInfo->VidModes[Hook->Mode].width > 640)
			{
//				ClientWindow.Width	=640;
//				ClientWindow.Height	=(((float)ClientWindow.Width)/(float)Hook->Width)*(float)Hook->Height;
			}
		}

		if(!SetDDrawMode(Hook->Mode, VInfo))
		{
			return	FALSE;
		}
		ClientWindow.PixelPitch	=VInfo->VidModes[Hook->Mode].pitch>>2;
		cmode	=&VInfo->VidModes[Hook->Mode];
	}

	if(!SysInit())
	{	// LastDriverError should be set in here...
		DumpErrorLogToFile("softdrv.log");
		return	FALSE;		// so just return false, and assume it's set
	}

	SetLastDrvError(DRV_ERROR_NONE, "SOFT_DrvInit:  No error.");
	SetupMiscConstants();

	return	TRUE;
}

BOOL DRIVERCC DrvShutdown(void)
{
	SysShutdown();

	if(bWindowed)
	{
		SAL_shutdown();
	}
	else
	{
		FreeDDraw(VInfo);
	}

	DumpErrorLogToFile("softdrv.log");

	return	TRUE;
}

void DRIVERCC ErrorBox(char *Str)
{
	DrvShutdown();
	DumpErrorLogToFile("softdrv.log");
}

BOOL DRIVERCC SetGamma(float Gamma)
{
	return TRUE;
}

BOOL DRIVERCC GetGamma(float *Gamma)
{
	assert(Gamma);

	*Gamma = 1.0f;

	return TRUE;
}

geBoolean	DRIVERCC DrvUpdateWindow(void)
{
	return	GE_TRUE;
}

geBoolean DRIVERCC Drv_SetFogEnable(geBoolean Enable, float r, float g, float b, float Start, float End)
{
	Enable,r,g,b,Start,End;
	return GE_FALSE;
}

DRV_Driver SOFTDRV = 
{
	"Software driver. v"DRV_VMAJS"."DRV_VMINS". Copyright 1999, WildTangent Inc.; All Rights Reserved.",
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

	CreateTexture,
	DestroyTexture,

	LockTextureHandle,
	UnLockTextureHandle,

	SetPalette,
	GetPalette,
	SetAlpha,
	GetAlpha,

	THandle_GetInfo,

	BeginScene,
	EndScene,
	BeginWorld,
	EndWorld,
	BeginMeshes,
	EndMeshes,
	BeginModels,
	EndModels,

	RenderGouraudPoly,
	RenderWorldPoly,
	RenderMiscTexturePoly,

	DrawDecal,

	0,0,0,
	
	&CacheInfo,

	ScreenShot,

	SetGamma,
	GetGamma,

	Drv_SetFogEnable,

	NULL,
	NULL,								// Init to NULL, engine SHOULD set this (SetupLightmap)
	NULL
};

DRV_EngineSettings	EngineSettings;

DllExport BOOL DriverHook(DRV_Driver **Driver)
{
	EngineSettings.CanSupportFlags = (DRV_SUPPORT_ALPHA|DRV_SUPPORT_COLORKEY);
	EngineSettings.PreferenceFlags = (DRV_PREFERENCE_NO_MIRRORS | DRV_PREFERENCE_DRAW_WALPHA_IN_BSP);

	SOFTDRV.EngineSettings = &EngineSettings;

	*Driver = &SOFTDRV;

	// Make sure the error string ptr is not null, or invalid!!!
	SOFTDRV.LastErrorStr = LastErrorStr;

	SetLastDrvError(DRV_ERROR_NONE, "SOFT_DRV:  No error.");

	return TRUE;
}

void SetLastDrvError(S32 Error, char *ErrorStr)
{
	LastError = Error;
	
	if (ErrorStr)
	{
		strcpy(LastErrorStr, ErrorStr);
	}
	else
		LastErrorStr[0] = 0;

	SOFTDRV.LastErrorStr = LastErrorStr;
	SOFTDRV.LastError = LastError;
}

BOOL DRIVERCC ScreenShot(const char *Name)
{
	return FALSE;
}

BOOL DRIVERCC EnumSubDrivers(DRV_ENUM_DRV_CB *Cb, void *Context)
{
	int		i;
	char	szTemp[256];

	NumDevices	=0;

	GetCPUInfo();
	if (ProcessorInfo.Has3DNow)
	{
		DoEnumeration(VidInfo);
		for(i=0;i < NumDevices;i++)
		{
			sprintf(szTemp, "Software AMD 3DNow!(tm) v"DRV_VMAJS"."DRV_VMINS"."); //, VidInfo[i].DeviceInfo.szDescription);
			if(!Cb(i, szTemp, Context))
			{
				return	TRUE;
			}
		}
		if(!NumDevices)
		{
			sprintf(szTemp, "Software AMD 3DNow!(tm) v"DRV_VMAJS"."DRV_VMINS".");
			if(!Cb(i, szTemp, Context))
			{
				return	TRUE;
			}
		}
	}
	return	TRUE;
}

BOOL DRIVERCC EnumModes(S32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, void *Context)
{
	int		i;
	RECT	window_rect;	//for resetting (thanks mike!)
	char	szTemp[256];

	window_rect.left = -1;
	window_rect.top = -1;
	window_rect.right = -1;
	window_rect.bottom = -1;
		
	SetLastError( ERROR_SUCCESS );
	VInfo		=&VidInfo[Driver];
	VInfo->bpp	=16;

	GetCPUInfo();
	if (!ProcessorInfo.Has3DNow)
		return TRUE;
		
	if(NumDevices)
	{
		DoModeEnumeration(VInfo);
	}

	for(i=0;i < VInfo->NumVidModes;i++)
	{
		if(VInfo->VidModes[i].flags & MODEXMODE)
		{
			sprintf(szTemp, "%dx%dx%d ModeX", VInfo->VidModes[i].width, VInfo->VidModes[i].height, VInfo->VidModes[i].bpp);
			if(ProcessorInfo.Has3DNow)
			{
				//strcat(szTemp, " 3DNow!");
			}
			Cb(i, szTemp, VInfo->VidModes[i].width, VInfo->VidModes[i].height, Context);
		}
		else if(VInfo->VidModes[i].flags & STRETCHMODE)
		{
			sprintf(szTemp, "%dx%dx%d Stretched", VInfo->VidModes[i].width, VInfo->VidModes[i].height, VInfo->VidModes[i].bpp);
			if(ProcessorInfo.Has3DNow)
			{
				//strcat(szTemp, " 3DNow!");
			}
			Cb(i, szTemp, VInfo->VidModes[i].width, VInfo->VidModes[i].height, Context);
		}
		else
		{
			sprintf(szTemp, "%dx%dx%d", VInfo->VidModes[i].width, VInfo->VidModes[i].height, VInfo->VidModes[i].bpp);
			if(ProcessorInfo.Has3DNow)
			{
				//strcat(szTemp, " 3DNow!");
			}
			Cb(i, szTemp, VInfo->VidModes[i].width, VInfo->VidModes[i].height, Context);
		}
	}
	Cb(i, "Window Mode", -1, -1, Context);

	SetLastError( ERROR_SUCCESS );

	return TRUE;
}

geBoolean DRIVERCC EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context)
{
	geRDriver_PixelFormat	Pixie;

	if(!cmode && !bWindowed)
	{
		//no mode set 
		return	GE_FALSE;
	}
	else
	{
		if(ProcessorInfo.Has3DNow)
		{
			Pixie.PixelFormat	=GE_PIXELFORMAT_8BIT;
			Pixie.Flags			=RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}
			Pixie.Flags			=RDRIVER_PF_2D | RDRIVER_PF_CAN_DO_COLORKEY;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}
			Pixie.Flags			=RDRIVER_PF_3D;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}
			Pixie.Flags			=RDRIVER_PF_3D | RDRIVER_PF_ALPHA;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}
			Pixie.Flags			=RDRIVER_PF_3D | RDRIVER_PF_HAS_ALPHA;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

			Pixie.PixelFormat	=GE_PIXELFORMAT_32BIT_XRGB;
			Pixie.Flags			=RDRIVER_PF_PALETTE;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

			Pixie.PixelFormat	=GE_PIXELFORMAT_32BIT_ARGB;
			Pixie.Flags			=RDRIVER_PF_PALETTE;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

		}
		else
		{
			if(ClientWindow.G_mask == 0x3e0)	//555
			{
				Pixie.PixelFormat	=GE_PIXELFORMAT_16BIT_555_RGB;
			}
			else
			{
				Pixie.PixelFormat	=GE_PIXELFORMAT_16BIT_565_RGB;
			}

			Pixie.Flags			=RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

			Pixie.Flags			=RDRIVER_PF_3D;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

			Pixie.Flags			=RDRIVER_PF_2D;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

			Pixie.Flags			=RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP | RDRIVER_PF_CAN_DO_COLORKEY;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

			Pixie.Flags			=RDRIVER_PF_3D | RDRIVER_PF_CAN_DO_COLORKEY;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}

			Pixie.Flags			=RDRIVER_PF_2D | RDRIVER_PF_CAN_DO_COLORKEY;
			if(!Cb(&Pixie, Context))
			{
				return	GE_TRUE;
			}
		}

		Pixie.PixelFormat	=GE_PIXELFORMAT_24BIT_RGB;
		Pixie.Flags			=RDRIVER_PF_LIGHTMAP;
		if(!Cb(&Pixie, Context))
		{
			return	GE_TRUE;
		}
	}	
	return	TRUE;
}

BOOL DRIVERCC SetDrvRenderState(S32 State, U32 Flag)
{
	return TRUE;
}

BOOL DRIVERCC GetDrvRenderState(S32 State, U32 *Flag)
{
	*Flag = 0;
	return TRUE;
}

int Flag_CPUID = FALSE; 
int Flag_RDTSC = FALSE;


void Test_CPU_bits(void)
{
	Flag_CPUID = FALSE;
	Flag_RDTSC = FALSE;
_asm
    {
	pushad                    // (1) play nice and save everything
    pushfd                    // eax = ebx = extended flags
    pop     eax                              
    mov     ebx,eax

    xor     eax,200000h       // toggle bit 21

    push    eax               // extended flags = eax
	popfd
    xor     eax,ebx           // if bit 21 r/w then eax <> 0

    jz      done              // can't toggle id bit (21) no cpuid here
    mov     eax,1             // get standard features
    mov     Flag_CPUID,eax    // (and set cpuid flag to true)

	CPUID
    test    edx,10h           // is rdtsc available?
    jz      done

    mov     Flag_RDTSC,1
done:
	popad                     // (1) restore everything
    }
}




static U32	GetCPUIDEAX(U32 funcNum)
{
	U32	retval;

	Test_CPU_bits();
	if (Flag_CPUID)
		{
			__try
			{
				_asm
				{
					mov	eax,funcNum
					CPUID
					mov	retval,eax
				}
			}__except(EXCEPTION_EXECUTE_HANDLER)
			{
				retval	=0;
			}
		}
	else
		{
			retval = 0;
		}

	return	retval;
}

static U32	GetCPUIDEDX(U32 funcNum)
{
	U32	retval;

	Test_CPU_bits();
	if (Flag_CPUID)
		{
			__try
			{
				_asm
				{
					mov	eax,funcNum
					CPUID
					mov	retval,edx
				}
			}__except(EXCEPTION_EXECUTE_HANDLER)
			{
				retval	=0;
			}
		}
	else
		{
			retval = 0;
		}
	
	return	retval;
}

static U32	GetCPUIDString(U32 funcNum, char *szId)
{
	U32	retval;

	Test_CPU_bits();
	if (Flag_CPUID)
		{
			__try
			{
				_asm
				{
					mov	eax,funcNum
					CPUID
					mov	retval,eax
					mov	eax,szId
					mov	dword ptr[eax],ebx
					mov	dword ptr[eax+4],edx
					mov	dword ptr[eax+8],ecx
				}
			}__except(EXCEPTION_EXECUTE_HANDLER)
			{
				retval	=0;
			}
		}
	else
		{
			retval = 0;
		}

	return	retval;
}

static void	GetCPUIDStringAMD(U32 funcNum, char *szId)
{
	U32	retval;
	
	Test_CPU_bits();
	if (Flag_CPUID)
		{
			__try
				{
					_asm
					{
						mov eax,funcNum
						CPUID
						mov	retval,eax
						mov	eax,szId
						mov	dword ptr[eax+4],ebx
						mov	dword ptr[eax+8],ecx
						mov	ebx,retval
						mov	dword ptr[eax+12],edx
						mov	dword ptr[eax],ebx
					}
				}__except(EXCEPTION_EXECUTE_HANDLER)
				{
					retval	=0;
				}
		}
	else
		{
			retval = 0;
		}
}

// For out of order processors, the cpuid does serliaization
// On all processors, additional overhead is added
/*
static QWORD	GetRDTSC(void)
{
	QWORD	clock;

	_asm
	{
		push	ebx
		push	ecx
		xor		eax,eax
		CPUID
		RDTSC
		mov		dword ptr clock,eax
		mov		dword ptr clock+4,edx
		xor		eax,eax
		CPUID
		pop		ecx
		pop		ebx
	}
	return	clock;
}
*/

/*
static U32	GetRDTSCOverHead(void)
{
	U32		elap, MinOverhead	=0xffffff;
	QWORD	start;
	int		x;

	for(x=0;x < 50;x++)
	{
		start		=GetRDTSC();
		elap		=(U32)(GetRDTSC() - start);
		MinOverhead =min(MinOverhead, elap);
	}
	return	MinOverhead;
}
*/

/*
static U32	GetProcessorSpeed(void)
{
	QWORD	StartClock, ElapClock;
	U32		StartTime, RetVal, times	=0;
	
	// try to get rid of the variability
	StartClock	=GetRDTSC();
	StartTime	=timeGetTime();

	// this loop should take 1 sec +- 1 ms
	while(timeGetTime() < StartTime + 250);

	ElapClock	=GetRDTSC() - StartClock + 500000;
	
	// try to get rid of the variability
	StartClock	=GetRDTSC();
	StartTime	=timeGetTime();

	// this loop should take 1 sec +- 1 ms
	while(timeGetTime() < StartTime + 1000);

	ElapClock	=GetRDTSC() - StartClock + 500000;
	RetVal		=(U32)(ElapClock/1000000);
	return		RetVal;
}
*/

static	char	buffer[32768];		//this is ... cautious
static	char	buf3[4096];

void ErrorPrintf(char *text, ...)
{
	va_list argptr;

	va_start(argptr,text);
	vsprintf(buf3, text,argptr);
	va_end(argptr);

	strcat(buffer, buf3);
}

void DumpErrorLogToFile(char *fname)
{
	FILE		*f;
	SYSTEMTIME	Time;

	f	=fopen(fname, "a+t");

	if(f)
	{
		GetSystemTime(&Time);

		fprintf(f,"=================================================================\n");
		fprintf(f,"Time: %2i:%2i:%2i\n", Time.wHour, Time.wMinute, Time.wSecond);
		fprintf(f,"Date: %2i-%2i-%4i\n", Time.wMonth, Time.wDay, Time.wYear);

		fwrite(buffer, 1, strlen(buffer), f);
		fclose(f);
	}
}

static void	GetCPUInfo(void)
{
	memset(&ProcessorInfo, 0, sizeof(ProcessorInfo));

	ProcessorInfo.MaxCPUIDVal		=GetCPUIDString(0, ProcessorInfo.VendorString);
	ProcessorInfo.VendorString[13]	=0;
	if(strncmp(ProcessorInfo.VendorString, "AuthenticAMD", 12)==0)
	{
		U32	TypeFlags	=GetCPUIDEAX(0x80000000);
		if(TypeFlags)	//extended functions supported
		{
			TypeFlags	=GetCPUIDEDX(0x80000001);
			GetCPUIDStringAMD(0x80000002, ProcessorInfo.ProcName);
			GetCPUIDStringAMD(0x80000003, ProcessorInfo.ProcName+16);
			GetCPUIDStringAMD(0x80000004, ProcessorInfo.ProcName+32);
			ErrorPrintf("CPU Family:  %s\n", ProcessorInfo.ProcName);
			ProcessorInfo.HasMMX		=TypeFlags & (1<<23);
			ProcessorInfo.Has3DNow		=TypeFlags & (1<<31);
			ProcessorInfo.HasFCMOV		=TypeFlags & (1<<16);
			ProcessorInfo.HasRDTSC		=TypeFlags & (1<<4);
		}
		else
		{
			U32	TypeFlags				=GetCPUIDEDX(0x1);
			ProcessorInfo.HasMMX		=TypeFlags & (1<<23);
			ProcessorInfo.HasFCMOV		=((TypeFlags & (1<<15 | 1))==(1<<15 | 1))? TRUE : FALSE;
			ProcessorInfo.HasRDTSC		=TypeFlags & (1<<4);
		}
		TypeFlags					=GetCPUIDEAX(1);
		ProcessorInfo.ProcType		=(TypeFlags>>12)&0x3;
		ProcessorInfo.ProcFamily	=(TypeFlags>>8)&0xf;
		ProcessorInfo.ProcModel		=(TypeFlags>>4)&0xf;
		ProcessorInfo.ProcStepping	=(TypeFlags)&0x7;
		//ProcessorInfo.ProcSpeed		=GetProcessorSpeed();
	}
	else if(strncmp(ProcessorInfo.VendorString, "GenuineIntel", 12)==0)
	{
		U32	TypeFlags				=GetCPUIDEDX(0x1);
		ProcessorInfo.HasMMX		=TypeFlags & (1<<23);
		ProcessorInfo.HasFCMOV		=((TypeFlags & (1<<15 | 1))==(1<<15 | 1))? TRUE : FALSE;
		ProcessorInfo.HasRDTSC		=TypeFlags & (1<<4);

		TypeFlags					=GetCPUIDEAX(1);
		ProcessorInfo.ProcType		=(TypeFlags>>12)&0x3;
		ProcessorInfo.ProcFamily	=(TypeFlags>>8)&0xf;
		ProcessorInfo.ProcModel		=(TypeFlags>>4)&0xf;
		ProcessorInfo.ProcStepping	=(TypeFlags)&0x7;
		//ProcessorInfo.ProcSpeed		=GetProcessorSpeed();

		ErrorPrintf("CPU Family:  %d\n", ProcessorInfo.ProcFamily);
		ErrorPrintf("CPU Model:  %d\n", ProcessorInfo.ProcModel);
	}
	else
	{
		U32	TypeFlags	=GetCPUIDEAX(0x80000000);
		if(TypeFlags)	//extended functions supported
		{
			TypeFlags	=GetCPUIDEDX(0x80000001);
			GetCPUIDStringAMD(0x80000002, ProcessorInfo.ProcName);
			GetCPUIDStringAMD(0x80000003, ProcessorInfo.ProcName+16);
			GetCPUIDStringAMD(0x80000004, ProcessorInfo.ProcName+32);
			ErrorPrintf("CPU Family:  %s\n", ProcessorInfo.ProcName);
			ProcessorInfo.HasMMX		=TypeFlags & (1<<23);
			ProcessorInfo.Has3DNow		=TypeFlags & (1<<31);
			ProcessorInfo.HasFCMOV		=TypeFlags & (1<<16);
			ProcessorInfo.HasRDTSC		=TypeFlags & (1<<4);
		}
		else
		{
			U32	TypeFlags				=GetCPUIDEDX(0x1);
			ProcessorInfo.HasMMX		=TypeFlags & (1<<23);
			ProcessorInfo.HasFCMOV		=((TypeFlags & (1<<15 | 1))==(1<<15 | 1))? TRUE : FALSE;
			ProcessorInfo.HasRDTSC		=TypeFlags & (1<<4);
		}
		TypeFlags					=GetCPUIDEAX(1);
		ProcessorInfo.ProcType		=(TypeFlags>>12)&0x3;
		ProcessorInfo.ProcFamily	=(TypeFlags>>8)&0xf;
		ProcessorInfo.ProcModel		=(TypeFlags>>4)&0xf;
		ProcessorInfo.ProcStepping	=(TypeFlags)&0x7;
		//ProcessorInfo.ProcSpeed		=GetProcessorSpeed();
	}

	ErrorPrintf("CPU Vendor String:  %s\n", ProcessorInfo.VendorString);
	ErrorPrintf("Processor Speed:  %d\n", ProcessorInfo.ProcSpeed);
	ErrorPrintf("Stepping:  %d\n", ProcessorInfo.ProcStepping);
	if(ProcessorInfo.HasMMX)
	{
		ErrorPrintf("MMX instructions detected\n");
	}
	if(ProcessorInfo.Has3DNow)
	{
		if (VInfo!=NULL)
		{
			VInfo->bpp	=32;
			ErrorPrintf("3DNow instructions detected\n");
		}
	}
	if(ProcessorInfo.HasFCMOV)
	{
		ErrorPrintf("FCMOV feature detected\n");
	}
	if(ProcessorInfo.HasRDTSC)
	{
		ErrorPrintf("Time Stamp Counter feature detected\n");
	}
}
