/****************************************************************************************/
/*  Frustum.c                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Frustum creation/clipping                                              */
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

#include "Camera.h"
#include "Frustum.h"
#include "Surface.h"

#include "Vec3d.h"

//#define RIGHT_HANDED

//=====================================================================================
//	Local Static Function prototypes
//=====================================================================================
static void SetWorldspaceClipPlane(const GFX_Plane *In, const geCamera *Camera, GFX_Plane *Out);
static void BackRotateVector(const geVec3d *In, geVec3d *Out, const geXForm3d *XForm);
static void SetUpFrustumBBox(Frustum_Info *Info);
int Frustum_Seed1=1768710981;
int Frustum_Seed2=560296816;
//================================================================================
//	Frustum_SetFromCamera
//================================================================================
void Frustum_SetFromCamera(Frustum_Info *Info, geCamera *Camera)
{
    geFloat		s, c, ZFar;
	geBoolean	ZFarEnable;
    geVec3d		Normal;
	int32		i;

    geCamera_GetViewAngleXSinCos(Camera,&s,&c);

    // Left clip plane
    Normal.X = s;
    Normal.Y = 0.0f;
    Normal.Z = -c;
	geVec3d_Normalize(&Normal);
	Info->Planes[0].Normal = Normal;

    // Right clip plane
    Normal.X = -s;
	geVec3d_Normalize(&Normal);
	Info->Planes[1].Normal = Normal;

    geCamera_GetViewAngleYSinCos(Camera,&s,&c);

    // Bottom clip plane
    Normal.X = 0.0f;
    Normal.Y = s;
    Normal.Z = -c;
	geVec3d_Normalize(&Normal);
	Info->Planes[2].Normal = Normal;

    // Top clip plane
    Normal.Y = -s;
	geVec3d_Normalize(&Normal);
	Info->Planes[3].Normal = Normal;

	Info->NumPlanes = 4;

	// Clear all distances
	for (i=0; i<Info->NumPlanes; i++)
	{
		Info->Planes[i].Dist = 0.0f;
		Info->Planes[i].Type = PLANE_ANY;
	}

    // Check to see if we need to use a far clip plane
	geCamera_GetFarClipPlane(Camera, &ZFarEnable, &ZFar);

	if (ZFarEnable)
	{
		geFloat		ZScale;

		ZScale = geCamera_GetZScale(Camera);

		// Far clip plane
		Normal.X = 0.0f;
		Normal.Y = 0.0f;
		Normal.Z = 1.0f;
		geVec3d_Normalize(&Normal);
		Info->Planes[4].Normal = Normal;

		Info->Planes[4].Dist = -(ZFar/ZScale);
		Info->Planes[4].Type = PLANE_ANY;

		Info->NumPlanes = 5;
	}

	// Get BBox info for fast BBox rejection against frustum...
	SetUpFrustumBBox(Info);
}

//================================================================================
//	Frustum_SetFromPoly
//	Create a frustum looking through a poly (from the origin)
//================================================================================
geBoolean Frustum_SetFromPoly(Frustum_Info *Info, geVec3d *Verts, int32 NumVerts, geBoolean Flip)
{
	int32		NextVert;
	geVec3d		*pVert1, *pVert2, Vect;
	GFX_Plane	*Planes;
	int32		i;

	if (NumVerts >= MAX_FCP)
		return GE_FALSE;		// Too many planes!!!
	
	Planes = Info->Planes;

	Info->NumPlanes = 0;

	for (i=0; i< NumVerts; i++)
	{
		NextVert = ((i+1) < NumVerts) ? (i+1) : 0;

		pVert1 = &Verts[i];
		pVert2 = &Verts[NextVert];

		if (geVec3d_Compare(pVert1, pVert2, 0.1f))	// Coplanar edge...
			continue;	// Coplanar edges will cause a plane to be duplicated, skip it or it will screw up 
						// the clipping stage of the frustum created from this poly...

		geVec3d_Subtract(pVert1, pVert2, &Vect);
		
		if (Flip)
			geVec3d_CrossProduct(pVert2, &Vect, &Planes->Normal);
		else
			geVec3d_CrossProduct(&Vect, pVert2, &Planes->Normal);

		geVec3d_Normalize(&Planes->Normal);

		Planes->Dist = 0.0f;
		Planes->Type = PLANE_ANY;

		Planes++;
		Info->NumPlanes++;
	}

	// Get BBox info for fast BBox rejection against frustum...
	SetUpFrustumBBox(Info);
return GE_TRUE;
}

//================================================================================
//	Frustum_RotateToWorldSpace
//================================================================================
void Frustum_RotateToWorldSpace(Frustum_Info *In, geCamera *Camera, Frustum_Info *Out)
{
    int32		i;
	GFX_Plane	*pPlane1, *pPlane2;

	assert(In != Out);

	pPlane1 = In->Planes;
	pPlane2 = Out->Planes;

	// Rotate all the planes
	for (i=0; i<In->NumPlanes; i++, pPlane1++, pPlane2++)
	{
		pPlane2->Type = pPlane1->Type;
		
		SetWorldspaceClipPlane(pPlane1, Camera, pPlane2);
		pPlane2->Dist = 0.0f;		// We are just rotating, so set dist to 0

		if (pPlane1->Dist)		// Add the original dist back in
		{
			geVec3d		Vect;

			geVec3d_Clear(&Vect);
			geVec3d_AddScaled(&Vect, &pPlane1->Normal, pPlane1->Dist, &Vect);

			BackRotateVector(&Vect, &Vect, geCamera_GetCameraSpaceXForm(Camera));

			pPlane2->Dist += geVec3d_DotProduct(&pPlane2->Normal, &Vect);
		}
	}

	Out->NumPlanes = In->NumPlanes;

	// Get BBox info for fast BBox rejection against frustum...
	SetUpFrustumBBox(Out);
}

//================================================================================
//	Frustum_TransformToWorldSpace
//================================================================================
void Frustum_TransformToWorldSpace(const Frustum_Info *In, const geCamera *Camera, Frustum_Info *Out)
{
    int32		i;
	GFX_Plane	*pPlane1, *pPlane2;

	assert(In != Out);

	pPlane1 = (GFX_Plane*)In->Planes;
	pPlane2 = Out->Planes;

	// Rotate all the planes
	for (i=0; i<In->NumPlanes; i++, pPlane1++, pPlane2++)
	{
		pPlane2->Type = pPlane1->Type;
		
		SetWorldspaceClipPlane(pPlane1, Camera, pPlane2);
		pPlane2->Dist = geVec3d_DotProduct(geCamera_GetPov(Camera), &pPlane2->Normal) - CLIP_PLANE_EPSILON;

		if (pPlane1->Dist)		// Add the original dist back in
		{
			geVec3d		Vect;

			geVec3d_Clear(&Vect);
			geVec3d_AddScaled(&Vect, &pPlane1->Normal, pPlane1->Dist, &Vect);

			BackRotateVector(&Vect, &Vect, geCamera_GetCameraSpaceXForm(Camera));

			pPlane2->Dist += geVec3d_DotProduct(&pPlane2->Normal, &Vect);
		}
	}
	Out->NumPlanes = In->NumPlanes;

	// Get BBox info for fast BBox rejection against frustum...
	SetUpFrustumBBox(Out);
}

//================================================================================
//	SetWorldSpaceClipPlane
//================================================================================
static void SetWorldspaceClipPlane(const GFX_Plane *In, const geCamera *Camera, GFX_Plane *Out)
{
	// Rotate the plane normal into worldspace
    BackRotateVector(&In->Normal, &Out->Normal, geCamera_GetCameraSpaceXForm(Camera));
}

//================================================================================
//	BackRotateVector
//	Rotate a vector from viewspace to worldspace.
//================================================================================
static void BackRotateVector(const geVec3d *In, geVec3d *Out, const geXForm3d *XForm)
{
    geVec3d	VRight, VUp, VIn, InCopy;

	InCopy = *In;
	
	//	Get the 3 vectors that make up the Xform axis 
	VRight.X = XForm->AX; VRight.Y = XForm->AY; VRight.Z = XForm->AZ;
	VUp.X    = XForm->BX; VUp.Y    = XForm->BY; VUp.Z    = XForm->BZ;
	VIn.X    = XForm->CX; VIn.Y    = XForm->CY; VIn.Z    = XForm->CZ;

    Out->X = (InCopy.X * VRight.X) + (InCopy.Y * VUp.X) + (InCopy.Z * VIn.X);
    Out->Y = (InCopy.X * VRight.Y) + (InCopy.Y * VUp.Y) + (InCopy.Z * VIn.Y);
    Out->Z = (InCopy.X * VRight.Z) + (InCopy.Y * VUp.Z) + (InCopy.Z * VIn.Z);
}

//================================================================================
//	SetUpFrustumBBox
//	Setup bbox min/max test for the quadrant the frustum planes are in...
//================================================================================
static void SetUpFrustumBBox(Frustum_Info *Info)
{
	int32		i, *Index;

	Index = Info->FrustumBBoxIndexes;

	for (i=0 ; i<Info->NumPlanes ; i++)
	{
		if (Info->Planes[i].Normal.X < 0)
		{
			Index[0] = 0;
			Index[3] = 3;
		}
		else
		{
			Index[0] = 3;
			Index[3] = 0;
		}
		if (Info->Planes[i].Normal.Y < 0)
		{
			Index[1] = 1;
			Index[4] = 4;
		}
		else
		{
			Index[1] = 4;
			Index[4] = 1;
		}
		if (Info->Planes[i].Normal.Z < 0)
		{
			Index[2] = 2;
			Index[5] = 5;
		}
		else
		{
			Index[2] = 5;
			Index[5] = 2;
		}

		Info->pFrustumBBoxIndexes[i] = Index;
		Index += 6;
	}
}

//================================================================================
//	gePlane_ClipVertsFanned
//	Clips and adds fanned verts (does not fan though the poly out though...)
//================================================================================
geBoolean gePlane_ClipVertsFanned(	const geVec3d *In, int32 NumIn, 
									const GFX_Plane *Plane, 
									geVec3d *Out, int32 *NumOut)
{
	#define MAX_VERT		128

	geVec3d		*pIn, *pOut, *pFirst;
	int32		i, Count[2], NextVert;
	float		Dist[MAX_VERT];
	int32		CurIn, NextIn, FirstIn;
	float		CurDist, FirstDist, PlaneDist, NextDist;

	assert(NumIn < MAX_VERT);
	
	PlaneDist = Plane->Dist;

	pIn = (geVec3d*)In;

	Count[0] = Count[1] = 0;	// Clear front back list

	// First, get all the dist of the verts from the plane
	for (i=0; i< NumIn; i++, pIn++)
	{
		Dist[i] = geVec3d_DotProduct(&Plane->Normal, pIn) - PlaneDist;

		if (Dist[i] >= 0.0f)
			Count[0]++;			// Front side
		else
			Count[1]++;			// Back side
	}

	if (!Count[0])
		return GE_FALSE;		// Poly totally clipped away
	else if (!Count[1])	
	{
		// Poly was totally in the ViewFrustum so...
		// Copy the poly to the out list
		memcpy((void*)In, Out, sizeof(geVec3d)*NumIn);	
		*NumOut = NumIn;
		return GE_TRUE;
	}

    FirstDist = Dist[0];
	FirstIn = (FirstDist >= 0.0f);		// Save the first one off for when we are fanning...

	CurDist = FirstDist;				// Save first as current
	CurIn = FirstIn;

	pFirst = pIn = (geVec3d*)In;
	pOut = Out;

	// The poly needs to be clipped...
	for (i=0; i< NumIn; i++, pIn++)
	{
		geVec3d	*pNext;

		NextVert = ((i+1)<NumIn) ? (i+1): 0;

		if (CurIn)
			*pOut++ = *pIn;

		pNext = (geVec3d*)&In[NextVert];

		NextDist = Dist[NextVert];
		NextIn = (NextDist >= 0.0f);

		// First clip the edges on the poly
		if (CurIn != NextIn)
		{
			float	Scale;

			Scale = (PlaneDist - CurDist) / (NextDist - CurDist);

            pOut->X = pIn->X + (pNext->X - pIn->X) * Scale;
            pOut->Y = pIn->Y + (pNext->Y - pIn->Y) * Scale;
            pOut->Z = pIn->Z + (pNext->Z - pIn->Z) * Scale;

            pOut++;
        }

		// Start clipping the fanned edges after we get past the first edge
		if (NextVert >= 2)
		{
			if (FirstIn != NextIn)		// The fanned edge crosses the plane, clip it, and store it in the out list
			{
				float	Scale;

				Scale = (Plane->Dist - FirstDist) / (NextDist - FirstDist);

				pOut->X = pFirst->X + (pNext->X - pFirst->X) * Scale;
				pOut->Y = pFirst->Y + (pNext->Y - pFirst->Y) * Scale;
				pOut->Z = pFirst->Z + (pNext->Z - pFirst->Z) * Scale;

				pOut++;
			}
		}

        CurIn = NextIn;
		CurDist = NextDist;
	}

	*NumOut = (pOut - Out);

	return (*NumOut >= 3);
}

//================================================================================
//	gePlane_ClipVertsFanned
//	Clips and adds fanned verts (does not fan though the poly out though...)
//================================================================================
geBoolean gePlane_ClipVertsFannedUVRGB(	const geVec3d *In, const Surf_TexVert *TIn, int32 NumIn, 
										const GFX_Plane *Plane, 
										geVec3d *Out, Surf_TexVert *TOut, int32 *NumOut)
{
	#define MAX_VERT		128

	geVec3d			*pIn, *pOut, *pFirst;
	Surf_TexVert	*pTIn, *pTOut, *pTFirst;
	int32			i, Count[2], FirstVert, NextVert;
	float			Dist[MAX_VERT];
	int32			CurIn, NextIn, FirstIn;
	float			CurDist, FirstDist, PlaneDist, NextDist;
	int32			Temp;

	assert(NumIn < MAX_VERT);
	
	PlaneDist = Plane->Dist;

	pIn = (geVec3d*)In;

	Count[0] = Count[1] = 0;	// Clear front back list

	// First, get all the dist of the verts from the plane
	for (i=0; i< NumIn; i++, pIn++)
	{
		Dist[i] = geVec3d_DotProduct(&Plane->Normal, pIn);
		
		if (Dist[i] >= PlaneDist)
		{
			Count[0]++;			// Front side
			FirstVert = i;		// The first vert MUST be inside the frustum
		}
		else
			Count[1]++;			// Back side
		
	}

	if (!Count[0])				// Nothing on the front
		return GE_FALSE;		
	else if (!Count[1])			// Nothing on the back
	{
		// Poly was totally in the ViewFrustum so...
		// Copy the poly to the out list
		memcpy(Out, In, sizeof(geVec3d)*NumIn);	
		memcpy(TOut, TIn, sizeof(Surf_TexVert)*NumIn);	
		*NumOut = NumIn;
		return GE_TRUE;
	}

    FirstDist = Dist[FirstVert];
	FirstIn = (FirstDist >= PlaneDist);	// Save the first one off for when we are fanning...

	CurDist = FirstDist;				// Save first as current
	CurIn = FirstIn;

	pFirst = pIn = (geVec3d*)&In[FirstVert];
	pTFirst = pTIn = (Surf_TexVert*)&TIn[FirstVert];

	pOut = Out;
	pTOut = TOut;

	Temp = 0;

	// The poly needs to be clipped...
	for (i=FirstVert; i< FirstVert+NumIn; i++, Temp++)
	{
		geVec3d			*pNext;
		Surf_TexVert	*pTNext;
		int32			ThisVert;

		ThisVert = i%NumIn;
		NextVert = (i+1)%NumIn;

		pIn = (geVec3d*)&In[ThisVert];
		pTIn = (Surf_TexVert*)&TIn[ThisVert];

		if (CurIn)
		{
			*pOut++ = *pIn;
			*pTOut++ = *pTIn;
		}

		pNext = (geVec3d*)&In[NextVert];
		pTNext = (Surf_TexVert*)&TIn[NextVert];

		NextDist = Dist[NextVert];
		NextIn = (NextDist >= PlaneDist);

		// First clip the edges on the poly
		if (CurIn != NextIn)
		{
			float	Scale;

			Scale = (PlaneDist - CurDist) / (NextDist - CurDist);

            pOut->X = pIn->X + (pNext->X - pIn->X) * Scale;
            pOut->Y = pIn->Y + (pNext->Y - pIn->Y) * Scale;
            pOut->Z = pIn->Z + (pNext->Z - pIn->Z) * Scale;

			pTOut->u = pTIn->u + (pTNext->u - pTIn->u) * Scale;
			pTOut->v = pTIn->v + (pTNext->v - pTIn->v) * Scale;

			pTOut->r = pTIn->r + (pTNext->r - pTIn->r) * Scale;
			pTOut->g = pTIn->g + (pTNext->g - pTIn->g) * Scale;
			pTOut->b = pTIn->b + (pTNext->b - pTIn->b) * Scale;

            pOut++;
			pTOut++;
        }
		
		// Start clipping the fanned edges after we get past the first edge
		if (Temp >= 1 && Temp+2 < NumIn)
		{
			if (FirstIn != NextIn)		// The fanned edge crosses the plane, clip it, and store it in the out list
			{
				float	Scale;

				Scale = (PlaneDist - FirstDist) / (NextDist - FirstDist);

				pOut->X = pFirst->X + (pNext->X - pFirst->X) * Scale;
				pOut->Y = pFirst->Y + (pNext->Y - pFirst->Y) * Scale;
				pOut->Z = pFirst->Z + (pNext->Z - pFirst->Z) * Scale;

				pTOut->u = pTFirst->u + (pTNext->u - pTFirst->u) * Scale;
				pTOut->v = pTFirst->v + (pTNext->v - pTFirst->v) * Scale;

				pTOut->r = pTFirst->r + (pTNext->r - pTFirst->r) * Scale;
				pTOut->g = pTFirst->g + (pTNext->g - pTFirst->g) * Scale;
				pTOut->b = pTFirst->b + (pTNext->b - pTFirst->b) * Scale;

				pOut++;
				pTOut++;
			}
		}
		
        CurIn = NextIn;
		CurDist = NextDist;
	}

	*NumOut = (pOut - Out);

	return (*NumOut >= 3);
}

//================================================================================
//	Frustum_ClipToPlane
//	Clips X, Y only
//================================================================================
geBoolean Frustum_ClipToPlane(	GFX_Plane *pPlane, 
								geVec3d *pIn, geVec3d *pOut,
								int32 NumVerts, int32 *OutVerts)
{
    int32	i, NextVert, CurIn, NextIn;
    float	CurDot, NextDot, Scale;
    geVec3d	*pInVert, *pOutVert, *pNext;
	geVec3d	*pNormal;

    pNormal = &pPlane->Normal;
	pInVert = pIn;
    pOutVert = pOut;

	CurDot = (pInVert->X * pNormal->X) + (pInVert->Y * pNormal->Y) + (pInVert->Z * pNormal->Z);
    CurIn = (CurDot >= pPlane->Dist);

    for (i=0 ; i<NumVerts ; i++)
    {
        NextVert = (i + 1);
		if (NextVert == NumVerts)
			NextVert = 0;

        // Keep the current vertex if it's inside the plane
        if (CurIn) 
            *pOutVert++ = *pInVert;

		pNext = &pIn[NextVert];
		
		NextDot = (pNext->X * pNormal->X) + (pNext->Y * pNormal->Y) + (pNext->Z * pNormal->Z);
		NextIn = (NextDot >= pPlane->Dist);

        // Add a clipped vertex if one end of the current edge is
        // inside the plane and the other is outside
        if (CurIn != NextIn)
        {
			Scale = (pPlane->Dist - CurDot) / (NextDot - CurDot);

            pOutVert->X = pInVert->X + (pNext->X - pInVert->X) * Scale;
            pOutVert->Y = pInVert->Y + (pNext->Y - pInVert->Y) * Scale;
            pOutVert->Z = pInVert->Z + (pNext->Z - pInVert->Z) * Scale;

            pOutVert++;
        }

        CurDot = NextDot;
        CurIn = NextIn;
        pInVert++;
    }

    *OutVerts = pOutVert - pOut;

    if (*OutVerts < 3)
        return GE_FALSE;

    return GE_TRUE;
}

//================================================================================
//	Frustum_ClipToPlaneUV
//	Clips X, Y, u, v
//================================================================================
geBoolean Frustum_ClipToPlaneUV(	GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts)
{
    int32		i, NextVert, CurIn, NextIn;
    float		CurDot, NextDot, Scale;
    geVec3d		*pInVert, *pOutVert, *pNext;
    Surf_TexVert *pTInVert, *pTOutVert, *pTNext;
	geVec3d		*pNormal;

    pNormal = &pPlane->Normal;
	pInVert = pIn;
    pOutVert = pOut;
	pTInVert = pTIn;
    pTOutVert = pTOut;

	CurDot = (pInVert->X * pNormal->X) + (pInVert->Y * pNormal->Y) + (pInVert->Z * pNormal->Z);
    CurIn = (CurDot >= pPlane->Dist);

    for (i=0 ; i<NumVerts ; i++)
    {
        NextVert = (i + 1);
		if (NextVert == NumVerts)
			NextVert = 0;

        // Keep the current vertex if it's inside the plane
        if (CurIn) 
		{
            *pOutVert++ = *pInVert;
            // VTune reports this entire copy of this structure is taking alot of time (JP)
			//*pTOutVert++ = *pTInVert;
			pTOutVert->u = pTInVert->u;
			pTOutVert->v = pTInVert->v;
			pTOutVert++;
		}

		pNext = &pIn[NextVert];
		pTNext = &pTIn[NextVert];
		
		NextDot = (pNext->X * pNormal->X) + (pNext->Y * pNormal->Y) + (pNext->Z * pNormal->Z);
		NextIn = (NextDot >= pPlane->Dist);

        // Add a clipped vertex if one end of the current edge is
        // inside the plane and the other is outside
        if (CurIn != NextIn)
        {
			Scale = (pPlane->Dist - CurDot) / (NextDot - CurDot);

            pOutVert->X = pInVert->X + (pNext->X - pInVert->X) * Scale;
            pOutVert->Y = pInVert->Y + (pNext->Y - pInVert->Y) * Scale;
            pOutVert->Z = pInVert->Z + (pNext->Z - pInVert->Z) * Scale;

            pTOutVert->u = pTInVert->u + (pTNext->u - pTInVert->u) * Scale;
            pTOutVert->v = pTInVert->v + (pTNext->v - pTInVert->v) * Scale;

            pOutVert++;
            pTOutVert++;
        }

        CurDot = NextDot;
        CurIn = NextIn;
        pInVert++;
        pTInVert++;
    }

    *OutVerts = pOutVert - pOut;

    if (*OutVerts < 3)
        return GE_FALSE;

    return GE_TRUE;
}

//================================================================================
//	Frustum_ClipToPlaneUVTGB
//	Clips X, Y, u, v, r, g, b
//================================================================================
geBoolean Frustum_ClipToPlaneUVRGB(GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts)
{
    int32		i, NextVert, CurIn, NextIn;
    float		CurDot, NextDot, Scale;
    geVec3d		*pInVert, *pOutVert, *pNext;
    Surf_TexVert *pTInVert, *pTOutVert, *pTNext;
	geVec3d		*pNormal;

    pNormal = &pPlane->Normal;
	pInVert = pIn;
    pOutVert = pOut;
	pTInVert = pTIn;
    pTOutVert = pTOut;

	CurDot = (pInVert->X * pNormal->X) + (pInVert->Y * pNormal->Y) + (pInVert->Z * pNormal->Z);
    CurIn = (CurDot >= pPlane->Dist);

    for (i=0 ; i<NumVerts ; i++)
    {
        NextVert = (i + 1);
		if (NextVert == NumVerts)
			NextVert = 0;

        // Keep the current vertex if it's inside the plane
        if (CurIn) 
		{
            *pOutVert++ = *pInVert;
            *pTOutVert++ = *pTInVert;
		}

		pNext = &pIn[NextVert];
		pTNext = &pTIn[NextVert];
		
		NextDot = (pNext->X * pNormal->X) + (pNext->Y * pNormal->Y) + (pNext->Z * pNormal->Z);
		NextIn = (NextDot >= pPlane->Dist);

        // Add a clipped vertex if one end of the current edge is
        // inside the plane and the other is outside
        if (CurIn != NextIn)
        {
			Scale = (pPlane->Dist - CurDot) / (NextDot - CurDot);

            pOutVert->X = pInVert->X + (pNext->X - pInVert->X) * Scale;
            pOutVert->Y = pInVert->Y + (pNext->Y - pInVert->Y) * Scale;
            pOutVert->Z = pInVert->Z + (pNext->Z - pInVert->Z) * Scale;

            pTOutVert->u = pTInVert->u + (pTNext->u - pTInVert->u) * Scale;
            pTOutVert->v = pTInVert->v + (pTNext->v - pTInVert->v) * Scale;

            pTOutVert->r = pTInVert->r + (pTNext->r - pTInVert->r) * Scale;
            pTOutVert->g = pTInVert->g + (pTNext->g - pTInVert->g) * Scale;
            pTOutVert->b = pTInVert->b + (pTNext->b - pTInVert->b) * Scale;

            pOutVert++;
            pTOutVert++;
        }

        CurDot = NextDot;
        CurIn = NextIn;
        pInVert++;
        pTInVert++;
    }

    *OutVerts = pOutVert - pOut;

    if (*OutVerts < 3)
        return GE_FALSE;

    return GE_TRUE;
}

//================================================================================
//	Frustum_ClipToPlaneUVTGB
//	Clips X, Y, u, v, r, g, b, a
//================================================================================
geBoolean Frustum_ClipToPlaneUVRGBA(GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts)
{
    int32		i, NextVert, CurIn, NextIn;
    float		CurDot, NextDot, Scale;
    geVec3d		*pInVert, *pOutVert, *pNext;
    Surf_TexVert *pTInVert, *pTOutVert, *pTNext;
	geVec3d		*pNormal;

    pNormal = &pPlane->Normal;
	pInVert = pIn;
    pOutVert = pOut;
	pTInVert = pTIn;
    pTOutVert = pTOut;

	CurDot = (pInVert->X * pNormal->X) + (pInVert->Y * pNormal->Y) + (pInVert->Z * pNormal->Z);
    CurIn = (CurDot >= pPlane->Dist);

    for (i=0 ; i<NumVerts ; i++)
    {
        NextVert = (i + 1);
		if (NextVert == NumVerts)
			NextVert = 0;

        // Keep the current vertex if it's inside the plane
        if (CurIn) 
		{
            *pOutVert++ = *pInVert;
            *pTOutVert++ = *pTInVert;
		}

		pNext = &pIn[NextVert];
		pTNext = &pTIn[NextVert];
		
		NextDot = (pNext->X * pNormal->X) + (pNext->Y * pNormal->Y) + (pNext->Z * pNormal->Z);
		NextIn = (NextDot >= pPlane->Dist);

        // Add a clipped vertex if one end of the current edge is
        // inside the plane and the other is outside
        if (CurIn != NextIn)
        {
			Scale = (pPlane->Dist - CurDot) / (NextDot - CurDot);

            pOutVert->X = pInVert->X + (pNext->X - pInVert->X) * Scale;
            pOutVert->Y = pInVert->Y + (pNext->Y - pInVert->Y) * Scale;
            pOutVert->Z = pInVert->Z + (pNext->Z - pInVert->Z) * Scale;

            pTOutVert->u = pTInVert->u + (pTNext->u - pTInVert->u) * Scale;
            pTOutVert->v = pTInVert->v + (pTNext->v - pTInVert->v) * Scale;

            pTOutVert->r = pTInVert->r + (pTNext->r - pTInVert->r) * Scale;
            pTOutVert->g = pTInVert->g + (pTNext->g - pTInVert->g) * Scale;
            pTOutVert->b = pTInVert->b + (pTNext->b - pTInVert->b) * Scale;
            pTOutVert->a = pTInVert->a + (pTNext->a - pTInVert->a) * Scale;

            pOutVert++;
            pTOutVert++;
        }

        CurDot = NextDot;
        CurIn = NextIn;
        pInVert++;
        pTInVert++;
    }

    *OutVerts = pOutVert - pOut;

    if (*OutVerts < 3)
        return GE_FALSE;

    return GE_TRUE;
}

//================================================================================
//	Frustum_ClipToPlaneRGB
//	Clips X, Y, r, g, b
//================================================================================
geBoolean Frustum_ClipToPlaneRGB(	GFX_Plane *pPlane, 
									geVec3d *pIn, geVec3d *pOut,
									Surf_TexVert *pTIn, Surf_TexVert *pTOut,
									int32 NumVerts, int32 *OutVerts)
{
    int32		i, NextVert, CurIn, NextIn;
    float		CurDot, NextDot, Scale;
    geVec3d		*pInVert, *pOutVert, *pNext;
    Surf_TexVert *pTInVert, *pTOutVert, *pTNext;
	geVec3d		*pNormal;

    pNormal = &pPlane->Normal;
	pInVert = pIn;
    pOutVert = pOut;
	pTInVert = pTIn;
    pTOutVert = pTOut;

	CurDot = (pInVert->X * pNormal->X) + (pInVert->Y * pNormal->Y) + (pInVert->Z * pNormal->Z);
    CurIn = (CurDot >= pPlane->Dist);

    for (i=0 ; i<NumVerts ; i++)
    {
        NextVert = (i + 1);
		if (NextVert == NumVerts)
			NextVert = 0;

        // Keep the current vertex if it's inside the plane
        if (CurIn) 
		{
            *pOutVert++ = *pInVert;
            *pTOutVert++ = *pTInVert;
		}

		pNext = &pIn[NextVert];
		pTNext = &pTIn[NextVert];
		
		NextDot = (pNext->X * pNormal->X) + (pNext->Y * pNormal->Y) + (pNext->Z * pNormal->Z);
		NextIn = (NextDot >= pPlane->Dist);

        // Add a clipped vertex if one end of the current edge is
        // inside the plane and the other is outside
        if (CurIn != NextIn)
        {
			Scale = (pPlane->Dist - CurDot) / (NextDot - CurDot);

            pOutVert->X = pInVert->X + (pNext->X - pInVert->X) * Scale;
            pOutVert->Y = pInVert->Y + (pNext->Y - pInVert->Y) * Scale;
            pOutVert->Z = pInVert->Z + (pNext->Z - pInVert->Z) * Scale;

            pTOutVert->r = pTInVert->r + (pTNext->r - pTInVert->r) * Scale;
            pTOutVert->g = pTInVert->g + (pTNext->g - pTInVert->g) * Scale;
            pTOutVert->b = pTInVert->b + (pTNext->b - pTInVert->b) * Scale;

            pOutVert++;
            pTOutVert++;
        }

        CurDot = NextDot;
        CurIn = NextIn;
        pInVert++;
        pTInVert++;
    }

    *OutVerts = pOutVert - pOut;

    if (*OutVerts < 3)
        return GE_FALSE;

    return GE_TRUE;
}

//================================================================================
//	Frustum_Project
//================================================================================
void Frustum_Project(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera)
{  
	int32			i;
	geVec3d			Out;

	assert( pIn    != NULL);
	assert( pTIn   != NULL);
	assert( pOut   != NULL);  
	assert( Camera != NULL);

	for (i=0; i<NumVerts; i++)
	{
		geCamera_ProjectAndClamp( Camera, &pIn[i], &Out );
		pOut->x = Out.X;
		pOut->y = Out.Y;
		pOut->z = Out.Z;
    	pOut->u = pTIn->u;  
		pOut->v = pTIn->v; 

		pOut++;
		pTIn++;
	}
}

//================================================================================
//	Frustum_Project
//================================================================================
void Frustum_ProjectRGB(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera)
{  
	int32	i;
	geVec3d Out;

	assert( pIn    != NULL);
	assert( pTIn   != NULL);
	assert( pOut   != NULL);  
	assert( Camera != NULL);

	for (i=0; i<NumVerts; i++)
	{
		geCamera_ProjectAndClamp( Camera, &pIn[i], &Out );
		pOut->x = Out.X;
		pOut->y = Out.Y;
		pOut->z = Out.Z;

    	pOut->u = pTIn->u;  
		pOut->v = pTIn->v; 
    	pOut->r = pTIn->r;  
		pOut->g = pTIn->g; 
		pOut->b = pTIn->b; 

		pOut++;
		pTIn++;
	}
}

//================================================================================
//	Frustum_Project
//================================================================================
void Frustum_ProjectRGBA(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera)
{  
	int32	i;
	geVec3d Out;
	assert( pIn    != NULL);
	assert( pTIn   != NULL);
	assert( pOut   != NULL);  
	assert( Camera != NULL);

	for (i=0; i<NumVerts; i++)
	{
		geCamera_ProjectAndClamp( Camera, &pIn[i], &Out );
		pOut->x = Out.X;
		pOut->y = Out.Y;
		pOut->z = Out.Z;
		pOut->u = pTIn->u;  
		pOut->v = pTIn->v; 
    	pOut->r = pTIn->r;  
		pOut->g = pTIn->g; 
		pOut->b = pTIn->b; 
		pOut->a = pTIn->a; 

		pOut++;
		pTIn++;
	}
}

//================================================================================
//	Frustum_Project
//================================================================================
void Frustum_ProjectRGBNoClamp(geVec3d *pIn, Surf_TexVert *pTIn, DRV_TLVertex *pOut, int32 NumVerts, const geCamera *Camera)
{  
	int32	i;
	geVec3d Out;
	assert( pIn    != NULL);
	assert( pTIn   != NULL);
	assert( pOut   != NULL);  
	assert( Camera != NULL);

	for (i=0; i<NumVerts; i++)
	{
		geCamera_Project( Camera, &pIn[i], &Out );
		pOut->x = Out.X;
		pOut->y = Out.Y;
		pOut->z = Out.Z;
	
    	pOut->u = pTIn->u;  
		pOut->v = pTIn->v; 
    	pOut->r = pTIn->r;  
		pOut->g = pTIn->g; 
		pOut->b = pTIn->b; 

		pOut++;
		pTIn++;
	}
}

//================================================================================
//	Frustum_PointsInFrustum
//================================================================================
geBoolean Frustum_PointsInFrustum(const geVec3d *Pin, const GFX_Plane *Plane, int32 NumVerts, int32 *c)
{
    int32	Count, i;

	Count = 0;

	for (i=0; i< NumVerts; i++)
	{
		if (geVec3d_DotProduct(Pin, &Plane->Normal) >= Plane->Dist)
			Count++;

		Pin++;
	}

	*c += Count;

	return Count;
}

//================================================================================
//	Frustum_PointInFrustum
//================================================================================
geBoolean Frustum_PointInFrustum(const Frustum_Info *Fi, const geVec3d *Point, float Radius)
{
	int32			i;
	const GFX_Plane	*Plane;
	float			Dist;

	Plane = Fi->Planes;

	for (i=0; i< Fi->NumPlanes; i++, Plane++)
	{
		Dist = geVec3d_DotProduct(Point, &Plane->Normal) - Plane->Dist;

		Dist += Radius;

		if (Dist < 0)			// Behind plane
			return GE_FALSE;

	}

	return GE_TRUE;
}

//================================================================================
//	Frustum_ClipAllPlanesL	(CB added)
//================================================================================
geBoolean Frustum_ClipAllPlanesL(const Frustum_Info * Fi,uint32 ClipFlags,GE_LVertex *Verts, int32 *pNumVerts)
{
uint32 mask;
GE_LVertex WorkVerts[32];
GE_LVertex *p1,*p2,*p3;
GFX_Plane * FPlane;

	p1 = Verts;
	p2 = WorkVerts;
	
	FPlane = (GFX_Plane *)Fi->Planes;

	for(mask=1; mask <= ClipFlags; mask += mask, FPlane++)
	{
		if ( ! (ClipFlags & mask) )
			continue;

		if (!Frustum_ClipToPlaneL(FPlane, p1,p2, *pNumVerts,pNumVerts) )
			return GE_FALSE;

		if ( (*pNumVerts) < 3 || (*pNumVerts) > 30 )
			return GE_FALSE;

		p3 = p1;
		p1 = p2;
		p2 = p3;
	}

	if ( p1 != Verts )
	{
		memcpy(Verts,p1,sizeof(GE_LVertex)*(*pNumVerts));
	}

return GE_TRUE;
}

//================================================================================
//	Frustum_ClipToPlaneL	(CB added)
//================================================================================
geBoolean Frustum_ClipToPlaneL(GFX_Plane *pPlane, 
								GE_LVertex *pIn, GE_LVertex *pOut,
								int32 NumVerts, int32 *NumOutVerts)
{
    int32		i, NextVert, CurIn, NextIn;
    float		CurDot, NextDot, Scale;
    GE_LVertex	*pInVert, *pOutVert, *pNext;
	geVec3d		*pNormal;

    pNormal = &pPlane->Normal;
	pInVert = pIn;
    pOutVert = pOut;

	CurDot = (pInVert->X * pNormal->X) + (pInVert->Y * pNormal->Y) + (pInVert->Z * pNormal->Z);
    CurIn = (CurDot >= pPlane->Dist);

    for (i=0 ; i<NumVerts ; i++)
    {
        NextVert = (i + 1);
		if (NextVert == NumVerts)
			NextVert = 0;

        // Keep the current vertex if it's inside the plane
        if (CurIn) 
		{
            *pOutVert++ = *pInVert;
		}

		pNext = &pIn[NextVert];
		
		NextDot = (pNext->X * pNormal->X) + (pNext->Y * pNormal->Y) + (pNext->Z * pNormal->Z);
		NextIn = (NextDot >= pPlane->Dist);

        // Add a clipped vertex if one end of the current edge is
        // inside the plane and the other is outside
        if (CurIn != NextIn)
        {
			Scale = (pPlane->Dist - CurDot) / (NextDot - CurDot);

            pOutVert->X = pInVert->X + (pNext->X - pInVert->X) * Scale;
            pOutVert->Y = pInVert->Y + (pNext->Y - pInVert->Y) * Scale;
            pOutVert->Z = pInVert->Z + (pNext->Z - pInVert->Z) * Scale;

            pOutVert->u = pInVert->u + (pNext->u - pInVert->u) * Scale;
            pOutVert->v = pInVert->v + (pNext->v - pInVert->v) * Scale;

            pOutVert->r = pInVert->r + (pNext->r - pInVert->r) * Scale;
            pOutVert->g = pInVert->g + (pNext->g - pInVert->g) * Scale;
            pOutVert->b = pInVert->b + (pNext->b - pInVert->b) * Scale;
            pOutVert->a = pInVert->a + (pNext->a - pInVert->a) * Scale;

            pOutVert++;
        }

        CurDot = NextDot;
        CurIn = NextIn;
        pInVert++;
    }

    *NumOutVerts = ((uint32)pOutVert - (uint32)pOut)/sizeof(*pOut);

    if ( *NumOutVerts < 3)
        return GE_FALSE;

return GE_TRUE;
}

