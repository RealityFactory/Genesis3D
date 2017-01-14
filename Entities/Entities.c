/****************************************************************************************/
/*  Entities.c                                                                          */
/*                                                                                      */
/*  Author: Eli Boling / John Pollard                                                   */
/*  Description: EntitySet creation / Entity Compiler                                   */
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
#include <Windows.h>
#include <Assert.h>

#include "Entities.h"
#include "BaseType.h"
#include "Errorlog.h"
#include "Vec3d.h"
#include "Ram.h"

// These are temporary until we find a better way to get models pointers into the entity stuff
#include "World.h"
#include "GBSPFile.h"


//=====================================================================================
//	Local Static Globals
//=====================================================================================

//=====================================================================================
//	Local Static Function prototypes
//=====================================================================================
static		geWorld	*GWorld;		// Temp global world for testing new entity stuff

//====================================================================================
//====================================================================================
static geBoolean InsertEntityInClassList(geWorld *World, geEntity *Entity)
{
	const char			*EntClassName;
	geWorld_EntClassSet	*WSet;
	int32				i;

	if (!Entity->Class)		// Ignore all no classes
		return GE_TRUE;
	
	assert(Entity->Class->Name);	// Must have a class name
	
	EntClassName = Entity->Class->Name;

	WSet = World->EntClassSets;

	for (i=0; i< World->NumEntClassSets; i++)
	{
		if (!WSet[i].ClassName)
			continue;

		if (!stricmp(WSet[i].ClassName, EntClassName))
		{
			// Add entity to this class set...
			if (!geEntity_EntitySetAddEntity(WSet[i].Set, Entity))
				return GE_FALSE;

			return GE_TRUE;
		}
	}

	if (i >= MAX_WORLD_ENT_CLASS_SETS)
		return GE_FALSE;					// oh well...

	// Create a new entity set
	WSet[i].Set = geEntity_EntitySetCreate();

	// Insert the entity into a new class set
	WSet[i].ClassName = EntClassName;
	geEntity_EntitySetAddEntity(WSet[i].Set, Entity);

	World->NumEntClassSets++;

	return GE_TRUE;
}

//====================================================================================
//	Ent_WorldInit
//	Lets this module initialize data that it owns in the world
//====================================================================================
geBoolean Ent_WorldInit(geWorld *World)
{
	GBSP_BSPData		*BSP;
	geEntity_EntitySet	*EntitySet;
	geEntity			*Entity;

	assert(World != NULL);
	
	GWorld = World;

	World->NumEntClassSets = 0;

	if ( ! World->CurrentBSP )
		return GE_TRUE;

	BSP = &(World->CurrentBSP->BSPData);

	if (BSP->NumGFXEntData == 0)
		return GE_TRUE;				// Nothing to do...

	EntitySet = LoadEntitySet(BSP->GFXEntData, BSP->NumGFXEntData);

	if (!EntitySet)
		return GE_FALSE;

	// Insert default set...
	World->EntClassSets[0].ClassName = NULL;
	World->EntClassSets[0].Set = EntitySet;
	World->NumEntClassSets++;
		
	// Build class sets
	Entity = NULL;
	while (1)
	{
		Entity = geEntity_EntitySetGetNextEntity(EntitySet, Entity);

		if (!Entity)
			break;		// Done

		InsertEntityInClassList(World, Entity);
	}

	return GE_TRUE;
}

//====================================================================================
//	Ent_WorldShutdown
//	Lets this module shutdown data that it owns in the world
//====================================================================================
void Ent_WorldShutdown(geWorld *World)
{
	int32		i;

	assert(World);
	
	for (i=0; i< World->NumEntClassSets; i++)
		geEntity_EntitySetDestroy(World->EntClassSets[i].Set);
}

//====================================================================================
//	CopyString
//====================================================================================
static char *CopyString(const char *String)
{
	char	*NewString;

	NewString = geRam_Allocate(strlen(String)+1);

	if (!NewString)
		return NULL;

	strcpy(NewString, String);

	return NewString;
}

//====================================================================================
//	geEntity_Create
//====================================================================================
geEntity *geEntity_Create(void)
{
	geEntity	*Entity;

	Entity = geRam_Allocate(sizeof(geEntity));

	if (!Entity)
		return NULL;

	memset(Entity, 0, sizeof(geEntity));

	return Entity;
}

//====================================================================================
//	geEntity_Destroy
//	NOTE - This currently does garbage collection on the epairs that were inserted in the entity
//====================================================================================
void geEntity_Destroy(geEntity *Entity)
{
	geEntity_Epair	*Epair, *Next;

	assert(Entity);

	for (Epair = Entity->Epairs; Epair; Epair = Next)
	{
		Next = Epair->Next;

		geEntity_EpairDestroy(Epair);
	}

	if (Entity->UserData)
		geRam_Free(Entity->UserData);

	geRam_Free(Entity);
}

//====================================================================================
//	geEntity_GetUserData
//====================================================================================
GENESISAPI void *geEntity_GetUserData(geEntity *Entity)
{
	assert(Entity);

	return Entity->UserData;
}

//====================================================================================
//	geEntity_GetModelNumForKey
//====================================================================================
geBoolean geEntity_GetModelNumForKey(geEntity *Entity, const char *Key, int32 *ModelNum)
{
	geEntity_Epair	*Epair;
	int32			Value;

	for (Epair = Entity->Epairs; Epair; Epair = Epair->Next)
	{
		if (!stricmp(Epair->Key, Key))
		{
			if (Epair->Value[0] == '*')
				sscanf(Epair->Value+1, "%d", &Value);
			else
				sscanf(Epair->Value, "%d", &Value);

			*ModelNum = (int32)Value;
			return GE_TRUE;
		}
	}
	return GE_FALSE;				// not found !

}

//====================================================================================
//	geEntity_AddEpair
//====================================================================================
geBoolean geEntity_AddEpair(geEntity *Entity, geEntity_Epair *Epair)
{
	geEntity_Epair	*Ep;

	assert(Entity);
	assert(Epair);

	assert(Epair->Next == NULL);		// Make sure this is a fresh one (ahh yahh)
	
	if (!Entity->Epairs)
	{
		Entity->Epairs = Epair;
		return GE_TRUE;
	}

	// Jump to end of list 
	for (Ep = Entity->Epairs; Ep->Next; Ep = Ep->Next);	

	Ep->Next = Epair;

	return GE_TRUE;
}

//====================================================================================
//	geEntity_GetStringForKey
//====================================================================================
const char *geEntity_GetStringForKey(const geEntity *Entity, const char *Key)
{
	geEntity_Epair	*Epair;

	for (Epair = Entity->Epairs; Epair; Epair = Epair->Next)
	{
		if (!stricmp(Epair->Key, Key))
		{
			return Epair->Value;
		}
	}
	return NULL;				// not found!
}

//====================================================================================
//	geEntity_EpairCreate
//====================================================================================
geEntity_Epair *geEntity_EpairCreate(void)
{
	geEntity_Epair	*Epair;

	Epair = geRam_Allocate(sizeof(geEntity_Epair));

	if (!Epair)
		return NULL;

	memset(Epair, 0, sizeof(geEntity_Epair));

	return Epair;
}

//====================================================================================
//	geEntity_EpairDestroy
//====================================================================================
void geEntity_EpairDestroy(geEntity_Epair *Epair)
{
	assert(Epair);

	if (Epair->Key)
		geRam_Free(Epair->Key);

	if (Epair->Value)
		geRam_Free(Epair->Value);

	geRam_Free(Epair);
}

//====================================================================================
//	geEntity_FieldCreate
//====================================================================================
geEntity_Field *geEntity_FieldCreate(const char *Name, int32 Offset, geEntity_Class *TypeClass)
{
	geEntity_Field	*Field;

	Field = GE_RAM_ALLOCATE_STRUCT(geEntity_Field);

	if (!Field)
		return NULL;

	memset(Field, 0, sizeof(geEntity_Field));

	if (Name)
		Field->Name = CopyString(Name);

	Field->Offset = Offset;
	Field->TypeClass = TypeClass;

	return Field;
}

//====================================================================================
//	geEntity_FieldDestroy
//====================================================================================
void geEntity_FieldDestroy(geEntity_Field *Field)
{
	assert(Field);

	if (Field->Name)
		geRam_Free(Field->Name);

	geRam_Free(Field);
}

//====================================================================================
//	geEntity_ClassCreate
//====================================================================================
geEntity_Class *geEntity_ClassCreate(geEntity_ClassType Type, const char *Name, int32 TypeSize)
{
	geEntity_Class	*Class;

	Class = GE_RAM_ALLOCATE_STRUCT(geEntity_Class);

	if (!Class)
		return NULL;

	memset(Class, 0, sizeof(geEntity_Class));

	Class->Type = Type;

	if (Name)
		Class->Name = CopyString(Name);

	Class->TypeSize = TypeSize;

	return Class;
}

//====================================================================================
//	geEntity_ClassDestroy
//====================================================================================
void geEntity_ClassDestroy(geEntity_Class *Class)
{
	geEntity_Field	*Field, *NextField;
	assert(Class);

	for (Field = Class->Fields; Field; Field = NextField )
	{
		NextField = Field->Next;
		geEntity_FieldDestroy(Field);
	}
	if (Class->Name)
		geRam_Free(Class->Name);

	geRam_Free(Class);
}

//====================================================================================
//	geEntity_ClassAddField
//====================================================================================
geBoolean geEntity_ClassAddField(geEntity_Class	*Class, geEntity_Field *Field)
{
	assert(Class);
	assert(Field);

	// Put at the beggining
	Field->Next = Class->Fields;
	Class->Fields = Field;

	// Grow fieldsize by Fields TypeClass size
	Class->FieldSize += Field->TypeClass->TypeSize;

	return GE_TRUE;
}

//====================================================================================
//	geEntity_ClassFindFieldByName
//====================================================================================
geEntity_Field *geEntity_ClassFindFieldByName(geEntity_Class *Class, const char *Name)
{
	geEntity_Field	*Field;

	assert(Class);

	for (Field = Class->Fields; Field; Field = Field->Next)
	{
		if	(!stricmp(Field->Name, Name))
			return Field;
		
	}

	return NULL;
}

//====================================================================================
//	geEntity_EntitySetCreate
//====================================================================================
geEntity_EntitySet *geEntity_EntitySetCreate(void)
{
	geEntity_EntitySet		*EntitySet;

	EntitySet = geRam_Allocate(sizeof(geEntity_EntitySet));

	if (!EntitySet)
		return NULL;

	memset(EntitySet, 0, sizeof(geEntity_EntitySet));

	return EntitySet;
}

//====================================================================================
//	geEntity_EntitySetDestroy
//	NOTE - This does garbage collection to anything added the the set
//====================================================================================
void geEntity_EntitySetDestroy(geEntity_EntitySet *EntitySet)
{
	geEntity_EntitySet	*Set, *Next;
	geEntity_Class		*Class, *NextClass;
	
	assert(EntitySet);

	if (EntitySet->OwnsEntities)		// Free stuff if we created all this stuff
	{
		for (Class = EntitySet->Classes; Class; Class = NextClass)
		{			   
			NextClass = Class->Next;

			geEntity_ClassDestroy(Class);
		}

		for (Set = EntitySet; Set; Set = Next)
		{
			Next = Set->Next;
			if	(Set->Entity)
				geEntity_Destroy(Set->Entity);
		}
	}

	// Finclaly destroy the sets themselves...
	for (Set = EntitySet; Set; Set = Next)
	{
		Next = Set->Next;

		geRam_Free(Set);
	}
}

//====================================================================================
//	geEntity_EntitySetFindClassByName
//====================================================================================
geEntity_Class *geEntity_EntitySetFindClassByName(geEntity_EntitySet *Set, const char *Name)
{
	geEntity_Class	*Class;

	assert(Set);

	for (Class = Set->Classes; Class; Class = Class->Next)
	{
		if	(!stricmp(Class->Name, Name))
			return Class;
	}

	return NULL;	// Not found!!!
}

GENESISAPI void geEntity_GetName(const geEntity *Entity, char *Buff, int MaxLen)
{
	const char *	EntName;
	int				Length;

	assert(Entity);
	assert(Buff);
	
	EntName = geEntity_GetStringForKey(Entity, "%Name%");
	assert(EntName);
	Length = strlen(EntName) + 1;
	memcpy(Buff, EntName, min(Length, MaxLen));
}

//====================================================================================
//	geEntity_EntitySetFindEntityByName
//====================================================================================
geEntity *geEntity_EntitySetFindEntityByName(geEntity_EntitySet *EntitySet, const char *Name)
{
	const char			*EntName;
	geEntity_EntitySet	*Set;

	assert(EntitySet);
	assert(Name);

	for (Set = EntitySet; Set; Set = Set->Next)
	{
		EntName = geEntity_GetStringForKey(Set->Entity, "%Name%");

		if (!EntName)
			continue;

		if (!stricmp(Name, EntName))
			return Set->Entity;
	}
	
	return NULL;
}

//====================================================================================
//	geEntity_EntitySetAddEntity
//====================================================================================
geBoolean geEntity_EntitySetAddEntity(geEntity_EntitySet *EntitySet, geEntity *Entity)
{
	geEntity_EntitySet	*NewSet, *Set;
	
	assert(EntitySet);
	assert(Entity);

	// If no entities in list, just make this one the first
	if (!EntitySet->Entity)
	{
		assert(EntitySet->Next == NULL);
		assert(EntitySet->Current == NULL);

		EntitySet->Entity = Entity;
		return GE_TRUE;
	}

	NewSet = geEntity_EntitySetCreate();

	if (!NewSet)
		return GE_FALSE;

	// Store the entity
	NewSet->Entity = Entity;
	
	// Jump to end of list (we allways want them to work on the first set in the list...)
	for (Set = EntitySet; Set->Next; Set = Set->Next);		

	// Add the newset
	Set->Next = NewSet;
return GE_TRUE;
}

//====================================================================================
//	geEntity_EntitySetGetNextEntity
//====================================================================================
GENESISAPI geEntity *geEntity_EntitySetGetNextEntity(geEntity_EntitySet *EntitySet, geEntity *Entity)
{
	geEntity_EntitySet		*Set, *NextSet;

	assert(EntitySet);

	if (!Entity)
	{
		EntitySet->Current = EntitySet;
		return EntitySet->Entity;
	}

	assert(EntitySet->Current);

	// Search is easy if this was the last entity we returned
	if (EntitySet->Current->Entity == Entity)
	{
		NextSet = EntitySet->Current->Next;

		if (!NextSet)
			return NULL;		// No more in the list

		EntitySet->Current = NextSet;
		return NextSet->Entity;
	}

	// Must do a search now, that'll teach'em to jump around...
	for (Set = EntitySet; Set; Set = Set->Next)
	{
		assert(Set->Entity);

		if (Set->Entity == Entity)
		{
			NextSet = Set->Next;

			EntitySet->Current = NextSet;
			return NextSet->Entity;
		}
	}

	return NULL;
}

//====================================================================================
//	geEntity_EntitySetAddClass
//====================================================================================
geBoolean geEntity_EntitySetAddClass(geEntity_EntitySet *EntitySet, geEntity_Class *Class)
{
	assert(EntitySet);
	assert(Class);

	assert(Class->Next == NULL);		// We want fresh ones only

	// Just put in front of list
	Class->Next = EntitySet->Classes;
	EntitySet->Classes = Class;

	return GE_TRUE;
}

//====================================================================================
//	BuildClassTypes
//====================================================================================
static geBoolean BuildClassTypes(geEntity_EntitySet *EntitySet)
{
	const char			*Name;
	geEntity_Class		*Class;
	geEntity_EntitySet	*Set;
	
	// Fill in the pre-defined atomic types
	Class = geEntity_ClassCreate(TYPE_INT, "int", sizeof(int32));
	geEntity_EntitySetAddClass(EntitySet, Class);

	Class = geEntity_ClassCreate(TYPE_FLOAT, "float", sizeof(float));
	geEntity_EntitySetAddClass(EntitySet, Class);

	Class = geEntity_ClassCreate(TYPE_COLOR, "color", sizeof(GE_RGBA));
	geEntity_EntitySetAddClass(EntitySet, Class);

	Class = geEntity_ClassCreate(TYPE_POINT, "point", sizeof(geVec3d));
	geEntity_EntitySetAddClass(EntitySet, Class);

	Class = geEntity_ClassCreate(TYPE_STRING, "string", sizeof(char *));
	geEntity_EntitySetAddClass(EntitySet, Class);

	Class = geEntity_ClassCreate(TYPE_PTR, "ptr", sizeof(void *));
	geEntity_EntitySetAddClass(EntitySet, Class);

	Class = geEntity_ClassCreate(TYPE_MODEL, "model", sizeof(void*));
	geEntity_EntitySetAddClass(EntitySet, Class);
	
	//	Find all the %typedef% keywords, and allocate them as TYPE_STRUCT's
	for (Set = EntitySet; Set; Set = Set->Next)
	{
		geEntity *Entity;
		
		Entity = Set->Entity;

		Name = geEntity_GetStringForKey(Entity, "classname");

		if (!Name)
			continue;	

		if (stricmp(Name, "%typedef%"))	// No %typedef% info
			continue;

		Name = geEntity_GetStringForKey(Entity, "%typename%");

		if (!Name)
			return GE_FALSE;

		Class = geEntity_ClassCreate(TYPE_STRUCT, Name, sizeof(void*));

		if (!Class)
			goto ExitWithError;

		if (!geEntity_EntitySetAddClass(EntitySet, Class))
			goto ExitWithError;
	}

	for (Set = EntitySet; Set; Set = Set->Next)
	{
		geEntity		*Entity;
		geEntity_Epair	*Epair;

		Entity = Set->Entity;

		Name = geEntity_GetStringForKey(Entity, "classname");

		if (!Name)		// Not a class entity (Error?)
			continue;	

		if (stricmp(Name, "%typedef%"))	// Not a typedef
			continue;
		
		Name = geEntity_GetStringForKey(Entity, "%typename%");

		if (!Name)
			return GE_FALSE;

		Class = geEntity_EntitySetFindClassByName(EntitySet, Name);

		if (!Class)
			continue;	// Oops, this %typedef% will have no fields

		for (Epair = Entity->Epairs; Epair; Epair = Epair->Next)
		{
			geEntity_Class	*TypeClass;
			geEntity_Field	*Field;

			// Skip classname and %typename%
			if	(!stricmp(Epair->Key, "classname"))
				continue;
			if	(!stricmp(Epair->Key, "%typename%"))
				continue;

			assert(stricmp(Epair->Key, "%defaultvalue%"));

			TypeClass = geEntity_EntitySetFindClassByName(EntitySet, Epair->Value);

			if (!TypeClass)
				goto ExitWithError;		// Type not defined for this field!!!

			Field = geEntity_FieldCreate(Epair->Key, Class->FieldSize, TypeClass);

			if (!Field)
				goto ExitWithError;

			if (!geEntity_ClassAddField(Class, Field))
				goto ExitWithError;

			Epair = Epair->Next;
			assert(Epair);
			assert(!stricmp(Epair->Key, "%defaultvalue%"));

		}	
	}

	return GE_TRUE;

	// ** ERROR **
	ExitWithError:
	{
		if (Class)
			geEntity_ClassDestroy(Class);

		return GE_FALSE;
	}
}

//====================================================================================
//	ParseTypedUserData
//====================================================================================
static geBoolean ParseClassUserData(geEntity_EntitySet *EntitySet, geEntity *Entity)
{
	geEntity_Epair	*Epair;
	
	if (!Entity->Class)
		return GE_TRUE;
	
	for (Epair = Entity->Epairs; Epair; Epair = Epair->Next)
	{
		char			*UData;
		geEntity_Field	*Field;

		// Skip %name% and classname keywords in entity
		if	(!stricmp(Epair->Key, "classname"))
			continue;

		if (Epair->Key[0] == '%')		// Skip special names
			continue;

		// Find the field in this class
		Field = geEntity_ClassFindFieldByName(Entity->Class, Epair->Key);
		
		if	(!Field)		
			return GE_FALSE;
			//continue;

		UData = Entity->UserData;

		switch (Field->TypeClass->Type)
		{
			double			f;
			double			X, Y, Z;
			int32			M;
			geEntity		*Ent;

			case TYPE_INT:
				sscanf(Epair->Value, "%d", UData + Field->Offset);
				break;

			case TYPE_FLOAT:
				sscanf(Epair->Value, "%lf", &f);
				*(float *)(UData + Field->Offset) = (float)f;
				break;

			case TYPE_POINT:
				sscanf(Epair->Value, "%lf %lf %lf", &X, &Y, &Z);
				((geVec3d *)(UData + Field->Offset))->X = (float)X;
				((geVec3d *)(UData + Field->Offset))->Y = (float)Y;
				((geVec3d *)(UData + Field->Offset))->Z = (float)Z;
				break;

			case TYPE_COLOR:
				sscanf(Epair->Value, "%lf %lf %lf", &X, &Y, &Z);
				((GE_RGBA *)(UData + Field->Offset))->r = (float)X;
				((GE_RGBA *)(UData + Field->Offset))->g = (float)Y;
				((GE_RGBA *)(UData + Field->Offset))->b = (float)Z;
				((GE_RGBA *)(UData + Field->Offset))->a = 0.0f;
				break;

			case TYPE_STRING:
				// Point to string in Epair
				*(char **)(UData + Field->Offset) = Epair->Value;
				break;

			case TYPE_PTR:
				// Must be some arbitrary pointer definition the user had in the private section.
				// Always initialize these to NULL.
				*(void **)(UData + Field->Offset) = NULL;
				break;

			case TYPE_MODEL:

				Ent = geEntity_EntitySetFindEntityByName(EntitySet, Epair->Value);

				if (!Ent)
				{
					geErrorLog_Add(GE_ERR_MODEL_NOT_FOUND, NULL);
					return GE_FALSE;
				}

				if (!geEntity_GetModelNumForKey(Ent, "Model", &M))
				{
					geErrorLog_Add(GE_ERR_MODEL_NOT_IN_ENTITY, NULL);
					return GE_FALSE;
				}
			
				// Point their structure directly to the model
				*(geWorld_Model**)(UData + Field->Offset) = &GWorld->CurrentBSP->Models[M];
			
				break;

			case TYPE_STRUCT:
				Ent = geEntity_EntitySetFindEntityByName(EntitySet, Epair->Value);

				if	(!Ent)
					return GE_FALSE;

				// Point the void to the other entity
				*(void **)(UData + Field->Offset) = Ent->UserData;

				assert(*(void **)(UData + Field->Offset) != NULL);
				break;

			default:
				//assert(!"Illegal top type");
				geErrorLog_Add(GE_ERR_INVALID_ENTITY_FIELD_TYPE, NULL);
				return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//====================================================================================
//	ParseClassData
//====================================================================================
static geBoolean ParseClassData(geEntity_EntitySet *EntitySet)
{
	geEntity_EntitySet	*Set;
	const char			*Name;

	//
	//  Do this in two passes.  First pass, we setup the entities, and allocate all the user data.
	//  Second pass, we actually parse the user data.  This is so that we can resolve forward
	//  references in the user structures in the bsp file.
	//

	
	for (Set = EntitySet; Set; Set = Set->Next)
	{
		geEntity		*Entity;
		geEntity_Class	*Class;

		Entity = Set->Entity;

		Name = geEntity_GetStringForKey(Entity, "classname");

		if (!Name)
			continue;
		
		Class = geEntity_EntitySetFindClassByName(EntitySet, Name);

		Entity->Class = NULL;
		Entity->UserData = NULL;

		if	(!Class)		// Oops, this entity will not show up, it was not %typedef%'d
			continue;

		// Let this entity remember it's type class
		Entity->Class = Class;
		// Make user data big enough to hold all possible data from class type
		Entity->UserData = GE_RAM_ALLOCATE_ARRAY(char, Class->FieldSize);
		
		if	(!Entity->UserData)
		{
			geErrorLog_Add(GE_ERR_GET_ENTITY_DATA_ERROR, NULL);
			return GE_FALSE;
		}
		
		// Clear the userdata
		memset(Entity->UserData, 0, Class->FieldSize);
	}

	for (Set = EntitySet; Set; Set = Set->Next)
	{
		geEntity		*Entity;

		Entity = Set->Entity;

		Name = geEntity_GetStringForKey(Entity, "classname");

		if (!Name)
			continue;
		
		if (!ParseClassUserData(EntitySet, Entity))
		{
			geErrorLog_Add(GE_ERR_GET_ENTITY_DATA_ERROR, NULL);
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//====================================================================================
//	geEntity_EntitySetBuildClasses
//====================================================================================
geBoolean geEntity_EntitySetBuildClasses(geEntity_EntitySet *Set)
{
	if (!BuildClassTypes(Set))
		return GE_FALSE;

	if (!ParseClassData(Set))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//	geEntity_EntitySetLoadEntities
//====================================================================================
geBoolean geEntity_EntitySetLoadEntities(geEntity_EntitySet *EntitySet, geVFile *VFile)
{
	int32		i, NumEntities;

	if (!geVFile_Read(VFile, &NumEntities, sizeof(int32)))
		return GE_FALSE;

	for (i=0; i< NumEntities; i++)
	{
		geEntity		*Entity;
		int32			e, NumEpairs;

		// Create the entity
		Entity = geEntity_Create();

		if (!Entity)
			return GE_FALSE;

		// Add it to the main set
		if (!geEntity_EntitySetAddEntity(EntitySet, Entity))
			return GE_FALSE;

		// Load epairs
		if (!geVFile_Read(VFile, &NumEpairs, sizeof(int32)))
			return GE_FALSE;

		for (e=0; e<NumEpairs; e++)
		{
			geEntity_Epair	*Epair;
			int32			Size;

			Epair = geEntity_EpairCreate();

			if (!Epair)
				return GE_FALSE;

			// Get the Key Size
			if (!geVFile_Read(VFile, &Size, sizeof(int32)))
				return GE_FALSE;

			Epair->Key = GE_RAM_ALLOCATE_ARRAY(char, Size);

			if (!Epair->Key)
				return GE_FALSE;

			// Read the key
			if (!geVFile_Read(VFile, Epair->Key, sizeof(char)*Size))
				return GE_FALSE;

			// Get the Value Size
			if (!geVFile_Read(VFile, &Size, sizeof(int32)))
				return GE_FALSE;

			Epair->Value = GE_RAM_ALLOCATE_ARRAY(char, Size);

			if (!Epair->Value)
				return GE_FALSE;

			// Read the Value
			if (!geVFile_Read(VFile, Epair->Value, sizeof(char)*Size))
				return GE_FALSE;

			// Add the epair to the entity
			if (!geEntity_AddEpair(Entity, Epair))
				return GE_FALSE;
		}

	}

	return GE_TRUE;

}

//====================================================================================
//	geEntity_LoadEntitySet
//====================================================================================
geEntity_EntitySet *LoadEntitySet(const char *EntityData, int32 EntityDataSize)
{
	geEntity_EntitySet		*EntitySet;
	geVFile_MemoryContext	Context;
	geVFile					*MemFile;

	// Default some NULLS here
	EntitySet = NULL;
	MemFile = NULL;

	// Setup the context for the memfile
	Context.Data = (void*)EntityData;
	Context.DataLength = EntityDataSize;

	MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_READONLY);

	if (!MemFile)
		return FALSE;

	// Create the entityset
	EntitySet = geEntity_EntitySetCreate();

	if (!EntitySet)		// Out of memory!!
		goto ExitWithError;

	EntitySet->OwnsEntities = GE_TRUE;		// So this set will be the one to free everything...

	if (!geEntity_EntitySetLoadEntities(EntitySet, MemFile))
		goto ExitWithError;

	geVFile_Close(MemFile);
	MemFile = NULL;
		
	if (!geEntity_EntitySetBuildClasses(EntitySet))
		goto ExitWithError;

	return EntitySet;

	// ** ERROR **
	ExitWithError:
	{
		if (EntitySet)
			geEntity_EntitySetDestroy(EntitySet);

		if (MemFile)
			geVFile_Close(MemFile);

		return NULL;
	}
}
