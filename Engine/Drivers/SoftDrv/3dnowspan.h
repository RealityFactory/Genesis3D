/****************************************************************************************/
/*  3dnowspan.h                                                                         */
/*                                                                                      */
/*  Author:       Ken Baird                                                             */
/*  Description:  Headers for 3dnow assembly calls                                      */
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

typedef struct EdgeAsmTag EdgeAsm;
typedef struct EdgeAsmFPUTag EdgeAsmFPU;

//3dnow based non world faces
//DRV_RENDER_NO_ZMASK
extern void DrawScanLineGouraudNoZBufferZWrite_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
extern void DrawScanLineGouraudNoZBufferZWriteTrans_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//solid color
extern void DrawScanLineGouraudNoZBufferZWriteSolid_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);

//DRV_RENDER_NO_ZMASK | DRV_RENDER_NO_ZWRITE
extern void DrawScanLineGouraudNoZ_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
extern void DrawScanLineGouraudNoZTrans_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//solid color
extern void DrawScanLineGouraudNoZSolid_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);

//DRV_RENDER_NO_ZMASK | DRV_RENDER_NO_ZWRITE | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudNoZAlphaTex_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_NO_ZMASK | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudNoZBufferZWriteAlphaTex_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_NO_ZMASK | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudZBufferNoZWriteAlphaTex_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_ALPHA
extern void DrawScanLineGouraudZBufferAlphaTex_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);

//DRV_RENDER_NO_ZMASK | DRV_RENDER_NO_ZWRITE | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudNoZAlphaARGB_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_NO_ZMASK | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudNoZBufferZWriteAlphaARGB_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_NO_ZMASK | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudZBufferNoZWriteAlphaARGB_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_ALPHA
extern void DrawScanLineGouraudZBufferAlphaARGB_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);

//!(DRV_RENDER_NO_ZMASK | DRV_RENDER_NO_ZWRITE)
extern void DrawScanLineGouraudZBuffer_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
extern void DrawScanLineGouraudZBufferTrans_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//solid color
extern void DrawScanLineGouraudZBufferSolid_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);

//DRV_RENDER_NO_ZWRITE
extern void DrawScanLineGouraudZBufferNoZWrite_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
extern void DrawScanLineGouraudZBufferNoZWriteTrans_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);
//solid color
extern void DrawScanLineGouraudZBufferNoZWriteSolid_Asm3DNow(EdgeAsm *pLeft, EdgeAsm *pRight);


//world perspective correct faces
extern	void DrawSpan32_AsmLit3DNow(int32 x1, int32 x2, int32 y);
extern	void DrawSpan32_AsmLitZWrite3DNow(int32 x1, int32 x2, int32 y);
extern	void DrawSpan32_AsmLitZBuffer3DNow(int32 x1, int32 x2, int32 y);
extern	void DrawSpan32_AsmGouraud3DNow(int32 x1, int32 x2, int32 y, float r1, float g1, float b1, float r2, float g2, float b2);
extern	void DrawSpan32_AsmGouraudZWrite3DNow(int32 x1, int32 x2, int32 y, float r1, float g1, float b1, float r2, float g2, float b2);
extern	void DrawSpan32_AsmGouraudZBuffer3DNow(int32 x1, int32 x2, int32 y, float r1, float g1, float b1, float r2, float g2, float b2);
extern	void DrawSpan32_AsmGouraudZBufferAlphaARGB3DNow(int32 x1, int32 x2, int32 y, float r1, float g1, float b1, float r2, float g2, float b2);
extern	void DrawSpan32_AsmGouraudZBufferVertexAlpha3DNow(int32 x1, int32 x2, int32 y, float r1, float g1, float b1, float r2, float g2, float b2);

typedef struct	EdgeAsmWorldTag	EdgeAsmWorld;

extern	void Femms3DNow(void);
extern	void StepWorld3DNow(EdgeAsmWorld *edge);