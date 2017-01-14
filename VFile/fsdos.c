/****************************************************************************************/
/*  FSDOS.C                                                                             */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: DOS file system implementation                                         */
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

#include	"fsdos.h"

//	"DF01"
#define	DOSFILE_SIGNATURE	0x31304644

//	"DF02"
#define	DOSFINDER_SIGNATURE	0x32304644

#define	CHECK_HANDLE(H)	assert(H);assert(H->Signature == DOSFILE_SIGNATURE);
#define	CHECK_FINDER(F)	assert(F);assert(F->Signature == DOSFINDER_SIGNATURE);

typedef struct	DosFile
{
	unsigned int	Signature;
	HANDLE			FileHandle;
	char *			FullPath;
	const char *	Name;
	geBoolean		IsDirectory;
}	DosFile;

typedef	struct	DosFinder
{
	unsigned int	Signature;
	HANDLE			FindHandle;
	WIN32_FIND_DATA	FindData;
	geBoolean		FirstStillCached;
	int				OffsetToName;
}	DosFinder;

static	geBoolean	BuildFileName(
	const DosFile *	File,
	const char *	Name,
	char *			Buff,
	char **			NamePtr,
	int 			MaxLen)
{
	int		DirLength;
	int		NameLength;

	if ( ! Name )
		return GE_FALSE;

	if	(File)
	{
		if	(File->IsDirectory == GE_FALSE)
			return GE_FALSE;

		assert(File->FullPath);
		DirLength = strlen(File->FullPath);

		if	(DirLength > MaxLen)
			return GE_FALSE;

		memcpy(Buff, File->FullPath, DirLength);
	}
	else
	{
		DirLength = 0;
	}

	NameLength = strlen(Name);
	if	(DirLength + NameLength + 2 > MaxLen || ! Buff )
		return GE_FALSE;

	if	(DirLength != 0)
	{
		Buff[DirLength] = '\\';
		memcpy(Buff + DirLength + 1, Name, NameLength + 1);
		if	(NamePtr)
			*NamePtr = Buff + DirLength + 1;
	}
	else
	{
		memcpy(Buff, Name, NameLength + 1);
		if	(NamePtr)
			*NamePtr = Buff;

		// Special case: no directory, no file name.  We meant something like ".\"
		if	(!*Buff)
		{
			strcpy (Buff, ".\\");
		}
	}

	return GE_TRUE;
}

static	void *	GENESISCC FSDos_FinderCreate(
	geVFile *			FS,
	void *			Handle,
	const char *	FileSpec)
{
	DosFinder *		Finder;
	DosFile *		File;
	char *			NamePtr;
	char			Buff[_MAX_PATH];

	assert(FileSpec != NULL);

	File = Handle;

	CHECK_HANDLE(File);

	Finder = geRam_Allocate(sizeof(*Finder));
	if	(!Finder)
		return NULL;

	memset(Finder, 0, sizeof(*Finder));

	if	(BuildFileName(File, FileSpec, Buff, &NamePtr, sizeof(Buff)) == GE_FALSE)
	{
		geRam_Free(Finder);
		return NULL;
	}

	Finder->OffsetToName = NamePtr - Buff;

	Finder->FindHandle = FindFirstFile(Buff, &Finder->FindData);

	Finder->FirstStillCached = GE_TRUE;

	Finder->Signature = DOSFINDER_SIGNATURE;
	return (void *)Finder;
}

static	geBoolean	GENESISCC FSDos_FinderGetNextFile(void *Handle)
{
	DosFinder *	Finder;

	Finder = Handle;

	CHECK_FINDER(Finder);

	if	(Finder->FindHandle == INVALID_HANDLE_VALUE)
		return GE_FALSE;

	if	(Finder->FirstStillCached == GE_TRUE)
	{
		Finder->FirstStillCached = GE_FALSE;

		if	(Finder->FindData.cFileName[0] != '.')
			return GE_TRUE;
	}
	
	while	(FindNextFile(Finder->FindHandle, &Finder->FindData) == TRUE)
	{
		if	(Finder->FindData.cFileName[0] != '.')
			return GE_TRUE;
	}

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_FinderGetProperties(void *Handle, geVFile_Properties *Props)
{
	DosFinder *			Finder;
	geVFile_Attributes	Attribs;
	int					Length;

	assert(Props);

	Finder = Handle;

	CHECK_FINDER(Finder);

	if	(Finder->FindHandle == INVALID_HANDLE_VALUE)
		return GE_FALSE;

	Attribs = 0;
	if	(Finder->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		Attribs |= GE_VFILE_ATTRIB_DIRECTORY;
	if	(Finder->FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		Attribs |= GE_VFILE_ATTRIB_READONLY;

	Props->Time.Time1 = Finder->FindData.ftLastWriteTime.dwLowDateTime;
	Props->Time.Time2 = Finder->FindData.ftLastWriteTime.dwHighDateTime;

	Props->AttributeFlags = Attribs;
	Props->Size = Finder->FindData.nFileSizeLow;
	Props->Hints.HintData = NULL;
	Props->Hints.HintDataLength = 0;

	Length = strlen(Finder->FindData.cFileName);
	if	(Length > sizeof(Props->Name) - 1)
		return GE_FALSE;
	memcpy(Props->Name, Finder->FindData.cFileName, Length + 1);

	return GE_TRUE;
}

static	void GENESISCC FSDos_FinderDestroy(void *Handle)
{
	DosFinder *	Finder;

	Finder = Handle;

	CHECK_FINDER(Finder);

	if	(Finder->FindHandle != INVALID_HANDLE_VALUE)
		FindClose(Finder->FindHandle);

	Finder->Signature = 0;
	geRam_Free(Finder);
}

// Terrible function.  It mutated, and now it modifies its argument.
static	geBoolean	IsRootDirectory(char *Path)
{
	int		SlashCount;

	// Drive letter test
	if	(Path[1] == ':' && Path[2] == '\\' && Path[3] == '\0')
	{
		Path[2] = '\0';
		return GE_TRUE;
	}

	// Now UNC path test
	SlashCount = 0;
	if	(Path[0] == '\\' && Path[1] == '\\')
	{
		Path += 2;
		while	(*Path)
		{
			if	(*Path++ == '\\')
				SlashCount++;
		}
	}

	if	(SlashCount == 1)
		return GE_TRUE;

	return GE_FALSE;
}

static	void *	GENESISCC FSDos_Open(
	geVFile *		FS,
	void *			Handle,
	const char *	Name,
	void *			Context,
	unsigned int 	OpenModeFlags)
{
	DosFile *	DosFS;
	DosFile *	NewFile;
	char		Buff[_MAX_PATH];
	int			Length;
	char *		NamePtr;

	DosFS = Handle;

	if	(DosFS && DosFS->IsDirectory != GE_TRUE)
		return NULL;

	NewFile = geRam_Allocate(sizeof(*NewFile));
	if	(!NewFile)
		return NewFile;

	memset(NewFile, 0, sizeof(*NewFile));

	if	(BuildFileName(DosFS, Name, Buff, &NamePtr, sizeof(Buff)) == GE_FALSE)
		goto fail;

	Length = strlen(Buff);
	NewFile->FullPath = geRam_Allocate(Length + 1);
	if	(!NewFile->FullPath)
		goto fail;

	NewFile->Name = NewFile->FullPath + (NamePtr - &Buff[0]);

	memcpy(NewFile->FullPath, Buff, Length + 1);

	if	(OpenModeFlags & GE_VFILE_OPEN_DIRECTORY)
	{
		WIN32_FIND_DATA	FileInfo;
		HANDLE			FindHandle;
		geBoolean		IsDirectory;

		assert(!DosFS || DosFS->IsDirectory == GE_TRUE);

		memset(&FileInfo, 0, sizeof(FileInfo));
		FindHandle = FindFirstFile(NewFile->FullPath, &FileInfo);
		if	(FindHandle != INVALID_HANDLE_VALUE &&
			 FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			IsDirectory = GE_TRUE;
		}
		else
		{
			IsDirectory = IsRootDirectory(NewFile->FullPath);
		}

		FindClose (FindHandle);

		if	(OpenModeFlags & GE_VFILE_OPEN_CREATE)
		{
			if	(IsDirectory == GE_TRUE)
				goto fail;

			if	(CreateDirectory(NewFile->FullPath, NULL) == FALSE)
				goto fail;
		}
		else
		{
			if	(IsDirectory != GE_TRUE)
				goto fail;
		}

		NewFile->IsDirectory = GE_TRUE;
		NewFile->FileHandle = INVALID_HANDLE_VALUE;
	}
	else
	{
		DWORD			ShareMode;
		DWORD			CreationMode;
		DWORD			Access;
		DWORD			LastError;

		CreationMode = OPEN_EXISTING;

		switch	(OpenModeFlags & (GE_VFILE_OPEN_READONLY |
								  GE_VFILE_OPEN_UPDATE	 |
								  GE_VFILE_OPEN_CREATE))
		{
		case	GE_VFILE_OPEN_READONLY:
			Access = GENERIC_READ;
			ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			break;

		case	GE_VFILE_OPEN_CREATE:
			CreationMode = CREATE_ALWAYS;
			// Fall through

		case	GE_VFILE_OPEN_UPDATE:
			Access = GENERIC_READ | GENERIC_WRITE;
			ShareMode = FILE_SHARE_READ;
			break;

		default:
			assert(!"Illegal open mode flags");
			break;
		}

		NewFile->FileHandle = CreateFile(NewFile->FullPath,
										 Access,
										 ShareMode,
										 NULL,
										 CreationMode,
										 0,
										 NULL);
		if	(NewFile->FileHandle == INVALID_HANDLE_VALUE)
			{
				LastError = GetLastError();
				goto fail;
			}
	}

	NewFile->Signature = DOSFILE_SIGNATURE;

	return (void *)NewFile;

fail:
	if	(NewFile->FullPath)
		geRam_Free(NewFile->FullPath);
	geRam_Free(NewFile);
	return NULL;
}

static	void *	GENESISCC FSDos_OpenNewSystem(
	geVFile *			FS,
	const char *	Name,
	void *			Context,
	unsigned int 	OpenModeFlags)
{
	return FSDos_Open(FS, NULL, Name, Context, OpenModeFlags);
}

static	geBoolean	GENESISCC FSDos_UpdateContext(
	geVFile *		FS,
	void *			Handle,
	void *			Context,
	int 			ContextSize)
{
	return GE_FALSE;
}

static	void	GENESISCC FSDos_Close(void *Handle)
{
	DosFile *	File;
	
	File = Handle;
	
	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_FALSE)
	{
		assert(File->FileHandle != INVALID_HANDLE_VALUE);

		CloseHandle(File->FileHandle);
	}
	
	assert(File->FullPath);
	File->Signature = 0;
	geRam_Free(File->FullPath);
	geRam_Free(File);
}

static	geBoolean	GENESISCC FSDos_GetS(void *Handle, void *Buff, int MaxLen)
{
	DosFile *	File;
	DWORD		BytesRead;
	BOOL		Result;
	char *		p;
	char *		End;

	assert(Buff);
	assert(MaxLen != 0);

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

	Result = ReadFile(File->FileHandle, Buff, MaxLen - 1, &BytesRead, NULL);
	if	(BytesRead == 0)
	{
#if 0
		if	(Result == FALSE)
			return GE_FALSE;
		
		// The Win32 API is vague about this, so we're being weird with the asserts
		assert(Result != TRUE);
#endif
		return GE_FALSE;
	}

	End = (char *)Buff + BytesRead;
	p = Buff;
	while	(p < End)
	{
		/*
		  This code will terminate a line on one of three conditions:
			\r	Character changed to \n, next char set to 0
			\n	Next char set to 0
			\r\n	First \r changed to \n.  \n changed to 0.
		*/
		if	(*p == '\r')
		{
			int Skip = 0;
			
			*p = '\n';		// set end of line
			p++;			// and skip to next char
			// If the next char is a newline, then skip it too  (\r\n case)
			if (*p == '\n')
			{
				Skip = 1;
			}
			*p = '\0';
			// Set the file pointer back a bit since we probably overran
			SetFilePointer(File->FileHandle, -(int)(BytesRead - ((p + Skip) - (char *)Buff)), NULL, FILE_CURRENT); 
			assert(p - (char *)Buff <= MaxLen);
			return GE_TRUE;
		}
		else if	(*p == '\n')
		{
			// Set the file pointer back a bit since we probably overran
			p++;
			SetFilePointer(File->FileHandle, -(int)(BytesRead - (p - (char *)Buff)), NULL, FILE_CURRENT); 
			*p = '\0';
			assert(p - (char *)Buff <= MaxLen);
			return GE_TRUE;
		}
		p++;
	}

	return GE_FALSE;
}


static	geBoolean	GENESISCC FSDos_Read(void *Handle, void *Buff, int Count)
{
	DosFile *	File;
	DWORD		BytesRead;

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#ifdef	ELIDEBUG
{
	FILE *	fp;
	long	Position;

	Position = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
	fp = fopen("c:\\vfs.eli", "ab+");
	fprintf(fp, "FSDos_Read: %-8d bytes @ %-8d\r\n", Count, Position);
	fclose(fp);
}
#endif

	if	(ReadFile(File->FileHandle, Buff, Count, &BytesRead, NULL) == FALSE)
		return GE_FALSE;

	if	(BytesRead == 0)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Write(void *Handle, const void *Buff, int Count)
{
	DosFile *	File;
	DWORD		BytesWritten;

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#ifdef	ELIDEBUG
{
	FILE *	fp;
	long	Position;

	Position = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
	fp = fopen("c:\\vfs.eli", "ab+");
	fprintf(fp, "FSDos_Write: %-8d bytes @ %-8d\r\n", Count, Position);
	fclose(fp);
}
#endif

	if	(WriteFile(File->FileHandle, Buff, Count, &BytesWritten, NULL) == FALSE)
		return GE_FALSE;

	if	((int)BytesWritten != Count)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Seek(void *Handle, int Where, geVFile_Whence Whence)
{
	int			RTLWhence;
	DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

	switch	(Whence)
	{
	case	GE_VFILE_SEEKCUR:
		RTLWhence = FILE_CURRENT;
		break;

	case	GE_VFILE_SEEKEND:
		RTLWhence = FILE_END;
		break;

	case	GE_VFILE_SEEKSET:
		RTLWhence = FILE_BEGIN;
		break;
	default:
		assert(!"Unknown seek kind");
	}

	if	(SetFilePointer(File->FileHandle, Where, NULL, RTLWhence) == 0xffffffff)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_EOF(const void *Handle)
{
	const DosFile *	File;
	DWORD			CurPos;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	CurPos = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
	assert(CurPos != 0xffffffff);

	if	(CurPos == GetFileSize(File->FileHandle, NULL))
		return GE_TRUE;

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_Tell(const void *Handle, long *Position)
{
	const DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	*Position = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
	if	(*Position == -1L)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Size(const void *Handle, long *Size)
{
	const DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	*Size = GetFileSize(File->FileHandle, NULL);
	if	(*Size != (long)0xffffffff)
		return GE_TRUE;

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_GetProperties(const void *Handle, geVFile_Properties *Properties)
{
	const DosFile *				File;
	geVFile_Attributes			Attribs;
	BY_HANDLE_FILE_INFORMATION	Info;
	int							Length;

	assert(Properties);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
	{
		memset(Properties, 0, sizeof(*Properties));
		Properties->AttributeFlags = FILE_ATTRIBUTE_DIRECTORY;
#pragma message ("FSDos_GetProperties: Time support is not there for directories")
	}
	else
	{
		assert(File->FileHandle != INVALID_HANDLE_VALUE);
	
		if	(GetFileInformationByHandle(File->FileHandle, &Info) == FALSE)
			return GE_FALSE;
	
		Attribs = 0;
		if	(Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			Attribs |= GE_VFILE_ATTRIB_DIRECTORY;
		if	(Info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			Attribs |= GE_VFILE_ATTRIB_READONLY;
	
		Properties->Time.Time1 = Info.ftLastWriteTime.dwLowDateTime;
		Properties->Time.Time2 = Info.ftLastWriteTime.dwHighDateTime;
	
		Properties->AttributeFlags 		 = Attribs;
		Properties->Size		  		 = Info.nFileSizeLow;
		Properties->Hints.HintData		 = NULL;
		Properties->Hints.HintDataLength = 0;
	}

	Length = strlen(File->Name) + 1;
	if	(Length > sizeof(Properties->Name))
		return GE_FALSE;
	memcpy(Properties->Name, File->Name, Length);

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_SetSize(void *Handle, long size)
{
	DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_FALSE)
	{
		assert(File->FileHandle != INVALID_HANDLE_VALUE);
	
		if	(SetFilePointer(File->FileHandle, 0, NULL, FILE_END) == 0xffffffff)
			return GE_FALSE;
	
		if	(SetEndOfFile(File->FileHandle) == FALSE)
			return GE_FALSE;
	}

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_SetAttributes(void *Handle, geVFile_Attributes Attributes)
{
	DosFile *	File;
	DWORD		Win32Attributes;

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

	if	(Attributes & GE_VFILE_ATTRIB_READONLY)
		Win32Attributes = FILE_ATTRIBUTE_READONLY;
	else
		Win32Attributes = FILE_ATTRIBUTE_NORMAL;

	if	(SetFileAttributes(File->FullPath, Win32Attributes) == FALSE)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_SetTime(void *Handle, const geVFile_Time *Time)
{
	DosFile *	File;
	FILETIME	Win32Time;

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	Win32Time.dwLowDateTime  = Time->Time1;
	Win32Time.dwHighDateTime = Time->Time2;
	if	(SetFileTime(File->FileHandle, &Win32Time, &Win32Time, &Win32Time) == FALSE)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_SetHints(void *Handle, const geVFile_Hints *Hints)
{
	DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_FileExists(geVFile *FS, void *Handle, const char *Name)
{
	DosFile *	File;
	char		Buff[_MAX_PATH];

	File = Handle;

	if	(File && File->IsDirectory == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, Name, Buff, NULL, sizeof(Buff)) == GE_FALSE)
		return GE_FALSE;

	if	(GetFileAttributes(Buff) == 0xffffffff)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Disperse(
	geVFile *		FS,
	void *		Handle,
	const char *Directory,
	geBoolean	Recursive)
{
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_DeleteFile(geVFile *FS, void *Handle, const char *Name)
{
	DosFile *	File;
	char		Buff[_MAX_PATH];

	File = Handle;

	if	(File && File->IsDirectory == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, Name, Buff, NULL, sizeof(Buff)) == GE_FALSE)
		return GE_FALSE;

	if	(DeleteFile(Buff) == FALSE)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_RenameFile(geVFile *FS, void *Handle, const char *Name, const char *NewName)
{
	DosFile *	File;
	char		Old[_MAX_PATH];
	char		New[_MAX_PATH];

	File = Handle;

	if	(File && File->IsDirectory == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, Name, Old, NULL, sizeof(Old)) == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, NewName, New, NULL, sizeof(New)) == GE_FALSE)
		return GE_FALSE;

	if	(MoveFile(Old, New) == FALSE)
		return GE_FALSE;

	return GE_TRUE;
}

static	geVFile_SystemAPIs	FSDos_APIs =
{
	FSDos_FinderCreate,
	FSDos_FinderGetNextFile,
	FSDos_FinderGetProperties,
	FSDos_FinderDestroy,

	FSDos_OpenNewSystem,
	FSDos_UpdateContext,
	FSDos_Open,
	FSDos_DeleteFile,
	FSDos_RenameFile,
	FSDos_FileExists,
	FSDos_Disperse,
	FSDos_Close,

	FSDos_GetS,
	FSDos_Read,
	FSDos_Write,
	FSDos_Seek,
	FSDos_EOF,
	FSDos_Tell,
	FSDos_Size,

	FSDos_GetProperties,

	FSDos_SetSize,
	FSDos_SetAttributes,
	FSDos_SetTime,
	FSDos_SetHints,
};

const geVFile_SystemAPIs *GENESISCC FSDos_GetAPIs(void)
{
	return &FSDos_APIs;
}

