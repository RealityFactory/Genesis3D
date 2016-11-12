#ifndef BITMAP_GAMMA_H
#define BITMAP_GAMMA_H

#ifndef BITMAP_PRIVATE_H
Intentional Error : bitmap_blidata only allowed in bitmap internals!
#endif

/****************************************************************************************/
/*  Bitmap_Gamma.h                                                                      */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  The Bitmap_Gamma_Apply function                                       */
/*					Fast Gamma correction routines for various pixel formats			*/
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


extern geBoolean geBitmap_Gamma_Apply(geBitmap * Bitmap,geBoolean Invert);

#endif //BITMAP_GAMMA_H

