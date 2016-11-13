/****************************************************************************************/
/*  DirTreeCommon.h                                                                     */
/*                                                                                      */
/*  Author: Samuel Seay                                                                 */
/*  Description: Common functions between Dirtree files                                 */
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
#ifndef	DIRTREE_COMMON_H
#define	DIRTREE_COMMON_H


#include	"VFile.h"

typedef	struct	DirTree_Header
{
	unsigned long	Signature;
	int				Size;

}	DirTree_Header;

#define	DIRTREE_LIST_TERMINATED		0xffffffff
#define DIRTREE_LIST_NOTTERMINATED	0

#ifndef	MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
		((unsigned long)(unsigned char)(ch0) | ((unsigned long)(unsigned char)(ch1) << 8) |   \
		((unsigned long)(unsigned char)(ch2) << 16) | ((unsigned long)(unsigned char)(ch3) << 24 ))
#endif

char *	DuplicateString(const char *String);
const char *GetNextDir(const char *Path, char *Buff);
geBoolean	MatchPattern(const char *Source, const char *Pattern);
geBoolean	PathHasDir(const char *Path);

#ifdef	DEBUG
static	void	indent(int i);
#endif

#endif
