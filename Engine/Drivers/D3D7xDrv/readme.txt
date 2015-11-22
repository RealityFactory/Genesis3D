
Changes for D3Dx7 driver

32 bit textures
---------------

All changes are in D3D_Main.cpp and start with the comment line :

// start 32 bit change

The driver reads in a file called D3D24.ini and from the first line gets the video depth.
This can be 16, 24 0r 32. The second line is the zbuffer depth and this can be 16 or 32.
If the video card is a Voodoo card then both values must be 16 or a crash will occur. 
If no file is found the driver defaults to 16 bit color.

Screenshot
----------

The screenshot function in D3Ddrv7x.cpp has been modified to provide a screenshot
saved to disk.

Disable Fog on Polys
--------------------

A new flag has been added to allow a poly to be rendered without applying fog to it.
The changes are flagged by the comment :

// poly fog

To make this work a new definition has been added to Dcommon.h as such :

#define DRV_RENDER_POLY_NO_FOG	(1<<5)  // Don't render this poly with fog

The engine must also be modified if you wish to use this feature, which was aimed
at allowing the skybox to render even if there is fog. The changes are :

In Genesis.h add this line :

#define GE_RENDER_NO_FOG				(1<<5)

In User.c in function RenderTexturedPoly add this :

if (Poly->RenderFlags & GE_RENDER_CLAMP_UV)
	RenderFlags |= DRV_RENDER_CLAMP_UV;

if (Poly->RenderFlags & GE_RENDER_NO_FOG) // add these lines
	RenderFlags |= DRV_RENDER_POLY_NO_FOG;

// Render it...
assert(geWorld_HasBitmap(gWorld, pBitmap));

In World.c in function RenderSkyThroughFrustum change this line :

SkyFlags = DRV_RENDER_FLUSH;

to

SkyFlags = DRV_RENDER_FLUSH | DRV_RENDER_POLY_NO_FOG;

