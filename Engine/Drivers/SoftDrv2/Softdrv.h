/****************************************************************************************/
/*  SoftDrv.H                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  This is the API layer for the genesis software driver.                */
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
#include "Display.h"


#ifdef __cplusplus
extern "C" {
#endif

extern DRV_Window			 ClientWindow;

extern Display				*SD_Display;
extern geBoolean             SD_ProcessorHas3DNow;
extern geBoolean             SD_ProcessorHasMMX;
extern geBoolean			 SD_DIBDisplayMode;
extern geBoolean			 SD_Active;
extern DRV_Driver			 SOFTDRV;
extern int32				 RenderMode;

#ifdef __cplusplus
}
#endif

#endif