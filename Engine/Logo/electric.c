/****************************************************************************************/
/*  ELECTRIC.C                                                                          */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Animated electrical bolt special effect implementation                 */
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
#include	<windows.h>
#include	<math.h>
#include	<assert.h>

#include	"genesis.h"
#include	"ErrorLog.h"

#include	"Electric.h"
#include	"ram.h"

static	int		logBase2(int n)
{
	int	i = 0;

	assert(n != 0);

	while	(!(n & 1))
	{
		n = n >> 1;
		i++;
	}

	assert((n & ~1) == 0);

	return i;
}

static	geBoolean	IsPowerOf2(int n)
{
	if	(n == 0)
		return GE_TRUE;

	while	(!(n & 1))
		n = n >> 1;

	if	(n & ~1)
		return GE_FALSE;

	return GE_TRUE;
}

_Electric_BoltEffect * _Electric_BoltEffectCreate(
 	int NumPolys,
 	int Width,
	geFloat Wildness)
{
	_Electric_BoltEffect *	be;
	GE_RGBA					color;

	assert(Wildness >= 0.0f && Wildness <= 1.0f);

	/* Asserts power of 2 */
	logBase2(NumPolys);

	be = (_Electric_BoltEffect *)geRam_Allocate(sizeof(*be));
	if	(!be)
		return be;

	memset(be, 0, sizeof(*be));

	be->beCenterPoints = (geVec3d *)geRam_Allocate(sizeof(*be->beCenterPoints) * (NumPolys + 1));
	if	(!be->beCenterPoints)
		goto fail;

	be->beNumPoints	= NumPolys;
	be->beWildness	= Wildness;
	be->beWidth		= Width;

	color.r = 160.0f;
	color.g = 160.0f;
	color.b = 255.0f;
	_Electric_BoltEffectSetColorInfo(be, &color, ELECTRIC_BOLT_BLUEDOMINANT);

	return be;

fail:
	if	(be->beCenterPoints)
		geRam_Free(be->beCenterPoints);
	if (be)
		geRam_Free(be);

	return NULL;
}

void _Electric_BoltEffectDestroy(_Electric_BoltEffect *Effect)
{
	geRam_Free(Effect->beCenterPoints);
	geRam_Free(Effect);
}

static	geFloat	GaussRand(void)
{
	int	i;
	int	r;

	r = 0;

	for	(i = 0; i < 6; i++)
		r = r + rand() - rand();

	return (geFloat)r / ((geFloat)RAND_MAX * 6.0f);
}

static	void	subdivide(
	_Electric_BoltEffect *	be,
	const geVec3d *			start,
	const geVec3d *			end,
	geFloat 				s,
	int 					n)
{
	geVec3d	tmp;

	if	(n == 0)
	{
		be->beCurrentPoint++;
		*be->beCurrentPoint = *end;
		return;
	}
	
	tmp.X = (end->X + start->X) / 2 + s * GaussRand();
	tmp.Y = (end->Y + start->Y) / 2 + s * GaussRand();
	tmp.Z = (end->Z + start->Z) / 2 + s * GaussRand();
	subdivide(be,  start, &tmp, s / 2, n - 1);
	subdivide(be, &tmp,    end, s / 2, n - 1);
}

#define	LIGHTNINGWIDTH 8.0f

static	void	genLightning(
	_Electric_BoltEffect *	be,
	int 					RangeLow,
	int 					RangeHigh,
	const geVec3d *			start,
	const geVec3d *			end)
{
	geFloat	length;
	int		seed;

	assert(be);
	assert(start);
	assert(end);
	assert(RangeHigh > RangeLow);
	assert(IsPowerOf2(RangeHigh - RangeLow));

	/* Manhattan length is good enough for this */
	length = (geFloat)(fabs(start->X - end->X) +
						fabs(start->Y - end->Y) +
						fabs(start->Z - end->Z));

	seed = rand();

	srand(seed);
	be->beCurrentPoint					= be->beCenterPoints + RangeLow;
	be->beCenterPoints[RangeLow]		= *start;
	be->beCenterPoints[RangeHigh] 		= *end;
	subdivide(be, start, end, length * be->beWildness, logBase2(RangeHigh - RangeLow));
}

void _Electric_BoltEffectSetColorInfo(
	_Electric_BoltEffect *	Effect,
	GE_RGBA *				BaseColor,
	int						DominantColor)
{
	Effect->beBaseColors[0]		= BaseColor->r;
	Effect->beBaseColors[1]		= BaseColor->g;
	Effect->beBaseColors[2]		= BaseColor->b;
	Effect->beCurrentColors[0]	= BaseColor->r;
	Effect->beCurrentColors[1]	= BaseColor->g;
	Effect->beCurrentColors[2]	= BaseColor->b;
	Effect->beDominantColor 	= DominantColor;
}

void _Electric_BoltEffectAnimate(
	_Electric_BoltEffect *	Effect,
	const geVec3d *			start,
	const geVec3d *			end)
{
	int		dominant;
	int		nonDominant1;
	int		nonDominant2;
	geVec3d	SubdivideStart;
	geVec3d	SubdivideEnd;
	int		LowIndex;
	int		HighIndex;

	Effect->beStart = *start;
	Effect->beEnd	= *end;

	dominant = Effect->beDominantColor;
	nonDominant1 = (dominant + 1) % 3;
	nonDominant2 = (dominant + 2) % 3;
	if	(Effect->beBaseColors[nonDominant1] == Effect->beCurrentColors[nonDominant1])
	{
		int	DecayRate;
		int	Spike;

		DecayRate = rand() % (int)(Effect->beBaseColors[dominant] - Effect->beBaseColors[nonDominant1]);
		DecayRate = max(DecayRate, 5);
		Effect->beDecayRate = DecayRate;
		if	(Effect->beBaseColors[nonDominant1] >= 1.0f)
			Spike = rand() % (int)(Effect->beBaseColors[nonDominant1]);
		else
			Spike = 0;
		Effect->beCurrentColors[nonDominant1] -= Spike;
		Effect->beCurrentColors[nonDominant2] -= Spike;
	}
	else
	{
		Effect->beCurrentColors[nonDominant1] += Effect->beDecayRate;
		Effect->beCurrentColors[nonDominant2] += Effect->beDecayRate;
		if	(Effect->beCurrentColors[nonDominant1] > Effect->beBaseColors[nonDominant1])
		{
			Effect->beCurrentColors[nonDominant1] = Effect->beBaseColors[nonDominant1];
			Effect->beCurrentColors[nonDominant2] = Effect->beBaseColors[nonDominant2];
		}
	}

	Effect->beInitialized = 1;
	LowIndex = 0;
	HighIndex = Effect->beNumPoints;
	SubdivideStart = *start;
	SubdivideEnd   = *end;

	genLightning(Effect, LowIndex, HighIndex, &SubdivideStart, &SubdivideEnd);
}

//#define	LIGHTNINGALPHA	160.0f
#define	LIGHTNINGALPHA	220.0f

void _Electric_BoltEffectRender(
	geWorld *				World,
	_Electric_BoltEffect *	be,
	const geXForm3d *		XForm)
{
	geVec3d			perp;
	geVec3d			temp;
	geVec3d			in;
	GE_LVertex 		verts[4];
	int				i;

	geVec3d_Subtract(&be->beStart, &be->beEnd, &temp);
	geXForm3d_GetIn(XForm, &in);

	geVec3d_CrossProduct(&in, &temp, &perp);
	geVec3d_Normalize(&perp);

	geVec3d_Scale(&perp, be->beWidth / 2.0f, &perp);

	/*
		We've got the perpendicular to the camera in the
		rough direction of the electric bolt center.  Walk
		the left and right sides, constructing verts, then
		do the drawing.
	*/
	for	(i = 0; i < be->beNumPoints - 1; i++)
	{
		geVec3d	temp;

		geVec3d_Subtract(&be->beCenterPoints[i], &perp, &temp);
		verts[0].X = temp.X;
		verts[0].Y = temp.Y;
		verts[0].Z = temp.Z;
		verts[0].u = 0.0f;
		verts[0].v = 0.0f;
		verts[0].r = be->beCurrentColors[0];
		verts[0].g = be->beCurrentColors[1];
		verts[0].b = be->beCurrentColors[2];
		verts[0].a = LIGHTNINGALPHA;

		geVec3d_Subtract(&be->beCenterPoints[i + 1], &perp, &temp);
		verts[1].X = temp.X;
		verts[1].Y = temp.Y;
		verts[1].Z = temp.Z;
		verts[1].u = 0.0f;
		verts[1].v = 1.0f;
		verts[1].r = be->beCurrentColors[0];
		verts[1].g = be->beCurrentColors[1];
		verts[1].b = be->beCurrentColors[2];
		verts[1].a = LIGHTNINGALPHA;

		geVec3d_Add(&be->beCenterPoints[i + 1], &perp, &temp);
		verts[2].X = temp.X;
		verts[2].Y = temp.Y;
		verts[2].Z = temp.Z;
		verts[2].u = 1.0f;
		verts[2].v = 1.0f;
		verts[2].r = be->beCurrentColors[0];
		verts[2].g = be->beCurrentColors[1];
		verts[2].b = be->beCurrentColors[2];
		verts[2].a = LIGHTNINGALPHA;

		geVec3d_Add(&be->beCenterPoints[i], &perp, &temp);
		verts[3].X = temp.X;
		verts[3].Y = temp.Y;
		verts[3].Z = temp.Z;
		verts[3].u = 1.0f;
		verts[3].v = 0.0f;
		verts[3].r = be->beCurrentColors[0];
		verts[3].g = be->beCurrentColors[1];
		verts[3].b = be->beCurrentColors[2];
		verts[3].a = LIGHTNINGALPHA;

		geWorld_AddPolyOnce(World,
							verts,
							4,
							NULL,
							GE_GOURAUD_POLY,
							GE_RENDER_DO_NOT_OCCLUDE_OTHERS,
							1.0f);

	}
}
