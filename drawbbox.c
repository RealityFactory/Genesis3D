/*
	This file is provided as a debugging aid.  It contains code for drawing an axial
	aligned bounding box.  You can use this code as you see fit.
*/

static	void	DrawFace(geWorld *World, const geVec3d **Verts)
{
	GE_LVertex	LVerts[4];
	int			i;

	for	(i = 0; i < 4; i++)
	{
		LVerts[i].r = 40.0f;
		LVerts[i].g = 40.0f;
		LVerts[i].b = 80.0f + 20.0f * (geFloat)i;
		LVerts[i].a = 128.0f;
		LVerts[i].X = Verts[i]->X;
		LVerts[i].Y = Verts[i]->Y;
		LVerts[i].Z = Verts[i]->Z;
	}

	geWorld_AddPolyOnce(World, &LVerts[0], 4, NULL, GE_GOURAUD_POLY, GE_FX_TRANSPARENT, 1.0f);
}

void	DrawBoundBox(geWorld *World, const geVec3d *Pos, const geVec3d *Min, const geVec3d *Max)
{
	geFloat	dx;
	geFloat	dy;
	geFloat	dz;
static	geVec3d		Verts[8];
static	geVec3d *	Faces[6][4] =
{
	{ &Verts[0], &Verts[1], &Verts[2], &Verts[3] },	//Top
	{ &Verts[4], &Verts[5], &Verts[6], &Verts[7] },	//Bottom
	{ &Verts[3], &Verts[2], &Verts[6], &Verts[7] }, //Side
	{ &Verts[1], &Verts[0], &Verts[4], &Verts[5] }, //Side
	{ &Verts[0], &Verts[3], &Verts[7], &Verts[4] }, //Front
	{ &Verts[2], &Verts[1], &Verts[5], &Verts[6] }, //Back
};
	int			i;

	for	(i = 0; i < 8; i++)
		geVec3d_Add(Pos, Min, &Verts[i]);

	dx = Max->X - Min->X;
	dy = Max->Y - Min->Y;
	dz = Max->Z - Min->Z;

	Verts[0].Y += dy;
	Verts[3].Y += dy;
	Verts[3].X += dx;
	Verts[7].X += dx;

	Verts[1].Y += dy;
	Verts[1].Z += dz;
	Verts[5].Z += dz;
	Verts[6].Z += dz;
	Verts[6].X += dx;

	Verts[2].X += dx;
	Verts[2].Y += dy;
	Verts[2].Z += dz;

	for	(i = 0; i < 6; i++)
		DrawFace(World, &Faces[i][0]);
}

