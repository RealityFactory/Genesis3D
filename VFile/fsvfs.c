/****************************************************************************************/
/*  FSVFS.C                                                                             */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Collection file system implementation                                  */
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
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	"ram.h"

#include	"fsvfs.h"
#include	"dirtree.h"

//	"VF00"
#define	VFSFILEHEADER_SIGNATURE	0x30304656

//	"VF01"
#define	VFSFILE_SIGNATURE		0x31304656

//	"VF02"
#define	VFSFINDER_SIGNATURE		0x32304656

#define	HEADER_VERSION	0

typedef	struct	VFSFileHeader
{
	unsigned int	Signature;
	unsigned short	Version;			// Version number
	geBoolean		Dispersed;			// Is this VFS dispersed?
	long			DirectoryOffset;	// File offset to directory
	long			DataLength;			// Length of all file data, including VFS header
	long			EndPosition;		// End Position in the RWOps file we were written to
}	VFSFileHeader;

// In the above structure, EndPosition should be the same as DataLength.  We use this for
// asserts.

typedef	struct	VFSFile
{
	unsigned int	Signature;

	geVFile *		RWOps;				// Parent file for read/write ops
	struct VFSFile *System;				// If we're a child, we need a back pointer

	DirTree *		DirEntry;			// Directory entry for this file
	DirTree *		Directory;			// Directory information for the VFS

	long			RWOpsStartPos;		// Starting position in the RWOps file
	long			CurrentRelPos;		// Current file pointer (relative to our start)
	long			Length;				// Current file size

	unsigned int	OpenModeFlags;

	// Things that are specific to the Root node
	long			EndPosition;		// End position in the RWOps file if we're a system
	geBoolean		IsSystem;			// Am I the owner of the Directory?
	long			DataLength;			// Current size of the aggregate including VFS header
	geBoolean		Dispersed;			// Is this VFS dispersed?

}	VFSFile;

typedef	struct	VFSFinder
{
	unsigned int		Signature;
	VFSFile *			File;
	DirTree_Finder *	Finder;
	DirTree *			LastFind;
}	VFSFinder;

#define	CHECK_HANDLE(H)	assert(H);assert(H->Signature == VFSFILE_SIGNATURE);
#define	CHECK_FINDER(F)	assert(F);assert(F->Signature == VFSFINDER_SIGNATURE);

static	void *	GENESISCC FSVFS_FinderCreate(
	geVFile *		FS,
	void *			Handle,
	const char *	FileSpec)
{
	VFSFinder *		Finder;
	VFSFile *		File;

	assert(FileSpec != NULL);

	File = Handle;

	CHECK_HANDLE(File);

	if	(!File->Directory)
		return NULL;

	Finder = geRam_Allocate(sizeof(*Finder));
	if	(!Finder)
		return NULL;

	memset(Finder, 0, sizeof(*Finder));

	Finder->Signature = VFSFINDER_SIGNATURE;
	Finder->File	  = File;
	Finder->Finder	  = DirTree_CreateFinder(File->Directory, FileSpec);
	if	(!Finder->Finder)
	{
		geRam_Free(Finder);
		return NULL;
	}

	return (void *)Finder;
}

static	geBoolean	GENESISCC FSVFS_FinderGetNextFile(void *Handle)
{
	VFSFinder *	Finder;

	Finder = Handle;

	CHECK_FINDER(Finder);

	Finder->LastFind = DirTree_FinderGetNextFile(Finder->Finder);
	if	(Finder->LastFind)
		return GE_TRUE;

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSVFS_FinderGetProperties(void *Handle, geVFile_Properties *Properties)
{
	VFSFinder *		Finder;

	assert(Properties);

	Finder = Handle;

	CHECK_FINDER(Finder);

	if	(!Finder->LastFind)
		return GE_FALSE;

	DirTree_GetFileTime(Finder->LastFind, &Properties->Time);
	DirTree_GetFileAttributes(Finder->LastFind, &Properties->AttributeFlags);
	DirTree_GetFileSize(Finder->LastFind, &Properties->Size);
	DirTree_GetFileHints(Finder->LastFind, &Properties->Hints);
	return DirTree_GetName(Finder->LastFind, &Properties->Name[0], sizeof(Properties->Name));
}

static	void GENESISCC FSVFS_FinderDestroy(void *Handle)
{
	VFSFinder *	Finder;

	Finder = Handle;

	CHECK_FINDER(Finder);

	assert(Finder->Finder);

	Finder->Signature = 0;
	DirTree_DestroyFinder(Finder->Finder);
	geRam_Free(Finder);
}

static	void *	GENESISCC FSVFS_Open(
	geVFile *		FS,
	void *			Handle,
	const char *	Name,
	void *			Dummy,
	unsigned int 	OpenModeFlags)
{
	VFSFile *	Context;
	VFSFile *	NewFile;
	DirTree *	FileEntry;

	Context = Handle;

	CHECK_HANDLE(Context);

	assert(Name);

	if	(!Context->Directory)
		return NULL;

	/*
		Right now, we only support update operations to a VFS which is being
		created.  We can do create operations to a VFS which is being created,
		or which already exists.  We can also support directory open operations
		at anytime.
	*/
	if	((OpenModeFlags & GE_VFILE_OPEN_UPDATE)   &&
		 !(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY) &&
		 !(OpenModeFlags & GE_VFILE_OPEN_CREATE) &&
		 !(Context->System->OpenModeFlags & GE_VFILE_OPEN_CREATE))
		return NULL;

	FileEntry = DirTree_FindExact(Context->Directory, Name);
	if	(OpenModeFlags & GE_VFILE_OPEN_CREATE)
	{
		if	(FileEntry)
			return NULL;

		FileEntry = DirTree_AddFile(Context->Directory,
									Name,
									(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY) ? GE_TRUE : GE_FALSE);
		if	(!FileEntry)
			return NULL;
	}
	else
	{
		if	(!FileEntry)
			return NULL;
	}

	NewFile = geRam_Allocate(sizeof(*NewFile));
	if	(!NewFile)
		return NewFile;

	memset(NewFile, 0, sizeof(*NewFile));

	NewFile->Signature 	  = VFSFILE_SIGNATURE;
	NewFile->DirEntry  	  = FileEntry;
	NewFile->RWOps	   	  = Context->RWOps;
	NewFile->Dispersed	  = GE_FALSE;
	NewFile->System		  = Context->System;

	// If we're a directory, make us a first class operator with the child
	if	(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY)
	{
		NewFile->Directory = FileEntry;
	}
	else
	{
		if	(OpenModeFlags & GE_VFILE_OPEN_CREATE)
		{
			NewFile->RWOpsStartPos = Context->System->DataLength +
									 Context->System->RWOpsStartPos;
		}
		else
		{
			DirTree_GetFileOffset(FileEntry, &NewFile->RWOpsStartPos);
		}
	}

	NewFile->OpenModeFlags = OpenModeFlags;

	if	(!(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY))
	{
		if	(OpenModeFlags & GE_VFILE_OPEN_CREATE)
		{
			DirTree_SetFileOffset(FileEntry, NewFile->RWOpsStartPos);
		}
		else
		{
			assert(!(OpenModeFlags & GE_VFILE_OPEN_UPDATE));
			DirTree_GetFileSize(FileEntry, &NewFile->Length);
		}
	}

	// Only a VFS opened with OpenNewSystem gets to be the owner
	NewFile->IsSystem = GE_FALSE;

	return (void *)NewFile;
}

static	void *	GENESISCC FSVFS_OpenNewSystem(
	geVFile *		RWOps,
	const char *	Name,
	void *			Context,
	unsigned int 	OpenModeFlags)
{
	VFSFile *	NewFS;
	long		RWOpsStartPos;

	assert(RWOps != NULL);
	assert(Name == NULL);
	assert(Context == NULL);

	// All VFS are directories
	if	(!(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY))
		return NULL;

	if	(geVFile_Tell(RWOps, &RWOpsStartPos) == GE_FALSE)
		return NULL;

	if	(!(OpenModeFlags & GE_VFILE_OPEN_CREATE))
	{
		VFSFileHeader	Header;
		long			DirectoryStartPos;
		long			DirectoryEndPos;

//#pragma message  ("FSVFS_OpenNewSystem: READ/WRITE opens not supported")

		if	(geVFile_Read(RWOps, &Header, sizeof(Header)) == GE_FALSE)
			return NULL;

		if	(Header.Signature != VFSFILEHEADER_SIGNATURE)
			return NULL;

		if	(Header.Version != HEADER_VERSION)
			return NULL;

		if	(Header.Dispersed == GE_TRUE)
		{
			assert(!"Not implemented");
			return NULL;
		}

		// Go to the directory
		if	(geVFile_Seek(RWOps, Header.DirectoryOffset, GE_VFILE_SEEKSET) == GE_FALSE)
			return NULL;

		// Remember where we started reading the directory
		if	(geVFile_Tell(RWOps, &DirectoryStartPos) == GE_FALSE)
			return NULL;

		NewFS = geRam_Allocate(sizeof(*NewFS));
		if	(!NewFS)
			return NewFS;
		memset(NewFS, 0, sizeof(*NewFS));

		NewFS->RWOps = RWOps;
		NewFS->RWOpsStartPos = RWOpsStartPos;
		NewFS->DataLength = Header.DataLength;
		NewFS->EndPosition = Header.EndPosition;

		// Read the directory
		NewFS->Directory = DirTree_CreateFromFile(RWOps);
		if	(!NewFS->Directory)
		{
			geRam_Free(NewFS);
			return NULL;
		}

		// Get the end position for paranoia checking
		if	(geVFile_Tell(RWOps, &DirectoryEndPos) == GE_FALSE)
		{
			DirTree_Destroy(NewFS->Directory);
			geRam_Free(NewFS);
			return NULL;
		}
	}
	else
	{
		NewFS = geRam_Allocate(sizeof(*NewFS));
		if	(!NewFS)
			return NewFS;
		memset(NewFS, 0, sizeof(*NewFS));

		NewFS->RWOps = RWOps;
		NewFS->RWOpsStartPos = RWOpsStartPos;
		NewFS->Directory = DirTree_Create();
		NewFS->DataLength = sizeof(VFSFileHeader);
	}

	NewFS->Signature	 = VFSFILE_SIGNATURE;
	NewFS->IsSystem		 = GE_TRUE;
	NewFS->System		 = NewFS;
	NewFS->OpenModeFlags = OpenModeFlags;

	return NewFS;
}

static	geBoolean	GENESISCC FSVFS_UpdateContext(
	geVFile *		FS,
	void *			Handle,
	void *			Context,
	int 			ContextSize)
{
	return GE_FALSE;
}

static	void	GENESISCC FSVFS_Close(void *Handle)
{
	VFSFile *	File;

	File = Handle;
	
	CHECK_HANDLE(File);

	if	(File->Directory)
	{
		if	(File->IsSystem == GE_TRUE)
		{
			// Hmmm.  We're the top level
			assert(File == File->System);

			if	(File->OpenModeFlags & (GE_VFILE_OPEN_CREATE | GE_VFILE_OPEN_UPDATE))
			{
				VFSFileHeader	Header;
				long			EndPosition;

				// Have to update the directory
				if	(geVFile_Seek(File->RWOps, File->RWOpsStartPos + File->DataLength, GE_VFILE_SEEKSET) == GE_FALSE)
				{
					// What to do on failure?
					assert(!"Can't fail");
				}
				if	(DirTree_WriteToFile(File->Directory, File->RWOps) == GE_FALSE)
				{
					// What to do on failure?
					assert(!"Can't fail");
				}
				geVFile_Tell(File->RWOps, &EndPosition);
				if	(geVFile_Seek(File->RWOps, File->RWOpsStartPos, GE_VFILE_SEEKSET) == GE_FALSE)
				{
					// What to do on failure?
					assert(!"Can't fail");
				}
				Header.Signature = VFSFILEHEADER_SIGNATURE;
				Header.Version = HEADER_VERSION;
				Header.Dispersed = GE_FALSE;
				Header.EndPosition = EndPosition;

				Header.DirectoryOffset = File->RWOpsStartPos + File->DataLength;
				Header.DataLength = File->DataLength;
				if	(geVFile_Write(File->RWOps, &Header, sizeof(Header)) == GE_FALSE)
				{
					// What to do on failure?
					assert(!"Can't fail");
				}

				// Make sure that we end up at the end of the RWOps file
				geVFile_Seek(File->RWOps, EndPosition, GE_VFILE_SEEKSET);
			}
			else
			{
				// Have to make sure that we leave the file pointer at the end
				// of our data in the RWOps file that we come from.

				geVFile_Seek(File->RWOps, File->EndPosition, GE_VFILE_SEEKSET);
			}

			DirTree_Destroy(File->Directory);
		}
	}
	else
	{
		// Update the system with the length of this file.  Subsequent
		// file operations will follow this file.
		assert(File->System);
		File->System->DataLength += File->Length;
		DirTree_SetFileSize(File->DirEntry, File->Length);
	}

	geRam_Free(File);
}

static	geBoolean	GENESISCC ForceFilePos(VFSFile *File)
{
	assert(File);
	assert(File->RWOps);
	assert(!File->Directory);
	
	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	return geVFile_Seek(File->RWOps,
					  File->RWOpsStartPos + File->CurrentRelPos,
					  GE_VFILE_SEEKSET);
}

static	int			ClampOperationSize(const VFSFile *File, int Size)
{
	assert(!File->Directory);
	assert(File->CurrentRelPos >= 0);
	return min(File->Length - File->CurrentRelPos, Size);
}

static	void		GENESISCC UpdateFilePos(VFSFile *File)
{
	long	RWOpsPos;

	assert(!File->Directory);
	assert(File->CurrentRelPos >= 0);

	geVFile_Tell(File->RWOps, &RWOpsPos);

	File->CurrentRelPos = RWOpsPos - File->RWOpsStartPos;
	if	(File->CurrentRelPos > File->Length)
		File->Length = File->CurrentRelPos;

	assert(File->CurrentRelPos >= 0);
}

static	geBoolean	GENESISCC FSVFS_GetS(void *Handle, void *Buff, int MaxLen)
{
	VFSFile *	File;
	geBoolean	Res;

	assert(Buff);
	assert(MaxLen != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Directory)
		return GE_FALSE;

	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	if	(!ForceFilePos(File))
		return GE_FALSE;

	MaxLen = ClampOperationSize(File, MaxLen);

	Res = geVFile_GetS(File->RWOps, Buff, MaxLen);

	UpdateFilePos(File);

	return Res;
}

static	geBoolean	GENESISCC FSVFS_Read(void *Handle, void *Buff, int Count)
{
	VFSFile *	File;
	geBoolean	Res;
#ifndef	NDEBUG
	int			CurRelPos;
#endif

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Directory)
		return GE_FALSE;

	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	if	(!ForceFilePos(File))
		return GE_FALSE;

	if	(ClampOperationSize(File, Count) != Count)
		return GE_FALSE;

#ifndef	NDEBUG
	CurRelPos = File->CurrentRelPos;
#endif
	Res = geVFile_Read(File->RWOps, Buff, Count);

	UpdateFilePos(File);
	assert(File->CurrentRelPos - CurRelPos == Count);

	return Res;
}

static	geBoolean	GENESISCC FSVFS_Write(void *Handle, const void *Buff, int Count)
{
	VFSFile *	File;
	geBoolean	Res;
#ifndef	NDEBUG
	int			CurRelPos;
#endif

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Directory)
		return GE_FALSE;

	if	(File->OpenModeFlags & GE_VFILE_OPEN_READONLY)
		return GE_FALSE;

	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	if	(!ForceFilePos(File))
		return GE_FALSE;

#ifndef	NDEBUG
	CurRelPos = File->CurrentRelPos;
#endif
	Res = geVFile_Write(File->RWOps, Buff, Count);

	UpdateFilePos(File);
	assert(File->CurrentRelPos - CurRelPos == Count);

	return Res;
}

static	geBoolean	GENESISCC FSVFS_Seek(void *Handle, int Where, geVFile_Whence Whence)
{
	VFSFile *	File;
	geBoolean	Res;
	long		AbsolutePos;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Directory)
		return GE_FALSE;

	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	switch	(Whence)
	{
	case	GE_VFILE_SEEKSET:
		AbsolutePos = File->RWOpsStartPos + Where;
		break;

	case	GE_VFILE_SEEKEND:
		AbsolutePos = File->RWOpsStartPos + File->Length - Where;
		break;

	case	GE_VFILE_SEEKCUR:
		AbsolutePos = File->RWOpsStartPos + File->CurrentRelPos + Where;
		break;

	default:
		assert(!"Illegal seek case");
	}

	if	(AbsolutePos < File->RWOpsStartPos)
		return GE_FALSE;

	Res = geVFile_Seek(File->RWOps, AbsolutePos, GE_VFILE_SEEKSET);

	UpdateFilePos(File);

	return Res;
}

static	geBoolean	GENESISCC FSVFS_EOF(const void *Handle)
{
	const VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Directory)
		return GE_FALSE;

	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	if	(File->CurrentRelPos == File->Length)
		return GE_TRUE;

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSVFS_Tell(const void *Handle, long *Position)
{
	const VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Directory)
		return GE_FALSE;

	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	*Position = File->CurrentRelPos;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSVFS_Size(const void *Handle, long *Size)
{
	const VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->Directory != NULL)
		return GE_FALSE;

	assert(File->CurrentRelPos >= 0);
	assert(File->CurrentRelPos <= File->Length);

	*Size = File->Length;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSVFS_GetProperties(const void *Handle, geVFile_Properties *Properties)
{
	const VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->DirEntry);

	DirTree_GetFileTime(File->DirEntry, &Properties->Time);
	DirTree_GetFileAttributes(File->DirEntry, &Properties->AttributeFlags);
	DirTree_GetFileSize(File->DirEntry, &Properties->Size);
	DirTree_GetFileHints(File->DirEntry, &Properties->Hints);
	return DirTree_GetName(File->DirEntry, &Properties->Name[0], sizeof(Properties->Name));
}

static	geBoolean	GENESISCC FSVFS_SetSize(void *Handle, long Size)
{
	assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSVFS_SetAttributes(void *Handle, geVFile_Attributes Attributes)
{
	const VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->DirEntry);

	if	(Attributes & ~GE_VFILE_ATTRIB_READONLY)
		return GE_FALSE;

	DirTree_SetFileAttributes(File->DirEntry, Attributes);

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSVFS_SetTime(void *Handle, const geVFile_Time *Time)
{
	const VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->DirEntry);

	DirTree_SetFileTime(File->DirEntry, Time);

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSVFS_SetHints(void *Handle, const geVFile_Hints *Hints)
{
	const VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->DirEntry);

	return DirTree_SetFileHints(File->DirEntry, Hints);
}

static	geBoolean	GENESISCC FSVFS_FileExists(geVFile *FS, void *Handle, const char *Name)
{
	VFSFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(!File->Directory)
		return GE_FALSE;

	return DirTree_FileExists(File->Directory, Name);
}

static	geBoolean	GENESISCC FSVFS_Disperse(
	geVFile *	FS,
	void *		Handle,
	const char *Directory,
	geBoolean	Recursive)
{
assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSVFS_DeleteFile(geVFile *FS, void *Handle, const char *Name)
{
assert(!"Not implemented");
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSVFS_RenameFile(geVFile *FS, void *Handle, const char *Name, const char *NewName)
{
assert(!"Not implemented");
	return GE_FALSE;
}

static	geVFile_SystemAPIs	FSVFS_APIs =
{
	FSVFS_FinderCreate,
	FSVFS_FinderGetNextFile,
	FSVFS_FinderGetProperties,
	FSVFS_FinderDestroy,

	FSVFS_OpenNewSystem,
	FSVFS_UpdateContext,
	FSVFS_Open,
	FSVFS_DeleteFile,
	FSVFS_RenameFile,
	FSVFS_FileExists,
	FSVFS_Disperse,
	FSVFS_Close,

	FSVFS_GetS,
	FSVFS_Read,
	FSVFS_Write,
	FSVFS_Seek,
	FSVFS_EOF,
	FSVFS_Tell,
	FSVFS_Size,

	FSVFS_GetProperties,

	FSVFS_SetSize,
	FSVFS_SetAttributes,
	FSVFS_SetTime,
	FSVFS_SetHints,
};

const geVFile_SystemAPIs * GENESISCC FSVFS_GetAPIs(void)
{
	return &FSVFS_APIs;
}

