/****************************************************************************************/
/*  GMain.h                                                                             */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef GMAIN_H
#define GMAIN_H

#include <Windows.h>

#include "GTHandle.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GMAIN_MAX_TMU_SUPPORT						8

typedef struct
{
	uint32		MainRam;
	int32		NumTMU;
	uint32		TmuRam[GMAIN_MAX_TMU_SUPPORT];
} GMain_BoardInfo;

extern GrChipID_t			TMU[3];					// TMU number table

extern DRV_Window			ClientWindow;
extern GrHwConfiguration	GlideHwConfig;
extern GMain_BoardInfo		g_BoardInfo;			// Global board info for current hardware

extern geBoolean			g_FogEnable;
extern float				g_FogR;
extern float				g_FogG;
extern float				g_FogB;

//============================================================================================
//============================================================================================

geBoolean GMain_Startup(DRV_DriverHook *Hook);
void GMain_Shutdown(void);
geBoolean GMain_GetBoardInfo(GMain_BoardInfo *Info);
geBoolean GMain_InitGlideRegisters(void);
geBoolean GMain_ResetAll(void);
geBoolean DRIVERCC GMain_ScreenShot(const char *Name);
geBoolean DRIVERCC GMain_SetFogEnable(geBoolean Enable, float r, float g, float b, float Start, float End);

#ifdef __cplusplus
}
#endif

#endif