/****************************************************************************************/
/*  LOG.C                                                                               */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Debugging logger implementation                                        */
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
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void Log_Out(const char * string)
{
#ifdef _DEBUG
	OutputDebugString(string);
#endif
	printf(string);
}


void Log_Puts(const char * string)
{
	Log_Out(string);
	Log_Out("\n");
}

void Log_Printf(const char * String, ...)
{
	va_list			ArgPtr;
    char			TempStr[1024];

	va_start(ArgPtr, String);
    vsprintf(TempStr, String, ArgPtr);
	va_end(ArgPtr);

	Log_Out(TempStr);
}

#ifdef _LOG
#pragma message("LOG on")
#endif