/****************************************************************************************/
/*  RAM.H                                                                               */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Replacement for malloc, realloc and free                               */
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
#ifndef GE_RAM_H
#define GE_RAM_H

#include "basetype.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (* geRam_CriticalCallbackFunction)(void);

/*
  Set the critical callback function.  ram_allocate will call the critical
  callback function if it's unable to allocate memory.
*/
GENESISAPI geRam_CriticalCallbackFunction geRam_SetCriticalCallback
    (
      geRam_CriticalCallbackFunction callback
    );

/*
  increments or decrements a counter .  if the counter is >0
  the critical callback function (if set) is called for a failed memory allocation.
  add is added to the current counter value.  the new counter value is returned.
*/
GENESISAPI int geRam_EnableCriticalCallback(int add);


/*
  Allocate memory of the given size.  In debug mode, the memory is filled
  with 0xA5, and we keep track of the amount of memory allocated.  Also, in debug
  mode, we track where the memory was allocated and can optionally provide a
  report of allocated blocks.  See geRam_ReportAllocations.
*/
#ifndef NDEBUG

#define geRam_Allocate(size) _geRam_DebugAllocate(size, __FILE__, __LINE__)

// Do not call _geRam_DebugAllocate directly.
GENESISAPI void* _geRam_DebugAllocate(uint32 size, const char* pFile, int line);

#else

GENESISAPI void *geRam_Allocate(uint32 size);

#endif

/*
  Free an allocated memory block.
*/
GENESISAPI void geRam_Free_(void *ptr);

		extern void *StupidUnusedPointer;    // never used, except to mask the
		// possible warning you get if you use the geRam_Free macro below, without
		// using the xxx pointer again in the same block.  This is ugly.
 
#define geRam_Free(xxx) geRam_Free_(xxx) ,(xxx)=NULL, StupidUnusedPointer=(xxx)

/*
  Reallocate memory.  This function supports shrinking and expanding blocks,
  and will also act like ram_allocate if the pointer passed to it is NULL.
  It won't, however, free the memory if you pass it a 0 size.
*/
#ifndef NDEBUG

#define geRam_Realloc(ptr, newsize) _geRam_DebugRealloc(ptr, newsize, __FILE__, __LINE__)

// Do not call _geRam_DebugRealloc directly.
GENESISAPI void* _geRam_DebugRealloc(void* ptr, uint32 size, const char* pFile, int line);

#else

GENESISAPI void *geRam_Realloc(void *ptr,uint32 newsize);

#endif

#ifndef NDEBUG

GENESISAPI void geRam_ReportAllocations(void);

#else

#define geRam_ReportAllocations() 

#endif

#ifndef NDEBUG
    extern int32 geRam_CurrentlyUsed;
    extern int32 geRam_NumberOfAllocations;
    extern int32 geRam_MaximumUsed;
    extern int32 geRam_MaximumNumberOfAllocations;

GENESISAPI     void geRam_AddAllocation(int n,uint32 size);
#else
    #define geRam_AddAllocation(n,s)
#endif

// allocate the ram & clear it. (calloc)
GENESISAPI void * geRam_AllocateClear(uint32 size);

#define GE_RAM_ALLOCATE_STRUCT(type)      (type *)geRam_Allocate (sizeof (type))
#define GE_RAM_ALLOCATE_ARRAY(type,count) (type *)geRam_Allocate (sizeof (type) * (count))

#ifndef NDEBUG
#define GE_RAM_REALLOC_ARRAY(ptr,type,count)  (type *)geRam_Realloc(  (ptr), sizeof(type) * (count) );{type *XX=(ptr);}
#else
#define GE_RAM_REALLOC_ARRAY(ptr,type,count)  (type *)geRam_Realloc(  (ptr), sizeof(type) * (count) )
#endif

#ifndef NDEBUG
geBoolean geRam_IsValidPtr(void *ptr);
#endif

#ifdef __cplusplus
  }
#endif

#endif

