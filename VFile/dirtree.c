/****************************************************************************************/
/*  DIRTREE.C                                                                           */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Directory tree implementation                                          */
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

#include	<assert.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"basetype.h"
#include	"ram.h"

#include	"dirtree.h"

#ifndef	MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
		((unsigned long)(unsigned char)(ch0) | ((unsigned long)(unsigned char)(ch1) << 8) |   \
		((unsigned long)(unsigned char)(ch2) << 16) | ((unsigned long)(unsigned char)(ch3) << 24 ))
#endif

#define	DIRTREE_FILE_SIGNATURE	MAKEFOURCC('D', 'T', '0', '1')
static int DirTree_SignatureBase=0x696C6345;
static int DirTree_SignatureOffset=0x21657370;

typedef struct	DirTree
{
	char *				Name;
	geVFile_Time		Time;
	geVFile_Attributes	AttributeFlags;
	long				Size;
	geVFile_Hints		Hints;
	long				Offset;
	struct DirTree *	Parent;
	struct DirTree *	Children;
	struct DirTree *	Siblings;
}	DirTree;

typedef struct	DirTree_Finder
{
	char *		MatchName;
	char *		MatchExt;
	DirTree *	Current;
}	DirTree_Finder;

static	char *	DuplicateString(const char *String)
{
	int		Length;
	char *	NewString;

	Length = strlen(String) + 1;
	NewString = geRam_Allocate(Length);
	if	(NewString)
		memcpy(NewString, String, Length);
	return NewString;
}

DirTree *DirTree_Create(void)
{
	DirTree *	Tree;

	Tree = geRam_Allocate(sizeof(*Tree));
	if	(!Tree)
		return Tree;

	memset(Tree, 0, sizeof(*Tree));
	Tree->Name = DuplicateString("");
	if	(!Tree->Name)
	{
		geRam_Free(Tree);
		return NULL;
	}

	Tree->AttributeFlags |= GE_VFILE_ATTRIB_DIRECTORY;

	return Tree;
}

void	DirTree_Destroy(DirTree *Tree)
{
	assert(Tree);
	assert(Tree->Name);

	if	(Tree->Children)
		DirTree_Destroy(Tree->Children);

	if	(Tree->Siblings)
		DirTree_Destroy(Tree->Siblings);

	geRam_Free(Tree->Name);
	geRam_Free(Tree);
}

typedef	struct	DirTree_Header
{
	unsigned long	Signature;
	int				Size;
	
}	DirTree_Header;

#define	DIRTREE_LIST_TERMINATED		0xffffffff
#define DIRTREE_LIST_NOTTERMINATED	0

static	geBoolean	WriteTree(const DirTree *Tree, geVFile *File)
{
	int		Length;
	int		Terminator;

	assert(Tree);
	assert(Tree->Name);

	Terminator = DIRTREE_LIST_NOTTERMINATED;
	if	(geVFile_Write(File, &Terminator, sizeof(Terminator)) == GE_FALSE)
		return GE_FALSE;

	// Write out the name
	Length = strlen(Tree->Name) + 1;
	if	(geVFile_Write(File, &Length, sizeof(Length)) == GE_FALSE)
		return GE_FALSE;
	if	(Length > 0)
	{
		if	(geVFile_Write(File, Tree->Name, Length) == GE_FALSE)
			return GE_FALSE;
	}

	// Write out the attribute information
	if	(geVFile_Write(File, &Tree->Time, sizeof(Tree->Time)) == GE_FALSE)
		return GE_FALSE;

	if	(geVFile_Write(File, &Tree->AttributeFlags, sizeof(Tree->AttributeFlags)) == GE_FALSE)
		return GE_FALSE;

	if	(geVFile_Write(File, &Tree->Size, sizeof(Tree->Size)) == GE_FALSE)
		return GE_FALSE;

	if	(geVFile_Write(File, &Tree->Offset, sizeof(Tree->Offset)) == GE_FALSE)
		return GE_FALSE;
	
	if	(geVFile_Write(File, &Tree->Hints.HintDataLength, sizeof(Tree->Hints.HintDataLength)) == GE_FALSE)
		return GE_FALSE;

	if	(Tree->Hints.HintDataLength != 0)
		if	(geVFile_Write(File, &Tree->Hints.HintData, Tree->Hints.HintDataLength) == GE_FALSE)
			return GE_FALSE;
	
	// Write out the Children
	if	(Tree->Children)
	{
		WriteTree(Tree->Children, File);
	}
	else
	{
		Terminator = DIRTREE_LIST_TERMINATED;
		if	(geVFile_Write(File, &Terminator, sizeof(Terminator)) == GE_FALSE)
			return GE_FALSE;
	}

	// Write out the Siblings
	if	(Tree->Siblings)
	{
		WriteTree(Tree->Siblings, File);
	}
	else
	{
		Terminator = DIRTREE_LIST_TERMINATED;
		if	(geVFile_Write(File, &Terminator, sizeof(Terminator)) == GE_FALSE)
			return GE_FALSE;
	}
	
	return GE_TRUE;
}

static	geBoolean DirTree_WriteToFile1(const DirTree *Tree, geVFile *File, long *Size)
{
	DirTree_Header	Header;
	long			StartPosition;
	long			EndPosition;
	
	if	(geVFile_Tell(File, &StartPosition) == GE_FALSE)
		return GE_FALSE;

	Header.Signature = DIRTREE_FILE_SIGNATURE;
	if	(geVFile_Seek(File, sizeof(Header), GE_VFILE_SEEKCUR) == GE_FALSE)
		return GE_FALSE;
	
	if	(WriteTree(Tree, File) == GE_FALSE)
		return GE_FALSE;

	geVFile_Tell(File, &EndPosition);
	Header.Size = EndPosition - StartPosition;
	geVFile_Seek(File, StartPosition, GE_VFILE_SEEKSET);
	if	(geVFile_Write(File, &Header, sizeof(Header)) == GE_FALSE)
		return GE_FALSE;

	// Make sure that we end up at the end of the directory.
	geVFile_Seek(File, EndPosition, GE_VFILE_SEEKSET);

	*Size = Header.Size;

	return GE_TRUE;
}

void DirTree_SetFileSize(DirTree *Tree, long Size)
{
	assert(Tree);
	Tree->Size = Size;
}

void DirTree_GetFileSize(DirTree *Tree, long *Size)
{
	assert(Tree);
	*Size = Tree->Size;
}

geBoolean DirTree_WriteToFile(const DirTree *Tree, geVFile *File)
{
	geBoolean	Res;
	long		Size;

	Res = DirTree_WriteToFile1(Tree, File, &Size);
	if	(Res == GE_FALSE)
		return Res;

	return GE_TRUE;
}

geBoolean DirTree_GetSize(const DirTree *Tree, long *Size)
{
	geVFile *				FS;
	geVFile_MemoryContext	Context;

	/*
		This function is implemented via a write to a memory file for
		a few reasons.  First, it makes it easier to maintain this code.  We
		don't have to track format information in Write, Read and Size functions,
		just in Write and Read.  Second, it gets us testing of the memory
		file system for free.  Third, it was cute.  The last one doesn't count,
		of course, but the other two are compelling.  This API ends up being
		inefficient, but the assumption is that it will be called rarely.
	*/

	Context.Data	   = NULL;
	Context.DataLength = 0;

	FS = geVFile_OpenNewSystem(NULL,
							 GE_VFILE_TYPE_MEMORY,
							 NULL,
							 &Context,
							 GE_VFILE_OPEN_CREATE);
	if	(!FS)
		return GE_FALSE;

	if	(DirTree_WriteToFile1(Tree, FS, Size) == GE_FALSE)
		return GE_FALSE;

	if	(geVFile_Size(FS, Size) == GE_FALSE)
		return GE_FALSE;

	geVFile_Close(FS);

	return GE_TRUE;
}

static	geBoolean	ReadTree(geVFile *File, DirTree **TreePtr)
{
	int			Terminator;
	int			Length;
	DirTree *	Tree;

	if	(geVFile_Read(File, &Terminator, sizeof(Terminator)) == GE_FALSE)
		return GE_FALSE;

	if	(Terminator == DIRTREE_LIST_TERMINATED)
	{
		*TreePtr = NULL;
		return GE_TRUE;
	}

	Tree = geRam_Allocate(sizeof(*Tree));
	if	(!Tree)
		return GE_FALSE;
	memset(Tree, 0, sizeof(*Tree));

	// Read the name
	if	(geVFile_Read(File, &Length, sizeof(Length)) == GE_FALSE)
		goto fail;

	assert(Length > 0);
	Tree->Name = geRam_Allocate(Length);
	if	(!Tree->Name)
	{
		geRam_Free(Tree);
		return GE_FALSE;
	}
	
	if	(geVFile_Read(File, Tree->Name, Length) == GE_FALSE)
		goto fail;

//printf("Reading '%s'\n", Tree->Name);

	// Read out the attribute information
	if	(geVFile_Read(File, &Tree->Time, sizeof(Tree->Time)) == GE_FALSE)
		goto fail;

	if	(geVFile_Read(File, &Tree->AttributeFlags, sizeof(Tree->AttributeFlags)) == GE_FALSE)
		goto fail;

	if	(geVFile_Read(File, &Tree->Size, sizeof(Tree->Size)) == GE_FALSE)
		goto fail;

	if	(geVFile_Read(File, &Tree->Offset, sizeof(Tree->Offset)) == GE_FALSE)
		goto fail;

	if	(geVFile_Read(File, &Tree->Hints.HintDataLength, sizeof(Tree->Hints.HintDataLength)) == GE_FALSE)
		goto fail;

	if	(Tree->Hints.HintDataLength != 0)
	{
		Tree->Hints.HintData = geRam_Allocate(Tree->Hints.HintDataLength);
		if	(!Tree->Hints.HintData)
			goto fail;
		if	(geVFile_Read(File, &Tree->Hints.HintData, Tree->Hints.HintDataLength) == GE_FALSE)
			goto fail;
	}

//printf("Reading children of '%s'\n", Tree->Name);
	// Read the children
	if	(ReadTree(File, &Tree->Children) == GE_FALSE)
		goto fail;

//printf("Reading siblings of '%s'\n", Tree->Name);
	// Read the Siblings
	if	(ReadTree(File, &Tree->Siblings) == GE_FALSE)
		goto fail;

//DirTree_Dump(Tree);

	*TreePtr = Tree;

	return GE_TRUE;

fail:
	DirTree_Destroy(Tree);
	return GE_FALSE;
}

DirTree *DirTree_CreateFromFile(geVFile *File)
{
	DirTree *		Res;
	DirTree_Header	Header;
	long			StartPosition;
	long			EndPosition;
	
	if	(geVFile_Tell(File, &StartPosition) == GE_FALSE)
		return GE_FALSE;

	if	(geVFile_Read(File, &Header, sizeof(Header)) == GE_FALSE)
		return NULL;

	if	(Header.Signature != DIRTREE_FILE_SIGNATURE)
		return GE_FALSE;

	if	(ReadTree(File, &Res) == GE_FALSE)
		return NULL;

	geVFile_Tell(File, &EndPosition);
	if	(Header.Size != EndPosition - StartPosition)
	{
		DirTree_Destroy(Res);
		return NULL;
	}

	return Res;
}

static	const char *GetNextDir(const char *Path, char *Buff)
{
	while	(*Path && *Path != '\\')
		*Buff++ = *Path++;
	*Buff = '\0';

	if	(*Path == '\\')
		Path++;

	return Path;
}

DirTree *DirTree_FindExact(const DirTree *Tree, const char *Path)
{
	static char	Buff[_MAX_PATH];
	DirTree *	Siblings;

	assert(Tree);
	assert(Path);

	if	(*Path == '\\')
		return NULL;

	if	(*Path == '\0')
		return (DirTree *)Tree;

	Path = GetNextDir(Path, Buff);

	Siblings = Tree->Children;
	while	(Siblings)
	{
		if	(!stricmp(Siblings->Name, Buff))
		{
			if	(!*Path)
				return Siblings;
			return DirTree_FindExact(Siblings, Path);
		}
		Siblings = Siblings->Siblings;
	}

	return NULL;
}

DirTree *DirTree_FindPartial(
	const DirTree *	Tree,
	const char *	Path,
	const char **	LeftOvers)
{
	static char	Buff[_MAX_PATH];
	DirTree *	Siblings;

	assert(Tree);
	assert(Path);

	if	(*Path == '\\')
		return NULL;

	*LeftOvers = Path;

	if	(*Path == '\0')
		return (DirTree *)Tree;

	Path = GetNextDir(Path, Buff);

	Siblings = Tree->Children;
	while	(Siblings)
	{
		if	(!stricmp(Siblings->Name, Buff))
		{
			*LeftOvers = Path;
			if	(!*Path)
				return Siblings;
			return DirTree_FindPartial(Siblings, Path, LeftOvers);
		}
		Siblings = Siblings->Siblings;
	}

	return (DirTree *)Tree;
}

static	geBoolean	PathHasDir(const char *Path)
{
	if	(strchr(Path, '\\'))
		return GE_TRUE;

	return GE_FALSE;
}

DirTree * DirTree_AddFile(DirTree *Tree, const char *Path, geBoolean IsDirectory)
{
	DirTree *		NewEntry;
	const char *	LeftOvers;

	assert(Tree);
	assert(Path);
	assert(IsDirectory == GE_TRUE || IsDirectory == GE_FALSE);

	assert(strlen(Path) > 0);

	if	(PathHasDir(Path))
	{
		Tree = DirTree_FindPartial(Tree, Path, &LeftOvers);
		if	(!Tree)
			return NULL;
	
		if	(PathHasDir(LeftOvers))
			return NULL;
	
		Path = LeftOvers;
	}

	NewEntry = geRam_Allocate(sizeof(*NewEntry));
	if	(!NewEntry)
		return NULL;

	memset(NewEntry, 0, sizeof(*NewEntry));
	NewEntry->Name = DuplicateString(Path);
	if	(!NewEntry->Name)
	{
		geRam_Free(NewEntry->Name);
		geRam_Free(NewEntry);
		return NULL;
	}

	NewEntry->Siblings = Tree->Children;
						 Tree->Children = NewEntry;

	if	(IsDirectory == GE_TRUE)
		NewEntry->AttributeFlags |= GE_VFILE_ATTRIB_DIRECTORY;

	return NewEntry;
}

geBoolean DirTree_Remove(DirTree *Tree, DirTree *SubTree)
{
	DirTree 	Siblings;
	DirTree * 	pSiblings;
	DirTree *	Parent;
	DirTree *	ParanoiaCheck;

	assert(Tree);
	assert(SubTree);

	Parent = SubTree->Parent;
	assert(Parent);

	ParanoiaCheck = Parent;
	while	(ParanoiaCheck && ParanoiaCheck != Tree)
		ParanoiaCheck = ParanoiaCheck->Parent;
	if	(!ParanoiaCheck)
		return GE_FALSE;

	Siblings.Siblings = Parent->Children;
	assert(Siblings.Siblings);
	pSiblings = &Siblings;
	while	(pSiblings->Siblings)
	{
		if	(pSiblings->Siblings == SubTree)
		{
			pSiblings->Siblings = SubTree->Siblings;
			if	(SubTree == Parent->Children)
				Parent->Children = SubTree->Siblings;
			SubTree->Siblings = NULL;
			DirTree_Destroy(SubTree);
			return GE_TRUE;
		}
		pSiblings = pSiblings->Siblings;
	}

	assert(!"Shouldn't be a way to get here");
	return GE_FALSE;
}

void DirTree_SetFileAttributes(DirTree *Tree, geVFile_Attributes Attributes)
{
	assert(Tree);
	assert(Attributes);

	// Only support the read only flag
	assert(!(Attributes & ~GE_VFILE_ATTRIB_READONLY));
	assert(!(Tree->AttributeFlags & GE_VFILE_ATTRIB_DIRECTORY));

	Tree->AttributeFlags = (Tree->AttributeFlags & ~GE_VFILE_ATTRIB_READONLY)  | Attributes;
}

void DirTree_GetFileAttributes(DirTree *Tree, geVFile_Attributes *Attributes)
{
	assert(Tree);
	assert(Attributes);

	*Attributes = Tree->AttributeFlags;
}

void DirTree_SetFileOffset(DirTree *Leaf, long Offset)
{
	assert(Leaf);
	assert(!(Leaf->AttributeFlags & GE_VFILE_ATTRIB_DIRECTORY));

	Leaf->Offset = Offset;
}

void DirTree_GetFileOffset(DirTree *Leaf, long *Offset)
{
	assert(Leaf);
	assert(!(Leaf->AttributeFlags & GE_VFILE_ATTRIB_DIRECTORY));

	*Offset = Leaf->Offset;
}

void DirTree_SetFileTime(DirTree *Tree, const geVFile_Time *Time)
{
	assert(Tree);

	Tree->Time = *Time;
}

void DirTree_GetFileTime(DirTree *Tree, geVFile_Time *Time)
{
	assert(Tree);

	*Time = Tree->Time;
}

geBoolean DirTree_SetFileHints(DirTree *Tree, const geVFile_Hints *Hints)
{
	if	(Tree->Hints.HintData)
		geRam_Free(Tree->Hints.HintData);

	if	(Hints->HintData)
	{
		Tree->Hints.HintData = geRam_Allocate(Hints->HintDataLength);
		if	(!Tree->Hints.HintData)
			return GE_FALSE;
		memcpy(Tree->Hints.HintData, Hints->HintData, Hints->HintDataLength);
	}
	Tree->Hints.HintDataLength = Hints->HintDataLength;
	return GE_TRUE;
}

void DirTree_GetFileHints(DirTree *Tree, geVFile_Hints *Hints)
{
	*Hints = Tree->Hints;
}

geBoolean DirTree_GetName(DirTree *Tree, char *Buff, int MaxLen)
{
	int	Length;

	assert(Tree);
	assert(Buff);
	assert(MaxLen > 0);

	Length = strlen(Tree->Name);
	if	(Length > MaxLen)
		return GE_FALSE;

	memcpy(Buff, Tree->Name, Length + 1);

	return GE_TRUE;
}

geBoolean DirTree_FileExists(const DirTree *Tree, const char *Path)
{
	if	(DirTree_FindExact(Tree, Path) == NULL)
		return GE_FALSE;

	return GE_TRUE;
}

DirTree_Finder * DirTree_CreateFinder(DirTree *Tree, const char *Path)
{
	DirTree_Finder *	Finder;
	DirTree *			SubTree;
	char				Directory[_MAX_PATH];
	char				Name[_MAX_FNAME];
	char				Ext[_MAX_EXT];

	assert(Tree);
	assert(Path);

	_splitpath(Path, NULL, Directory, Name, Ext);

	SubTree = DirTree_FindExact(Tree, Directory);
	if	(!SubTree)
		return NULL;

	Finder = geRam_Allocate(sizeof(*Finder));
	if	(!Finder)
		return Finder;

	Finder->MatchName = DuplicateString(Name);
	if	(!Finder->MatchName)
	{
		geRam_Free(Finder);
		return NULL;
	}

	// The RTL leaves the '.' on there.  That won't do.
	if	(*Ext == '.')
		Finder->MatchExt = DuplicateString(&Ext[1]);
	else
		Finder->MatchExt = DuplicateString(&Ext[0]);

	if	(!Finder->MatchExt)
	{
		geRam_Free(Finder->MatchName);
		geRam_Free(Finder);
		return NULL;
	}

	Finder->Current = SubTree->Children;

	return Finder;
}

void DirTree_DestroyFinder(DirTree_Finder *Finder)
{
	assert(Finder);
	assert(Finder->MatchName);
	assert(Finder->MatchExt);

	geRam_Free(Finder->MatchName);
	geRam_Free(Finder->MatchExt);
	geRam_Free(Finder);
}

static geBoolean	MatchPattern(const char *Source, const char *Pattern)
{
	assert(Source);
	assert(Pattern);

	switch	(*Pattern)
	{
	case	'\0':
		if	(*Source)
			return GE_FALSE;
		break;

	case	'*':
		if	(*(Pattern + 1) != '\0')
		{
			Pattern++;
			while	(*Source)
			{
				if	(MatchPattern(Source, Pattern) == GE_TRUE)
					return GE_TRUE;
				Source++;
			}
			return GE_FALSE;
		}
		break;

	case	'?':
		return MatchPattern(Source + 1, Pattern + 1);

	default:
		if	(*Source == *Pattern)
			return MatchPattern(Source + 1, Pattern + 1);
		else
			return GE_FALSE;
	}

	return GE_TRUE;
}

DirTree * DirTree_FinderGetNextFile(DirTree_Finder *Finder)
{
	DirTree *	Res;
	char		Name[_MAX_FNAME];
	char		Ext[_MAX_EXT];

	assert(Finder);

	Res = Finder->Current;

	if	(!Res)
		return Res;

	do
	{
		_splitpath(Res->Name, NULL, NULL, Name, Ext);
		if	(MatchPattern(Name, Finder->MatchName) == GE_TRUE &&
			 MatchPattern(Ext,  Finder->MatchExt) == GE_TRUE)
		{
			break;
		}

		Res = Res->Siblings;

	}	while	(Res);

	if	(Res)
		Finder->Current = Res->Siblings;

	return Res;
}

#ifdef	DEBUG
static	void	indent(int i)
{
	while	(i--)
		printf(" ");
}

static	void DirTree_Dump1(const DirTree *Tree, int i)
{
	DirTree *	Temp;

	indent(i);
	if	(Tree->AttributeFlags & GE_VFILE_ATTRIB_DIRECTORY)
		printf("\\%s\n", Tree->Name);
	else
		printf("%-*s  %08x  %08x\n", 40 - i, Tree->Name, Tree->Offset, Tree->Size);
	Temp = Tree->Children;
	while	(Temp)
	{
		DirTree_Dump1(Temp, i + 2);
		Temp = Temp->Siblings;
	}
}

void DirTree_Dump(const DirTree *Tree)
{
	printf("%-*s  %-8s  %-8s\n", 40, "Name", "Offset", "Size");
	printf("------------------------------------------------------------\n");
	DirTree_Dump1(Tree, 0);
}

#endif

