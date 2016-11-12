/****************************************************************************************/
/*  MEMPOOL.H                                                                           */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description: Fixed size block memory allocator interface                            */
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
#ifndef MEMPOOL_H
#define MEMPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemPool MemPool;

extern MemPool * MemPool_Create(int HunkLength,int NumHunks,int AutoExtendNumItems);
extern int MemPool_Extend(MemPool * Pool,int NumHunks);
extern void MemPool_Destroy(MemPool ** pPool);
extern void MemPool_Reset(MemPool * Pool);
extern void * MemPool_GetHunk(MemPool * Pool);
extern int MemPool_FreeHunk(MemPool * Pool,void *Hunk);

 /* NOTEZ: MemPool_Get clears the memory block to zeros*/

#ifdef _DEBUG
extern int MemPool_IsValid(MemPool * Pool);
#endif

#ifdef __cplusplus
}
#endif

#endif /*CRB_MEMPOOL_H*/
