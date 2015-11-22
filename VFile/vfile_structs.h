#ifndef	VFILESTRUCT_H
#define	VFILESTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
