#ifndef GE_BRANDO_PALCREATE_H
#define GE_BRANDO_PALCREATE_H

#include "basetype.h"
#include "bitmap.h"

/****************************************************************************************/
/*  PalCreate                                                                           */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Palette Creation code                                                 */
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

#ifdef __cplusplus
extern "C" {
#endif

extern geBitmap_Palette * createPaletteGood(const geBitmap_Info * Info,const void * Bits);
extern geBitmap_Palette * createPaletteFast(const geBitmap_Info * Info,const void * Bits);

typedef geBitmap_Palette * (*paletteCreater) (const geBitmap_Info * Info,const void * Bits);
extern void setCreatePaletteFunc(paletteCreater func);

extern geBitmap_Palette * createPalette(const geBitmap_Info * Info,const void * Bits);

extern geBitmap_Palette * createPaletteFromBitmap(const geBitmap * Bitmap,geBoolean Optimize);

extern void PalCreate_Start(void);
extern void PalCreate_Stop(void);

#ifdef __cplusplus
}
#endif

#endif
