/****************************************************************************************/
/*  VFILE.C                                                                             */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Virtual file implementation                                            */
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
#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>

#include	<stdio.h>
#include	<assert.h>
#include	<stdarg.h>
#include	<string.h>

#include	"basetype.h"
#include	"ram.h"

#include	"vfile.h"
#include	"vfile._h"

#include	"fsdos.h"
#include	"fsmemory.h"
#include	"fsvfs.h"

typedef	struct	FSSearchList
{
	geVFile *				FS;
	struct FSSearchList *	Next;
}	FSSearchList;


typedef	struct	geVFile
{
	geVFile_TypeIdentifier		SystemType;
	const geVFile_SystemAPIs *	APIs;
	void *						FSData;
	geVFile *					Context;
	FSSearchList *				SearchList;
	CRITICAL_SECTION			CriticalSection;
	geVFile *					BaseFile;
}	geVFile;

typedef struct	geVFile_Finder
{
	const geVFile_SystemAPIs *	APIs;
	void *						Data;
}	geVFile_Finder;

static	geVFile_SystemAPIs **		RegisteredAPIs;
static	int							SystemCount;
static	geBoolean					BuiltInAPIsRegistered = GE_FALSE;

#ifndef	NDEBUG
static	geBoolean					SystemInitialized = GE_FALSE;
#endif

static	CRITICAL_SECTION			MainCriticalSection;

static	geBoolean GENESISCC geVFile_RegisterFileSystemInternal(const geVFile_SystemAPIs *APIs, geVFile_TypeIdentifier *Type)
{
	geVFile_SystemAPIs **	NewList;

	NewList = geRam_Realloc(RegisteredAPIs, sizeof(*RegisteredAPIs) * (SystemCount + 1));
	if	(!NewList)
		return GE_FALSE;

	RegisteredAPIs = NewList;
#pragma message ("Casting away const in geVFile_RegisterFileSystem")
	RegisteredAPIs[SystemCount++] = (geVFile_SystemAPIs *)APIs;
	*Type = SystemCount;

	return GE_TRUE;
}

static	geBoolean	RegisterBuiltInAPIs(void)
{
	geVFile_TypeIdentifier 	Type;

	if	(BuiltInAPIsRegistered == GE_TRUE)
		return GE_TRUE;

	if	(geVFile_RegisterFileSystemInternal(FSDos_GetAPIs(), &Type) == GE_FALSE)
		return GE_FALSE;
	if	(Type != GE_VFILE_TYPE_DOS)
		return GE_FALSE;

	if	(geVFile_RegisterFileSystemInternal(FSMemory_GetAPIs(), &Type) == GE_FALSE)
		return GE_FALSE;
	if	(Type != GE_VFILE_TYPE_MEMORY)
		return GE_FALSE;

	if	(geVFile_RegisterFileSystemInternal(FSVFS_GetAPIs(), &Type) == GE_FALSE)
		return GE_FALSE;
	if	(Type != GE_VFILE_TYPE_VIRTUAL)
		return GE_FALSE;

	BuiltInAPIsRegistered = GE_TRUE;

	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geVFile_RegisterFileSystem(const geVFile_SystemAPIs *APIs, geVFile_TypeIdentifier *Type)
{
	geBoolean	Result;

	assert(APIs);
	assert(Type);

	if	(RegisterBuiltInAPIs() == GE_FALSE)
		return GE_FALSE;

	Result = geVFile_RegisterFileSystemInternal(APIs, Type);

	return Result;
}

static	geBoolean	GENESISCC	CheckOpenFlags(unsigned int OpenModeFlags)
{
	int 			FlagCount;
	unsigned int	AccessFlags;

	// Test to see that the open mode for this thing is mutually exclusive in
	// the proper flags.
	FlagCount = 0;
	AccessFlags = OpenModeFlags & (GE_VFILE_OPEN_READONLY | GE_VFILE_OPEN_UPDATE | GE_VFILE_OPEN_CREATE);
	if	(AccessFlags & GE_VFILE_OPEN_READONLY)
		FlagCount++;
	if	(AccessFlags & GE_VFILE_OPEN_UPDATE)
		FlagCount++;
	if	(AccessFlags & GE_VFILE_OPEN_CREATE)
		FlagCount++;

	if	(FlagCount != 1 && !(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY))
		return GE_FALSE;

	return GE_TRUE;
}

GENESISAPI geVFile * GENESISCC geVFile_OpenNewSystem(
	geVFile *				FS,
	geVFile_TypeIdentifier 	FileSystemType,
	const char *			Name,
	void *					Context,
	unsigned int 			OpenModeFlags)
{
	const geVFile_SystemAPIs *	APIs;
	geVFile *					File;
	void *						FSData;
	geVFile *					BaseFile;


	if	(RegisterBuiltInAPIs() == GE_FALSE)
		return GE_FALSE;

	if	((FileSystemType == 0) || (FileSystemType > SystemCount))
		return NULL;

	if	(CheckOpenFlags(OpenModeFlags) == GE_FALSE)
		return NULL;

	// Sugarcoating support for a taste test
	if	(FS == NULL && FileSystemType == GE_VFILE_TYPE_VIRTUAL)
	{
		assert(Name);
		BaseFile = geVFile_OpenNewSystem(NULL,
										 GE_VFILE_TYPE_DOS,
										 Name,
										 NULL,
										 OpenModeFlags & ~GE_VFILE_OPEN_DIRECTORY);
		if	(!BaseFile)
			return NULL;
		FS = BaseFile;
		Name = NULL;
	}
	else
		BaseFile = NULL;

	APIs = RegisteredAPIs[FileSystemType - 1];
	assert(APIs);
	FSData = APIs->OpenNewSystem(FS, Name, Context, OpenModeFlags);

	if	(!FSData)
	{
		if	(BaseFile)
			geVFile_Close(BaseFile);
		return NULL;
	}

	File = geRam_Allocate(sizeof(*File));
	if	(!File)
	{
		if	(BaseFile)
			geVFile_Close(BaseFile);
		APIs->Close(FSData);
		return NULL;
	}

	File->SystemType =	FileSystemType;
	File->APIs = 		APIs;
	File->FSData = 		FSData;
	File->SearchList = 	geRam_Allocate(sizeof(*File->SearchList));
	File->BaseFile = 	BaseFile;

	if	(!File->SearchList)
	{
		if	(BaseFile)
			geVFile_Close(BaseFile);
		geRam_Free(File);
		APIs->Close(FSData);
		return NULL;
	}

	File->SearchList->FS 	= File;
	File->SearchList->Next	= NULL;

	return File;
}

GENESISAPI geBoolean GENESISCC geVFile_UpdateContext(geVFile *FS, void *Context, int ContextSize)
{
	assert(FS);
	assert(Context);

	return FS->APIs->UpdateContext(FS, FS->FSData, Context, ContextSize);
}

GENESISAPI geVFile * GENESISCC geVFile_Open(
	geVFile *		FS,
	const char *	Name,
	unsigned int 	OpenModeFlags)
{
	FSSearchList *	SearchList;
	geVFile *		StartContext;
	geVFile *		File;
	void *			FSData;

	assert(Name);

	if	(!FS)
		return NULL;

	if	(CheckOpenFlags(OpenModeFlags) == GE_FALSE)
		return NULL;

	StartContext = FS;

	SearchList = FS->SearchList;
	assert(SearchList);
	assert(SearchList->FS == FS);
	if	(!(OpenModeFlags & GE_VFILE_OPEN_CREATE))
	{
		while	(SearchList)
		{
			FS = SearchList->FS;
			if	(FS->APIs->FileExists(FS, FS->FSData, Name))
				break;
			SearchList = SearchList->Next;
		}
	}

	if	(!SearchList)
		return NULL;

	FSData = FS->APIs->Open(FS, FS->FSData, Name, NULL, OpenModeFlags);
	if	(!FSData)
		return NULL;

	File = geRam_Allocate(sizeof(*File));
	if	(!File)
	{
		FS->APIs->Close(FSData);
		return NULL;
	}

	memset(File, 0, sizeof(*File));

	File->SystemType =	0;
	File->APIs = 		FS->APIs;
	File->FSData = 		FSData;
	File->SearchList = 	geRam_Allocate(sizeof(*File->SearchList));
	File->Context =		FS;

	if	(!File->SearchList)
	{
		geRam_Free(File);
		FS->APIs->Close(FSData);
		return NULL;
	}

	File->SearchList->FS 	= File;
	File->SearchList->Next	= StartContext->SearchList;

	return File;
}

GENESISAPI geVFile * GENESISCC geVFile_GetContext(const geVFile *File)
{
	assert(File);

	return File->Context;
}

static	void			DestroySearchList(FSSearchList *SearchList)
{
	while	(SearchList)
	{
		FSSearchList *	Temp;

		Temp = SearchList;
		SearchList = SearchList->Next;
		geRam_Free(Temp);
	}
}

static	FSSearchList *	CopySearchList(const FSSearchList *SearchList)
{
	FSSearchList *	NewList;
	FSSearchList *	Tail;

	NewList = Tail = NULL;
	while	(SearchList)
	{
		FSSearchList *	Temp;

		Temp = geRam_Allocate(sizeof(*Tail));
		if	(!Temp)
		{
			DestroySearchList(NewList);
			return NULL;
		}
		if	(Tail)
			Tail->Next = Temp;
		else
		{
			assert(!NewList);
			NewList = Temp;
		}
		Tail = Temp;
		Tail->FS = SearchList->FS;
		Tail->Next = NULL;
		SearchList = SearchList->Next;
	}
	return NewList;
}

GENESISAPI geBoolean GENESISCC geVFile_AddPath(geVFile *FS1, const geVFile *FS2, geBoolean Append)
{
	FSSearchList *	SearchList;

	assert(FS1);
	assert(FS2);

	SearchList = CopySearchList(FS2->SearchList);
	if	(!SearchList)
		return GE_FALSE;

	if	(Append == GE_FALSE)
	{
		SearchList->Next = FS1->SearchList;
		FS1->SearchList = SearchList;
	}
	else
	{
		FSSearchList	Temp;
		FSSearchList *	pTemp;

		Temp.Next = FS1->SearchList;
		pTemp = &Temp;
		while	(pTemp->Next)
		{
			pTemp = pTemp->Next;
		}

		pTemp->Next = SearchList;
	}

	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geVFile_DeleteFile(geVFile *FS, const char *FileName)
{
	assert(FS);
	assert(FileName);

	return FS->APIs->DeleteFile(FS, FS->FSData, FileName);
}

GENESISAPI geBoolean GENESISCC geVFile_RenameFile(geVFile *FS, const char *FileName, const char *NewName)
{
	assert(FS);
	assert(FileName);
	assert(NewName);

	return FS->APIs->RenameFile(FS, FS->FSData, FileName, NewName);
}

GENESISAPI geBoolean GENESISCC geVFile_FileExists(geVFile *FS, const char *FileName)
{
	return FS->APIs->FileExists(FS, FS->FSData, FileName);
}

GENESISAPI geBoolean GENESISCC geVFile_Close(geVFile *File)
{
	assert(File);

	File->APIs->Close(File->FSData);

	if	(File->BaseFile)
		geVFile_Close(File->BaseFile);

	geRam_Free(File->SearchList);
	geRam_Free(File);
#pragma message ("Need to propagate returns through VFile_Close")
	return GE_TRUE;
}


GENESISAPI geBoolean GENESISCC geVFile_GetS(geVFile *File, void *Buff, int MaxLen)
{
	assert(File);
	assert(Buff);

	if	(MaxLen == 0)
		return GE_FALSE;

	return File->APIs->GetS(File->FSData, Buff, MaxLen);
}

GENESISAPI geBoolean GENESISCC geVFile_Read(geVFile *File, void *Buff, int Count)
{
	assert(File);
	assert(Buff);

	if	(Count == 0)
		return GE_TRUE;

	return File->APIs->Read(File->FSData, Buff, Count);
}

GENESISAPI geBoolean GENESISCC geVFile_Write(geVFile *File, const void *Buff, int Count)
{
	assert(File);
	assert(Buff);

	if	(Count == 0)
		return GE_TRUE;

	return File->APIs->Write(File->FSData, Buff, Count);
}

GENESISAPI geBoolean GENESISCC geVFile_Seek(geVFile *File, int Where, geVFile_Whence Whence)
{
	assert(File);

	return File->APIs->Seek(File->FSData, Where, Whence);
}

GENESISAPI geBoolean GENESISCC geVFile_Printf(geVFile *File, const char *Format, ...)
{
	char	Temp[8096];
	va_list	ArgPtr;

	assert(File);
	assert(Format);

	va_start(ArgPtr, Format);
	vsprintf(Temp, Format, ArgPtr);
	va_end(ArgPtr);

	return File->APIs->Write(File->FSData, &Temp[0], strlen(Temp));
}

GENESISAPI geBoolean GENESISCC geVFile_EOF   (const geVFile *File)
{
	assert(File);
	
	return File->APIs->Eof(File->FSData);
}

GENESISAPI geBoolean GENESISCC geVFile_Tell  (const geVFile *File, long *Position)
{
	assert(File);
	
	return File->APIs->Tell(File->FSData, Position);
}

GENESISAPI geBoolean GENESISCC geVFile_Size  (const geVFile *File, long *Size)
{
	assert(File);
	
	return File->APIs->Size(File->FSData, Size);
}

GENESISAPI geBoolean GENESISCC geVFile_GetProperties(const geVFile *File, geVFile_Properties *Properties)
{
	assert(File);
	
	return File->APIs->GetProperties(File->FSData, Properties);
}

GENESISAPI geBoolean GENESISCC geVFile_SetSize(geVFile *File, long Size)
{
	assert(File);
	
	return File->APIs->SetSize(File->FSData, Size);
}

GENESISAPI geBoolean GENESISCC geVFile_SetAttributes(geVFile *File, geVFile_Attributes Attributes)
{
	assert(File);
	
	return File->APIs->SetAttributes(File->FSData, Attributes);
}

GENESISAPI geBoolean GENESISCC geVFile_SetTime(geVFile *File, const geVFile_Time *Time)
{
	assert(File);
	
	return File->APIs->SetTime(File->FSData, Time);
}

GENESISAPI geBoolean GENESISCC geVFile_SetHints(geVFile *File, const geVFile_Hints *Hints)
{
	assert(File);

	return File->APIs->SetHints(File->FSData, Hints);
}

GENESISAPI geVFile_Finder * GENESISCC geVFile_CreateFinder(
	geVFile *FileSystem,
	const char *FileSpec)
{
	geVFile_Finder *	Finder;

	assert(FileSystem);
	assert(FileSpec);

	Finder = geRam_Allocate(sizeof(*Finder));
	if	(!Finder)
		return Finder;

	Finder->Data = FileSystem->APIs->FinderCreate(FileSystem, FileSystem->FSData, FileSpec);
	if	(!Finder->Data)
	{
		geRam_Free(Finder);
		return NULL;
	}

	Finder->APIs = FileSystem->APIs;

	return Finder;
}

GENESISAPI void GENESISCC geVFile_DestroyFinder(geVFile_Finder *Finder)
{
	assert(Finder);
	assert(Finder->APIs);

	Finder->APIs->FinderDestroy(Finder->Data);
	geRam_Free(Finder);
}

GENESISAPI geBoolean GENESISCC geVFile_FinderGetNextFile(geVFile_Finder *Finder)
{
	assert(Finder);
	assert(Finder->APIs);
	assert(Finder->Data);
	return Finder->APIs->FinderGetNextFile(Finder->Data);
}

GENESISAPI geBoolean GENESISCC geVFile_FinderGetProperties(const geVFile_Finder *Finder, geVFile_Properties *Properties)
{
	assert(Finder);
	assert(Finder->APIs);
	assert(Finder->Data);

	return Finder->APIs->FinderGetProperties(Finder->Data, Properties);
}


GENESISAPI void GENESISCC geVFile_TimeToWin32FileTime(const geVFile_Time *Time, LPFILETIME Win32FileTime)
{
	*Win32FileTime = *(LPFILETIME)Time;
}
