/****************************************************************************************/
/*  TClip                                                                               */
/*                                                                                      */
/*  Author: Mike Sandige & Charles Bloom                                                */
/*  Description: Triangle Clipping to the screen rectangle                              */
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

#ifndef GE_TCLIP_H
#define GE_TCLIP_H

#include "basetype.h"
#include "getypes.h"
#include "bitmap.h"
#include "engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******

TClip is a state machine like OpenGL

you should call it like :

	_Push()
	_SetupEdges()
	_SetTexture()
	_Triangle()
	_Triangle()
	_SetTexture()
	_Triangle()
	_Triangle()
	...
	_Pop()

********/

void		GENESISCC geTClip_SetupEdges(geEngine *Engine,
						geFloat	LeftEdge, 
						geFloat RightEdge,
						geFloat TopEdge ,
						geFloat BottomEdge,
						geFloat BackEdge);

geBoolean	GENESISCC geTClip_Push(void);
geBoolean	GENESISCC geTClip_Pop(void);

geBoolean	GENESISCC geTClip_SetTexture(const geBitmap * Bitmap);
void		GENESISCC geTClip_Triangle(const GE_LVertex TriVertex[3]);


#ifdef __cplusplus
}
#endif


#endif


