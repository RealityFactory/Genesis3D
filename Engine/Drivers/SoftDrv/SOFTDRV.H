/****************************************************************************************/
/*  softdrv.h                                                                           */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  header for softdrv                                                    */
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
#ifndef SOFTDRV_H
#define SOFTDRV_H

#include "DCommon.h"
#include "ddraw.h"

#ifdef __cplusplus
extern "C" DRV_Window		ClientWindow;
#else
extern DRV_Window		ClientWindow;
#endif

extern DRV_Driver			SOFTDRV;
extern DRV_CacheInfo		CacheInfo;

enum	VidSurfFlags
{
	SYSTEM				=1,	//store buffer in system
	VIDEO				=2,	//or video ram
	HARDWARE			=4,	//refresh choices
	DMABLT				=8,
	FASTBLT				=16,
	SAFEBLT				=32,
	FLIP				=64,
	DMAPAGELOCKREQUIRED	=128,//pagelock for dma req
	DMAASYNCH			=256,//can do asynch dma
	STRETCHMODE			=512,//stretch to fit
	MODEXMODE			=1024
};

typedef struct VidModeListTag
{
	uint32	width;
	uint32	height;
	uint32	pitch;
	uint32	flags;				//capabilities
	uint32	current;			//caps in use
	uint32	bpp;
}	VidModeList;

typedef struct VidEnumInfoTag
{
	DDDEVICEIDENTIFIER	DeviceInfo;
	DDDEVICEIDENTIFIER	DeviceInfoHost;
	GUID				*DeviceGuid;
	VidModeList			VidModes[128];	//let's be safe
	int					NumVidModes;
	int					CurrentVidMode;
	uint32					bpp;			//bytes per pixel
}	VidEnumInfo;

typedef struct	CPUInfoTag
{
	uint32		MaxCPUIDVal;
	char	VendorString[16];
	char	ProcName[48];
	uint32		ProcType;
	uint32		ProcFamily;
	uint32		ProcModel;
	uint32		ProcStepping;
	uint32		ProcSpeed;
	BOOL	HasRDTSC;
	BOOL	HasMMX;
	BOOL	Has3DNow;
	BOOL	HasFCMOV;
}	CPUInfo;

BOOL DRIVERCC DrvShutdown(void);
void DRIVERCC ErrorBox(char *Str);
void SetLastDrvError(S32 Error, char *ErrorStr);

#ifdef __cplusplus
extern "C" {
#endif

void	ErrorPrintf(char *text, ...);
void	DumpErrorLogToFile(char *fname);

#ifdef __cplusplus
}
#endif

#endif