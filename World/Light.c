/****************************************************************************************/
/*  Light.c                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Handles lightmaps, dynamic light, fog, etc                             */
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
#include <Assert.h>
#include <Windows.h>
#include <Math.h>

#include "BaseType.h"
#include "Ram.h"
#include "System.h"
#include "Vec3d.h"
#include "Light.h"
#include "Surface.h"
#include "GBSPFile.h"
#include "Plane.h"
#include "World.h"

#include "Trace.h"

#define LIGHT_FRACT		8
//=====================================================================================
//	Local Globals
//=====================================================================================
static	geEngine		*CEngine = NULL;
static	geWorld			*CWorld = NULL;
static	World_BSP		*CBSP;
static	GBSP_BSPData	*BSPData;		// This is in globals, but is also kept here for speed
static	Light_LightInfo	*LightInfo;
static  Surf_SurfInfo	*GSurfInfo;

// Temporary light arrays, used for animating lights, and overlaying maps.
static	DRV_RGB			BlankRGB[MAX_LMAP_SIZE*MAX_LMAP_SIZE];
static	DRV_RGB			TempRGB[MAX_LMAP_SIZE*MAX_LMAP_SIZE];
static	int32			TempRGB32[MAX_LMAP_SIZE*MAX_LMAP_SIZE*3];
static	DRV_RGB			TempRGBFog[MAX_LMAP_SIZE*MAX_LMAP_SIZE];
static	int32			TempRGB32Fog[MAX_LMAP_SIZE*MAX_LMAP_SIZE*3];

// Fast sqrt stuff
// MOST_SIG_OFFSET gives the (int *) offset from the address of the double
// to the part of the number containing the sign and exponent.
// You will need to find the relevant offset for your architecture.

#define MOST_SIG_OFFSET 1

// SQRT_TAB_SIZE - the size of the lookup table - must be a power of four.

#define SQRT_TAB_SIZE 16384

// MANT_SHIFTS is the number of shifts to move mantissa into position.
// If you quadruple the table size subtract two from this constant,
// if you quarter the table size then add two.
// Valid values are: (16384, 7) (4096, 9) (1024, 11) (256, 13)

#define MANT_SHIFTS 7

#define FastSqrtFloat	double
#define EXP_BIAS		1023			// Exponents are always positive
#define EXP_SHIFTS		20				// Shifs exponent to least sig. bits
#define EXP_LSB			(1<<EXP_SHIFTS)	//0x00100000		// 1 << EXP_SHIFTS
#define MANT_MASK		(EXP_LSB-1)		//0x000FFFFF		// Mask to extract mantissa
/*
#define FastSqrtFloat	double
#define EXP_BIAS		1023			// Exponents are always positive
#define EXP_SHIFTS		20				// Shifs exponent to least sig. bits
#define EXP_LSB			(1<<EXP_SHIFTS)	//0x00100000		// 1 << EXP_SHIFTS
#define MANT_MASK		0x000FFFFF		// Mask to extract mantissa
*/

int32 SqrtTab[SQRT_TAB_SIZE];
//=====================================================================================
//	Local Static Function prototypes
//=====================================================================================
static void UpdateLTypeTables(geWorld *World);
static int32 FastDist(int32 dx, int32 dy, int32 dz);
static void SetupWavyColorLight1(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh);
static void SetupWavyColorLight2(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh);
static void SetupColorLight1(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh, float intensity);
static void SetupColorLight2(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh, float intensity);
static BOOL CombineDLightWithRGBMap(int32 *LightData, Light_DLight *Light, GFX_Face *Face, Surf_SurfInfo *SInfo);
static BOOL CombineDLightWithRGBMapWithShadow(int32 *LightData, Light_DLight *Light, GFX_Face *Face, Surf_SurfInfo *SInfo);
static void BuildLightLUTS(geEngine *Engine);
static void SetupDynamicLight_r(Light_DLight *pLight, geVec3d *Pos, int32 LNum, int32 Node);

static void AddLightType0(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh);
static void AddLightType(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh, int32 Intensity);
static void AddLightType1(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh, int32 Intensity);
static void AddLightTypeWavy1(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh);
static void AddLightTypeWavy(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh);

static void InitSqrtTab(void);
static FastSqrtFloat FastSqrt(FastSqrtFloat f);
//=====================================================================================
//	Global support functions
//=====================================================================================

//=====================================================================================
//	Light_EngineInit
//=====================================================================================
geBoolean Light_EngineInit(geEngine *Engine)
{
	BuildLightLUTS(Engine);

	InitSqrtTab();

	return GE_TRUE;
}

//=====================================================================================
//	Light_EngineShutdown
//=====================================================================================
void Light_EngineShutdown(geEngine *Engine)
{
	CEngine = NULL;
	CWorld = NULL;
	BSPData = NULL;
}

//=====================================================================================
//	Light_WorldInit
//=====================================================================================
geBoolean Light_WorldInit(geWorld *World)
{
	assert(World != NULL);

	World->LightInfo = GE_RAM_ALLOCATE_STRUCT(Light_LightInfo);

	if (!World->LightInfo)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return GE_FALSE;
	}

	memset(World->LightInfo, 0, sizeof(Light_LightInfo));

	UpdateLTypeTables(World);			// Update ltype tables for first frame

	return GE_TRUE;
}

//=====================================================================================
//	Light_WorldShutdown
//=====================================================================================
void Light_WorldShutdown(geWorld *World)
{
	assert(World != NULL);

	if (!World->LightInfo)
		return;

	geRam_Free(World->LightInfo);

	World->LightInfo = NULL;
}

//=====================================================================================
//	Light_SetEngine
//	Lets this module know that the engine has changed
//=====================================================================================
geBoolean Light_SetEngine(geEngine *Engine)
{
	assert (Engine != NULL);

	CEngine = Engine;

	return GE_TRUE;
}

//=====================================================================================
//	Light_SetWorld
//	Lets this module know that the world has changed
//=====================================================================================
geBoolean Light_SetWorld(geWorld *World)
{
	assert(World != NULL);
	
	CWorld = World;

	LightInfo = World->LightInfo;
	GSurfInfo = World->CurrentBSP->SurfInfo;

	return GE_TRUE;
}

//=====================================================================================
//	Light_SetGBSP
//=====================================================================================
geBoolean Light_SetGBSP(World_BSP *BSP)
{
	assert(BSP != NULL);

	// Make quick pointer to the world bsp data
	CBSP = BSP;
	BSPData = &BSP->BSPData;

	return GE_TRUE;
}

//=====================================================================================
//	Light_FogVerts
//=====================================================================================
void Light_FogVerts(const geFog *Fog, const geVec3d *POV, const geVec3d *Verts, Surf_TexVert *TexVerts, int32 NumVerts)
{
	geVec3d		FogRay;
	geFloat		Radius, RadiusSq, FogRayDot, OneOverRadiusDotBright;
	geFloat		EyeDist;
	int32		i;

	Radius = Fog->VolumeRadius;
	RadiusSq = Fog->VolumeRadiusSquared;
	OneOverRadiusDotBright = (1.0f/Radius)*Fog->VolumeBrightness;

	geVec3d_Subtract(&Fog->Pos, POV, &FogRay);
	FogRayDot = geVec3d_DotProduct(&FogRay,&FogRay);
	EyeDist = (float)sqrt(FogRayDot);

	for (i=0; i< NumVerts; i++, Verts++, TexVerts++)
	{
		geVec3d		Ray, Ray2, Impact1, Impact2;
		geFloat		DistSq, Dist, Disc, v;
		geFloat		t0, t1, t, VertDist, d, d2;

		geVec3d_Subtract(Verts, POV, &Ray);
		geVec3d_Normalize(&Ray);
		
		v = geVec3d_DotProduct(&FogRay, &Ray);

		Disc = (RadiusSq) - (FogRayDot - (v*v));

		if (Disc <= 0)
			continue;
				
		d = (geFloat)sqrt(Disc);

		t0 = v - d;
		t1 = v + d;
					
		if (t0 > 0)
			t = t0;
		else if (t1 > 0)
			t = t1;
		else
			continue;

		geVec3d_Subtract(Verts, &Fog->Pos, &Ray2);
		DistSq = geVec3d_DotProduct(&Ray2, &Ray2);
		VertDist = (geFloat)sqrt(DistSq);
					
		if (EyeDist < Radius && VertDist < Radius)		// Both inside sphere
		{
			Impact1 = *Verts;
			Impact2 = *POV;
		}
		else if (EyeDist < Radius)						// Eye inside
		{
			Impact1 = *POV;
			geVec3d_AddScaled(POV, &Ray, t, &Impact2);
		}
		else if (VertDist < Radius)						// Vert is inside
		{
			Impact1 = *Verts;
			geVec3d_AddScaled(POV, &Ray, t, &Impact2);
		}
		else											// Both lie outside sphere
		{
			geVec3d_AddScaled(POV, &Ray, t0, &Impact1);
			geVec3d_AddScaled(POV, &Ray, t1, &Impact2);
			geVec3d_Subtract(&Impact1, POV, &Ray);
					
			d2 = geVec3d_DotProduct(&Ray, &Ray);

			if (d2 > DistSq)
				continue;
		}
		geVec3d_Subtract(&Impact1, &Impact2, &Ray);
	
		Dist = geVec3d_Length(&Ray)*OneOverRadiusDotBright;

	#if 1
		// Fog the Specular RGB
		TexVerts->r += Dist*Fog->Color.r;
		TexVerts->g += Dist*Fog->Color.g;
		TexVerts->b += Dist*Fog->Color.b;
	#else
		TexVerts->r = 255.0f;
		TexVerts->g = 255.0f;
		TexVerts->b = 255.0f;
	#endif
	}
}

extern int32	MirrorRecursion;						// GLOBAL!!! (defined in World.c)
geVec3d			GlobalEyePos = {100.0f, 0.0f, 0.0f};

//=====================================================================================
//	FogLightmap
//	This is slow, nothing has been pre-computed, but it works, and is being tested!!!
//=====================================================================================
static geBoolean FogLightmap1(geFog *Fog, int32 *LightData, GFX_Face *Face, Surf_SurfInfo *SInfo)
{
	int32		w, h;
	geBoolean	Hit;
	geVec3d		Start, UV, Ray;
	int32		Light1, Light2, Light3;
	float		Dist, d2, UVDist, DistSq;
	geVec3d		FogRay, Ray2;
	float		v,disc, d;
	geVec3d		Impact1, Impact2;
	float		Radius, Red, Grn, Blu, EyeDist;
	float		RadiusSq, FogRayDot, Radius2;
	geVec3d		UAdd, VAdd, FogPos;
	int32		Width, Height;
	
	Hit = GE_FALSE;

	Start = UV = SInfo->TexOrg;

	FogPos = Fog->Pos;
	
	geVec3d_Subtract(&FogPos, &GlobalEyePos, &FogRay);
	
	Ray2 = FogRay;
	EyeDist = geVec3d_Normalize(&Ray2);

	Radius = Fog->VolumeRadius;
	RadiusSq = Fog->VolumeRadiusSquared;
	Radius2 = Fog->VolumeRadius2;

	FogRayDot = geVec3d_DotProduct(&FogRay,&FogRay);
	
	Red = Fog->Color.r;
	Grn = Fog->Color.g;
	Blu = Fog->Color.b;

	//if (EyeDist < Radius)
	//	return GE_FALSE;
	
	UAdd = SInfo->T2WVecs[0];
	VAdd = SInfo->T2WVecs[1];

	Width = SInfo->LInfo.Width;
	Height = SInfo->LInfo.Height;

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			float OneOver;

			Ray.X = UV.X - GlobalEyePos.X;
			Ray.Y = UV.Y - GlobalEyePos.Y;
			Ray.Z = UV.Z - GlobalEyePos.Z;
				
			DistSq = Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z;
			Dist = (float)FastSqrt(DistSq);

			OneOver = 1.0f / Dist;
			Ray.X *= OneOver;
			Ray.Y *= OneOver;
			Ray.Z *= OneOver;

			v = FogRay.X*Ray.X + FogRay.Y*Ray.Y + FogRay.Z*Ray.Z;

			disc = (RadiusSq) - (FogRayDot - (v*v));

			if (disc > 0)
			{
				float t0, t1, t;

				d = (float)FastSqrt(disc);

				t0 = v - d;
				t1 = v + d;
					
				if (t0 > 0)
					t = t0;
				else if (t1 > 0)
					t = t1;
				else
				{
					*LightData++ = 0;
					*LightData++ = 0;
					*LightData++ = 0;
		
					UV.X += UAdd.X;
					UV.Y += UAdd.Y;
					UV.Z += UAdd.Z;
					continue;
				}
					
			#if 1
				Ray2.X = UV.X - FogPos.X;
				Ray2.Y = UV.Y - FogPos.Y;
				Ray2.Z = UV.Z - FogPos.Z;

				//UVDist = sqrt(Ray2.X*Ray2.X + Ray2.Y*Ray2.Y + Ray2.Z*Ray2.Z);
				UVDist = (float)FastSqrt(Ray2.X*Ray2.X + Ray2.Y*Ray2.Y + Ray2.Z*Ray2.Z);
					
				if (EyeDist < Radius && UVDist < Radius)
				{
					Impact1 = UV;
					Impact2 = GlobalEyePos;
				}
				else if (EyeDist < Radius)
				{
					Impact1 = GlobalEyePos;

					Impact2.X = GlobalEyePos.X + t*Ray.X;
					Impact2.Y = GlobalEyePos.Y + t*Ray.Y;
					Impact2.Z = GlobalEyePos.Z + t*Ray.Z;
				}
				else if (UVDist < Radius)							// UV is inside
				{
					Impact1 = UV;

					Impact2.X = GlobalEyePos.X + t*Ray.X;
					Impact2.Y = GlobalEyePos.Y + t*Ray.Y;
					Impact2.Z = GlobalEyePos.Z + t*Ray.Z;
	
				}
				else											// Both lie outside sphere
				{
					Impact1.X = GlobalEyePos.X + t0*Ray.X;
					Impact1.Y = GlobalEyePos.Y + t0*Ray.Y;
					Impact1.Z = GlobalEyePos.Z + t0*Ray.Z;
					
					Impact2.X = GlobalEyePos.X + t1*Ray.X;
					Impact2.Y = GlobalEyePos.Y + t1*Ray.Y;
					Impact2.Z = GlobalEyePos.Z + t1*Ray.Z;
					
					Ray.X = Impact1.X - GlobalEyePos.X;
					Ray.Y = Impact1.Y - GlobalEyePos.Y;
					Ray.Z = Impact1.Z - GlobalEyePos.Z;
					
					//d2 = sqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);
					//d2 = (float)FastSqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);
					d2 = Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z;

					if (d2 > DistSq)
					{
						/*
						Ray.X = Impact2.X - GlobalEyePos.X;
						Ray.Y = Impact2.Y - GlobalEyePos.Y;
						Ray.Z = Impact2.Z - GlobalEyePos.Z;
						d3 = sqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);

						if (d3 > Dist)
						*/
						{
							*LightData++ = 0;
							*LightData++ = 0;
							*LightData++ = 0;
						
							UV.X += UAdd.X;
							UV.Y += UAdd.Y;
							UV.Z += UAdd.Z;
							continue;
						}
					}
					
				}
			#endif
				// Get the distance of the part of the ray that is in the fog
				//geVec3d_Subtract(&Impact1, &Impact2, &Ray);
				//d = geVec3d_Length(&Ray);
				Ray.X = Impact1.X - Impact2.X;
				Ray.Y = Impact1.Y - Impact2.Y;
				Ray.Z = Impact1.Z - Impact2.Z;

				//d = (float)FastSqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);
				//d = ((Radius / d)*18);
				//Dist = (Fog->VolumeBrightness) / (d*d);
				d = Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z;
				d = ((RadiusSq / d)*18);
				Dist = (Fog->VolumeBrightness) / (d);
				//Dist = 255.0f;

				Hit = GE_TRUE;
					
				Light1 = (int32)(Dist*Red);
				Light2 = (int32)(Dist*Grn);
				Light3 = (int32)(Dist*Blu);

				*LightData++ = Light1;
				*LightData++ = Light2;
				*LightData++ = Light3;
			}
			else
			{
				*LightData++ = 0;
				*LightData++ = 0;
				*LightData++ = 0;
			}

			UV.X += UAdd.X;
			UV.Y += UAdd.Y;
			UV.Z += UAdd.Z;

		}	

		Start.X += VAdd.X;
		Start.Y += VAdd.Y;
		Start.Z += VAdd.Z;
		UV = Start;
	}

	return Hit;
}

//=====================================================================================
//	FogLightmap
//	This is slow, nothing has been pre-computed, but it works, and is being tested!!!
//=====================================================================================
static geBoolean FogLightmap2(geFog *Fog, int32 *LightData, GFX_Face *Face, Surf_SurfInfo *SInfo)
{
	int32		w, h;
	geBoolean	Hit;
	geVec3d		Start, UV, Ray;
	int32		Light1, Light2, Light3;
	float		Dist, d2, UVDist, DistSq;
	geVec3d		FogRay, Ray2;
	float		v,disc, d;
	geVec3d		Impact1, Impact2;
	float		Radius, Red, Grn, Blu, EyeDist;
	float		RadiusSq, FogRayDot, Radius2;
	geVec3d		UAdd, VAdd, FogPos;
	int32		LightAdd, Width, Height;
	
	Hit = GE_FALSE;

	Start = UV = SInfo->TexOrg;

	FogPos = Fog->Pos;
	
	geVec3d_Subtract(&FogPos, &GlobalEyePos, &FogRay);
	
	Ray2 = FogRay;
	EyeDist = geVec3d_Normalize(&Ray2);

	Radius = Fog->VolumeRadius;
	RadiusSq = Fog->VolumeRadiusSquared;
	Radius2 = Fog->VolumeRadius2;

	FogRayDot = geVec3d_DotProduct(&FogRay,&FogRay);
	
	Red = Fog->Color.r;
	Grn = Fog->Color.g;
	Blu = Fog->Color.b;

	//if (EyeDist < Radius)
	//	return GE_FALSE;
	
	UAdd = SInfo->T2WVecs[0];
	VAdd = SInfo->T2WVecs[1];

	Width = SInfo->LInfo.Width;
	Height = SInfo->LInfo.Height;

	LightAdd = (SInfo->LInfo.Width - Width)*3;

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			float OneOver;

			Ray.X = UV.X - GlobalEyePos.X;
			Ray.Y = UV.Y - GlobalEyePos.Y;
			Ray.Z = UV.Z - GlobalEyePos.Z;
				
			//Dist = (float)sqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);
			DistSq = Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z;
			Dist = (float)FastSqrt(DistSq);

			OneOver = 1.0f / Dist;
			Ray.X *= OneOver;
			Ray.Y *= OneOver;
			Ray.Z *= OneOver;

			v = FogRay.X*Ray.X + FogRay.Y*Ray.Y + FogRay.Z*Ray.Z;

			disc = (RadiusSq) - (FogRayDot - (v*v));

			if (disc > 0)
			{
				float t0, t1, t;

				d = (float)FastSqrt(disc);
				//d = (float)sqrt(disc);

				t0 = v - d;
				t1 = v + d;
					
				if (t0 > 0)
					t = t0;
				else if (t1 > 0)
					t = t1;
				else
				{
					LightData+=3;
		
					UV.X += UAdd.X;
					UV.Y += UAdd.Y;
					UV.Z += UAdd.Z;
					continue;
				}
					
			#if 1
				Ray2.X = UV.X - FogPos.X;
				Ray2.Y = UV.Y - FogPos.Y;
				Ray2.Z = UV.Z - FogPos.Z;

				//UVDist = sqrt(Ray2.X*Ray2.X + Ray2.Y*Ray2.Y + Ray2.Z*Ray2.Z);
				UVDist = (float)FastSqrt(Ray2.X*Ray2.X + Ray2.Y*Ray2.Y + Ray2.Z*Ray2.Z);
					
				if (EyeDist < Radius && UVDist < Radius)
				{
					Impact1 = UV;
					Impact2 = GlobalEyePos;
				}
				else if (EyeDist < Radius)
				{
					Impact1 = GlobalEyePos;

					Impact2.X = GlobalEyePos.X + t*Ray.X;
					Impact2.Y = GlobalEyePos.Y + t*Ray.Y;
					Impact2.Z = GlobalEyePos.Z + t*Ray.Z;
				}
				else if (UVDist < Radius)							// UV is inside
				{
					Impact1 = UV;

					Impact2.X = GlobalEyePos.X + t*Ray.X;
					Impact2.Y = GlobalEyePos.Y + t*Ray.Y;
					Impact2.Z = GlobalEyePos.Z + t*Ray.Z;
	
				}
				else											// Both lie outside sphere
				{
					Impact1.X = GlobalEyePos.X + t0*Ray.X;
					Impact1.Y = GlobalEyePos.Y + t0*Ray.Y;
					Impact1.Z = GlobalEyePos.Z + t0*Ray.Z;
					
					Impact2.X = GlobalEyePos.X + t1*Ray.X;
					Impact2.Y = GlobalEyePos.Y + t1*Ray.Y;
					Impact2.Z = GlobalEyePos.Z + t1*Ray.Z;
					
					Ray.X = Impact1.X - GlobalEyePos.X;
					Ray.Y = Impact1.Y - GlobalEyePos.Y;
					Ray.Z = Impact1.Z - GlobalEyePos.Z;
					
					//d2 = sqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);
					//d2 = (float)FastSqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);
					d2 = Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z;

					if (d2 > DistSq)
					{
						/*
						Ray.X = Impact2.X - GlobalEyePos.X;
						Ray.Y = Impact2.Y - GlobalEyePos.Y;
						Ray.Z = Impact2.Z - GlobalEyePos.Z;
						d3 = sqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);

						if (d3 > Dist)
						*/
						{
							LightData+=3;
						
							UV.X += UAdd.X;
							UV.Y += UAdd.Y;
							UV.Z += UAdd.Z;
							continue;
						}
					}
					
				}
			#endif
				// Get the distance of the part of the ray that is in the fog
				//geVec3d_Subtract(&Impact1, &Impact2, &Ray);
				//d = geVec3d_Length(&Ray);
				Ray.X = Impact1.X - Impact2.X;
				Ray.Y = Impact1.Y - Impact2.Y;
				Ray.Z = Impact1.Z - Impact2.Z;

				//d = (float)FastSqrt(Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z);
				//d = ((Radius / d)*18);
				//Dist = (Fog->VolumeBrightness) / (d*d);
				d = Ray.X*Ray.X + Ray.Y*Ray.Y + Ray.Z*Ray.Z;
				d = ((RadiusSq / d)*18);
				Dist = (Fog->VolumeBrightness) / (d);
				//Dist = 255.0f;

				Hit = GE_TRUE;
					
				Light1 = (int32)(Dist*Red);
				Light2 = (int32)(Dist*Grn);
				Light3 = (int32)(Dist*Blu);

				*LightData += Light1;
				LightData++;
				*LightData += Light2;
				LightData++;
				*LightData += Light3;
				LightData++;
			}
			else
			{
				LightData+=3;
			}

			UV.X += UAdd.X;
			UV.Y += UAdd.Y;
			UV.Z += UAdd.Z;

		}	

		Start.X += VAdd.X;
		Start.Y += VAdd.Y;
		Start.Z += VAdd.Z;
		UV = Start;
		
		LightData += LightAdd;

	}

	return Hit;
}

//=====================================================================================
//	Light_SetupLightmap
//=====================================================================================
void Light_SetupLightmap(DRV_LInfo *LInfo, BOOL *Dynamic)
{
	int32			LightOffset;
	geBoolean		IsDyn, HasDLight;
	int32			NumLTypes;
	int32			i, Ln;
	Surf_SurfInfo	*SInfo;
	GFX_Face		*Face;
	int32			Intensity;
	uint8			*LightData;
	int32			lWidth, lHeight, LMapSize, MapNum, SIndex;
	int32			*pRGB1;
	DRV_RGB			*pRGB2;

	assert (CBSP != NULL);
	assert(BSPData != NULL);
	assert(BSPData->GFXFaces != NULL);
	assert(BSPData->GFXLightData != NULL);
	assert(BSPData->GFXTexInfo != NULL);
	assert(CWorld->LightInfo != NULL);

	SInfo = &CBSP->SurfInfo[LInfo->Face];
	Face = &BSPData->GFXFaces[LInfo->Face];

	NumLTypes = SInfo->NumLTypes;
	
	LightOffset = Face->LightOfs;
	
	lWidth = LInfo->Width;
	lHeight = LInfo->Height;
	LMapSize = lHeight * lWidth;

	IsDyn = HasDLight = GE_FALSE;

	if (SInfo->DLightFrame == CWorld->CurFrameDynamic)
	if (LightInfo->NumDynamicLights > 0 && !(BSPData->GFXTexInfo[Face->TexInfo].Flags & TEXINFO_FULLBRIGHT)) 
		HasDLight = GE_TRUE;

	// We can early out if no dlights on this surface, and no styled lighting...
	if (!HasDLight && !(SInfo->Flags & SURFINFO_LTYPED))
	{
		if (LightOffset >=0) 
			LInfo->RGBLight[0] = (DRV_RGB*)&BSPData->GFXLightData[LightOffset+1];
		else
			LInfo->RGBLight[0] = BlankRGB;
		goto FogOnly;
	}

	CEngine->DebugInfo.LMap1++;

	// If there is light data
	if (LightOffset >=0) 
	{
		// FIXME:	Take out one byte RGB check...
		//	Lightmaps are allways RGB now...
		LightData = &BSPData->GFXLightData[LightOffset+1];

		// layer all styles on first map, using its intensity table
		for (MapNum = 0; MapNum < 4; MapNum++) 
		{
			SIndex = Face->LTypes[MapNum];
			
			if (SIndex == 255)
				break;
			
			if (LightInfo->LTypeDynamic[SIndex])
				IsDyn = GE_TRUE;

			Intensity = LightInfo->LTypeIntensities[SIndex];

			if (SIndex == 11)
			{
				if (MapNum == 0)
					AddLightTypeWavy1(TempRGB32, LightData, lWidth, lHeight);
				else
					AddLightTypeWavy(TempRGB32, LightData, lWidth, lHeight);

				IsDyn = GE_TRUE;		// Need to force to true
			}
			else
			{
				if (SIndex == 0)		// LType 0 is always in first light slot
					AddLightType0(TempRGB32, LightData, lWidth, lHeight);
				else
				{
					if (MapNum == 0)
						AddLightType1(TempRGB32, LightData, lWidth, lHeight, Intensity);
					else
						AddLightType(TempRGB32, LightData, lWidth, lHeight, Intensity);
				}
			}

			LightData += LMapSize*3;
		}
	}
	else
		memset(TempRGB32, 0, LMapSize*3*sizeof(TempRGB32[0]));
	
	// Tack on dynamic lights
	if (HasDLight)
	{
		Light_DLight	*DLights = LightInfo->DynamicLights;

		for (Ln = 0; Ln <MAX_DYNAMIC_LIGHTS; Ln++, DLights++) 
		{
			if (!DLights->Active) 
				continue;

			if (!(SInfo->DLights & (1<<Ln)))
				continue;

			CEngine->DebugInfo.NumDLights++;

			if (DLights->CastShadow)
			{
				if (CombineDLightWithRGBMapWithShadow(TempRGB32, DLights, Face, SInfo))
					IsDyn = GE_TRUE;
			}
			else
			{
				if (CombineDLightWithRGBMap(TempRGB32, DLights, Face, SInfo))
					IsDyn = GE_TRUE;
			}
		}
	}

	// Put the light into a driver compatible pointer, and clamp it 
	pRGB1 = TempRGB32;
	pRGB2 = TempRGB;
	for (i=0; i< LMapSize; i++)
	{
		Intensity = (*pRGB1++) >> LIGHT_FRACT;
		if (Intensity > 255)
			Intensity = 255;
		else if (Intensity < 0 )
			Intensity = 0;
		pRGB2->r = (uint8)Intensity;

		Intensity = (*pRGB1++) >> LIGHT_FRACT;
		if (Intensity > 255)
			Intensity = 255;
		else if (Intensity < 0 )
			Intensity = 0;
		pRGB2->g = (uint8)Intensity;

		Intensity = (*pRGB1++) >> LIGHT_FRACT;
		if (Intensity > 255)
			Intensity = 255;
		else if (Intensity < 0 )
			Intensity = 0;
		pRGB2->b = (uint8)Intensity;

		pRGB2++;
	}
	
	// Point the lightmap to the data
	LInfo->RGBLight[0] = TempRGB;

	FogOnly:		// Jump to here, for fog lightmap only...

	//
	// Fog the lightmap (setup lightmap 1)
	//
	LInfo->RGBLight[1] = NULL;

#if 1
	if (!MirrorRecursion)		// Only do fog on first pass, not in mirrors...
	{
		geFog		*Fog;
		geBoolean	WasFog;
		
		WasFog = GE_FALSE;
		
		if (CWorld->NumVisibleFog)
		{
			for (i=0; i< CWorld->NumVisibleFog; i++)
			{
				Fog = CWorld->VisibleFog[i];
			
				if (i == 0)		// Use FogLightmap1 for first one ONLY
				{
					if (FogLightmap1(Fog, TempRGB32Fog, Face, SInfo))
						WasFog = GE_TRUE;
				}
				else			// All other fog lights use FogLightmap2
				{
					if (FogLightmap2(Fog, TempRGB32Fog, Face, SInfo))
						WasFog = GE_TRUE;
				}
			}
		}
		
		if (WasFog)
		{
			CEngine->DebugInfo.LMap2++;

			// Put the light into a driver compatible pointer, and clamp it 
			pRGB1 = TempRGB32Fog;
			pRGB2 = TempRGBFog;
			for (i=0; i< LMapSize; i++)
			{
				Intensity = (*pRGB1++) >> LIGHT_FRACT;
				if (Intensity > 255)
					Intensity = 255;
				pRGB2->r = (uint8)Intensity;

				Intensity = (*pRGB1++) >> LIGHT_FRACT;
				if (Intensity > 255)
					Intensity = 255;
				pRGB2->g = (uint8)Intensity;

				Intensity = (*pRGB1++) >> LIGHT_FRACT;
				if (Intensity > 255)
					Intensity = 255;
				pRGB2->b = (uint8)Intensity;

				pRGB2++;
			}
			IsDyn = TRUE;			// Force this face to be dynamic
			LInfo->RGBLight[1] = TempRGBFog;
		}
	}
#endif

	if (Dynamic)
	{
		*Dynamic = IsDyn;
	}
}

//=====================================================================================
//	AddLightType
//=====================================================================================
static void AddLightType(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh, int32 Intensity)
{	
	int32	h;

	assert(LightDest != NULL);
	assert(LightData != NULL);

	for (h = 0; h < lw*lh; h++)
	{
		*LightDest++ += *LightData++ * Intensity;
		*LightDest++ += *LightData++ * Intensity;
		*LightDest++ += *LightData++ * Intensity;
	}
}

//=====================================================================================
//	AddLightType0
//=====================================================================================
static void AddLightType0(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh)
{	
	int32	h;

	assert(LightDest != NULL);
	assert(LightData != NULL);

	for (h = 0; h < lw*lh; h++)
	{
		*LightDest++ = (*LightData++) << LIGHT_FRACT;
		*LightDest++ = (*LightData++) << LIGHT_FRACT;
		*LightDest++ = (*LightData++) << LIGHT_FRACT;
	}
}

//=====================================================================================
//	AddLightType1
//=====================================================================================
static void AddLightType1(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh, int32 Intensity)
{	
	int32	h;

	assert(LightDest != NULL);
	assert(LightData != NULL);

	for (h = 0; h < lw*lh; h++)
	{
		*LightDest++ = *LightData++ * Intensity;
		*LightDest++ = *LightData++ * Intensity;
		*LightDest++ = *LightData++ * Intensity;
	}
}

//=====================================================================================
//	CombineDLightWithRGBMap
//=====================================================================================
static BOOL CombineDLightWithRGBMap(int32 *LightData, Light_DLight *Light, GFX_Face *Face, Surf_SurfInfo *SInfo)
{
	BOOL		Hit;
	float		Radius, Dist;
	GFX_Plane	*Plane;
	GFX_TexInfo	*TexInfo;
	geVec3d		LPos;
	int32		Sx, Sy, x, y, u, v, Val;
	int32		ColorR, ColorG, ColorB, Radius2, Dist2;
	int32		FixedX, FixedY, XStep, YStep;

	assert(BSPData != NULL);
	assert(BSPData->GFXTexInfo != NULL);
	assert(BSPData->GFXPlanes != NULL);

	TexInfo = &BSPData->GFXTexInfo[Face->TexInfo];

	Radius = Light->Radius;
	Plane = &BSPData->GFXPlanes[Face->PlaneNum];

	geVec3d_Copy(&Light->Pos, &LPos);

	Dist = geVec3d_DotProduct(&LPos, &Plane->Normal) - Plane->Dist;

	// Shrink the radius by the dist so we can avoid using z in the fast sqrt routine...
	Radius -= (float)fabs(Dist);

	if (Radius <= 0)		// We can leave now if the dist is > radius
		return FALSE;

	// Calculate where light is projected onto the 2d-plane
	Sx = (int32)(geVec3d_DotProduct(&LPos, &TexInfo->Vecs[0]));
	Sy = (int32)(geVec3d_DotProduct(&LPos, &TexInfo->Vecs[1]));

	// Align the light with the upper left corner of the ligtmap
	Sx -= SInfo->LInfo.MinU;		
	Sy -= SInfo->LInfo.MinV;

	// Scale by the texture scaling (1:21:10 fixed)
	Sx *= SInfo->XScale;
	Sy *= SInfo->YScale;
	
	Hit = FALSE;

	ColorR = Light->FColorR; 
	ColorG = Light->FColorG; 
	ColorB = Light->FColorB; 

	Radius2 = (int32)Radius;
	
	XStep = SInfo->XStep;
	YStep = SInfo->YStep;

	FixedY = Sy;

	for (v=0; v< SInfo->LInfo.Height; v++)
	{
		y = FixedY >> 10;

		if (y < 0)
			y = -y;

		FixedX = Sx;
		
		for (u=0; u< SInfo->LInfo.Width; u++)
		{
			x = FixedX >> 10;

			if (x<0)
				x = -x;

			if (x > y)
				Dist2 = (x + (y>>1));
			else
				Dist2 = (y + (x>>1));
			
			if (Dist2 < Radius2)
			{
				Hit = TRUE;
				
				Val = (Radius2 - Dist2);

				*LightData++ += (int32)(Val * ColorR);
				*LightData++ += (int32)(Val * ColorG);
				*LightData++ += (int32)(Val * ColorB);
			}
			else
				LightData+=3;

			FixedX -= XStep;
		}

		FixedY -= YStep;
	}

	return Hit;
}

//=====================================================================================
//	CombineDLightWithRGBMap
//=====================================================================================
static BOOL CombineDLightWithRGBMapWithShadow(int32 *LightData, Light_DLight *Light, GFX_Face *Face, Surf_SurfInfo *SInfo)
{
	BOOL		Hit;
	float		Radius, Dist;
	GFX_Plane	*Plane;
	GFX_TexInfo	*TexInfo;
	geVec3d		LPos, LMapPos, Right, Down, Start;
	int32		Sx, Sy, x, y, u, v, Val;
	int32		ColorR, ColorG, ColorB, Radius2, Dist2;
	int32		FixedX, FixedY, XStep, YStep;

	assert(BSPData != NULL);
	assert(BSPData->GFXTexInfo != NULL);
	assert(BSPData->GFXPlanes != NULL);

	TexInfo = &BSPData->GFXTexInfo[Face->TexInfo];

	Radius = Light->Radius;
	Plane = &BSPData->GFXPlanes[Face->PlaneNum];

	geVec3d_Copy(&Light->Pos, &LPos);

	Dist = geVec3d_DotProduct(&LPos, &Plane->Normal) - Plane->Dist;

	// Shrink the radius by the dist so we can avoid using z in the fast sqrt routine...
	Radius -= (float)fabs(Dist);

	if (Radius <= 0)		// We can leave now if the dist is > radius
		return FALSE;

	// Calculate where light is projected onto the 2d-plane
	Sx = (int32)(geVec3d_DotProduct(&LPos, &TexInfo->Vecs[0]));// + TexInfo->Shift[0]);
	Sy = (int32)(geVec3d_DotProduct(&LPos, &TexInfo->Vecs[1]));// + TexInfo->Shift[1]);

	// Shift into lightmap
	Sx -= SInfo->LInfo.MinU;
	Sy -= SInfo->LInfo.MinV;

	// Scale by the texture scaling (1:21:10 fixed)
	Sx *= SInfo->XScale;
	Sy *= SInfo->YScale;
		
	Hit = FALSE;

	ColorR = Light->FColorR; 
	ColorG = Light->FColorG; 
	ColorB = Light->FColorB; 

	Radius2 = (int32)Radius;
	
	XStep = SInfo->XStep;
	YStep = SInfo->YStep;

	FixedY = Sy;

	LMapPos = Start = SInfo->TexOrg;
	Right = SInfo->T2WVecs[0];
	Down = SInfo->T2WVecs[1];

	Trace_SetupIntersect(CWorld);		// Setup intersection test with current world...

	for (v=0; v< SInfo->LInfo.Height; v++)
	{
		y = FixedY >> 10;

		if (y < 0)
			y = -y;

		FixedX = Sx;
		
		for (u=0; u< SInfo->LInfo.Width; u++)
		{
			x = FixedX >> 10;

			if (x<0)
				x = -x;

			if (x > y)
				Dist2 = (x + (y>>1));
			else
				Dist2 = (y + (x>>1));
			
			if (Dist2 < Radius2)
			{
				if (Trace_IntersectWorldBSP(&LPos, &LMapPos, 0))
				{
					LightData += 3;
					FixedX -= XStep;
					geVec3d_Add(&LMapPos, &Right, &LMapPos);
					continue;
				}

				Hit = TRUE;
				
				Val = (Radius2 - Dist2);

				*LightData++ += (int32)(Val * ColorR);
				*LightData++ += (int32)(Val * ColorG);
				*LightData++ += (int32)(Val * ColorB);
			}
			else
				LightData+=3;

			geVec3d_Add(&LMapPos, &Right, &LMapPos);

			FixedX -= XStep;
		}

		geVec3d_Add(&Start, &Down, &Start);
		LMapPos = Start;
		
		FixedY -= YStep;
	}

	return Hit;
}

//=====================================================================================
//	Light_SetupLights
//	Post processes the lights render q so the world can use them while rendering
//=====================================================================================
geBoolean Light_SetupLights(geWorld *World)
{
	int32			i, Node;
	Light_DLight	*DLights;

	assert(World != NULL);

	assert(BSPData != NULL);
	assert(BSPData->GFXModels != NULL);
	assert(LightInfo != NULL);

	// Update the intensity tables for dynamic ltyped lighting
	UpdateLTypeTables(World);

	DLights = LightInfo->DynamicLights;

#if 0
	Node = BSPData->GFXModels[0].RootNode[0];

	for (i=0; i< MAX_DYNAMIC_LIGHTS; i++, DLights++)
	{
		if (!DLights->Active)
			continue;

		SetupDynamicLight_r(DLights, &DLights->Pos, i, Node);
	}
#else
	for (i=0; i< MAX_DYNAMIC_LIGHTS; i++, DLights++)
	{
		int32		m;
		geVec3d		NewPos;
		geWorld_Model *Model;

		if (!DLights->Active)
			continue;

		Model = CBSP->Models;

		for (m=0; m< BSPData->NumGFXModels; m++, Model++)
		{
			if (Model->VisFrame != World->CurFrameDynamic)
				continue;

			Node = BSPData->GFXModels[m].RootNode[0];
			
			geVec3d_Subtract(&DLights->Pos, &Model->Pivot, &NewPos);
			// InverseTransform the light about models center of rotation
			geXForm3d_TransposeTransform(&Model->XForm, &NewPos, &NewPos);
			
			geVec3d_Add(&NewPos , &Model->Pivot, &NewPos);

			SetupDynamicLight_r(DLights, &NewPos, i, Node);
		}
	}
#endif

	return GE_TRUE;
}
//=====================================================================================
//	Light_WorldGetLTypeCurent
//=====================================================================================
char		Light_WorldGetLTypeCurrent(geWorld *World, int32 LType)
{
	Light_LightInfo	*LInfo;
	assert( World != NULL );
	assert( LType >= 0 );
	assert( LType < MAX_LTYPES );

	LInfo = World->LightInfo;
	assert( LInfo != NULL );

	return (LInfo->LTypeTable[LType][LInfo->IPos[LType]]) ;
}


//=====================================================================================
//	UpdateLTypeTables
//=====================================================================================
static void UpdateLTypeTables(geWorld *World)
{
	int32			s;
	float			i;
	Light_LightInfo	*LInfo;

	LInfo = World->LightInfo;
		
	for (s =0; s <MAX_LTYPES; s++) 
	{
		LInfo->IPos[s]++;

		if (LInfo->LTypeTable[s][LInfo->IPos[s]] == 0) 
			LInfo->IPos[s] = 0;

		i = (float)(LInfo->LTypeTable[s][LInfo->IPos[s]]-96) / 27.0f;

		LInfo->LTypeIntensities[s] = (int32)(i * (1<<LIGHT_FRACT));

		if (LInfo->LTypeIntensities2[s] != LInfo->LTypeTable[s][LInfo->IPos[s]]-96)
			LInfo->LTypeDynamic[s] = TRUE;
		else
			LInfo->LTypeDynamic[s] = FALSE;
	
		LInfo->LTypeIntensities2[s] = LInfo->LTypeTable[s][LInfo->IPos[s]]-96;
	}
}   

//=====================================================================================
//	Light_WorldAddLight
//=====================================================================================
Light_DLight *Light_WorldAddLight(geWorld *World)
{
	Light_LightInfo	*LInfo;
	Light_DLight	*DLights;
	int32			i;

	assert(World != NULL);

	LInfo = World->LightInfo;

	assert(LInfo);

	DLights = LInfo->DynamicLights;

	for (i=0; i< MAX_DYNAMIC_LIGHTS; i++, DLights++)
	{
		if (!DLights->Active)
			break;
	}

	if (i >= MAX_DYNAMIC_LIGHTS)
	{
		geErrorLog_Add(GE_ERR_RENDERQ_OVERFLOW, NULL);
		return  NULL;
	}

	// Set it's attributes to some default...
	memset(DLights, 0, sizeof(Light_DLight));

	DLights->Active = GE_TRUE;
	LInfo->NumDynamicLights++;
	

	return DLights;
}

//=====================================================================================
//	Light_WorldRemoveLight
//=====================================================================================
void Light_WorldRemoveLight(geWorld *World, Light_DLight *DLight)
{
	assert(World);
	assert(World->LightInfo);
	assert(DLight);
	assert(DLight->Active);

	if (!DLight->Active)
		return;

	DLight->Active = GE_FALSE;
	World->LightInfo->NumDynamicLights--;
}

//=====================================================================================
//	Light_SetAttributes
//=====================================================================================
geBoolean Light_SetAttributes(	Light_DLight *Light, 
								const geVec3d *Pos, 
								const GE_RGBA *RGBA, 
								float Radius,
								geBoolean CastShadow)
{
	assert(Light != NULL);
	
	Light->Pos = *Pos;
	Light->Color = *RGBA;
	Light->Radius = Radius;

	// Pre-compute fixed point light colors
	Light->FColorR = (int32)((Light->Color.r/195.0f) * (1<<LIGHT_FRACT));
	Light->FColorG = (int32)((Light->Color.g/195.0f) * (1<<LIGHT_FRACT));
	Light->FColorB = (int32)((Light->Color.b/195.0f) * (1<<LIGHT_FRACT));

	Light->CastShadow = CastShadow;

	return GE_TRUE;
}

//=====================================================================================
//	Light_SetLTypeTable
//=====================================================================================
geBoolean Light_WorldSetLTypeTable(geWorld *World, int32 LType, const char *Table)
{
	assert(World != NULL);
	assert(World->LightInfo != NULL);

	if (LType < 0 || LType >= MAX_LTYPES)
	{
		geErrorLog_Add(GE_ERR_INVALID_LTYPE, NULL);
		return GE_FALSE;
	}

	assert(strlen(Table) < 70);

	strcpy(World->LightInfo->LTypeTable[LType], Table);

	return GE_TRUE;
}

//=====================================================================================
//	Local static support functions
//=====================================================================================

//=====================================================================================
//	FastDist
//	Quick sqrt routine
//=====================================================================================
static int32 FastDist(int32 dx, int32 dy, int32 dz)
{
	int32		Max, Med, Min;	// biggest, smallest, and middle values
	int32		Tmp;

	Max = (dx < 0) ? -dx : dx;
	Med = (dz < 0) ? -dz : dz;
	Min = (dy < 0) ? -dy : dy;

	if (Max < Med) 
	{ 
		Tmp = Max; Max = Med; Med = Tmp;
	}
	if (Max < Min) 
	{ 
		Tmp = Max; Max = Min; Min = Tmp;
	}
	Max <<= 2;
	Max += Med + Min;
	return Max>>2;
}

#define A_MOD 7							// How much to modulus the anim too

//=====================================================================================
//	SetupWavyColorLight1
//=====================================================================================
static void SetupWavyColorLight1(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh)
{ 
	int32	h;
	DRV_RGB *rgb;
	U16		r,g,b, II;
	DRV_RGB *lm;
	uint16	Index;
	uint8	*SLUT;
	int16	*WaveTable;

	assert(light1 != NULL);
	assert(RGBM != NULL);

	assert(CEngine != NULL);

	WaveTable = CEngine->WaveTable;
	
	lm = light1;
	rgb = RGBM;
	Index = 0;

	SLUT = CEngine->StyleLUT1[0];

	for (h = 0; h < lh*lw; h++)
	{
		// We know that WaterColor table is from 0-255...
		II = WaveTable[Index++];
		if (Index > A_MOD)
			Index -= A_MOD;
		
		II >>= 2;
		if (II > 63) II = 63;
		II <<= 8;
		r = SLUT[II + lm->r];
		g = SLUT[II + lm->g];
		b = SLUT[II + lm->b];

		rgb->r = (uint8)r;
		rgb->g = (uint8)g;
		rgb->b = (uint8)b;

		lm++;
		rgb++;
	}
}

//=====================================================================================
//	SetupWavyColorLight2
//=====================================================================================
static void SetupWavyColorLight2(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh)
{
	int32 h;
	DRV_RGB		*rgb;
	U16			r,g,b, II;
	DRV_RGB		*lm;
	uint16		Index;
	uint8		*SLUT;
	int16		*WaveTable;

	assert(light1 != NULL);
	assert(RGBM != NULL);

	assert(CEngine != NULL);
	
	WaveTable = CEngine->WaveTable;

	lm = light1;
	rgb = RGBM;
	Index = 0;

	SLUT = CEngine->StyleLUT1[0];

	for (h = 0; h < lh*lw; h++)
	{
		// We know that WaterColor table is from 0-255...
		II = WaveTable[Index++];
		if (Index > A_MOD)
			Index -= A_MOD;
		
		II >>= 2;
		if (II > 63) 
			II = 63;
		II <<= 8;

		r = rgb->r + SLUT[II + lm->r];
		g = rgb->g + SLUT[II + lm->g];
		b = rgb->b + SLUT[II + lm->b];
		
		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;

		rgb->r = (uint8)r;
		rgb->g = (uint8)g;
		rgb->b = (uint8)b;

		lm++;
		rgb++;
	}
}

//=====================================================================================
//	SetupColorLight1
//=====================================================================================
static void SetupColorLight1(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh, float intensity)
{	
	int32	h;
	DRV_RGB *rgb;
	uint16	r,g,b;
	DRV_RGB *lm;
	uint16	II;
	uint8	*SLUT;
	
	assert(light1 != NULL);
	assert(RGBM != NULL);
	assert(CEngine != NULL);

	lm = light1;
	rgb = RGBM;
	II = (uint16)(intensity * 63);

	if (II > 63) II = 63;

	SLUT = &CEngine->StyleLUT1[II][0];
	
	for (h = 0; h < lh*lw; h++)
	{
		r = SLUT[lm->r];
		g = SLUT[lm->g];
		b = SLUT[lm->b];

		rgb->r = (uint8)r;
		rgb->g = (uint8)g;
		rgb->b = (uint8)b;

		lm++;
		rgb++;
	}
}

//=====================================================================================
//	SetupColorLight2
//=====================================================================================
static void SetupColorLight2(DRV_RGB *light1, DRV_RGB *RGBM, int32 lw, int32 lh, float intensity)
{
	int32	h;
	DRV_RGB *rgb;
	uint16	r,g,b;
	DRV_RGB *lm;
	uint16	II;
	uint8	*SLUT;

	assert(light1 != NULL);
	assert(RGBM != NULL);
	assert(CEngine != NULL);

	lm = light1;
	rgb = RGBM;
	II = (uint16)(intensity * 63);

	if (II > 63) 
		II = 63;
	
	SLUT = &CEngine->StyleLUT1[II][0];
	
	for (h = 0; h < lh*lw; h++)
	{
		
		r = rgb->r + SLUT[lm->r];
		g = rgb->g + SLUT[lm->g];
		b = rgb->b + SLUT[lm->b];

		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;

		rgb->r = (uint8)r;
		rgb->g = (uint8)g;
		rgb->b = (uint8)b;
		
		lm++;
		rgb++;
	}
}

//=====================================================================================
//	AddLightTypeWavy1
//=====================================================================================
static void AddLightTypeWavy1(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh)
{ 
	uint16	*WaveTable, II;
	int32	Index, h;
	uint8	*SLUT;

	assert(LightDest != NULL);
	assert(LightData != NULL);
	assert(CEngine != NULL);

	WaveTable = CEngine->WaveTable;
	
	Index = 0;

	SLUT = CEngine->StyleLUT1[0];

	for (h = 0; h < lh*lw; h++)
	{
		// We know that WaterColor table is from 0-255...
		II = WaveTable[Index++];
		
		if (Index > A_MOD)
			Index -= A_MOD;
		
		II >>= 2;
		if (II > 30) II = 30;
		II <<= 8;

		*LightDest++ = SLUT[II + *LightData++] << (LIGHT_FRACT);
		*LightDest++ = SLUT[II + *LightData++] << (LIGHT_FRACT);
		*LightDest++ = SLUT[II + *LightData++] << (LIGHT_FRACT);
	}
}

//=====================================================================================
//	AddLightTypeWavy
//=====================================================================================
static void AddLightTypeWavy(int32 *LightDest, uint8 *LightData, int32 lw, int32 lh)
{ 
	uint16	*WaveTable, II;
	int32	Index, h;
	uint8	*SLUT;

	assert(LightDest != NULL);
	assert(LightData != NULL);
	assert(CEngine != NULL);

	WaveTable = CEngine->WaveTable;
	
	Index = 0;

	SLUT = CEngine->StyleLUT1[0];

	for (h = 0; h < lh*lw; h++)
	{
		// We know that WaterColor table is from 0-255...
		II = WaveTable[Index++];
		
		if (Index > A_MOD)
			Index -= A_MOD;
		
		II >>= 2;
		if (II > 30) II = 30;
		II <<= 8;

		*LightDest++ += SLUT[II + *LightData++] << (LIGHT_FRACT);
		*LightDest++ += SLUT[II + *LightData++] << (LIGHT_FRACT);
		*LightDest++ += SLUT[II + *LightData++] << (LIGHT_FRACT);
	}
}

//=====================================================================================
//	BuildStyleTables
//=====================================================================================
static void BuildLightLUTS(geEngine *Engine)
{
	S32 i, k;

	for (i=0; i< 256; i++)
	{
		for (k=0; k< 64; k++)
		{
			float ii = (float)i;
			float kk = (float)k;
			float Intensity = (ii * (kk / 62.0f));// * 5.0f;
			if (Intensity > 255) 
				Intensity = 255.0f;
			Engine->StyleLUT1[k][i] = (U8)Intensity;
		}
	}
}

//=====================================================================================
//	SetupDynamicLight_r
//=====================================================================================
static void SetupDynamicLight_r(Light_DLight *pLight, geVec3d *Pos, int32 LNum, int32 Node)
{
	float			Dist;
	GFX_Plane		*pPlane;
	GFX_Node		*pNode;
	Surf_SurfInfo	*pSInfo;
	int32			i;

	if (Node < 0)	// Hit a leaf no more searching
		return;

	pNode = &BSPData->GFXNodes[Node];
	pPlane = &BSPData->GFXPlanes[pNode->PlaneNum];

	Dist = Plane_PlaneDistanceFast(pPlane, Pos);

	if (Dist > pLight->Radius)
	{
		SetupDynamicLight_r(pLight, Pos, LNum, BSPData->GFXNodes[Node].Children[0]);
		return;
	}
	if (Dist <-pLight->Radius)
	{
		SetupDynamicLight_r(pLight, Pos, LNum, BSPData->GFXNodes[Node].Children[1]);
		return;
	}

	// The light is within range of this plane, mark it and go down both sides
	pSInfo = &CBSP->SurfInfo[pNode->FirstFace];
	for (i=0; i< pNode->NumFaces; i++, pSInfo++)
	{
		if (!Surf_InSurfBoundingBox(pSInfo, Pos, pLight->Radius) ) 
			continue;
		
		if (pSInfo->DLightFrame != CWorld->CurFrameDynamic)
		{
			pSInfo->DLightFrame = CWorld->CurFrameDynamic;
			pSInfo->DLights = 0;
		}
		
		// We might need to go to a linked list, if more than 32 lights are needed
		pSInfo->DLights |= 1<<LNum;
	}

	SetupDynamicLight_r(pLight, Pos, LNum, BSPData->GFXNodes[Node].Children[0]);
	SetupDynamicLight_r(pLight, Pos, LNum, BSPData->GFXNodes[Node].Children[1]);
}

//=====================================================================================
//	Light_GetLightmapRGB
//	Takes a point, and a face , and gets the light map value it was projected on...
//	NOTE: AFX_SetupLightmap should be called first... or Light, RGBLight will be NULL...
// 	(or invalid)
//=====================================================================================
geBoolean Light_GetLightmapRGB(Surf_SurfInfo *Surf, geVec3d *Pos, GE_RGBA *RGBA)
{

	DRV_RGB		*RGBLight;
	float		fr = 0.0f, fg = 0.0f, fb = 0.0f;
	geVec3d		VecU, VecV;
	float		TexU, TexV;
	int32		Index, Width, Height;
	
	// Make sure this is a lightmaped face
	if (!(Surf->Flags & SURFINFO_LIGHTMAP))
		return GE_FALSE;

	RGBLight = Surf->LInfo.RGBLight[0];
	
	if (!RGBLight)						// Make sure we are not null!!!
		return GE_FALSE;

	VecU = BSPData->GFXTexInfo[Surf->TexInfo].Vecs[0];
	VecV = BSPData->GFXTexInfo[Surf->TexInfo].Vecs[1];

	// Project the Point into the texture space
	TexU = geVec3d_DotProduct(Pos, &VecU) + BSPData->GFXTexInfo[Surf->TexInfo].Shift[0];
	TexV = geVec3d_DotProduct(Pos, &VecV) + BSPData->GFXTexInfo[Surf->TexInfo].Shift[1];

	// Align into lightmap space
	TexU -= Surf->LInfo.MinU;
	TexV -= Surf->LInfo.MinV;

	if (TexU < 0 || TexV < 0)
		return GE_FALSE;
	
	if (TexU > Surf->LInfo.Width<<4 || TexV > Surf->LInfo.Height<<4)
		return GE_FALSE;

	// Scale into lightmap space
	TexU /= (16);
	TexV /= (16);

	Width = Surf->LInfo.Width;
	Height = Surf->LInfo.Height;

	// Cap it off if invalid (for some funky reason)
	if (TexU > Width) TexU = (float)Width;
	if (TexV > Height) TexV = (float)Height;
	if (TexU < 0) TexU = 0.0f;
	if (TexV < 0) TexV = 0.0f;

	Index = (int32)TexV * Width + (int32)TexU;
	
	// Return the color dude...
	RGBA->r = (float)RGBLight[Index].r;
	RGBA->g = (float)RGBLight[Index].g;
	RGBA->b = (float)RGBLight[Index].b;

	return GE_TRUE;
}

//=====================================================================================
//	Light_GetLightmapRGBBlended
//=====================================================================================
geBoolean Light_GetLightmapRGBBlended(Surf_SurfInfo *Surf, geVec3d *Pos, GE_RGBA *RGBA)
{

	DRV_RGB		*RGBLight;
	float		fr = 0.0f, fg = 0.0f, fb = 0.0f;
	geVec3d		VecU, VecV;
	float		TexU, TexV;
	int32		Index, Width, Height;
	
	// Make sure this is a lightmaped face
	if (!(Surf->Flags & SURFINFO_LIGHTMAP))
		return GE_FALSE;

	RGBLight = Surf->LInfo.RGBLight[0];
	
	if (!RGBLight)						// Make sure we are not null!!!
		return GE_FALSE;

	TexU = geVec3d_DotProduct(Pos, &VecU) + BSPData->GFXTexInfo[Surf->TexInfo].Shift[0];
	TexV = geVec3d_DotProduct(Pos, &VecV) + BSPData->GFXTexInfo[Surf->TexInfo].Shift[1];

	// Align into lightmap space
	TexU -= Surf->LInfo.MinU;
	TexV -= Surf->LInfo.MinV;

	if (TexU < 0 || TexV < 0)
		return GE_FALSE;
	
	if (TexU > Surf->LInfo.Width<<4 || TexV > Surf->LInfo.Height<<4)
		return GE_FALSE;

	// Scale into lightmap space
	TexU /= (16);
	TexV /= (16);

	Width = Surf->LInfo.Width;
	Height = Surf->LInfo.Height;

	// Cap it off if invalid (for some funky reason)
	if (TexU > Width) TexU = (float)Width;
	if (TexV > Height) TexV = (float)Height;
	if (TexU < 0) TexU = 0.0f;
	if (TexV < 0) TexV = 0.0f;

	Index = (int32)TexV * Width + (int32)TexU;
	
	// Return the color dude...
	RGBA->r = (float)RGBLight[Index].r;
	RGBA->g = (float)RGBLight[Index].g;
	RGBA->b = (float)RGBLight[Index].b;

	return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
static void InitSqrtTab(void)
{
	int32	i;
	double	f;
	int32	*fi = (int32*) &f + MOST_SIG_OFFSET;

	for (i = 0; i < SQRT_TAB_SIZE/2; i++)
	{
		f = 0; /* Clears least sig part */
		*fi = (i << MANT_SHIFTS) | (EXP_BIAS << EXP_SHIFTS);
		f = sqrt(f);
		SqrtTab[i] = *fi & MANT_MASK;

		f = 0; /* Clears least sig part */
		*fi = (i << MANT_SHIFTS) | ((EXP_BIAS + 1) << EXP_SHIFTS);
		f = sqrt(f);
		SqrtTab[i + SQRT_TAB_SIZE/2] = *fi & MANT_MASK;
	}
}

//=====================================================================================
//=====================================================================================
static FastSqrtFloat FastSqrt(FastSqrtFloat f)
{
	int32 e;
	int32 *fi = (int32*) &f + MOST_SIG_OFFSET;

	//return sqrt(f);

	if (f == (FastSqrtFloat)0.0) 
		return((FastSqrtFloat)0.0);
	
	e = (*fi >> EXP_SHIFTS) - EXP_BIAS;
	*fi &= MANT_MASK;
	
	if (e & 1)
		*fi |= EXP_LSB;
	
	e >>= 1;
	*fi = (SqrtTab[*fi >> MANT_SHIFTS]) | ((e + EXP_BIAS) << EXP_SHIFTS);
	
	return(f);
}
#if 0
//=====================================================================================
//=====================================================================================
void DumpSqrtTab()
{
	int32 i, nl = 0;

	printf("unsigned int SqrtTab[] = {\n");

	for (i = 0; i < SQRT_TAB_SIZE-1; i++)
	{
		printf("0x%x,", sqrt_tab[i]);
		nl++;
		if (nl > 8) 
		{ 
			nl = 0; 
			putchar('\n'); 
		}
	}
	printf("0x%x\n", SqrtTab[SQRT_TAB_SIZE-1]);
	printf("};\n");
}
#endif
