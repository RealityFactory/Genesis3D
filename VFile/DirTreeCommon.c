/****************************************************************************************/
/*  DirTreeCommon.c                                                                     */
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

#include	<assert.h>
#include	<string.h>

#include	"Ram.h"
#include	"DirTreeCommon.h"

char *	DuplicateString(const char *String)
{
	int		Length;
	char *	NewString;

	Length = strlen(String) + 1;
	NewString = geRam_Allocate(Length);
	if	(NewString)
		memcpy(NewString, String, Length);
	return NewString;
}

const char *GetNextDir(const char *Path, char *Buff)
{
	while	(*Path && *Path != '\\')
		*Buff++ = *Path++;
	*Buff = '\0';

	if	(*Path == '\\')
		Path++;

	return Path;
}

geBoolean	MatchPattern(const char *Source, const char *Pattern)
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

geBoolean	PathHasDir(const char *Path)
{
	if	(strchr(Path, '\\'))
		return GE_TRUE;

	return GE_FALSE;
}

#ifdef	DEBUG
void	indent(int i)
{
	while	(i--)
		printf(" ");
}
#endif
