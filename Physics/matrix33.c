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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <assert.h>
#include <math.h>

#include "vec3d.h"
#include "xform3d.h"
#include "matrix33.h"

void Matrix33_MakeCrossProductMatrix33(const geVec3d* v,
	Matrix33* m)
{
	assert(v != NULL);
	assert(m != NULL);

	m->x[0][0] = m->x[1][1] = m->x[2][2] = 0.0f;
	m->x[0][1] = -v->Z; m->x[0][2] = v->Y;
	m->x[1][0] = v->Z; m->x[1][2] = -v->X;
	m->x[2][0] = -v->Y; m->x[2][1] = v->X;
}

void Matrix33_ExtractFromXForm3d(const geXForm3d* xform, Matrix33* m)
{
	assert(xform != NULL);
	assert(m != NULL);

	m->x[0][0] = xform->AX; m->x[0][1] = xform->AY; m->x[0][2] = xform->AZ;
	m->x[1][0] = xform->BX; m->x[1][1] = xform->BY; m->x[1][2] = xform->BZ;
	m->x[2][0] = xform->CX; m->x[2][1] = xform->CY; m->x[2][2] = xform->CZ;
}

void Matrix33_MultiplyVec3d(const Matrix33* m, const geVec3d* v, geVec3d* res)
{
	assert(m != NULL);
	assert(v != NULL);
	assert(res != NULL);

	res->X = m->x[0][0] * v->X + m->x[0][1] * v->Y + m->x[0][2] * v->Z;
	res->Y = m->x[1][0] * v->X + m->x[1][1] * v->Y + m->x[1][2] * v->Z;
	res->Z = m->x[2][0] * v->X + m->x[2][1] * v->Y + m->x[2][2] * v->Z;
}

void Matrix33_Multiply(const Matrix33* m1, const Matrix33* m2, Matrix33* res)
{
	int i, j, k;

	assert(m1 != NULL);
	assert(m2 != NULL);
	assert(res != NULL);

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			res->x[i][j] = 0.0f;

			for (k = 0; k < 3; k++)
				res->x[i][j] += m1->x[i][k] * m2->x[k][j];
		}
	}
}

void Matrix33_GetTranspose(const Matrix33* m, Matrix33* t)
{
	int i, j;

	assert(m != NULL);
	assert(t != NULL);

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			t->x[j][i] = m->x[i][j];
}

void Matrix33_Copy(const Matrix33* m, Matrix33* c)
{
	int i, j;

	assert(m != NULL);
	assert(c != NULL);

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			c->x[i][j] = m->x[i][j];
}

void Matrix33_SetIdentity(Matrix33* m)
{
	int i, j;

	assert(m != NULL);

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
		{
			if (i == j) 
				m->x[i][j] = 1.f;
			else m->x[i][j] = 0.f;
		}
}

void Matrix33_Add(const Matrix33* m1, const Matrix33* m2, Matrix33* res)
{
	int i, j;

	assert(m1 != NULL);
	assert(m2 != NULL);
	assert(res != NULL);

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			res->x[i][j] = m1->x[i][j] + m2->x[i][j];
}

void Matrix33_Subtract(const Matrix33* m1, const Matrix33* m2, Matrix33* res)
{
	int i, j;

	assert(m1 != NULL);
	assert(m2 != NULL);
	assert(res != NULL);

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			res->x[i][j] = m1->x[i][j] - m2->x[i][j];
}

void Matrix33_MultiplyScalar(float s, const Matrix33* m, Matrix33* res)
{
	int i, j;

	assert(m != NULL);
	assert(res != NULL);

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			res->x[i][j] = s * m->x[i][j];
}

void Matrix33_GetInverse(const Matrix33* m, Matrix33* inv)
{
	int i, j, k;
	Matrix33 copy;

	assert(m != NULL);
	assert(inv != NULL);

	Matrix33_Copy(m, &copy);
	Matrix33_SetIdentity(inv);

	for (i = 0; i < 3; i++)
	{
		if (copy.x[i][i] != 1.0f)
		{
			float divby = copy.x[i][i];

			assert(fabs(divby) >= 1e-5);
			
			for (j = 0; j < 3; j++)
			{
				inv->x[i][j] /= divby;
				copy.x[i][j] /= divby;
			}
		}
		for (j = 0; j < 3; j++)
		{
			if (j != i)
			{
				if (copy.x[j][i] != 0.0f)
				{
					float mulby = copy.x[j][i];
					for (k = 0; k < 3; k++)
					{
						copy.x[j][k] -= mulby * copy.x[i][k];
						inv->x[j][k] -= mulby * inv->x[i][k];
					}
				}
			}
		}
	}
}

