/****************************************************************************************/
/*  x86span555.h                                                                        */
/*                                                                                      */
/*  Author:       Ken Baird                                                             */
/*  Description:  header for x86 renderstates                                           */
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

//x86 fpu based non world faces 555
//DRV_RENDER_NO_ZMASK
extern void DrawScanLineGouraudNoZBufferZWrite_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
extern void DrawScanLineGouraudNoZBufferZWriteTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
//solid color
extern void DrawScanLineGouraudNoZBufferZWriteSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);

//DRV_RENDER_NO_ZMASK | DRV_RENDER_NO_ZWRITE
extern void DrawScanLineGouraudNoZ_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
extern void DrawScanLineGouraudNoZTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
//solid color
extern void DrawScanLineGouraudNoZSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);

//DRV_RENDER_NO_ZMASK | DRV_RENDER_NO_ZWRITE | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudNoZAlphaTex_Asm555X86FPU(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_NO_ZMASK | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudNoZBufferZWriteAlphaTex_Asm555X86FPU(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_NO_ZMASK | DRV_RENDER_ALPHA
extern void DrawScanLineGouraudZBufferNoZWriteAlphaTex_Asm555X86FPU(EdgeAsm *pLeft, EdgeAsm *pRight);
//DRV_RENDER_ALPHA
extern void DrawScanLineGouraudZBufferAlphaTex_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);

//!(DRV_RENDER_NO_ZMASK | DRV_RENDER_NO_ZWRITE)
extern void DrawScanLineGouraudZBuffer_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
extern void DrawScanLineGouraudZBufferTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
//solid color
extern void DrawScanLineGouraudZBufferSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);

//DRV_RENDER_NO_ZWRITE
extern void DrawScanLineGouraudZBufferNoZWrite_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
extern void DrawScanLineGouraudZBufferNoZWriteTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);
//solid color
extern void DrawScanLineGouraudZBufferNoZWriteSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight);


//world perspective correct faces
void	DrawSpan16_AsmLit555X86FPU(int32 x1, int32 x2, int32 y);
void	DrawSpan16_AsmLitZWrite555X86FPU(int32 x1, int32 x2, int32 y);
void	DrawSpan16_AsmLitZBuffer555X86FPU(int32 x1, int32 x2, int32 y);
void	DrawSpan16_AsmGouraud555X86FPU(int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2);
void	DrawSpan16_AsmGouraudZWrite555X86FPU(int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2);
void	DrawSpan16_AsmGouraudZBuffer555X86FPU(int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2);
void	DrawSpan16_AsmGouraudZBufferTrans555X86FPU(int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2);
