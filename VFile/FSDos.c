/****************************************************************************************/
/*  FSDos.c                                                                             */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                    */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved                            */
/*                                                                                      */
/****************************************************************************************/

#if defined(__linux__)
#include <linux/limits.h> // c11
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define DIRECTORY_SEPARATOR '/'
#define CURRENT_DIRECTORY "./"

#define INVALID_HANDLE_VALUE 0

#elif defined _WIN32 || defined _WIN64
#define WIN32_LEAN_AND_MEAN
#include	<windows.h>
#define DIRECTORY_SEPARATOR '\\'
#define CURRENT_DIRECTORY ".\\"
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	"BaseType.h"
#include	"Ram.h"

#include	"VFile.h"
#include	"VFile._h"

#include	"FSDos.h"

//	"DF01"
#define	DOSFILE_SIGNATURE	0x31304644

//	"DF02"
#define	DOSFINDER_SIGNATURE	0x32304644

#define	CHECK_HANDLE(H)	assert(H);assert(H->Signature == DOSFILE_SIGNATURE);
#define	CHECK_FINDER(F)	assert(F);assert(F->Signature == DOSFINDER_SIGNATURE);

typedef struct	DosFile
{
	uint32	Signature;
#if defined(__linux__)
	int32				FileHandle;
#elif defined _WIN32 || defined _WIN64
	HANDLE			FileHandle;
#endif
	char *			FullPath;
	const char *	Name;
	geBoolean		IsDirectory;
}	DosFile;

typedef	struct	DosFinder
{
	uint32	Signature;
#if defined(__linux__)
	DIR *			FindDir;
	struct dirent *FindHandle;
	char			DirName[PATH_MAX];
#elif defined _WIN32 || defined _WIN64
	HANDLE			FindHandle;
	WIN32_FIND_DATA	FindData;
#endif
	geBoolean		FirstStillCached;
	int32				OffsetToName;
}	DosFinder;

static	geBoolean	BuildFileName(
	const DosFile *	File,
	const char *	Name,
	char *			Buff,
	char **			NamePtr,
	int32 			MaxLen)
{
	int32		DirLength;
	int32		NameLength;

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
		Buff[DirLength] = DIRECTORY_SEPARATOR;
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
			strcpy (Buff, CURRENT_DIRECTORY);
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
#if defined(__linux__)
	char *			Buff;//[PATH_MAX];
#elif defined _WIN32 || defined _WIN64
	char			Buff[_MAX_PATH];
#endif
	(void)FS;

	assert(FileSpec != NULL);

	File = Handle;

	CHECK_HANDLE(File);

	Finder = geRam_Allocate(sizeof(*Finder));
	if	(!Finder)
		return NULL;

	memset(Finder, 0, sizeof(*Finder));

#if defined(__linux__)
	Buff = Finder->DirName; // on Linux save path for later usage
#endif
	if	(BuildFileName(File, FileSpec, Buff, &NamePtr, sizeof(Buff)) == GE_FALSE)
	{
		geRam_Free(Finder);
		return NULL;
	}

	Finder->OffsetToName = NamePtr - Buff;

#if defined(__linux__)
	Finder->FindDir = opendir(Finder->DirName);
	if	(!Finder->FindDir)
		return NULL;
	Finder->FindHandle = readdir(Finder->FindDir);
#elif defined _WIN32 || defined _WIN64
	Finder->FindHandle = FindFirstFile(Buff, &Finder->FindData);
#endif
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

#if defined(__linux__)
		if	(Finder->FindHandle->d_name[0] != '.')
#elif defined _WIN32 || defined _WIN64
		if	(Finder->FindData.cFileName[0] != '.')
#endif
			return GE_TRUE;
	}

#if defined(__linux__)
	while	((Finder->FindHandle = readdir(Finder->FindDir)) != 0)
#elif defined _WIN32 || defined _WIN64
	while	(FindNextFile(Finder->FindHandle, &Finder->FindData) != 0)
#endif
	{
#if defined(__linux__)
		if	(Finder->FindHandle->d_name[0] != '.')
#elif defined _WIN32 || defined _WIN64
		if	(Finder->FindData.cFileName[0] != '.')
#endif
			return GE_TRUE;
	}

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_FinderGetProperties(void *Handle, geVFile_Properties *Props)
{
	DosFinder *			Finder;
	geVFile_Attributes	Attribs;
	size_t				Length;

	assert(Props);

	Finder = Handle;

	CHECK_FINDER(Finder);

	if	(Finder->FindHandle == INVALID_HANDLE_VALUE)
		return GE_FALSE;

	Attribs = 0;
#if defined(__linux__)
	{
	char full_file_name[PATH_MAX];
	strcpy(full_file_name, Finder->DirName);
	strcat(full_file_name, "/");
	strcat(full_file_name, Finder->FindHandle->d_name);
	if	(Finder->FindHandle->d_name[0] == '.')
		return GE_FALSE;

	struct stat FindData;
	if (stat(full_file_name, &FindData) == -1)
		return GE_FALSE;
	if	(S_ISDIR(FindData.st_mode))
		Attribs |= GE_VFILE_ATTRIB_DIRECTORY;
	if (!(FindData.st_mode & S_IWUSR))
		Attribs |= GE_VFILE_ATTRIB_READONLY;

	Props->Time.Time1 = FindData.st_mtime;
#if defined __USE_MISC || defined __USE_XOPEN2K8
	Props->Time.Time2 = FindData.st_mtim.tv_nsec;
#else
	Props->Time.Time2 = FindData.st_mtimensec; // c11
#endif

	Props->AttributeFlags = Attribs;
	Props->Size = FindData.st_size;
	Props->Hints.HintData = NULL;
	Props->Hints.HintDataLength = 0;

	Length = strlen(Finder->FindHandle->d_name);
	if	(Length > sizeof(Props->Name) - 1)
		return GE_FALSE;
	memcpy(Props->Name, Finder->FindHandle->d_name, Length + 1);
	}
#else
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
#endif
	return GE_TRUE;
}

static	void GENESISCC FSDos_FinderDestroy(void *Handle)
{
	DosFinder *	Finder;

	Finder = Handle;

	CHECK_FINDER(Finder);

#if defined(__linux__)
	if	(Finder->FindDir)
		closedir(Finder->FindDir);
#elif defined _WIN32 || defined _WIN64
	if	(Finder->FindHandle != INVALID_HANDLE_VALUE)
		FindClose(Finder->FindHandle);
#endif

	Finder->Signature = 0;
	geRam_Free(Finder);
}

// Terrible function.  It mutated, and now it modifies its argument.
static	geBoolean	IsRootDirectory(char *Path)
{
#if defined(__linux__)
	if (Path[0] == '/' && Path[1] == '\0')
	{
		return GE_TRUE;
	}
#elif defined _WIN32 || defined _WIN64
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
#endif
	return GE_FALSE;
}

static	void *	GENESISCC FSDos_Open(
	geVFile *		FS,
	void *			Handle,
	const char *	Name,
	void *			Context,
	uint32	OpenModeFlags)
{
	DosFile *	DosFS;
	DosFile *	NewFile;
#if defined(__linux__)
	char		Buff[PATH_MAX];
#elif defined _WIN32 || defined _WIN64
	char		Buff[_MAX_PATH];
#endif

	int32		Length;
	char *		NamePtr;
	(void)FS;
	(void)Context;

	DosFS = Handle;

	if	(DosFS && DosFS->IsDirectory != GE_TRUE)
		return NULL;

	NewFile = (DosFile*)geRam_Allocate(sizeof(*NewFile));
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
#if defined(__linux__)
		struct stat		FileInfo;
#elif defined _WIN32 || defined _WIN64
		WIN32_FIND_DATA	FileInfo;
		HANDLE			FindHandle;
#endif
		geBoolean		IsDirectory;

		assert(!DosFS || DosFS->IsDirectory == GE_TRUE);

		memset(&FileInfo, 0, sizeof(FileInfo));

#if defined(__linux__)
		if	(stat(NewFile->FullPath, &FileInfo) != -1 &&
			 S_ISDIR(FileInfo.st_mode))
#elif defined _WIN32 || defined _WIN64
		FindHandle = FindFirstFile(NewFile->FullPath, &FileInfo);
		if	(FindHandle != INVALID_HANDLE_VALUE &&
			 FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#endif
		{
			IsDirectory = GE_TRUE;
		}
		else
		{
			IsDirectory = IsRootDirectory(NewFile->FullPath);
		}

#if defined(__linux__)
		//nothing to be done
#elif defined _WIN32 || defined _WIN64
		FindClose (FindHandle);
#endif

		if	(OpenModeFlags & GE_VFILE_OPEN_CREATE)
		{
			if	(IsDirectory == GE_TRUE)
				goto fail;

#if defined(__linux__)
			if	(mkdir(NewFile->FullPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
#elif defined _WIN32 || defined _WIN64
			if	(CreateDirectory(NewFile->FullPath, NULL) == FALSE)
#endif
				goto fail;
		}
		else
		{
			if	(IsDirectory != GE_TRUE)
				goto fail;
		}

		NewFile->IsDirectory = GE_TRUE;
#if defined(__linux__)
		NewFile->FileHandle = -1;
#elif defined _WIN32 || defined _WIN64
		NewFile->FileHandle = INVALID_HANDLE_VALUE;
#endif
	}
	else
	{
#if defined(__linux__)
		int32 OpenMode = 0;
		mode_t AccessMode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
#elif defined_WIN32 || defined _WIN64
		DWORD			ShareMode=0;
		DWORD			CreationMode;
		DWORD			Access=0;
		DWORD			LastError;

		CreationMode = OPEN_EXISTING;
#endif
		switch	(OpenModeFlags & (GE_VFILE_OPEN_READONLY |
								  GE_VFILE_OPEN_UPDATE	 |
								  GE_VFILE_OPEN_CREATE))
		{
		case	GE_VFILE_OPEN_READONLY:
#if defined(__linux__)
			OpenMode = O_RDONLY;
#elif defined_WIN32 || defined _WIN64
			Access = GENERIC_READ;
			ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
#endif
			break;

		case	GE_VFILE_OPEN_CREATE:
#if defined(__linux__)
			OpenMode = O_RDWR|O_CREAT|O_TRUNC;
			break;
#elif defined_WIN32 || defined _WIN64
			CreationMode = CREATE_ALWAYS;
			// Fall through
#endif

		case	GE_VFILE_OPEN_UPDATE:
#if defined(__linux__)
			OpenMode = O_RDWR;
#elif defined_WIN32 || defined _WIN64
			Access = GENERIC_READ | GENERIC_WRITE;
			ShareMode = FILE_SHARE_READ;
#endif
			break;

		default:
			assert(!"Illegal open mode flags");
			break;
		}

#if defined(__linux__)
		NewFile->FileHandle = open(NewFile->FullPath, OpenMode, AccessMode);

		if(NewFile->FileHandle == -1)
			goto fail;
#elif defined _WIN32 || defined _WIN64
		NewFile->FileHandle = CreateFile(NewFile->FullPath,
										 Access,
										 ShareMode,
										 NULL,
										 CreationMode,
										 0,
										 NULL);
		if	(NewFile->FileHandle == INVALID_HANDLE_VALUE)
		{
			goto fail;
		}
#endif
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
	uint32 	OpenModeFlags)
{
	return FSDos_Open(FS, NULL, Name, Context, OpenModeFlags);
}

static	geBoolean	GENESISCC FSDos_UpdateContext(
	geVFile *		FS,
	void *			Handle,
	void *			Context,
	int32			ContextSize)
{
	(void)FS;
	(void)Handle;
	(void)Context;
	(void)ContextSize;
	return GE_FALSE;
}

static	void	GENESISCC FSDos_Close(void *Handle)
{
	DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_FALSE)
	{
#if defined(__linux__)
		assert(File->FileHandle != -1);

		close(File->FileHandle);
#elif defined _WIN32 || defined _WIN64
		assert(File->FileHandle != INVALID_HANDLE_VALUE);

		CloseHandle(File->FileHandle);
#endif
	}

	assert(File->FullPath);
	File->Signature = 0;
	geRam_Free(File->FullPath);
	geRam_Free(File);
}

static	geBoolean	GENESISCC FSDos_GetS(void *Handle, void *Buff, int32 MaxLen)
{
	DosFile *	File;
#if defined(__linux__)
	ssize_t		BytesRead;
#elif defined _WIN32 || defined _WIN64
	DWORD		BytesRead;
	BOOL		Result;
#endif
	char *		p;
	char *		End;

	assert(Buff);
	assert(MaxLen != 0);

	File = Handle;

	CHECK_HANDLE(File);

#if defined(__linux__)
	assert(File->FileHandle != -1);
#elif defined _WIN32 || defined _WIN64
	assert(File->FileHandle != INVALID_HANDLE_VALUE);
#endif
	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#if defined(__linux__)
	BytesRead = read(File->FileHandle, Buff, MaxLen - 1);
#elif defined _WIN32 || defined _WIN64
	Result = ReadFile(File->FileHandle, Buff, MaxLen - 1, &BytesRead, NULL);
#endif
	if	(BytesRead <= 0)
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
#if defined(__linux__)
			lseek(File->FileHandle, -(int)(BytesRead - ((p + Skip) - (char *)Buff)), SEEK_CUR);
#elif defined _WIN32 || defined _WIN64
			SetFilePointer(File->FileHandle, -(int)(BytesRead - ((p + Skip) - (char *)Buff)), NULL, FILE_CURRENT);
#endif
			assert(p - (char *)Buff <= MaxLen);
			return GE_TRUE;
		}
		else if	(*p == '\n')
		{
			// Set the file pointer back a bit since we probably overran
			p++;
#if defined(__linux__)
			lseek(File->FileHandle, -(int)(BytesRead - (p - (char *)Buff)), SEEK_CUR);
#elif defined _WIN32 || defined _WIN64
			SetFilePointer(File->FileHandle, -(int)(BytesRead - (p - (char *)Buff)), NULL, FILE_CURRENT);
#endif
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
#if defined(__linux__)
	ssize_t		BytesRead;
#elif defined _WIN32 || defined _WIN64
	DWORD		BytesRead;
#endif

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#ifdef	ELIDEBUG
{
	FILE *	fp;
	int32	Position;

	Position = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
	fp = fopen("c:\\vfs.eli", "ab+");
	fprintf(fp, "FSDos_Read: %-8d bytes @ %-8d\r\n", Count, Position);
	fclose(fp);
}
#endif

#if defined(__linux__)
	BytesRead = read(File->FileHandle, Buff, Count);
#elif defined _WIN32 || defined _WIN64
	if	(ReadFile(File->FileHandle, Buff, Count, &BytesRead, NULL) == FALSE)
		return GE_FALSE;
#endif

	if	(BytesRead <= 0)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Write(void *Handle, const void *Buff, int Count)
{
	DosFile *	File;
#if defined(__linux__)
	ssize_t		BytesWritten;
#elif defined _WIN32 || defined _WIN64
	DWORD		BytesWritten;
#endif

	assert(Buff);
	assert(Count != 0);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#ifdef	ELIDEBUG
{
	FILE *	fp;
	int32	Position;

	Position = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
	fp = fopen("c:\\vfs.eli", "ab+");
	fprintf(fp, "FSDos_Write: %-8d bytes @ %-8d\r\n", Count, Position);
	fclose(fp);
}
#endif

#if defined(__linux__)
	BytesWritten = write(File->FileHandle, Buff, Count);
#elif defined _WIN32 || defined _WIN64
	if	(WriteFile(File->FileHandle, Buff, Count, &BytesWritten, NULL) == FALSE)
		return GE_FALSE;
#endif

	if	((int)BytesWritten != Count)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Seek(void *Handle, int Where, geVFile_Whence Whence)
{
	int32		RTLWhence=0;
	DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

	switch(Whence)
	{
	case	GE_VFILE_SEEKCUR:
#if defined(__linux__)
		RTLWhence = SEEK_CUR;
#elif defined _WIN32 || defined _WIN64
		RTLWhence = FILE_CURRENT;
#endif
		break;

	case	GE_VFILE_SEEKEND:
#if defined(__linux__)
		RTLWhence = SEEK_END;
#elif defined _WIN32 || defined _WIN64
		RTLWhence = FILE_END;
#endif
		break;

	case	GE_VFILE_SEEKSET:
#if defined(__linux__)
		RTLWhence = SEEK_SET;
#elif defined _WIN32 || defined _WIN64
		RTLWhence = FILE_BEGIN;
#endif
		break;
	default:
		assert(!"Unknown seek kind");
	}

#if defined(__linux__)
	if	(lseek(File->FileHandle, Where, RTLWhence) == -1)
#elif defined _WIN32 || defined _WIN64
	if	(SetFilePointer(File->FileHandle, Where, NULL, RTLWhence) == INVALID_SET_FILE_POINTER &&
		 GetLastError() != NO_ERROR)//0xffffffff)
#endif
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_EOF(const void *Handle)
{
	const DosFile *	File;
#if defined (__linux__)
	off_t			CurPos;
	struct stat		statbuf;
#elif defined _WIN32 || defined _WIN64
	DWORD			CurPos;
#endif

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#if defined (__linux__)
	assert(File->FileHandle != -1);

	CurPos = lseek(File->FileHandle, 0, SEEK_CUR);
	assert(CurPos != -1);

	if	(fstat(File->FileHandle, &statbuf) == -1)
		return GE_FALSE;

	if	(CurPos == statbuf.st_size)
#elif defined _WIN32 || defined _WIN64
	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	CurPos = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
	if(CurPos == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return GE_FALSE;

	if	(CurPos == GetFileSize(File->FileHandle, NULL))
#endif
		return GE_TRUE;

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_Tell(const void *Handle, int32 *Position)
{
	const DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#if defined (__linux__)
	assert(File->FileHandle != -1);
	*Position = lseek(File->FileHandle, 0, SEEK_CUR);
#elif defined _WIN32 || defined _WIN64
	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	*Position = SetFilePointer(File->FileHandle, 0, NULL, FILE_CURRENT);
#endif
	if	(*Position == -1L)
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Size(const void *Handle, int32 *Size)
{
	const DosFile *	File;
#if defined (__linux__)
	struct stat		statbuf;
#endif

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#if defined (__linux__)
	assert(File->FileHandle != -1);

	if	(fstat(File->FileHandle, &statbuf) == -1)
		return GE_FALSE;

	*Size = statbuf.st_size;
	return GE_TRUE;
#elif defined _WIN32 || defined _WIN64
	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	*Size = GetFileSize(File->FileHandle, NULL);
	if	(*Size != (int32)0xffffffff)
		return GE_TRUE;
#endif

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_GetProperties(const void *Handle, geVFile_Properties *Properties)
{
	const DosFile *				File;
	geVFile_Attributes			Attribs = 0;
#if defined (__linux__)
	struct stat		Info;
#elif defined _WIN32 || defined _WIN64
	BY_HANDLE_FILE_INFORMATION	Info;
#endif
	size_t						Length;

	assert(Properties);

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_TRUE)
	{
		memset(Properties, 0, sizeof(*Properties));
#if defined(__linux__)
		Properties->AttributeFlags = GE_VFILE_ATTRIB_DIRECTORY;
#elif defined _WIN32 || defined _WIN64
		Properties->AttributeFlags = FILE_ATTRIBUTE_DIRECTORY;
#pragma message ("FSDos_GetProperties: Time support is not there for directories")
#endif
	}
	else
	{
#if defined (__linux__)
		assert(File->FileHandle != -1);

		if	(fstat(File->FileHandle, &Info) == -1)
			return GE_FALSE;
		if	(S_ISDIR(Info.st_mode))
			Attribs |= GE_VFILE_ATTRIB_DIRECTORY;
		if (!(Info.st_mode & S_IWUSR))
			Attribs |= GE_VFILE_ATTRIB_READONLY;

		Properties->Time.Time1 = Info.st_mtime;
	#if defined __USE_MISC || defined __USE_XOPEN2K8
		Properties->Time.Time2 = Info.st_mtim.tv_nsec;
	#else
		Properties->Time.Time2 = Info.st_mtimensec; // c11
	#endif

		Properties->AttributeFlags = Attribs;
		Properties->Size = Info.st_size;
#elif defined _WIN32 || defined _WIN64
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
#endif
		Properties->Hints.HintData		 = NULL;
		Properties->Hints.HintDataLength = 0;
	}

	Length = strlen(File->Name) + 1;
	if	(Length > sizeof(Properties->Name))
		return GE_FALSE;
	memcpy(Properties->Name, File->Name, Length);

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_SetSize(void *Handle, int32 size)
{
	DosFile *	File;

	File = Handle;

	CHECK_HANDLE(File);

	if	(File->IsDirectory == GE_FALSE)
	{
#if defined (__linux__)
		assert(File->FileHandle != -1);
		if(ftruncate(File->FileHandle, size) == -1)
			return GE_FALSE;
#elif defined _WIN32 || defined _WIN64
		assert(File->FileHandle != INVALID_HANDLE_VALUE);

		if	(SetFilePointer(File->FileHandle, size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
			 GetLastError() != NO_ERROR)
			return GE_FALSE;

		if	(SetEndOfFile(File->FileHandle) == FALSE)
			return GE_FALSE;
#endif
		return GE_TRUE;
	}

	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_SetAttributes(void *Handle, geVFile_Attributes Attributes)
{
	DosFile *	File;
#if defined _WIN32 || defined _WIN64
	DWORD		Win32Attributes;
#endif

	File = Handle;

	CHECK_HANDLE(File);

#if defined (__linux__)
	assert(File->FileHandle != -1);
#elif defined _WIN32 || defined _WIN64
	assert(File->FileHandle != INVALID_HANDLE_VALUE);
#endif

	if	(File->IsDirectory == GE_TRUE)
		return GE_FALSE;

#if defined (__linux__)
	// TODO?
	(void)Attributes;
#elif defined _WIN32 || defined _WIN64
	if	(Attributes & GE_VFILE_ATTRIB_READONLY)
		Win32Attributes = FILE_ATTRIBUTE_READONLY;
	else
		Win32Attributes = FILE_ATTRIBUTE_NORMAL;

	if	(SetFileAttributes(File->FullPath, Win32Attributes) == FALSE)
		return GE_FALSE;
#endif

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_SetTime(void *Handle, const geVFile_Time *Time)
{
	DosFile *	File;
#if defined (__linux__)
	struct timespec LinuxTime[2];
#elif defined _WIN32 || defined _WIN64
	FILETIME	Win32Time;
#endif

	File = Handle;

	CHECK_HANDLE(File);

#if defined (__linux__)
	assert(File->FileHandle != -1);
	LinuxTime[0].tv_sec  = LinuxTime[1].tv_sec  = Time->Time1;
	LinuxTime[0].tv_nsec = LinuxTime[1].tv_nsec = Time->Time2;
	if	(futimens(File->FileHandle, LinuxTime) == -1)
		return GE_FALSE;
#elif defined _WIN32 || defined _WIN64
	assert(File->FileHandle != INVALID_HANDLE_VALUE);

	Win32Time.dwLowDateTime  = Time->Time1;
	Win32Time.dwHighDateTime = Time->Time2;
	if	(SetFileTime(File->FileHandle, &Win32Time, &Win32Time, &Win32Time) == FALSE)
		return GE_FALSE;
#endif

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_SetHints(void *Handle, const geVFile_Hints *Hints)
{
	DosFile *	File;
	(void)Hints;

	File = Handle;

	CHECK_HANDLE(File);

#if defined (__linux__)
	assert(File->FileHandle != -1);
#elif defined _WIN32 || defined _WIN64
	assert(File->FileHandle != INVALID_HANDLE_VALUE);
#endif

	(void)File;
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_FileExists(geVFile *FS, void *Handle, const char *Name)
{
	DosFile *	File;
#if defined(__linux__)
	char		Buff[PATH_MAX];
#elif defined _WIN32 || defined _WIN64
	char		Buff[_MAX_PATH];
#endif
	(void)FS;

	File = Handle;

	if	(File && File->IsDirectory == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, Name, Buff, NULL, sizeof(Buff)) == GE_FALSE)
		return GE_FALSE;

#if defined(__linux__)
	if	(access(Buff, F_OK) == -1)
#elif defined _WIN32 || defined _WIN64
	if	(GetFileAttributes(Buff) == 0xffffffff)
#endif
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_Disperse(
	geVFile *		FS,
	void *		Handle,
	const char *Directory,
	geBoolean	Recursive)
{
	(void)FS;
	(void)Handle;
	(void)Directory;
	(void)Recursive;
	return GE_FALSE;
}

static	geBoolean	GENESISCC FSDos_DeleteFile(geVFile *FS, void *Handle, const char *Name)
{
	DosFile *	File;
#if defined(__linux__)
	char		Buff[PATH_MAX];
#elif defined _WIN32 || defined _WIN64
	char		Buff[_MAX_PATH];
#endif
	(void)FS;

	File = Handle;

	if	(File && File->IsDirectory == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, Name, Buff, NULL, sizeof(Buff)) == GE_FALSE)
		return GE_FALSE;

#if defined(__linux__)
	if	(unlink(Buff) == -1) // remove() for files and folders
#elif defined _WIN32 || defined _WIN64
	if	(DeleteFile(Buff) == FALSE)v// RemoveDirectory for empty folders
#endif
		return GE_FALSE;

	return GE_TRUE;
}

static	geBoolean	GENESISCC FSDos_RenameFile(geVFile *FS, void *Handle, const char *Name, const char *NewName)
{
	DosFile *	File;
#if defined(__linux__)
	char		Old[PATH_MAX];
	char		New[PATH_MAX];
#elif defined _WIN32 || defined _WIN64
	char		Old[_MAX_PATH];
	char		New[_MAX_PATH];
#endif
	(void)FS;

	File = Handle;

	if	(File && File->IsDirectory == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, Name, Old, NULL, sizeof(Old)) == GE_FALSE)
		return GE_FALSE;

	if	(BuildFileName(File, NewName, New, NULL, sizeof(New)) == GE_FALSE)
		return GE_FALSE;

#if defined(__linux__)
	if	(rename(Old, New) == -1)
#elif defined _WIN32 || defined _WIN64
	if	(MoveFile(Old, New) == FALSE)
#endif
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

