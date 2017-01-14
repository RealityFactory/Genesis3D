/****************************************************************************************/
/*  DIRTREE.H                                                                           */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Directory tree interface                                               */
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
#ifndef	DIRTREE_H
#define	DIRTREE_H

#include	"vfile.h"

typedef struct DirTree			DirTree;
typedef struct DirTree_Finder	DirTree_Finder;

DirTree *DirTree_Create(void);

DirTree *DirTree_CreateFromFile(geVFile *File);

geBoolean DirTree_WriteToFile(const DirTree *Tree, geVFile *File);

geBoolean DirTree_GetSize(const DirTree *Tree, long *Size);
	// Gets the size of data that will be written to disk to persist
	// the tree.  This API is NOT efficient.

void DirTree_Destroy(DirTree *Tree);


DirTree *DirTree_FindExact(const DirTree *Tree, const char *Path);
DirTree *DirTree_FindPartial(
	const DirTree *	Tree,
	const char *	Path,
	const char **	LeftOvers);

DirTree * DirTree_AddFile(DirTree *Tree, const char *Path, geBoolean IsDirectory);

geBoolean DirTree_Remove(DirTree *Tree, DirTree *SubTree);

void DirTree_SetFileAttributes(DirTree *Tree, geVFile_Attributes Attributes);

void DirTree_GetFileAttributes(DirTree *Tree, geVFile_Attributes *Attributes);

void DirTree_SetFileOffset(DirTree *Tree, long Offset);

void DirTree_GetFileOffset(DirTree *Tree, long *Offset);

void DirTree_SetFileTime(DirTree *Tree, const geVFile_Time *Time);

void DirTree_GetFileTime(DirTree *Tree, geVFile_Time *Time);

void DirTree_SetFileSize(DirTree *Tree, long Size);

void DirTree_GetFileSize(DirTree *Tree, long *Size);

geBoolean DirTree_SetFileHints(DirTree *Tree, const geVFile_Hints *Hints);

void DirTree_GetFileHints(DirTree *Tree, geVFile_Hints *Hints);

geBoolean DirTree_GetName(DirTree *Tree, char *Buff, int MaxLen);

geBoolean DirTree_FileExists(const DirTree *Tree, const char *Path);


DirTree_Finder * DirTree_CreateFinder(DirTree *Tree, const char *Path);

void DirTree_DestroyFinder(DirTree_Finder *Finder);

DirTree * DirTree_FinderGetNextFile(DirTree_Finder *Finder);


#ifdef	DEBUG
void DirTree_Dump(const DirTree *Tree);
#endif

#endif

