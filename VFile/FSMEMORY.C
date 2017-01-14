/****************************************************************************************/
/*  FSMEMORY.C                                                                          */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Memory file system implementation                                      */
/*     Bug repair for 1.1 release - thanks to Tim Brengle                               */
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
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	"basetype.h"
#include	"ram.h"

#include	"vfile.h"
#include	"vfile._h"

#include	"fsmemory.h"

//	"MF01"
#define	MEMORYFILE_SIGNATURE	0x3130464D

//	"MF02"
#define	MEMORYFINDER_SIGNATURE	0x3230464D

#define	CHECK_HANDLE(H)	assert(H);assert(H->Signature == MEMORYFILE_SIGNATURE);
#define	CHECK_FINDER(F)	assert(F);assert(F->Signature == MEMORYFINDER_SIGNATURE);

#define	MEMORY_FILE_GROW	0x2000

typedef struct	MemoryFile
{
	unsigned int	Signature;
	char *			Memory;
	int				Size;
	int				AllocatedSize;
	int				Position;
	geBoolean		WeOwnMemory;
	geBoolean		ReadOnly;
}	MemoryFile;

static	void *	GENESISCC FSMemory_FinderCreate(
	geVFile *		FS,
	void *			Handle,
	const char *	FileSpec)
{
	return NULL;
}

static	geBoolean	GENESISCC FSMemory_FinderGetNextFile(void *Handle)
{
	assert(!Handle);
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_FinderGetProperties(void *Handle, geVFile_Properties *Props)
{
	assert(!Handle);
	return GE_FALSE;
}

static	void GENESISCC FSMemory_FinderDestroy(void *Handle)
{
	assert(!Handle);
}

static	void *	GENESISCC FSMemory_Open(
	geVFile *		FS,
	void *			Handle,
	const char *	Name,
	void *			Context,
	unsigned int 	OpenModeFlags)
{
	return NULL;
}

static	void *	GENESISCC FSMemory_OpenNewSystem(
	geVFile *		FS,
	const char *	Name,
	void *			Context,
	unsigned int 	OpenModeFlags)
{
	MemoryFile *			NewFS;
	geVFile_MemoryContext *	MemContext;

	if	(FS || Name || !Context)
		return NULL;

	MemContext = Context;

	// Don't allow the user to pass in memory pointer if we're updating or creating, because
	// we don't know what allocation functions we should use to resize their block if
	// necessary.  If you want to create a new file, you have to pass in NULL and let
	// us manage the allocations.
	if	(MemContext->Data && (OpenModeFlags & (GE_VFILE_OPEN_UPDATE | GE_VFILE_OPEN_CREATE)))
		return NULL;

	if	(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY)
		return NULL;

	NewFS = geRam_Allocate(sizeof(*NewFS));
	if	(!NewFS)
		return NewFS;
	memset(NewFS, 0, sizeof(*NewFS));

	NewFS->Memory = MemContext->Data;
	NewFS->Size = MemContext->DataLength;
	NewFS->AllocatedSize = NewFS->Size;

	if	(NewFS->Memory)
	{
		NewFS->ReadOnly = GE_TRUE;
		NewFS->WeOwnMemory = GE_FALSE;
	}
	else
	{
		NewFS->ReadOnly = GE_FALSE;
		NewFS->WeOwnMemory = GE_TRUE;
	}

	NewFS->Signature = MEMORYFILE_SIGNATURE;

	return NewFS;
}

static	geBoolean	GENESISCC FSMemory_UpdateContext(
	geVFile *		FS,
	void *			Handle,
	void *			Context,
	int 			ContextSize)
{
	MemoryFile *			File;
	geVFile_MemoryContext *	MemoryContext;

	assert(FS);
	assert(Context);
	
	File = Handle;
	
	CHECK_HANDLE(File);

	if	(ContextSize != sizeof(geVFile_MemoryContext))
		return GE_FALSE;

	MemoryContext = Context;
	
	MemoryContext->Data		  = File->Memory;
	MemoryContext->DataLength = File->Size;

	return GE_TRUE;
}

static	void	GENESISCC FSMemory_Close(void *Handle)
{
	MemoryFile *	File;
	
	File = Handle;
	
	CHECK_HANDLE(File);

	if	(File->WeOwnMemory == GE_TRUE && File->Memory)
		geRam_Free(File->Memory);
	geRam_Free(File);
}

static int GENESISCC ClampOperationSize(const MemoryFile *File, int Size)
{
	return min(File->Size - File->Position, Size);
}

static char * GENESISCC DataPtr(const MemoryFile *File)
{
	return File->Memory + File->Position;
}

static	geBoolean	GENESISCC FSMemory_GetS(void *Handle, void *Buff, int MaxLen)
{
	MemoryFile *	File;
	char *			p;
	char *			Start;
	char *			pBuff;

	assert(Buff);
	assert(MaxLen != 0);

	File = Handle;

	CHECK_HANDLE(File);

	MaxLen = ClampOperationSize(File, MaxLen);
	if	(MaxLen == 0)
		return GE_FALSE;

	p = DataPtr(File);
	pBuff = Buff;
	Start = p;

//---------
//   Bug fix thanks to Tim Brengle
//---------

	//while	(*p != '\n' && MaxLen > 0)
	while	(*p != '\n' && MaxLen > 1)
	{
		*pBuff++ = *p++;
		MaxLen--;
	}

	File->Position += p - Start + 1;
	assert(File->Position <= File->Size);
	assert(File->Size <= File->AllocatedSize);

	#if 0
		if	(MaxLen != 0)
		{
			*pBuff = *p;
			return GE_TRUE;
		}
		return GE_FALSE;
	#endif
	
	*(pBuff + 1) = 0;
	return GE_TRUE;
	
}

static	geBoolean	GENESISCC FSMemory_Read(void *Handle, void *Buff, int Count)
{
	MemoryFile *	File;

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(ClampOperationSize(File, Count) != Count)
		return GE_FALSE;

	memcpy(Buff, DataPtr(File), Count);

	File->Position += Count;
	assert(File->Position <= File->Size);
	assert(File->Size <= File->AllocatedSize);

	return GE_TRUE;
}

static	geBoolean	GENESISCC TestForExpansion(MemoryFile *File, int Size)
{
	assert(File);
	assert(File->ReadOnly == GE_FALSE);
	assert(File->WeOwnMemory == GE_TRUE);

	assert(File->AllocatedSize >= File->Size);
	assert(File->AllocatedSize >= File->Position);

	if	(File->AllocatedSize - File->Position < Size)
	{
		int		NewSize;
		char *	NewBlock;

		NewSize = ((File->AllocatedSize + Size + (MEMORY_FILE_GROW - 1)) / MEMORY_FILE_GROW) * MEMORY_FILE_GROW;
		NewBlock = geRam_Realloc(File->Memory, NewSize);
		if	(!NewBlock)
			return GE_FALSE;
		File->Memory = NewBlock;
		File->AllocatedSize = NewSize;
//printf("FSMemory: Expanded file to %d bytes\n", NewSize);
	}

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSMemory_Write(void *Handle, const void *Buff, int Count)
{
	MemoryFile *	File;

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->ReadOnly == GE_TRUE)
		return GE_FALSE;

	if	(TestForExpansion(File, Count) == GE_FALSE)
		return GE_FALSE;

	memcpy(DataPtr(File), Buff, Count);
	
	File->Position += Count;
	if	(File->Size < File->Position)
		File->Size = File->Position;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSMemory_Seek(void *Handle, int Where, geVFile_Whence Whence)
{
	MemoryFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	switch	(Whence)
	{
	int		NewPos;

	case	GE_VFILE_SEEKCUR:
		NewPos = File->Position + Where;
		if	(NewPos > File->AllocatedSize)
		{
			if	(File->ReadOnly == GE_TRUE)
				return GE_FALSE;
			if	(TestForExpansion(File, Where) == GE_FALSE)
				return GE_FALSE;
		}
		File->Position = NewPos;
		break;

	case	GE_VFILE_SEEKEND:
		if	(File->Size < Where)
			return GE_FALSE;
		File->Position = File->Size - Where;
		break;

	case	GE_VFILE_SEEKSET:
		if	(Where > File->AllocatedSize)
		{
			if	(File->ReadOnly == GE_TRUE)
				return GE_FALSE;
			if	(TestForExpansion(File, Where - File->Position) == GE_FALSE)
				return GE_FALSE;
		}
		File->Position = Where;
		break;

	default:
		assert(!"Unknown seek kind");
	}

	if	(File->Position > File->Size)
		File->Size = File->Position;
	
	assert(File->Size <= File->AllocatedSize);
	assert(File->Position <= File->AllocatedSize);

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSMemory_EOF(const void *Handle)
{
	const MemoryFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Position == File->Size)
		return GE_TRUE;

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_Tell(const void *Handle, long *Position)
{
	const MemoryFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	*Position = File->Position;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSMemory_Size(const void *Handle, long *Size)
{
	const MemoryFile *	File;

	File = Handle;

	CHECK_HANDLE(File);
	
	*Size = File->Size;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSMemory_GetProperties(const void *Handle, geVFile_Properties *Properties)
{
	assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_SetSize(void *Handle, long Size)
{
	assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_SetAttributes(void *Handle, geVFile_Attributes Attributes)
{
	assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_SetTime(void *Handle, const geVFile_Time *Time)
{
	assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_SetHints(void *Handle, const geVFile_Hints *Hints)
{
	assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_FileExists(geVFile *FS, void *Handle, const char *Name)
{
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_Disperse(
	geVFile *		FS,
	void *		Handle,
	const char *Directory,
	geBoolean	Recursive)
{
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_DeleteFile(geVFile *FS, void *Handle, const char *Name)
{
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSMemory_RenameFile(geVFile *FS, void *Handle, const char *Name, const char *NewName)
{
	return GE_FALSE;
}

static	geVFile_SystemAPIs	FSMemory_APIs =
{
	FSMemory_FinderCreate,
	FSMemory_FinderGetNextFile,
	FSMemory_FinderGetProperties,
	FSMemory_FinderDestroy,

	FSMemory_OpenNewSystem,
	FSMemory_UpdateContext,
	FSMemory_Open,
	FSMemory_DeleteFile,
	FSMemory_RenameFile,
	FSMemory_FileExists,
	FSMemory_Disperse,
	FSMemory_Close,

	FSMemory_GetS,
	FSMemory_Read,
	FSMemory_Write,
	FSMemory_Seek,
	FSMemory_EOF,
	FSMemory_Tell,
	FSMemory_Size,

	FSMemory_GetProperties,

	FSMemory_SetSize,
	FSMemory_SetAttributes,
	FSMemory_SetTime,
	FSMemory_SetHints,
};

const geVFile_SystemAPIs * GENESISCC FSMemory_GetAPIs(void)
{
	return &FSMemory_APIs;
}

