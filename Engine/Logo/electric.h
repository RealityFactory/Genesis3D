/****************************************************************************************/
/*  ELECTRIC.H                                                                          */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Animated electrical bolt special effect interface                      */
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
#ifndef	ELECTRIC_H
#define ELECTRIC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "Genesis.h"

#define	ELECTRIC_BOLT_REDDOMINANT	0
#define	ELECTRIC_BOLT_GREENDOMINANT	1
#define	ELECTRIC_BOLT_BLUEDOMINANT	2

#pragma warning( disable : 4068 )

typedef struct	_Electric_BoltEffect
{
	int			beInitialized;
	int			beNumPoints;
	geFloat		beWildness;

	/* For rendering */
	geVec3d		beStart;
	geVec3d		beEnd;

	/* For generating the geometry */
	geVec3d	*	beCenterPoints;
	geVec3d *	beCurrentPoint;

	geFloat		beBaseColors[3];
	geFloat		beCurrentColors[3];
	geFloat		beBaseBlue;
	int			beDecayRate;
	int			beDominantColor;

	int			beWidth;

}	_Electric_BoltEffect;

_Electric_BoltEffect * _Electric_BoltEffectCreate(
	int 					NumPolys,	/* Number of polys, must be power of 2 */
	int						Width,		/* Width in world units of the bolt */
	geFloat 				Wildness);	/* How wild the bolt is (0 to 1 inclusive) */

void _Electric_BoltEffectDestroy(_Electric_BoltEffect *Effect);

void _Electric_BoltEffectAnimate(
	_Electric_BoltEffect *	Effect,
	const geVec3d *			start,		/* Starting point of the bolt */
	const geVec3d *			end);		/* Ending point of the bolt */

void _Electric_BoltEffectRender(
	geWorld *				World,		/* World to render for */
	_Electric_BoltEffect *	Effect,		/* Bolt to render */
	const geXForm3d *		XForm);		/* Transform of our point of view */

void _Electric_BoltEffectSetColorInfo(
	_Electric_BoltEffect *	Effect,
	GE_RGBA *				BaseColor,		/* Base color of the bolt (2 colors should be the same */
	int						DominantColor);	/* Which color is the one to leave fixed */

#ifdef	__cplusplus
}
#endif

#endif

