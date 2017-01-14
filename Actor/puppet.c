/****************************************************************************************/
/*  PUPPET.C																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Puppet implementation.									.				*/
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
//#define CIRCULAR_SHADOW
#define SHADOW_MAP
//#define PROJECTED_SHADOW

#include <math.h>  //fabs()
#include <assert.h>

#include "light.h"
#include "world.h"
#include "trace.h"		//Trace_WorldCollisionExact2()
#include "surface.h"	// Surf_InSurfBoundingBox()

#include "xfarray.h"
#include "puppet.h"
#include "pose.h"
#include "ErrorLog.h"
#include "ram.h"
#include "tclip.h"

#include "Frustum.h"
#include "ExtBox.h"
#include "bodyinst.h"

#ifdef PROFILE
#include "rdtsc.h"
#endif

#include "bitmap.h"
#include "bitmap._h"

#define PUPPET_DEFAULT_MAX_DYNAMIC_LIGHTS 3

typedef struct gePuppet_Color
{
	geFloat				Red,Green,Blue;
} gePuppet_Color;

typedef struct gePuppet_Material
{
	gePuppet_Color		 Color;
	geBoolean			 UseTexture;
	geBitmap			*Bitmap;
	const char			*TextureName;
	const char			*AlphaName;
} gePuppet_Material;

typedef struct gePuppet
{
	geVFile *			 TextureFileContext;
	geBodyInst			*BodyInstance;
	int					 MaterialCount;
	gePuppet_Material	*MaterialArray;
	int					 MaxDynamicLightsToUse;
	int					 LightReferenceBoneIndex;
		
	geVec3d				 FillLightNormal;
	gePuppet_Color		 FillLightColor;			// 0..255
	geBoolean			 UseFillLight;				// use fill light normal
	
	gePuppet_Color		 AmbientLightIntensity;		// 0..1
	geBoolean			 AmbientLightFromFloor;		// use local lighting from floor

	geBoolean			 PerBoneLighting;

	geBoolean			 DoShadow;
	geFloat				 ShadowScale;
	const geBitmap		*ShadowMap;
	int					 ShadowBoneIndex;

		#pragma message ("this goes away!: World")
	geWorld				*World;
} gePuppet;

typedef struct
{
	geVec3d			Normal;
	gePuppet_Color	Color;
	geFloat			Distance;
	geFloat			Radius;
} gePuppet_Light;

typedef struct
{
	geBoolean		UseFillLight;
	geVec3d			FillLightNormal;
	gePuppet_Color	MaterialColor;
	gePuppet_Color  FillLightColor;
	gePuppet_Color	Ambient;
	geVec3d			SurfaceNormal;
	gePuppet_Light	Lights[MAX_DYNAMIC_LIGHTS];
	int				LightCount;
	geBoolean		PerBoneLighting;
} gePuppet_LightParamGroup;

typedef struct
{
	gePuppet_Light Lights[MAX_DYNAMIC_LIGHTS];
	int LightCount;
} gePuppet_BoneLight;

// Local info stored across multiple puppets to avoid resource waste.
gePuppet_LightParamGroup  gePuppet_StaticLightGrp;
gePuppet_BoneLight		 *gePuppet_StaticBoneLightArray=NULL;
int						  gePuppet_StaticBoneLightArraySize=0;
int						  gePuppet_StaticPuppetCount=0;
int						  gePuppet_StaticFlags[2]={1768710981,560296816};

static geBoolean GENESISCC gePuppet_FetchTextures(gePuppet *P, const geBody *B)
{
	int i;
	assert( P );
	
	P->MaterialCount = geBody_GetMaterialCount(B);
	if (P->MaterialCount <= 0)
	{
		return GE_TRUE;
	}
	
	P->MaterialArray = GE_RAM_ALLOCATE_ARRAY(gePuppet_Material, P->MaterialCount);
	if (P->MaterialArray == NULL)
	{
		geErrorLog_Add(ERR_PUPPET_ENOMEM, NULL);
		return GE_FALSE;
	}
	
	for (i=0; i<P->MaterialCount; i++)
	{
		const char *Name;
		geBitmap *Bitmap;
		gePuppet_Material *M = &(P->MaterialArray[i]);
		geBody_GetMaterial( B, i, &(Name), &(Bitmap),
						&(M->Color.Red),&(M->Color.Green),&(M->Color.Blue));

		if (Bitmap == NULL )
		{
			M->Bitmap     = NULL;
			M->UseTexture = GE_FALSE;
		}
		else
		{
			M->UseTexture = GE_TRUE;
			assert( P->World );

			M->Bitmap = Bitmap;
			geBitmap_CreateRef(Bitmap);

			if ( ! geWorld_AddBitmap(P->World,Bitmap) )
			{
				geErrorLog_AddString(-1,"Puppet_FetchTextures : World_AddBitmap", NULL);
				geRam_Free(P->MaterialArray);
				P->MaterialArray = NULL;
				P->MaterialCount = 0;
				return GE_FALSE;
			}
		}
	}

	return GE_TRUE;
}	

int GENESISCC gePuppet_GetMaterialCount( gePuppet *P )
{
	assert( P );
	return P->MaterialCount;
}

geBoolean GENESISCC gePuppet_GetMaterial( gePuppet *P, int MaterialIndex,
									geBitmap **Bitmap, 
									geFloat *Red, geFloat *Green, geFloat *Blue)
{
	assert( P      );
	assert( Red    );
	assert( Green  );
	assert( Blue   );
	assert( Bitmap );
	assert( MaterialIndex >= 0 );
	assert( MaterialIndex < P->MaterialCount );

	{
		gePuppet_Material *M = &(P->MaterialArray[MaterialIndex]);
		*Bitmap = M->Bitmap;
		*Red    = M->Color.Red;
		*Green  = M->Color.Green;
		*Blue   = M->Color.Blue;
	}
	return GE_TRUE;
}


geBoolean GENESISCC gePuppet_SetMaterial(gePuppet *P, int MaterialIndex, geBitmap *Bitmap, 
										geFloat Red, geFloat Green, geFloat Blue)
{
	assert( P );
	assert( MaterialIndex >= 0 );
	assert( MaterialIndex < P->MaterialCount );

	{
	geBitmap * OldBitmap;
	gePuppet_Material *M = P->MaterialArray + MaterialIndex;

		OldBitmap = M->Bitmap;

		M->Bitmap		= Bitmap;
		M->Color.Red    = Red;
		M->Color.Green  = Green;
		M->Color.Blue   = Blue;

		if ( OldBitmap != Bitmap ) 
		{		
			if ( OldBitmap )
			{
				assert( M->UseTexture );		
				geWorld_RemoveBitmap( P->World, OldBitmap );
				geBitmap_Destroy( &(OldBitmap) );
			}
			
			M->UseTexture = GE_FALSE;

			if ( Bitmap )
			{
				geBitmap_CreateRef(Bitmap);
						
				M->UseTexture = GE_TRUE;

				if ( ! geWorld_AddBitmap(P->World,Bitmap) )
				{
					geErrorLog_AddString(-1,"Puppet_SetMaterial : World_AddBitmap", NULL);
					return GE_FALSE;
				}
			}
		}
	}

	return GE_TRUE;
}
	

gePuppet *GENESISCC gePuppet_Create(geVFile *TextureFS, const geBody *B, geWorld *World)
{
	gePuppet *P;

	assert( geBody_IsValid(B)!=GE_FALSE );
	#pragma message ("Need geWorld_IsValid()")
	
	P = GE_RAM_ALLOCATE_STRUCT(gePuppet);
	if (P==NULL)
	{
		geErrorLog_Add(ERR_PUPPET_ENOMEM, NULL);
		return NULL;
	}
	
	memset(P,0,sizeof(*P));	

	P->BodyInstance = NULL;
	P->MaxDynamicLightsToUse = PUPPET_DEFAULT_MAX_DYNAMIC_LIGHTS;
	P->LightReferenceBoneIndex = GE_POSE_ROOT_JOINT;

	P->FillLightNormal.X = -0.2f;
	P->FillLightNormal.Y = 1.0f;
	P->FillLightNormal.Z = 0.4f;
	geVec3d_Normalize(&(P->FillLightNormal));
	P->FillLightColor.Red    = 0.25f;
	P->FillLightColor.Green  = 0.25f;
	P->FillLightColor.Blue   = 0.25f;
	P->UseFillLight = GE_TRUE;

	P->AmbientLightIntensity.Red   = 0.1f;
	P->AmbientLightIntensity.Green = 0.1f;
	P->AmbientLightIntensity.Blue  = 0.1f;
	P->AmbientLightFromFloor = GE_TRUE;

	P->DoShadow = GE_FALSE;
	P->ShadowScale = 0.0f;
	P->ShadowBoneIndex =  GE_POSE_ROOT_JOINT;
	P->TextureFileContext = TextureFS;

	P->World = World;
				
	if (gePuppet_FetchTextures(P,B)==GE_FALSE)
	{
		geRam_Free(P);
		return NULL;
	}

	P->BodyInstance = geBodyInst_Create(B);
	if (P->BodyInstance == NULL)
	{
		geErrorLog_Add(ERR_PUPPET_ENOMEM, NULL);
		gePuppet_Destroy( &P );
		return NULL;
	}
	gePuppet_StaticPuppetCount++;
	return P;
}


void GENESISCC gePuppet_Destroy(gePuppet **P)
{
	assert( P  );
	assert( *P );
	if ( (*P)->BodyInstance )
	{
		geBodyInst_Destroy( &((*P)->BodyInstance) );
		(*P)->BodyInstance = NULL;
	}
	if ( (*P)->MaterialArray )
	{
		gePuppet_Material *M;
		int i;

		for (i=0; i<(*P)->MaterialCount; i++)
		{
			M = &((*P)->MaterialArray[i]);
			if (M->UseTexture )
			{					
				assert( M->Bitmap );
				geWorld_RemoveBitmap( (*P)->World, M->Bitmap );
				geBitmap_Destroy( &(M->Bitmap) );
				M->UseTexture = GE_FALSE;
			}
		}


		geRam_Free( (*P)->MaterialArray );
		(*P)->BodyInstance = NULL;
	}
	if ( (*P)->ShadowMap )
	{
		geBitmap_Destroy((geBitmap **)&((*P)->ShadowMap));
		(*P)->ShadowMap = NULL;
	}

	geRam_Free( (*P) );
	*P = NULL;
	
	// clean up any shared resources.
	gePuppet_StaticPuppetCount--;
	if (gePuppet_StaticPuppetCount==0)
		{
			if (gePuppet_StaticBoneLightArray!=NULL)
				geRam_Free(gePuppet_StaticBoneLightArray);
			gePuppet_StaticBoneLightArray=NULL;
			gePuppet_StaticBoneLightArraySize = 0;
		}	
}


void GENESISCC gePuppet_GetLightingOptions(const gePuppet *P,
	geBoolean *UseFillLight,
	geVec3d *FillLightNormal,
	geFloat *FillLightRed,				
	geFloat *FillLightGreen,				
	geFloat *FillLightBlue,				
	geFloat *AmbientLightRed,			
	geFloat *AmbientLightGreen,			
	geFloat *AmbientLightBlue,			
	geBoolean *UseAmbientLightFromFloor,
	int *MaximumDynamicLightsToUse,		
	int *LightReferenceBoneIndex,
	geBoolean *PerBoneLighting
	)
{
	geFloat Scaler;
	assert( P != NULL);
	assert( UseFillLight );
	assert( FillLightNormal );
	assert( FillLightRed );	
	assert( FillLightGreen );	
	assert( FillLightBlue );	
	assert( AmbientLightRed );
	assert( AmbientLightGreen );			
	assert( AmbientLightBlue );			
	assert( UseAmbientLightFromFloor );
	assert( MaximumDynamicLightsToUse );	
	assert( LightReferenceBoneIndex );
		
	*UseFillLight = P->UseFillLight;

	*FillLightNormal = P->FillLightNormal;
	
	Scaler = 255.0f;
	*FillLightRed   = P->FillLightColor.Red * Scaler;
	*FillLightGreen = P->FillLightColor.Green * Scaler;
	*FillLightBlue  = P->FillLightColor.Blue * Scaler;
	
	*AmbientLightRed    = P->AmbientLightIntensity.Red * Scaler;
	*AmbientLightGreen  = P->AmbientLightIntensity.Green * Scaler;
	*AmbientLightBlue   = P->AmbientLightIntensity.Blue * Scaler;
	
	*UseAmbientLightFromFloor  = P->AmbientLightFromFloor;
	*MaximumDynamicLightsToUse = P->MaxDynamicLightsToUse;
	*LightReferenceBoneIndex   = P->LightReferenceBoneIndex;
	*PerBoneLighting		   = P->PerBoneLighting;
}	

void GENESISCC gePuppet_SetLightingOptions(gePuppet *P,
	geBoolean UseFillLight,
	const geVec3d *FillLightNormal,
	geFloat FillLightRed,				// 0 .. 255
	geFloat FillLightGreen,				// 0 .. 255
	geFloat FillLightBlue,				// 0 .. 255
	geFloat AmbientLightRed,			// 0 .. 255
	geFloat AmbientLightGreen,			// 0 .. 255
	geFloat AmbientLightBlue,			// 0 .. 255
	geBoolean UseAmbientLightFromFloor,
	int MaximumDynamicLightsToUse,		// 0 for none
	int LightReferenceBoneIndex,
	geBoolean PerBoneLighting
	)
{
	geFloat Scaler;
	assert( P!= NULL);
	assert( FillLightNormal );
	assert( geVec3d_IsNormalized(FillLightNormal) );
	assert( MaximumDynamicLightsToUse >= 0 );
	assert( (LightReferenceBoneIndex >=0) || (LightReferenceBoneIndex==GE_POSE_ROOT_JOINT));
		
	P->UseFillLight = UseFillLight;

	P->FillLightNormal = *FillLightNormal;
	
	Scaler = 1.0f/255.0f;

	P->FillLightColor.Red   = FillLightRed   * Scaler;
	P->FillLightColor.Green = FillLightGreen * Scaler;
	P->FillLightColor.Blue  = FillLightBlue  * Scaler;
	
	P->AmbientLightIntensity.Red   = AmbientLightRed   * Scaler;
	P->AmbientLightIntensity.Green = AmbientLightGreen * Scaler;
	P->AmbientLightIntensity.Blue  = AmbientLightBlue  * Scaler;
	
	P->AmbientLightFromFloor =UseAmbientLightFromFloor;
	P->MaxDynamicLightsToUse = MaximumDynamicLightsToUse;
	P->LightReferenceBoneIndex = LightReferenceBoneIndex;
	P->PerBoneLighting		 = 	PerBoneLighting;
}	


static int GENESISCC gePuppet_PrepLights(const gePuppet *P, 
	geWorld *World,
	gePuppet_Light *LP,
	const geVec3d *ReferencePoint)
{
	int i,j,cnt;

	Light_LightInfo *L;
	assert( P );
	assert( LP );

	L = (World->LightInfo);
	for (i=0,cnt=0; i<MAX_DYNAMIC_LIGHTS; i++)
		{
			if (L->DynamicLights[i].Active)
				{
					geVec3d *Position = &(L->DynamicLights[i].Pos);
					geVec3d Normal;

					geVec3d_Subtract(Position,ReferencePoint,&Normal);

					LP[cnt].Distance =	Normal.X * Normal.X + 
										Normal.Y * Normal.Y +
										Normal.Z * Normal.Z;
					if (LP[cnt].Distance < L->DynamicLights[i].Radius*L->DynamicLights[i].Radius)
						{
							LP[cnt].Color.Red = L->DynamicLights[i].Color.r;
							LP[cnt].Color.Green = L->DynamicLights[i].Color.g;
							LP[cnt].Color.Blue = L->DynamicLights[i].Color.b;
							LP[cnt].Radius = L->DynamicLights[i].Radius;
							LP[cnt].Normal = Normal;
							cnt++;
						}
				}
		}

	// sort dynamic lights by distance (squared)
	for (i=0; i<cnt; i++)
		for (j=0; j<cnt-1; j++)
			{
				if (LP[j].Distance > LP[j+1].Distance)
					{
						gePuppet_Light Swap = LP[j];
						LP[j] = LP[j+1];
						LP[j+1] = Swap;
					}
			}

	if (cnt > P->MaxDynamicLightsToUse)
		cnt = P->MaxDynamicLightsToUse;

	// go back and finish setting up closest lights
	for (i=0; i<cnt; i++)
		{
			geFloat Distance = (geFloat)sqrt(LP[i].Distance);
			geFloat OneOverDistance;
			geFloat Scale;
			if (Distance < 1.0f)
				Distance = 1.0f;
			OneOverDistance = 1.0f / Distance;
			LP[i].Normal.X *= OneOverDistance;
			LP[i].Normal.Y *= OneOverDistance;
			LP[i].Normal.Z *= OneOverDistance;

			LP[i].Distance = Distance;

			//assert( Distance  < LP[i].Radius );

			Scale = 1.0f - Distance / LP[i].Radius ;
			Scale *= (1.0f/255.0f);
			LP[i].Color.Red *= Scale;
			LP[i].Color.Green *= Scale;
			LP[i].Color.Blue *= Scale;
		}
			
	return cnt;			
}
	
static void GENESISCC gePuppet_ComputeAmbientLight(
		const gePuppet *P, 
		const geWorld *World, 
		gePuppet_Color *Ambient,
		const geVec3d *ReferencePoint)
{
	assert( P );
	assert( World );
	assert( Ambient );

	if (P->AmbientLightFromFloor != GE_FALSE)
		{
			#define GE_PUPPET_MAX_AMBIENT (0.3f)
			int32			Node, Plane, i;
			geVec3d			Pos1, Pos2, Impact;
			GFX_Node		*GFXNodes;
			Surf_SurfInfo	*Surf;
			GE_RGBA			RGBA;
			geBoolean		Col1, Col2;
			
			GFXNodes = World->CurrentBSP->BSPData.GFXNodes;
			
			Pos1 = *ReferencePoint;
			
			Pos2 = Pos1;

			Pos2.Y -= 30000.0f;

			// Get shadow hit plane impact point
			Col1 = Trace_WorldCollisionExact2((geWorld*)World, &Pos1, &Pos1, &Impact, &Node, &Plane, NULL);
			Col2 = Trace_WorldCollisionExact2((geWorld*)World, &Pos1, &Pos2, &Impact, &Node, &Plane, NULL);

			// Now find the color of the mesh by getting the lightmap point he is standing on...
			if (!Col1 && Col2)
				{
					Surf = &(World)->CurrentBSP->SurfInfo[GFXNodes[Node].FirstFace];
					if (Surf->LInfo.Face<0)
						{	// FIXME?  surface has no light...
							Ambient->Red = Ambient->Green = Ambient->Blue = 0.0f;
							return;
						}

					for (i=0; i< GFXNodes[Node].NumFaces; i++)
						{
							if (Surf_InSurfBoundingBox(Surf, &Impact, 20.0f))
								{
									Light_SetupLightmap(&Surf->LInfo, NULL);			

									if (Light_GetLightmapRGB(Surf, &Impact, &RGBA))
										{
											geFloat Scale = 1.0f / 255.0f;
											Ambient->Red   = RGBA.r * Scale;
											Ambient->Green = RGBA.g * Scale;
											Ambient->Blue  = RGBA.b * Scale;
											if (Ambient->Red > GE_PUPPET_MAX_AMBIENT) 
												{
													Ambient->Red = GE_PUPPET_MAX_AMBIENT;
												}
											if (Ambient->Green > GE_PUPPET_MAX_AMBIENT) 
												{
													Ambient->Green = GE_PUPPET_MAX_AMBIENT;
												}
											if (Ambient->Blue > GE_PUPPET_MAX_AMBIENT) 
												{
													Ambient->Blue = GE_PUPPET_MAX_AMBIENT;
												}
											break;
										}
								}
							Surf++;
						}
				}
			else
				{
					*Ambient = P->AmbientLightIntensity;
				}
		}
	else
		{
			*Ambient = P->AmbientLightIntensity;
		}
}



static void GENESISCC gePuppet_SetVertexColor(
	GE_LVertex *v,int BoneIndex)
{
	geFloat RedIntensity,GreenIntensity,BlueIntensity;
	geFloat Color;						
	int l;

	assert( v );
	
	RedIntensity   = gePuppet_StaticLightGrp.Ambient.Red;
	GreenIntensity = gePuppet_StaticLightGrp.Ambient.Green;
	BlueIntensity  = gePuppet_StaticLightGrp.Ambient.Blue;

	if (gePuppet_StaticLightGrp.UseFillLight)
		{
			geFloat Intensity;
			Intensity = gePuppet_StaticLightGrp.FillLightNormal.X * gePuppet_StaticLightGrp.SurfaceNormal.X + 
						gePuppet_StaticLightGrp.FillLightNormal.Y * gePuppet_StaticLightGrp.SurfaceNormal.Y + 
						gePuppet_StaticLightGrp.FillLightNormal.Z * gePuppet_StaticLightGrp.SurfaceNormal.Z;
			if (Intensity > 0.0)
				{
					RedIntensity   += Intensity * gePuppet_StaticLightGrp.FillLightColor.Red;
					GreenIntensity += Intensity * gePuppet_StaticLightGrp.FillLightColor.Green;
					BlueIntensity  += Intensity * gePuppet_StaticLightGrp.FillLightColor.Blue;
				}
		}

	if (gePuppet_StaticLightGrp.PerBoneLighting)
		{
			gePuppet_BoneLight *L;
			L=&(gePuppet_StaticBoneLightArray[BoneIndex]);

			for (l=0; l<L->LightCount; l++)
				{
					geVec3d *LightNormal;
					float Intensity;
				
					LightNormal = &(L->Lights[l].Normal);

					Intensity=	LightNormal->X * gePuppet_StaticLightGrp.SurfaceNormal.X + 
								LightNormal->Y * gePuppet_StaticLightGrp.SurfaceNormal.Y + 
								LightNormal->Z * gePuppet_StaticLightGrp.SurfaceNormal.Z;
					if (Intensity > 0.0f)
						{
							RedIntensity   += Intensity * L->Lights[l].Color.Red;
							GreenIntensity += Intensity * L->Lights[l].Color.Green;
							BlueIntensity  += Intensity * L->Lights[l].Color.Blue;
						}
				}
		}
	else
		{
			for (l=0; l<gePuppet_StaticLightGrp.LightCount; l++)
				{
					geVec3d *LightNormal;
					float Intensity;
				
					LightNormal = &(gePuppet_StaticLightGrp.Lights[l].Normal);

					Intensity=	LightNormal->X * gePuppet_StaticLightGrp.SurfaceNormal.X + 
								LightNormal->Y * gePuppet_StaticLightGrp.SurfaceNormal.Y + 
								LightNormal->Z * gePuppet_StaticLightGrp.SurfaceNormal.Z;
					if (Intensity > 0.0f)
						{
							RedIntensity   += Intensity * gePuppet_StaticLightGrp.Lights[l].Color.Red;
							GreenIntensity += Intensity * gePuppet_StaticLightGrp.Lights[l].Color.Green;
							BlueIntensity  += Intensity * gePuppet_StaticLightGrp.Lights[l].Color.Blue;
						}
				}
		}

	Color = gePuppet_StaticLightGrp.MaterialColor.Red * RedIntensity;
	if (Color > 255.0f)
		Color = 255.0f;
	if (Color < 0.0f)
		Color = 0.0f;
	v->r = Color;

	Color = gePuppet_StaticLightGrp.MaterialColor.Green * GreenIntensity;
	if (Color > 255.0f)
		Color = 255.0f;
	if (Color < 0.0f)
		Color = 0.0f;
	v->g = Color;

	Color = gePuppet_StaticLightGrp.MaterialColor.Blue * BlueIntensity;
	if (Color > 255.0f)
		Color = 255.0f;
	if (Color < 0.0f)
		Color = 0.0f;
	v->b = Color;

}


static void GENESISCC gePuppet_DrawShadow(const gePuppet *P, 
						const gePose *Joints, 
						geEngine *Engine, 
						geWorld *World, 
						const geCamera *Camera)
{
	GE_LVertex v[3];
	
	geVec3d Impact;
	geBoolean GoodImpact;
	GFX_Plane		Plane;
	geXForm3d RootTransform;
	
	assert( P );
	assert( World );
	assert( Camera );
	assert( Joints );

	assert( (P->ShadowBoneIndex < gePose_GetJointCount(Joints)) || (P->ShadowBoneIndex ==GE_POSE_ROOT_JOINT));
	assert( (P->ShadowBoneIndex >=0)					    	|| (P->ShadowBoneIndex ==GE_POSE_ROOT_JOINT));

	gePose_GetJointTransform(Joints,P->ShadowBoneIndex,&RootTransform);
	
	{
		geVec3d			Pos1, Pos2;
		GFX_Node		*GFXNodes;
		geWorld_Model	*Model;
		Mesh_RenderQ	*Mesh;
		geActor         *Actor;

			
		GFXNodes = (World)->CurrentBSP->BSPData.GFXNodes;
		
		Pos1 = RootTransform.Translation;
			
		Pos2 = Pos1;

		Pos2.Y -= 30000.0f;

		// Get shadow hit plane impact point
		GoodImpact = Trace_WorldCollisionExact(World, 
									&Pos1,&Pos2,GE_COLLIDE_MODELS,&Impact,&Plane,&Model,&Mesh,&Actor,0, NULL, NULL);

	}

	Impact.Y += 1.0f;

	v[0].r = v[0].b = v[0].g = 0.0f;
	v[1].r = v[1].b = v[1].g = 0.0f;
	v[2].r = v[2].b = v[2].g = 0.0f;
	
#ifdef SHADOW_MAP
	{
		int i;
		GE_LVertex s[4];
		geVec3d ws[4];
		geVec3d In,Left;
		geVec3d Up;
		geVec3d Zero = {0.0f,0.0f,0.0f};
		
		geVec3d_Subtract(&Impact,&(RootTransform.Translation),&Up);
		geVec3d_Normalize(&Up);
		geVec3d_CrossProduct(&(Plane.Normal),&Up,&Left);
		if (geVec3d_Compare(&Left,&Zero,0.001f)!=GE_FALSE)
			{
				geXForm3d_GetLeft(&(RootTransform),&Left);
			}
		geVec3d_CrossProduct(&Left,&(Plane.Normal),&In);

		geVec3d_Normalize(&Left);
		geVec3d_Normalize(&In);

		s[0].r = s[0].b = s[0].g = 0.0f;
		s[1].r = s[1].b = s[1].g = 0.0f;
		s[2].r = s[2].b = s[2].g = 0.0f;
		s[3].r = s[3].b = s[3].g = 0.0f;

		geVec3d_Scale(&In  ,P->ShadowScale,&In);
		geVec3d_Scale(&Left,P->ShadowScale,&Left);

		s[0].a = s[1].a = s[2].a = s[3].a  = 160.0f;

		s[0].u = 0.0f; s[0].v = 0.0f;
		s[1].u = 1.0f; s[1].v = 0.0f;
		s[2].u = 1.0f; s[2].v = 1.0f;
		s[3].u = 0.0f; s[3].v = 1.0f;
		ws[0].Y = ws[1].Y = ws[2].Y = ws[3].Y = Impact.Y;

		ws[0].X = RootTransform.Translation.X + Left.X - In.X;
		ws[0].Z = RootTransform.Translation.Z + Left.Z - In.Z;

		ws[1].X = RootTransform.Translation.X - Left.X - In.X;
		ws[1].Z = RootTransform.Translation.Z - Left.Z - In.Z;

		ws[2].X = RootTransform.Translation.X - Left.X + In.X;
		ws[2].Z = RootTransform.Translation.Z - Left.Z + In.Z;
		
		ws[3].X = RootTransform.Translation.X + Left.X + In.X;
		ws[3].Z = RootTransform.Translation.Z + Left.Z + In.Z;

		for (i=0; i<4; i++)
			{
				geCamera_Transform(Camera,&ws[i],&ws[i]);
				geCamera_Project(Camera,&ws[i],&ws[i]);
			}

		
		s[0].X = ws[0].X; s[0].Y = ws[0].Y; s[0].Z = ws[0].Z;
		s[1].X = ws[1].X; s[1].Y = ws[1].Y; s[1].Z = ws[1].Z;
		s[2].X = ws[2].X; s[2].Y = ws[2].Y; s[2].Z = ws[2].Z;
		s[3].X = ws[3].X; s[3].Y = ws[3].Y; s[3].Z = ws[3].Z;
		
		geTClip_SetTexture(P->ShadowMap);

		geTClip_Triangle(s);
		s[1] = s[2];
		s[2] = s[3];

		geTClip_Triangle(s);
	}
#endif
	
#ifdef CIRCULAR_SHADOW
	v[0].a = v[1].a = v[2].a = 160.0f;
	v[0].u = v[1].u = v[2].u = 0.5f;
	v[0].v = v[1].v = v[2].v = 0.5f;
	
	v[0].X = Impact.X;
	v[0].Y = v[1].Y = v[2].Y = Impact.Y;
	v[0].Z = Impact.Z;
	
	{
		int steps = 30;
		int i;
		geVec3d V;
		geFloat Angle = 0.0f;
		geFloat DAngleDStep = -(2.0f * 3.14159f / (geFloat)steps);
		geFloat Radius = P->ShadowScale;

		V = Impact;
		geCamera_Transform(Camera,&V,&V);
		geCamera_Project(Camera,&V,&V);
		v[0].X = V.X;
		v[0].Y = V.Y;
		v[0].Z = V.Z;

		geTClip_SetTexture(NULL);

		V = Impact;
		V.Z += Radius;
		geCamera_Transform(Camera,&V,&V);
		geCamera_Project(Camera,&V,&V);
		v[1].X = V.X;
		v[1].Y = V.Y;
		v[1].Z = V.Z;
		for (i=0; i<steps+1; i++)
			{
				v[2] = v[1];

				V = Impact;
				V.X += (geFloat)(sin( Angle ) * Radius);
				V.Z += (geFloat)(cos( Angle ) * Radius);
				geCamera_Transform(Camera,&V,&V);
				geCamera_Project(Camera,&V,&V);
				v[1].X = V.X;
				v[1].Y = V.Y;
				v[1].Z = V.Z;

				Angle = Angle + DAngleDStep;
				geTClip_Triangle(v);
			}
	}
#endif

#ifdef PROJECTED_SHADOW			
	{
		int i,j,Count;
		geBodyInst_Index *List;
		geBodyInst_Index Command;
		geBody_SkinVertex *SV;
		
		G = geBodyInst_GetShadowGeometry(P->BodyInstance,
						gePose_GetAllJointTransforms(Joints),0,Camera,&Impact);

		if ( G == NULL )
			{
				geErrorLog_Add(ERR_PUPPET_RENDER, NULL);
				return GE_FALSE;
			}

		geTClip_SetTexture(NULL);

		Count = G->FaceCount;
		List  = G->FaceList;

		for (i=0; i<Count; i++)
			{	
				Command = *List;
				List ++;
				//Material = *List;
				List ++;

				assert( Command == GE_BODY_FACE_TRIANGLE );

				{
					float AX,AY,BXMinusAX,BYMinusAY,CYMinusAY,CXMinusAX;
					geBodyInst_Index *List2;
					
					List2 = List;
					SV = &(G->SkinVertexArray[ *List2 ]);
					AX = SV->SVPoint.X;
					AY = SV->SVPoint.Y;
					List2++;
					List2++;
					
					SV = &(G->SkinVertexArray[ *List2 ]);
					BXMinusAX = SV->SVPoint.X - AX;
					BYMinusAY = SV->SVPoint.Y - AY;
					List2++;
					List2++;

					SV = &(G->SkinVertexArray[ *List2 ]);
					CXMinusAX = SV->SVPoint.X - AX;
					CYMinusAY = SV->SVPoint.Y - AY;
					List2++;
					List2++;

					// ZCROSS is z the component of a 2d vector cross product of ABxAC
					//#define ZCROSS(Ax,Ay,Bx,By,Cx,Cy)  ((((Bx)-(Ax))*((Cy)-(Ay))) - (((By)-(Ay))*((Cx)-(Ax))))

					// 2d cross product of AB cross AC   (A is vtx[0], B is vtx[1], C is vtx[2]
					if ( ((BXMinusAX * CYMinusAY) - (BYMinusAY * CXMinusAX)) > 0.0f )
						{
							List = List2;
							continue;
						}
				}						

			#define SOME_SCALE (  255.0f / 40.0f )
				for (j=0; j<3; j++)
					{
						SV = &(G->SkinVertexArray[ *List ]);
						List++;

						v[j].X = SV->SVPoint.X;
						v[j].Y = SV->SVPoint.Y;

						v[j].Z = SV->SVPoint.Z;
						v[j].u = SV->SVU;
						v[j].v = SV->SVV;
						
						List++;
						v[j].a = (255.0f- (SV->SVU * SOME_SCALE));
					}
			
				if ((v[0].a > 0) && (v[1].a > 0) && (v[2].a > 0))
					{
						geTClip_Triangle(v);
					}
			}
		assert( ((uint32)List) - ((uint32)G->FaceList) == (uint32)(G->FaceListSize) );
	}
#endif
}

int32		NumClips;

geBoolean GENESISCC gePuppet_RenderThroughFrustum(const gePuppet *P, 
						const gePose *Joints, 
						const geExtBox *Box, 
						geEngine *Engine, 
						geWorld *World, 
						const geCamera *Camera, 
						Frustum_Info *FInfo)
{
	int32		ClipFlags;
	geVec3d     Scale;
	const geXFArray *JointTransforms;

	const geBodyInst_Geometry *G;
	assert( P      );
	assert( Engine );
	assert( World  );
	assert( Camera );
	assert( Joints );

	JointTransforms = gePose_GetAllJointTransforms(Joints);

	#pragma message ("Level of detail hacked:")

	gePose_GetScale(Joints,&Scale);
	G = geBodyInst_GetGeometry(P->BodyInstance, &Scale, JointTransforms, 0, NULL);

	// Setup clip flags...
	ClipFlags = 0xffff;

	{
		geExtBox	MinMaxs;
		//geVec3d		d1, d2, d3;
		GFX_Plane	*Planes;
		int32		k;
		geVec3d		Expand = {150.0f, 150.f, 150.0f};
		int32		OriginalClips;

		OriginalClips = NumClips;

		MinMaxs = *Box;

		Planes = FInfo->Planes;

		for (k=0; k< FInfo->NumPlanes; k++, Planes++)
		{
			int32		Side;
			
			Planes->Type = PLANE_ANY;
			
			Side = Trace_BoxOnPlaneSide(&MinMaxs.Min, &MinMaxs.Max, Planes);

			if (Side == PSIDE_BACK)
			{
				NumClips = OriginalClips;
				return GE_TRUE;
			}
			
			if (Side == PSIDE_FRONT)
			{
				ClipFlags &= ~(1<<k);
			}
			else
				NumClips++;
			
		}
	}

	if (G == NULL)
	{
		geErrorLog_Add(ERR_PUPPET_RENDER, NULL);
		return GE_FALSE;
	}

	{
		int32			NumFaces;
		int32			i;
		geBodyInst_Index	*List;
		geXForm3d		RootTransform;

		gePuppet_StaticLightGrp.UseFillLight		 = P->UseFillLight;
		gePuppet_StaticLightGrp.FillLightNormal		 = P->FillLightNormal;
		gePuppet_StaticLightGrp.FillLightColor.Red	 = P->FillLightColor.Red;
		gePuppet_StaticLightGrp.FillLightColor.Green = P->FillLightColor.Green;
		gePuppet_StaticLightGrp.FillLightColor.Blue  = P->FillLightColor.Blue;
		gePuppet_StaticLightGrp.PerBoneLighting      = P->PerBoneLighting;

		gePose_GetJointTransform(Joints,P->LightReferenceBoneIndex,&(RootTransform));

		if (P->MaxDynamicLightsToUse > 0)
		{
			if (P->PerBoneLighting)
				{
					int BoneCount;
					const geXForm3d *XFA = geXFArray_GetElements(JointTransforms, &BoneCount);
					if (BoneCount>0)
						{
							if (gePuppet_StaticBoneLightArraySize < BoneCount)
								{
									gePuppet_BoneLight *LG;
							
									LG = geRam_Realloc(gePuppet_StaticBoneLightArray, sizeof(gePuppet_BoneLight) * BoneCount);
									if (LG==NULL)
										{
											geErrorLog_Add(ERR_PUPPET_RENDER,"Failed to allocate space for bone lighting info cache");
											return GE_FALSE;
										}
									gePuppet_StaticBoneLightArray = LG;
									gePuppet_StaticBoneLightArraySize = BoneCount;
								}
							for (i=0; i<BoneCount; i++)
								{
									gePuppet_StaticBoneLightArray[i].LightCount = gePuppet_PrepLights(P,World,
												gePuppet_StaticBoneLightArray[i].Lights,&(XFA[i].Translation));
								}
						}
				}
			else
				{
					gePuppet_StaticLightGrp.LightCount = gePuppet_PrepLights(P,World
								,gePuppet_StaticLightGrp.Lights,&(RootTransform.Translation));
				}
		}
		else
		{
			gePuppet_StaticLightGrp.LightCount = 0;
		}

		gePuppet_ComputeAmbientLight(P,World,&(gePuppet_StaticLightGrp.Ambient),&(RootTransform.Translation));
		NumFaces	= G->FaceCount;
		List		= G->FaceList;
		
		// For each face, clip it to the view frustum supplied...
		for (i=0; i<NumFaces; i++)
		{
			#define MAX_TEMP_VERTS		30		// To be safe...

			int32				Length1, Length2, v, p;
			geVec3d				Dest1[MAX_TEMP_VERTS], *pDest1;
			geVec3d				Dest2[MAX_TEMP_VERTS], *pDest2;
			geVec3d				Verts[MAX_TEMP_VERTS], *pVerts, v1, v2, v3;
			Surf_TexVert		TexVerts[MAX_TEMP_VERTS], Tex1[MAX_TEMP_VERTS], *pTexVerts;
			Surf_TexVert		Tex2[MAX_TEMP_VERTS], *pTex1, *pTex2;			
			Surf_TLVertex		ScreenPts[MAX_TEMP_VERTS];
			GFX_Plane			*FPlanes;
			geBodyInst_Index		Command, Material;
			gePuppet_Material	*PM;
			float				Dist;

			Command	= *List;
			List++;
			Material = *List;
			List ++;
			
			assert( Command == GE_BODYINST_FACE_TRIANGLE );
			assert( Material>=0 );
			assert( Material<P->MaterialCount);

			PM = &(P->MaterialArray[Material]);
			gePuppet_StaticLightGrp.MaterialColor = PM->Color;

			Length1 = 3;					//FIXME:  I'm assuming numverts == 3

			pVerts = Verts;
			pTexVerts = TexVerts;

			// AHHH!! Copy over till I get a better way...
			for (v=0; v< Length1; v++, pVerts++, pTexVerts++)
			{
				geBodyInst_SkinVertex	*SVert;
				GE_LVertex lvert;

				SVert = &G->SkinVertexArray[*List];
				List++;

				*pVerts = SVert->SVPoint;

				assert( ((float)fabs(1.0-geVec3d_Length( &(G->NormalArray[ *List ] ))))< 0.001f );
						
				gePuppet_StaticLightGrp.SurfaceNormal = (G->NormalArray[ *List ]);
						
				List++;

				//gePuppet_SetVertexColor2(P,PM,&Ambient,SurfaceNormal, Lights,LightCount, pTexVerts);
				gePuppet_SetVertexColor(&lvert,SVert->ReferenceBoneIndex);
				pTexVerts->r = lvert.r;
				pTexVerts->g = lvert.g;
				pTexVerts->b = lvert.b;
				pTexVerts->u = SVert->SVU;
				pTexVerts->v = SVert->SVV;
			}

			geVec3d_Subtract(&Verts[2], &Verts[1], &v1);
			geVec3d_Subtract(&Verts[0], &Verts[1], &v2);
			geVec3d_CrossProduct(&v1, &v2, &v3);
			geVec3d_Normalize(&v3);

			Dist = geVec3d_DotProduct(&v3, &Verts[0]);

			Dist = geVec3d_DotProduct(&v3, geCamera_GetPov(Camera)) - Dist;

			if (Dist <= 0)
				continue;
			
			pDest1 = Verts;
			pDest2 = Dest2;
			pTex1 = TexVerts;
			pTex2 = Tex2;

			FPlanes = FInfo->Planes;

			for (p=0; p< FInfo->NumPlanes; p++, FPlanes++)
			{
				if (!(ClipFlags & (1<<p)))
					continue;		// No need to clip...

				assert(Length1 < MAX_TEMP_VERTS);

				if (!Frustum_ClipToPlaneUVRGBA(FPlanes, pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
					break;

				assert(Length2 < MAX_TEMP_VERTS);
				
				if (pDest1 == Dest2)
				{
					pDest1 = Dest1;
					pDest2 = Dest2;
					pTex1 = Tex1;
					pTex2 = Tex2;
				}
				else
				{
					pDest1 = Dest2;
					pDest2 = Dest1;
					pTex1 = Tex2;
					pTex2 = Tex1;
				}
				Length1 = Length2;
			}
			
			assert(Length1 < MAX_TEMP_VERTS);
	  
			if (p != FInfo->NumPlanes)
				continue;				// Can't possibly be visible

			if (Length1 < 3)
				continue;				// Can't possibly be visible
			
			// Transform to world space...
			for (v=0; v<Length1; v++)
				geCamera_Transform(Camera, &pDest1[v], &pDest2[v]);

			// Project the face, and combine tex coords into one structure (Clipped1)
			Frustum_ProjectRGBA(pDest2, pTex1, (DRV_TLVertex*)ScreenPts, Length1, Camera);

			ScreenPts[0].a = 255.0f;

			geEngine_RenderPoly(Engine, (GE_TLVertex*)ScreenPts, Length1, PM->Bitmap, 0 );
		}
	}

	#pragma message ("BUG:  Shadow caused crash in mirrors (oops).  Need to write a RenderShadowThroughFrustum...")
	/*
	if (P->DoShadow)
		{
			gePuppet_DrawShadow(P,Engine,World,Camera);
		}
	*/
	return GE_TRUE;
}


#ifdef PROFILE
#define PUPPET_AVERAGE_ACROSS 60
double Puppet_AverageCount[PUPPET_AVERAGE_ACROSS]={
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
int Puppet_AverageIndex = 0;
#endif


geBoolean GENESISCC gePuppet_Render(	const gePuppet *P, 
							const gePose *Joints,
							geEngine *Engine, 
							geWorld *World, 
							const geCamera *Camera, 
							geExtBox *TestBox)
{
	const geXFArray *JointTransforms;
	geVec3d Scale;
	#ifdef PROFILE
	rdtsc_timer_type RDTSCStart,RDTSCEnd;
	#endif
	geRect ClippingRect;
	geBoolean Clipping = GE_TRUE;
	#define BACK_EDGE (1.0f)

	const geBodyInst_Geometry *G;
	assert( P      );
	assert( Engine );
	assert( World  );
	assert( Camera );

	#ifdef PROFILE
	rdtsc_read(&RDTSCStart);
    rdtsc_zero(&RDTSCEnd);
	#endif


	geCamera_GetClippingRect(Camera,&ClippingRect);
	
	if (TestBox != NULL)
	{
		// see if the test box is visible on the screen.  If not: don't draw actor.
		// (transform and project it to the screen, then check extents of that projection
		//  against the clipping rect)
		geVec3d				BoxCorners[8];
		const geXForm3d		*ObjectToCamera;
		geVec3d				Maxs,Mins;
		int					i;
		geBoolean			ZFarEnable;
		geFloat				ZFar;

		#define BIG_NUMBER (99e9f)  

		BoxCorners[0] = TestBox->Min;
		BoxCorners[1] = BoxCorners[0];  BoxCorners[1].X = TestBox->Max.X;
		BoxCorners[2] = BoxCorners[0];  BoxCorners[2].Y = TestBox->Max.Y;
		BoxCorners[3] = BoxCorners[0];  BoxCorners[3].Z = TestBox->Max.Z;
		BoxCorners[4] = TestBox->Max;
		BoxCorners[5] = BoxCorners[4];  BoxCorners[5].X = TestBox->Min.X;
		BoxCorners[6] = BoxCorners[4];  BoxCorners[6].Y = TestBox->Min.Y;
		BoxCorners[7] = BoxCorners[4];  BoxCorners[7].Z = TestBox->Min.Z;

		ObjectToCamera = geCamera_GetCameraSpaceXForm(Camera);
		assert( ObjectToCamera );

		geVec3d_Set(&Maxs,-BIG_NUMBER,-BIG_NUMBER,-BIG_NUMBER);
		geVec3d_Set(&Mins, BIG_NUMBER, BIG_NUMBER, BIG_NUMBER);
		for (i=0; i<8; i++)
			{
				geVec3d V;
				geXForm3d_Transform(  ObjectToCamera,&(BoxCorners[i]),&(BoxCorners[i]));
				geCamera_Project(  Camera,&(BoxCorners[i]),&V);
				if (V.X > Maxs.X ) Maxs.X = V.X;
				if (V.X < Mins.X ) Mins.X = V.X;
				if (V.Y > Maxs.Y ) Maxs.Y = V.Y;
				if (V.Y < Mins.Y ) Mins.Y = V.Y;
				if (V.Z > Maxs.Z ) Maxs.Z = V.Z;
				if (V.Z < Mins.Z ) Mins.Z = V.Z;
			}

		if (   (Maxs.X < ClippingRect.Left) 
			|| (Mins.X > ClippingRect.Right)
			|| (Maxs.Y < ClippingRect.Top) 
			|| (Mins.Y > ClippingRect.Bottom)
			|| (Maxs.Z < BACK_EDGE))
			{
				// not gonna draw: box is not visible.
				return GE_TRUE;
			}

			// Reject against ZFar clipplane if enabled...
			geCamera_GetFarClipPlane(Camera, &ZFarEnable, &ZFar);

			if (ZFarEnable)
			{
				if (Mins.Z > ZFar)
					return GE_TRUE;				// Beyond ZFar ClipPlane
			}
	}

	Engine->DebugInfo.NumActors++;

	geTClip_SetupEdges(Engine,
						(geFloat)ClippingRect.Left,
						(geFloat)ClippingRect.Right,
						(geFloat)ClippingRect.Top,
						(geFloat)ClippingRect.Bottom,
						BACK_EDGE);
		
	JointTransforms = gePose_GetAllJointTransforms(Joints);

	#pragma message ("Level of detail hacked:")
	gePose_GetScale(Joints,&Scale);
	G = geBodyInst_GetGeometry(P->BodyInstance, &Scale, JointTransforms, 0,Camera);

	if ( G == NULL )
		{
			geErrorLog_Add(ERR_PUPPET_RENDER, NULL);
			return GE_FALSE;
		}

#ifdef ONE_OVER_Z_PIPELINE
#define TEST_Z_OUT(zzz, edge) 		((zzz) > (edge)) 
#define TEST_Z_IN(zzz, edge) 		((zzz) < (edge)) 
#pragma message ("test this! this is untested")
#else
#define TEST_Z_OUT(zzz, edge) 		((zzz) < (edge)) 
#define TEST_Z_IN(zzz, edge) 		((zzz) > (edge)) 
#endif


	// check for trivial rejection:
	{
		if (   (G->Maxs.X < ClippingRect.Left) 
			|| (G->Mins.X > ClippingRect.Right)
			|| (G->Maxs.Y < ClippingRect.Top) 
			|| (G->Mins.Y > ClippingRect.Bottom)
			|| ( TEST_Z_OUT( G->Maxs.Z, BACK_EDGE) ) )
			{
				// not gonna draw
				return GE_TRUE;
			}

		if (   (G->Maxs.X < ClippingRect.Right) 
			&& (G->Mins.X > ClippingRect.Left)
			&& (G->Maxs.Y < ClippingRect.Bottom) 
			&& (G->Mins.Y > ClippingRect.Top)
			&& ( TEST_Z_IN( G->Mins.Z, BACK_EDGE) ) )
			{
				// not gonna clip
				Clipping = GE_FALSE;
			}
		else
			{
				Clipping = GE_TRUE;
			}
	}

	{
		GE_LVertex v[3];
		int i,j,Count;
		geBodyInst_Index *List;
		geBodyInst_Index Command;
		geBodyInst_SkinVertex *SV;
		geXForm3d RootTransform;
		gePuppet_Material *PM;
		geBodyInst_Index Material,LastMaterial;

		gePuppet_StaticLightGrp.UseFillLight		 = P->UseFillLight;
		gePuppet_StaticLightGrp.FillLightNormal		 = P->FillLightNormal;
		gePuppet_StaticLightGrp.FillLightColor.Red	 = P->FillLightColor.Red;
		gePuppet_StaticLightGrp.FillLightColor.Green = P->FillLightColor.Green;
		gePuppet_StaticLightGrp.FillLightColor.Blue  = P->FillLightColor.Blue;
		gePuppet_StaticLightGrp.PerBoneLighting		 = P->PerBoneLighting;
		
		gePose_GetJointTransform(Joints,P->LightReferenceBoneIndex,&(RootTransform));

		if (P->MaxDynamicLightsToUse > 0)
		{
			if (P->PerBoneLighting)
				{
					int BoneCount;
					const geXForm3d *XFA = geXFArray_GetElements(JointTransforms, &BoneCount);
					if (BoneCount>0)
						{
							if (gePuppet_StaticBoneLightArraySize < BoneCount)
								{
									gePuppet_BoneLight *LG;
							
									LG = geRam_Realloc(gePuppet_StaticBoneLightArray, sizeof(gePuppet_BoneLight) * BoneCount);
									if (LG==NULL)
										{
											geErrorLog_Add(ERR_PUPPET_RENDER,"Failed to allocate space for bone lighting info cache");
											return GE_FALSE;
										}
									gePuppet_StaticBoneLightArray = LG;
									gePuppet_StaticBoneLightArraySize = BoneCount;
								}
							for (i=0; i<BoneCount; i++)
								{
									gePuppet_StaticBoneLightArray[i].LightCount = gePuppet_PrepLights(P,World,
												gePuppet_StaticBoneLightArray[i].Lights,&(XFA[i].Translation));
								}
						}
				}
			else
				{
					gePuppet_StaticLightGrp.LightCount = gePuppet_PrepLights(P,World
								,gePuppet_StaticLightGrp.Lights,&(RootTransform.Translation));
				}
		}
		else
		{
			gePuppet_StaticLightGrp.LightCount = 0;
		}

		gePuppet_ComputeAmbientLight(P,World,&(gePuppet_StaticLightGrp.Ambient),&(RootTransform.Translation));
		
		Count = G->FaceCount;
		List  = G->FaceList;
		v[0].a = v[1].a= v[2].a = 255.0f;

		LastMaterial = -1;

		for (i=0; i<Count; i++)
		{	

			Command = *List;
			List ++;
			Material = *List;
			List ++;

			assert( Command == GE_BODYINST_FACE_TRIANGLE );
			assert( Material>=0 );
			assert( Material<P->MaterialCount);

			{
				float AX,AY,BXMinusAX,BYMinusAY,CYMinusAY,CXMinusAX;
				geBodyInst_Index *List2;
				
				List2 = List;
				SV = &(G->SkinVertexArray[ *List2 ]);
				AX = SV->SVPoint.X;
				AY = SV->SVPoint.Y;
				List2++;
				List2++;
				
				SV = &(G->SkinVertexArray[ *List2 ]);
				BXMinusAX = SV->SVPoint.X - AX;
				BYMinusAY = SV->SVPoint.Y - AY;
				List2++;
				List2++;

				SV = &(G->SkinVertexArray[ *List2 ]);
				CXMinusAX = SV->SVPoint.X - AX;
				CYMinusAY = SV->SVPoint.Y - AY;
				List2++;
				List2++;

				// ZCROSS is z the component of a 2d vector cross product of ABxAC
				//#define ZCROSS(Ax,Ay,Bx,By,Cx,Cy)  ((((Bx)-(Ax))*((Cy)-(Ay))) - (((By)-(Ay))*((Cx)-(Ax))))

				// 2d cross product of AB cross AC   (A is vtx[0], B is vtx[1], C is vtx[2]
				if ( ((BXMinusAX * CYMinusAY) - (BYMinusAY * CXMinusAX)) > 0.0f )
				{
					List = List2;
					continue;
				}
			}

			if ( Material != LastMaterial )
			{
				PM = &(P->MaterialArray[Material]);
				geTClip_SetTexture(PM->Bitmap);
				gePuppet_StaticLightGrp.MaterialColor = PM->Color;
			}

			for (j=0; j<3; j++)
			{
				SV = &(G->SkinVertexArray[ *List ]);
				List++;

				v[j].X = SV->SVPoint.X;
				v[j].Y = SV->SVPoint.Y;

				v[j].Z = SV->SVPoint.Z;
				v[j].u = SV->SVU;
				v[j].v = SV->SVV;
				
				assert( ((float)fabs(1.0-geVec3d_Length( &(G->NormalArray[ *List ] ))))< 0.001f );
				
				gePuppet_StaticLightGrp.SurfaceNormal = (G->NormalArray[ *List ]);
				List++;

				gePuppet_SetVertexColor(&(v[j]),SV->ReferenceBoneIndex);
			}
		
			if (Clipping)
			{
				geTClip_Triangle(v);
			}
			else
			{
				geEngine_RenderPoly(Engine, (GE_TLVertex *)v, 3, PM->Bitmap, 0 );
			}

		}
		assert( ((uint32)List) - ((uint32)G->FaceList) == (uint32)(G->FaceListSize) );
	}


	if (P->DoShadow)
	{
		gePuppet_DrawShadow(P,Joints,Engine,World,Camera);
	}


	#ifdef PROFILE
	{
		double Count=0.0;
		int i;

		rdtsc_read(&RDTSCEnd);
		rdtsc_delta(&RDTSCStart,&RDTSCEnd,&RDTSCEnd);
		geEngine_Printf(Engine, 320,10,"Puppet Render Time=%f",(double)(rdtsc_cycles(&RDTSCEnd)/200000000.0));
		geEngine_Printf(Engine, 320,30,"Puppet Render Cycles=%f",(double)(rdtsc_cycles(&RDTSCEnd)));
		#if 1
		Puppet_AverageCount[(Puppet_AverageIndex++)%PUPPET_AVERAGE_ACROSS] = rdtsc_cycles(&RDTSCEnd);
		for (i=0; i<PUPPET_AVERAGE_ACROSS; i++)
			{	
				Count+=Puppet_AverageCount[i];
			}
		Count /= (double)PUPPET_AVERAGE_ACROSS;

		geEngine_Printf(Engine, 320,60,"Puppet AVG Render Time=%f",(double)(Count/200000000.0));
		geEngine_Printf(Engine, 320,90,"Puppet AVG Render Cycles=%f",(double)(Count));
		#endif
				
	}
	#endif

	return GE_TRUE;
}

void GENESISCC gePuppet_SetShadow(gePuppet *P, geBoolean DoShadow, 
		geFloat Scale, const geBitmap *ShadowMap,
		int BoneIndex)
{
	assert( P );
	assert( (DoShadow==GE_FALSE) || (DoShadow==GE_TRUE));

	if ( P->ShadowMap )
		geBitmap_Destroy((geBitmap **)&(P->ShadowMap));

	P->DoShadow = DoShadow;
	P->ShadowScale = Scale;
	P->ShadowMap = ShadowMap;
	P->ShadowBoneIndex = BoneIndex;

	if ( P->ShadowMap )
		geBitmap_CreateRef((geBitmap *)P->ShadowMap);
}

