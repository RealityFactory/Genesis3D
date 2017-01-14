/****************************************************************************************/
/*  VFILE.H                                                                             */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Virtual file interface                                                 */
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
#ifndef	VFILE_H
#define	VFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"basetype.h"

typedef	struct	geVFile			geVFile;

//--------- Finder (Directory) --------------
typedef	struct	geVFile_Finder	geVFile_Finder;

typedef int     		geVFile_TypeIdentifier;
typedef unsigned int    geVFile_Attributes;

typedef struct	geVFile_Hints
{
	void *	HintData;
	int		HintDataLength;
}	geVFile_Hints;

typedef	struct	geVFile_Time
{
	unsigned long	Time1;
	unsigned long	Time2;
}	geVFile_Time;

#define	GE_VFILE_ATTRIB_READONLY	0x00000001
#define	GE_VFILE_ATTRIB_DIRECTORY	0x00000002

typedef	struct	geVFile_Properties
{
	geVFile_Time				Time;
	geVFile_Attributes		AttributeFlags;
	long					Size;
	geVFile_Hints				Hints;
	char					Name[1024];
}	geVFile_Properties;

#ifdef _INC_WINDOWS
GENESISAPI void GENESISCC geVFile_TimeToWin32FileTime(const geVFile_Time *, LPFILETIME Win32FileTime);
	// Converts a geVFile time to a Win32 FILETIME structure.  This API is the
	// way to get the file time into a format to do standardized date/time
	// operations on.  We do not provide date/time operations natively.
#endif

GENESISAPI geVFile_Finder * GENESISCC geVFile_CreateFinder(
	geVFile *		FileSystem,
	const char *	FileSpec);
	// Creates a finder object from which you can get iterated file names.
	// This is findfirst/findnext functionality.  

GENESISAPI void GENESISCC geVFile_DestroyFinder(geVFile_Finder *Finder);
	// Destroys a Finder object

GENESISAPI geBoolean GENESISCC geVFile_FinderGetNextFile(geVFile_Finder *Finder);
	// Tracks to the next file in the finder directory

GENESISAPI geBoolean GENESISCC geVFile_FinderGetProperties(const geVFile_Finder *Finder, geVFile_Properties *Properties);
	// Gets the file properties from a geVFile_Finder.  You cannot set properties for
	// a file through a finder.  You have to set the properties through a geVFile.

//--------- File System Operations ----

typedef	struct	geVFile_MemoryContext
{
	void *	Data;
	int		DataLength;
}	geVFile_MemoryContext;

#define GE_VFILE_TYPE_DOS	    ( (geVFile_TypeIdentifier) 1L )
#define GE_VFILE_TYPE_MEMORY	( (geVFile_TypeIdentifier) 2L )
#define GE_VFILE_TYPE_VIRTUAL	( (geVFile_TypeIdentifier) 3L )

// First three flags are mutually exclusive.  Combining them will result in failure
// returns for both geVFile_OpenNewSystem and geVFile_Open.
#define	GE_VFILE_OPEN_READONLY	 0x00000001
#define	GE_VFILE_OPEN_UPDATE 	 0x00000002
#define	GE_VFILE_OPEN_CREATE	 0x00000004

#define GE_VFILE_OPEN_DIRECTORY  0x00000008

#if 0
geBoolean GENESISCC	geVFile_Startup(void);
	// Initializes the VFile System.  This API MUST be called before any other
	// VFile APIs are called.  This API ensures that the rest of the VFile systems
	// will be thread safe.  Hence the application should ensure that this function
	// is called once and only once.
#endif

GENESISAPI geVFile * GENESISCC geVFile_OpenNewSystem(
	geVFile *					FS,
	geVFile_TypeIdentifier	FileSystemType,  // { DOS, MEMORY, ETC ... },
	const char *			Name, 
	void *					Context, 
	unsigned int 			OpenModeFlags);
	// Opens a file / file system.

GENESISAPI geBoolean GENESISCC geVFile_UpdateContext(geVFile *FS, void *Context, int ContextSize);

GENESISAPI geVFile * GENESISCC geVFile_GetContext(const geVFile *File);
	// Returns the outer context in which File was opened.

GENESISAPI geVFile * GENESISCC geVFile_Open( 
	geVFile *			FS,
	const char *	Name, 
	unsigned int 	OpenModeFlags);

/*
typedef	enum
{
	geVFile_AppendPath,
	geVFile_PrependPath,
}	geVFile_SearchOrder;
*/

GENESISAPI geBoolean GENESISCC geVFile_AddPath(geVFile *FS1, const geVFile *FS2, geBoolean Append);
	// Appends (or prepends) the path associated with FS2 into FS1.
	//    Append==GE_TRUE   causes the FS2 to be searched AFTER FS1
	//    Append==GE_FALSE  causes the FS2 to be searched BEFORE FS1

/*  perhaps geVFile_AppendPath and geVFile_PrependPath */


GENESISAPI geBoolean GENESISCC geVFile_DeleteFile(geVFile *FS, const char *FileName);
	// Deletes a file within a file system.  Returns GE_TRUE on success, GE_FALSE
	// on failure.

GENESISAPI geBoolean GENESISCC geVFile_RenameFile(geVFile *FS, const char *FileName, const char *NewName);
	// Renames a file within a file system.  Returns GE_TRUE on success, GE_FALSE
	// on failure.

GENESISAPI geBoolean GENESISCC geVFile_FileExists(geVFile *FS, const char *FileName);
	// Returns GE_TRUE if the file FileName exists in FS, GE_FALSE otherwise.
	// Does not do any searching (?)

//geVFile_VFileType geVFile_Register( all kinds of stuff );

GENESISAPI geBoolean GENESISCC geVFile_Close (geVFile *File);
	// closes and destroys the File

//---------- File Specific Operations -----------

typedef	enum
{
	GE_VFILE_SEEKCUR	= 0,
	GE_VFILE_SEEKEND	= 1,
	GE_VFILE_SEEKSET	= 2
}	geVFile_Whence;

GENESISAPI geBoolean GENESISCC geVFile_GetS  		 (		geVFile *File, void *Buff, int MaxLen);
GENESISAPI geBoolean GENESISCC geVFile_Read  		 (		geVFile *File, void *Buff, int Count);
GENESISAPI geBoolean GENESISCC geVFile_Write 		 (		geVFile *File, const void *Buff, int Count);
GENESISAPI geBoolean GENESISCC geVFile_Seek  		 (		geVFile *File, int where, geVFile_Whence Whence);
GENESISAPI geBoolean GENESISCC geVFile_Printf		 (		geVFile *File, const char *Format, ...);
GENESISAPI geBoolean GENESISCC geVFile_EOF   		 (const geVFile *File);
GENESISAPI geBoolean GENESISCC geVFile_Tell  		 (const geVFile *File, long *Position);
GENESISAPI geBoolean GENESISCC geVFile_GetProperties(const geVFile *File, geVFile_Properties *Properties);
//geBoolean geVFile_GetName(geVFile *File, char *Buff, int MaxBuffLen);
	// Gets the name of the file

GENESISAPI geBoolean GENESISCC geVFile_Size  		 (const geVFile *File, long *Size);
GENESISAPI geBoolean GENESISCC geVFile_SetSize		 (		geVFile *File, long Size);
GENESISAPI geBoolean GENESISCC geVFile_SetAttributes(		geVFile *File, geVFile_Attributes Attributes);
GENESISAPI geBoolean GENESISCC geVFile_SetTime		 (		geVFile *File, const geVFile_Time *Time);
GENESISAPI geBoolean GENESISCC geVFile_SetHints	 (		geVFile *File, const geVFile_Hints *Hints);


#ifdef __cplusplus
}
#endif

#endif
