/****************************************************************************************/
/*  GE.c                                                                                */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Old interface to engine (REMOVE???)                                    */
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
#include <Math.h>
#include <Assert.h>

#include "Genesis.H"
#include "System.h"
#include "Ram.h"

//#include "Sound.h"
//#include "Sound3d.h"

#include "World.h"
#include "Surface.h"
//#include "Camera.h"
#include "Light.h"
#include "Plane.h"
#include "Entities.h"
#include "Trace.h"
#include "User.h"
//#include "Host.h"
#include "Motion.h"
#include "Vis.h"

//=====================================================================================
//	local static globals
//=====================================================================================
static	geEngine			*CEngine;				// The current engine object
static char *geTag="Eclipse!";

//=====================================================================================
//	local static function prototypes
//=====================================================================================
geBoolean geEngine_SetWorld(geEngine *Engine, geWorld *World);

//=====================================================================================
//	Engine
//=====================================================================================

//=====================================================================================
//	geEngine_CreateWithVersion
//=====================================================================================
GENESISAPI geEngine *geEngine_CreateWithVersion(HWND hWnd, const char *AppName, const char *DriverDirectory, uint32 Version)
{
	assert(AppName != NULL);

	//
	// Initialize the engine level resources
	//
	return Sys_EngineCreate(hWnd, AppName, DriverDirectory, Version);
}

//=====================================================================================
//	geEngine_Free
//=====================================================================================
GENESISAPI void geEngine_Free(geEngine *Engine)
{
	Sys_EngineFree(Engine);
}

//=====================================================================================
//	geEngine_FillRect
//=====================================================================================
GENESISAPI void		geEngine_FillRect(geEngine *Engine, const GE_Rect *Rect, const GE_RGBA *Color)
{
	User_EngineFillRect(Engine, Rect, Color);
}

//=====================================================================================
//	GE_ScreenShot
//=====================================================================================
GENESISAPI geBoolean geEngine_ScreenShot(geEngine *Engine, const char *FileName)
{
	assert(Engine);

	return Engine->DriverInfo.RDriver->ScreenShot(FileName);
}

//=====================================================================================
//	geEngine_EnabledFrameRateCounter
//=====================================================================================
GENESISAPI void	geEngine_EnableFrameRateCounter(geEngine *Engine, geBoolean Enabled)
{
	assert(Engine);
	Engine->DisplayFrameRateCounter = Enabled;
}

//=====================================================================================
//	Sound
//=====================================================================================
//========================================================================================
//	Camera
//========================================================================================

#ifdef	MESHES
//========================================================================================
//	geWorld_CreateMesh
//	Create a mesh definition object
//========================================================================================
GENESISAPI geMesh_Def *geWorld_CreateMesh(geWorld *World, const char *BitmapPath, const char *FileName)
{
	assert(World != NULL);
	assert(FileName != NULL);

	return (geMesh_Def*)Sys_WorldCreateMesh(World, BitmapPath, FileName);
}

//========================================================================================
//	geWorld_FreeMesh
//========================================================================================
GENESISAPI void geWorld_FreeMesh(geWorld *World, geMesh_Def *MeshDef)
{
	assert(World != NULL);
	assert(MeshDef != NULL);

	Sys_WorldFreeMesh(World,(Mesh_MeshDef*)MeshDef);
}

//========================================================================================
//	geWorld_AddMesh
//========================================================================================
GENESISAPI geMesh *geWorld_AddMesh(geWorld *World, geMesh_Def *MeshDef, uint32 Flags, uint32 UserFlags)
{
	return (geMesh*)Mesh_WorldAddMesh(World,(Mesh_MeshDef*)MeshDef, Flags, UserFlags);
}

//========================================================================================
//	geWorld_RemoveMesh
//========================================================================================
GENESISAPI void geWorld_RemoveMesh(geWorld *World, geMesh *Mesh)
{
	assert(World != NULL);
	assert(Mesh!= NULL);

	Mesh_WorldRemoveMesh(World, (Mesh_RenderQ*)Mesh);
}

//========================================================================================
//	geWorld_SetMeshXForm
//========================================================================================
GENESISAPI geBoolean geWorld_SetMeshXForm(geWorld *World, geMesh *Mesh, const geXForm3d *XForm)
{
	assert(World != NULL);
	assert(Mesh!= NULL);
	assert(XForm != NULL);

	return Mesh_SetMeshXForm((Mesh_RenderQ*)Mesh, XForm);
}

//========================================================================================
//	geWorld_SetMeshFrame
//========================================================================================
GENESISAPI geBoolean geWorld_SetMeshFrame(geWorld *World, geMesh *Mesh, int32 Frame)
{
	assert(World != NULL);
	assert(Mesh!= NULL);

	return Mesh_SetMeshFrame((Mesh_RenderQ*)Mesh, Frame);
}

//========================================================================================
//	geWorld_GetMeshBox
//========================================================================================
GENESISAPI geBoolean geWorld_GetMeshBox(geWorld *World, geMesh_Def *MeshDef, geVec3d *Mins, geVec3d *Maxs)
{
	assert (World != NULL);
	assert (MeshDef != NULL);
	assert (Mins != NULL);
	assert (Maxs != NULL);
	return Mesh_MeshGetBox(World,(Mesh_MeshDef*)MeshDef, Mins, Maxs);
}

//========================================================================================
//	geWorld_GetMeshFrameCount
//========================================================================================
GENESISAPI int			geWorld_GetMeshFrameCount(const geWorld *World, const geMesh *Mesh)
{
	Mesh_RenderQ *	mesh;

	mesh = (Mesh_RenderQ *)Mesh;
	assert(mesh);
	assert(mesh->MeshDef);
	return (int)mesh->MeshDef->NumFrames;
}
#endif

#if 0
//========================================================================================
//	geWorld_SetModelXForm
//========================================================================================
GENESISAPI geBoolean geWorld_SetModelXForm(geWorld *World, geWorld_Model *Model, const geXForm3d *XForm)
{
	return World_SetModelXForm(World, Model, XForm);
}

GENESISAPI geBoolean geWorld_ModelMotionGetTimeExtents(
	const GE_ModelMotion *	M,
	GE_TimeType *			StartTime,
	GE_TimeType *			EndTime)
{
	return geMotion_GetTimeExtents((const geMotion *)M, StartTime, EndTime);
}

GENESISAPI GE_ModelMotion *geWorld_ModelGetMotion(geWorld_Model *Model)
{
	return (GE_ModelMotion *)World_ModelGetMotion(Model);
}

GENESISAPI void geWorld_ModelMotionSample(
	GE_ModelMotion *	M,
	GE_TimeType 		Time,
	geXForm3d *			XForm)
{
	gePath *	P;

	P = geMotion_GetPath((const geMotion *)M, 0);
	assert(P);
	gePath_Sample(P, Time, XForm);
}

GENESISAPI void * geWorld_ModelGetUserData(const geWorld_Model *Model)
{
	return World_ModelGetUserData(Model);
}

GENESISAPI void geWorld_ModelSetUserData(geWorld_Model *Model, void *UserData)
{
	World_ModelSetUserData(Model, UserData);
}
#endif

//========================================================================================
//	geWorld_AddLight
//========================================================================================
GENESISAPI geLight *geWorld_AddLight(geWorld *World)
{
	assert(World != NULL);

	return (geLight*)Light_WorldAddLight(World);
}

//========================================================================================
//	geWorld_RemoveLight
//========================================================================================
GENESISAPI void geWorld_RemoveLight(geWorld *World, geLight *Light)
{
	assert(World != NULL);
	assert(Light != NULL);

	Light_WorldRemoveLight(World, (Light_DLight*)Light);
}

//========================================================================================
//	geWorld_SetLightAttributes
//========================================================================================
GENESISAPI geBoolean geWorld_SetLightAttributes(	geWorld *World,
										geLight		*Light, 
										const		geVec3d *Pos, 
										const		GE_RGBA *RGBA, 
										float		Radius,
										geBoolean	CastShadow)
{
	assert(World != NULL);
	assert(Light != NULL);
	assert(Pos != NULL);
	assert(RGBA != NULL);
	
	return Light_SetAttributes((Light_DLight*)Light, Pos, RGBA, Radius, CastShadow);
}

//========================================================================================
//	geWorld_SetLTypeTable
//========================================================================================
GENESISAPI geBoolean geWorld_SetLTypeTable(geWorld *World, int32 LType, const char *Table)
{
	assert(World != NULL);
	assert(Table != NULL);

	return Light_WorldSetLTypeTable(World, LType, Table);
}

//========================================================================================
// geWorld_TestModelMove
//========================================================================================
GENESISAPI geBoolean geWorld_TestModelMove(	geWorld			*World, 
											geWorld_Model	*Model, 
											const geXForm3d	*DXForm, 
											const geVec3d	*Mins, const geVec3d *Maxs,
											const geVec3d	*In, geVec3d *Out)
{
	return Trace_TestModelMove(	World, Model, DXForm, 
								Mins, Maxs,
								In, Out);
}

//========================================================================================
// geWorld_ModelCollision
//========================================================================================
GENESISAPI geBoolean geWorld_ModelCollision(geWorld			*World, 
											geWorld_Model	*Model, 
											const geXForm3d	*DXForm, 
											GE_Collision	*Collision)
{
	Collision->Mesh  = NULL;
	Collision->Actor = NULL;
	Collision->Model = NULL;
	return Trace_ModelCollision(World,
								Model,
								DXForm,
								Collision,
								//&(Mesh_RenderQ *)Collision->Mesh,
								&Collision->Impact);
}

//========================================================================================
//	geWorld_Collision
//========================================================================================

GENESISAPI geBoolean geWorld_Collision(geWorld *World, const geVec3d *Mins, const geVec3d *Maxs, const geVec3d *Front, const geVec3d *Back, uint32 Contents, uint32 CollideFlags, uint32 UserFlags, GE_CollisionCB *CollisionCB, void *Context, GE_Collision *Col)
{
	return Trace_GEWorldCollision(World, Mins, Maxs, Front, Back, Contents, CollideFlags, UserFlags, CollisionCB, Context, Col);
}

//========================================================================================
//	geWorld_GetContents
//========================================================================================
GENESISAPI geBoolean geWorld_GetContents(geWorld *World, const geVec3d *Pos, const geVec3d *Mins, const geVec3d *Maxs, uint32 Flags, uint32 UserFlags, GE_CollisionCB *CollisionCB, void *Context, GE_Contents *Contents)
{
	return Trace_GetContents(World, Pos, Mins, Maxs, Flags, UserFlags, CollisionCB, Context, Contents);
}

//========================================================================================
//	NetPlay
//========================================================================================


