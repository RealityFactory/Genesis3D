/****************************************************************************************/
/*  OglMisc.h                                                                           */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Miscellaneous support functions for OpenGL driver                      */
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
/*                                                                                      */
/****************************************************************************************/

#ifndef OGLMISC_H
#define OGLMISC_H

void InitMatrices(int width, int height);
geBoolean ExtensionExists(const char *extension);
void CkBlit24_32(GLubyte *dstPtr, GLint width, GLint dstHeight, GLubyte *srcPtr, GLint srcWidth, GLint srcHeight);
void Blit32(GLubyte *dstPtr, GLint dstPitch, GLubyte *srcPtr, GLint srcWidth, GLint srcHeight,
			GLint srcPitch);

#endif