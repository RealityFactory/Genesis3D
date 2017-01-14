/****************************************************************************************/
/*  scene.h                                                                             */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  scene header                                                          */
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
#ifndef SCENE_H
#define SCENE_H

#include <Windows.H>

#include "DCommon.h"

#define RENDER_NONE			0
#define RENDER_WORLD		1
#define RENDER_MESHES		2
#define RENDER_MODELS		3

extern int32	RenderMode;

BOOL DRIVERCC BeginScene(BOOL Clear, BOOL ClearZ, RECT *WorldRect);
BOOL DRIVERCC EndScene(void);
BOOL DRIVERCC BeginWorld(void);
BOOL DRIVERCC EndWorld(void);
BOOL DRIVERCC BeginMeshes(void);
BOOL DRIVERCC EndMeshes(void);
BOOL DRIVERCC BeginModels(void);
BOOL DRIVERCC EndModels(void);

#endif

