/****************************************************************************************/
/*  LOG.H                                                                               */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Debugging logger interface                                             */
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
#ifndef GE_LOG_H
#define GE_LOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	_LOG

void Log_Puts(	const char * string);
void Log_Printf(const char * string, ...);

#else	// _LOG

static _inline void Log_Printf(const char * str, ...) { }
#define Log_Puts(string)

#endif	// _LOG

#ifdef __cplusplus
}
#endif

#endif // LOG_H

