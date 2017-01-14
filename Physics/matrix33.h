/****************************************************************************************/
/*  MATRIX33.H                                                                          */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Pure 3x3 matrix                                                        */
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
#ifndef	MATRIX33_H
#define MATRIX33_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	float x[3][3];
}	Matrix33;

void Matrix33_Copy(const Matrix33* m, Matrix33* c);
void Matrix33_SetIdentity(Matrix33* m);
void Matrix33_Add(const Matrix33* m1, const Matrix33* m2, Matrix33* res);
void Matrix33_Subtract(const Matrix33* m1, const Matrix33* m2, Matrix33* res);
void Matrix33_MultiplyVec3d(const Matrix33* m, const geVec3d* v, geVec3d* res);
void Matrix33_Multiply(const Matrix33* m1, const Matrix33* m2, Matrix33* res);
void Matrix33_MultiplyScalar(float s, const Matrix33* m, Matrix33* res);
void Matrix33_GetTranspose(const Matrix33* m, Matrix33* t);
void Matrix33_GetInverse(const Matrix33* m, Matrix33* inv);
void Matrix33_MakeCrossProductMatrix33(const geVec3d* v, Matrix33* m);
void Matrix33_ExtractFromXForm3d(const geXForm3d* xform, Matrix33* m);

#ifdef __cplusplus
}
#endif

#endif

