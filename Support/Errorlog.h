/****************************************************************************************/
/*  ERRORLOG.H                                                                          */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description: Generic error logging system interface                                 */
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
#ifndef GE_ERRORLOG_H
#define GE_ERRORLOG_H

#include "basetype.h"

#ifndef NDEBUG 
	#define ERRORLOG_FULL_REPORTING
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	GE_ERR_INVALID_DRIVER_HANDLE,			// Driver not supported
	GE_ERR_INVALID_MODE_HANDLE,				// Mode not supported
	GE_ERR_DRIVER_INIT_FAILED,				// Could not init Driver
	GE_ERR_DRIVER_ALLREADY_INITIALIZED,		// Driver init failure
	GE_ERR_DRIVER_NOT_FOUND,				// File open error for driver
	GE_ERR_DRIVER_NOT_INITIALIZED,			// Driver shutdown failure
	GE_ERR_INVALID_DRIVER,					// Wrong driver version, or bad driver
	GE_ERR_DRIVER_BEGIN_SCENE_FAILED,
	GE_ERR_DRIVER_END_SCENE_FAILED,
	GE_ERR_CREATE_SOUND_MANAGER_FAILED,
	GE_ERR_CREATE_SOUND_BUFFER_FAILED,
	GE_ERR_DS_ERROR,
	GE_ERR_INVALID_WAV,
	GE_ERR_NO_PERF_FREQ,
	GE_ERR_FILE_OPEN_ERROR,
	GE_ERR_FILE_READ_ERROR,
	GE_ERR_FILE_WRITE_ERROR,
	GE_ERR_PALETTE_LOAD_FAILURE,
	GE_ERR_GBSP_LOAD_FAILURE,
	GE_ERR_INVALID_PARMS,
	GE_ERR_INVALID_CAMERA,
	GE_ERR_RENDER_WORLD_FAILED,
	GE_ERR_BEGIN_WORLD_FAILED,
	GE_ERR_END_WORLD_FAILED,
	GE_ERR_BEGIN_MODELS_FAILED,
	GE_ERR_END_MODELS_FAILED,
	GE_ERR_BEGIN_MESHES_FAILED,
	GE_ERR_END_MESHES_FAILED,
	GE_ERR_RENDER_MESH_FAILED,
	GE_ERR_BAD_LMAP_EXTENTS,
	GE_ERR_INVALID_TEXTURE,
	GE_ERR_REGISTER_WORLD_TEXTURE_FAILED,
	GE_ERR_REGISTER_LIGHTMAPS_FAILED,
	GE_ERR_REGISTER_WORLD_PALETTE_FAILED,
	GE_ERR_REGISTER_MISC_TEXTURE_FAILED,
	GE_ERR_INVALID_MESH_FILE,
	GE_ERR_LOAD_BITMAP_FAILED,
	GE_ERR_MAX_MESH_DEFS,
	GE_ERR_MESH_MAX_NODES,
	GE_ERR_INVALID_MESH_MATERIAL,
	GE_ERR_MAX_MESH_MATERIALS,
	GE_ERR_MAX_MESH_CLIP_PLANES,
	GE_ERR_RENDERQ_OVERFLOW,
	GE_ERR_INVALID_LTYPE,
	GE_ERR_MAX_ENTITIES,
	GE_ERR_GET_ENTITY_DATA_ERROR,
	GE_ERR_INVALID_ENTITY_FIELD_TYPE,
	GE_ERR_MODEL_NOT_FOUND,
	GE_ERR_MODEL_NOT_IN_ENTITY,
	GE_ERR_MAX_TEXTURES,
	GE_ERR_MAX_DECALS,
	GE_ERR_MAX_VERTS,
	GE_ERR_OUT_OF_MEMORY,
	GE_ERR_INVALID_BSP_TAG,
	GE_ERR_INVALID_BSP_VERSION,
	GE_ERR_ERROR_READING_BSP_CHUNK,
	ERR_PATH_CREATE_ENOMEM,				// failure to create a path (memory allocation failed)
	ERR_PATH_INSERT_R_KEYFRAME,			// failure to insert a rotation keyframe
	ERR_PATH_INSERT_T_KEYFRAME,			// failure to insert a translation keyframe
	ERR_PATH_DELETE_R_KEYFRAME,			// failure to delete a rotation keyframe
	ERR_PATH_DELETE_T_KEYFRAME,			// failure to delete a translation keyframe
	ERR_PATH_FILE_READ,					// failure to read from file
	ERR_PATH_FILE_VERSION,				// tried to create path from file with wrong/bad version
	ERR_PATH_FILE_PARSE,				// failure to parse file (unexpected format problem)
	ERR_PATH_FILE_WRITE,				// failure to read from file
	ERR_MOTION_CREATE_ENOMEM,			// failure to create (memory allocation failed)
	ERR_MOTION_ADDPATH_ENOMEM,			// failure to add path into motion (memory allocation failed)
	ERR_MOTION_ADDPATH_PATH,			// failure to add path into motion (path creation failed)
	ERR_MOTION_ADDPATH_BAD_NAME,		// failure to add path into motion due to name conflict
	ERR_MOTION_INSERT_EVENT,			// failure to insert event (memory allocation failed or duplicate key)
	ERR_MOTION_DELETE_EVENT,			// failure to insert event
	ERR_MOTION_FILE_READ,				// failure to read from file
	ERR_MOTION_FILE_WRITE,				// failure to write to file
	ERR_MOTION_FILE_PARSE,				// failure to parse file (unexpected format problem)
	ERR_TKARRAY_INSERT_IDENTICAL,		// failure to insert into list because of existing identical key
	ERR_TKARRAY_INSERT_ENOMEM,			// failure to insert into list because of memory allocation failure
	ERR_TKARRAY_DELETE_NOT_FOUND,		// failure to delete from list because key was not found
	ERR_TKARRAY_CREATE,					// failure to create TKArray object (out of memroy)
	ERR_TKARRAY_TOO_BIG,				// TKArray object can't be added to - it's list is as big as it can get
	ERR_VKARRAY_INSERT,					// insertion to VKArray failed
	ERR_QKARRAY_INSERT,					// insertion to QKArray failed
	ERR_POSE_CREATE_ENOMEM,				// Motion object failed to create (memory allocation failed)
	ERR_POSE_ADDJOINT_ENOMEM,			// Motion_AddJoint failed to allocate/reallocate memory for new joint
	ERR_TKEVENTS_CREATE_ENOMEM,			// failure to create TKEvents object (memory allocation failed)
	ERR_TKEVENTS_DELETE_NOT_FOUND,		// failure to delete from list because key was not found
	ERR_TKEVENTS_INSERT_ENOMEM,			// failure to insert into list because of memory allocation failure
	ERR_TKEVENTS_INSERT,				// failure to insert into list 
	ERR_TKEVENTS_FILE_READ,				// failure to read from data file
	ERR_TKEVENTS_FILE_WRITE,			// failure to write to data file
	ERR_TKEVENTS_FILE_VERSION,			// failure to read tkevents object: file has wrong version
	ERR_TKEVENTS_FILE_PARSE,			// failure to parse file (unexpected format problem)
	ERR_STRBLOCK_ENOMEM,				// failure to create, insert, or append (memory allocation failed)
	ERR_STRBLOCK_STRLEN,				// string too long to insert or append
	ERR_STRBLOCK_FILE_READ,				// failure to read from data file
	ERR_STRBLOCK_FILE_WRITE,			// failure to write to data file
	ERR_STRBLOCK_FILE_PARSE,			// failure to parse reading from input file (unexpected format problem)
	ERR_BODY_ENOMEM,					// failure to create, or add (memory allocation failed)
	ERR_BODY_FILE_PARSE,				// failure to parse reading from input file (unexpected format problem)
	ERR_BODY_FILE_READ,					// failure to read from data file
	ERR_BODY_FILE_WRITE,				// failure to write to data file
	ERR_BODY_BONEXFARRAY,				// XFArray object failed to return array, or array size doesn't match bone count
	ERR_XFARRAY_ENOMEM,					// failure to create. (memory allocation failure)
	ERR_PUPPET_ENOMEM,					// failure to create. (memory allocation failure)
	ERR_PUPPET_RENDER,					// failure to render. 
	ERR_PUPPET_NO_MATERIALS,			// failure to create: associated body has no materials.
	ERR_PUPPET_LOAD_TEXTURE,			// failure to load texture 
	ERR_TEXPOOL_ENOMEM,					// failure to create or add to. (memory allocation/reallocation failure)
	ERR_TEXPOOL_TOO_BIG,				// failure to add to pool, pool is too large.
	ERR_TEXPOOL_LOAD_TEXTURE,			// failure to load texture into pool
	ERR_TEXPOOL_TEXTURE_NOT_FREE,		// texture pool destroyed without first freeing all it's shared textures
	ERR_ACTOR_ENOMEM,					// failure to create. (memory allocation failure)
	ERR_ACTOR_RENDER_PREP,				// failure to prepare actor for rendering (bad Body or allocation failure)
	ERR_ACTOR_RENDER_FAILED,			// failure to render.  failure to get geometry from Body 
	ERR_ACTOR_TOO_MANY_MOTIONS,			// failure to add motion. too many.
	ERR_ACTOR_FILE_READ,				// failure to read from data file.
	ERR_ACTOR_FILE_PARSE,				// failure to parse reading from input file(unexpected format problem)
	ERR_ACTOR_FILE_WRITE,				// failure to write to data file.
	GE_ERR_INVALID_MODEL_MOTION_FILE,	// Bad model motion file (for bsp files)
	GE_ERR_BAD_BSP_FILE_CHUNK_SIZE,		// Chunk size does not match structure size of kind
} geErrorLog_ErrorIDEnumType;


typedef enum 
{
	GE_ERR_MEMORY_RESOURCE,
	GE_ERR_DISPLAY_RESOURCE,
	GE_ERR_SOUND_RESOURCE,
	GE_ERR_SYSTEM_RESOURCE,
	GE_ERR_INTERNAL_RESOURCE,
	
	GE_ERR_FILEIO_OPEN,
	GE_ERR_FILEIO_CLOSE,
	GE_ERR_FILEIO_READ,
	GE_ERR_FILEIO_WRITE,
	GE_ERR_FILEIO_FORMAT,
	GE_ERR_FILEIO_VERSION,
	
	GE_ERR_LIST_FULL,
	GE_ERR_DATA_FORMAT,
	GE_ERR_SEARCH_FAILURE,
} geErrorLog_ErrorClassType;

GENESISAPI void geErrorLog_Clear(void);
	// clears error history

GENESISAPI int  geErrorLog_Count(void);
	// reports size of current error log

GENESISAPI void geErrorLog_AddExplicit(geErrorLog_ErrorClassType,
	const char *ErrorIDString,
	const char *ErrorFileString,
	int LineNumber,
	const char *UserString,
	const char *Context);
	// not intended to be used directly: use ErrorLog_Add or ErrorLog_AddString


#ifdef ERRORLOG_FULL_REPORTING
	// 'Debug' version includes a textual error id, and the user string

	#define geErrorLog_Add(Error, Context) geErrorLog_AddExplicit(Error, #Error, __FILE__, __LINE__,"", Context)
		// logs an error.  

	#define geErrorLog_AddString(Error,String, Context) geErrorLog_AddExplicit(Error, #Error, __FILE__,__LINE__, String, Context)
		// logs an error with additional identifing string.  
	
GENESISAPI	geBoolean geErrorLog_AppendStringToLastError(const char *String);// use geErrorLog_AppendString

	#define geErrorLog_AppendString(XXX) geErrorLog_AppendStringToLastError(XXX)
		// adds text to the previous logged error

#else
	// 'Release' version does not include the textual error id, or the user string

	#define geErrorLog_Add(Error, Context) geErrorLog_AddExplicit(Error, "", __FILE__, __LINE__,"", Context)
		// logs an error.  

	#define geErrorLog_AddString(Error,String, Context) geErrorLog_AddExplicit(Error, "", __FILE__,__LINE__, "", Context)
		// logs an error with additional identifing string.  
	
	#define geErrorLog_AppendString(XXX)
		// adds text to the previous logged error

#endif

GENESISAPI geBoolean geErrorLog_Report(int History, geErrorLog_ErrorClassType *Error, const char **UserString);
	// reports from the error log.  
	// history is 0 for most recent,  1.. for second most recent etc.
	// returns GE_TRUE if report succeeded.  GE_FALSE if it failed.

#ifdef __cplusplus
}
#endif

#endif

