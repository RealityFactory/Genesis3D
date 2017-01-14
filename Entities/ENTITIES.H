/****************************************************************************************/
/*  Entities.h                                                                          */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef GE_ENTITIES_H
#define GE_ENTITIES_H

#include <Assert.h>
#include <Windows.h>

#include "BaseType.h"
#include "Errorlog.h"
#include "Vec3d.h"
#include "System.h"
#include "World.h"
#include "Ram.h"

#ifdef __cplusplus
extern "C" {
#endif

//=====================================================================================
//	Defines / Structure defines
//=====================================================================================
// Enumerated types
typedef	enum
{
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_COLOR,
	TYPE_POINT,
	TYPE_STRING,
	TYPE_MODEL,
	TYPE_STRUCT,
	TYPE_PTR,
} geEntity_ClassType;

typedef struct	geEntity_Class	geEntity_Class;

// Fields within a geEntity_Class
typedef struct geEntity_Field
{
	char					*Name;					// Name of the field
	geEntity_Class			*TypeClass;				// Atomic class the defines field
	int32					Offset;					// Offset in Entity's UserData

	struct geEntity_Field	*Next;					// Next Field list

} geEntity_Field;

typedef	struct geEntity_Class
{
	char					*Name;					// Class Name
	geEntity_ClassType		Type;					// Type of class
	int32					TypeSize;
	geEntity_Field			*Fields;				// Fields in this Class
	int32					FieldSize;				// Size of all fields

	struct geEntity_Class	*Next;

} geEntity_Class; 

typedef struct geEntity_Epair
{
	struct geEntity_Epair	*Next;
	char					*Key;
	char					*Value;
} geEntity_Epair;

typedef struct geEntity
{
	geEntity_Class			*Class;					// Class data

	geEntity_Epair			*Epairs;				// Parsed epair list from entity string list

	void					*UserData;				// User structure parsed from entity
} geEntity;

typedef struct geEntity_EntitySet
{
	struct geEntity_EntitySet	*Next;				// Next entity in this set
	struct geEntity_EntitySet	*Current;			// Current entity set as an iterator

	geBoolean					OwnsEntities;		// GE_TRUE if 

	geEntity					*Entity;			// The entity
	geEntity_Class				*Classes;			// List of classes for set

} geEntity_EntitySet;


//=====================================================================================
//	Function prototypes
//=====================================================================================
geBoolean	Ent_WorldInit(geWorld *World);
void		Ent_WorldShutdown(geWorld *World);

geEntity				*geEntity_Create(void);
void					geEntity_Destroy(geEntity *Entity);
geBoolean				geEntity_GetModelNumForKey(geEntity *Entity, const char *Key, int32 *ModelNum);
geBoolean				geEntity_AddEpair(geEntity *Entity, geEntity_Epair *Epair);
const char				*geEntity_GetStringForKey(const geEntity *Entity, const char *Key);
geEntity_Epair			*geEntity_EpairCreate(void);
void					geEntity_EpairDestroy(geEntity_Epair *Epair);
geEntity_Field			*geEntity_FieldCreate(const char *Name, int32 Offset, geEntity_Class *TypeClass);
void					geEntity_FieldDestroy(geEntity_Field *Field);
geEntity_Class			*geEntity_ClassCreate(geEntity_ClassType Type, const char *Name, int32 TypeSize);
void					geEntity_ClassDestroy(geEntity_Class *Class);
geBoolean				geEntity_ClassAddField(geEntity_Class	*Class, geEntity_Field *Field);
geEntity_Field			*geEntity_ClassFindFieldByName(geEntity_Class *Class, const char *Name);
geEntity_EntitySet		*geEntity_EntitySetCreate(void);
void					geEntity_EntitySetDestroy(geEntity_EntitySet *EntitySet);
geEntity_Class			*geEntity_EntitySetFindClassByName(geEntity_EntitySet *Set, const char *Name);
geEntity				*geEntity_EntitySetFindEntityByName(geEntity_EntitySet *EntitySet, const char *Name);
GENESISAPI void					geEntity_GetName(const geEntity *Entity, char *Buff, int MaxLen);
geBoolean 	geEntity_EntitySetAddEntity(geEntity_EntitySet *EntitySet, geEntity *Entity);
GENESISAPI	geEntity	*geEntity_EntitySetGetNextEntity(geEntity_EntitySet *EntitySet, geEntity *Entity);
geBoolean				geEntity_EntitySetAddClass(geEntity_EntitySet *EntitySet, geEntity_Class *Class);
geEntity_EntitySet		*LoadEntitySet(const char *EntityData, int32 EntityDataSize);

#ifdef __cplusplus
}
#endif

#endif
