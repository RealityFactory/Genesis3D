# Microsoft Developer Studio Generated NMAKE File, Based on D3DDrv.dsp
!IF "$(CFG)" == ""
CFG=D3DDrv - Win32 Debug
!MESSAGE No configuration specified. Defaulting to D3DDrv - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "D3DDrv - Win32 Release" && "$(CFG)" != "D3DDrv - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "D3DDrv.mak" CFG="D3DDrv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "D3DDrv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "D3DDrv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "D3DDrv - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\D3DDrv.dll"


CLEAN :
	-@erase "$(INTDIR)\D3d_err.obj"
	-@erase "$(INTDIR)\D3d_fx.obj"
	-@erase "$(INTDIR)\D3d_main.obj"
	-@erase "$(INTDIR)\D3dcache.obj"
	-@erase "$(INTDIR)\D3ddrv.obj"
	-@erase "$(INTDIR)\DDMemMgr.obj"
	-@erase "$(INTDIR)\Gspan.obj"
	-@erase "$(INTDIR)\Pcache.obj"
	-@erase "$(INTDIR)\Render.obj"
	-@erase "$(INTDIR)\Scene.obj"
	-@erase "$(INTDIR)\THandle.obj"
	-@erase "$(INTDIR)\tpage.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\D3DDrv.dll"
	-@erase "$(OUTDIR)\D3DDrv.exp"
	-@erase "$(OUTDIR)\D3DDrv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\..\.." /I "..\..\..\..\SdkDx6Sdk\Include" /I "..\\" /I "..\..\..\..\Sdk\Dx6Sdk\Include" /I "..\D3DDrv" /I "..\..\..\Support" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\Math" /I "..\..\..\Bitmap" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "D3DDRV_EXPORTS" /Fp"$(INTDIR)\D3DDrv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\D3DDrv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:no /pdb:"$(OUTDIR)\D3DDrv.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\D3DDrv.dll" /implib:"$(OUTDIR)\D3DDrv.lib" 
LINK32_OBJS= \
	"$(INTDIR)\D3d_err.obj" \
	"$(INTDIR)\D3d_fx.obj" \
	"$(INTDIR)\D3d_main.obj" \
	"$(INTDIR)\D3dcache.obj" \
	"$(INTDIR)\D3ddrv.obj" \
	"$(INTDIR)\DDMemMgr.obj" \
	"$(INTDIR)\Gspan.obj" \
	"$(INTDIR)\Pcache.obj" \
	"$(INTDIR)\Render.obj" \
	"$(INTDIR)\Scene.obj" \
	"$(INTDIR)\THandle.obj" \
	"$(INTDIR)\tpage.obj" \
	"..\..\..\..\MSDev60\lib\Wininet.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Libcmt.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\dxguid.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\ddraw.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\d3dim.lib"

"$(OUTDIR)\D3DDrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "D3DDrv - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\D3DDrv.dll"


CLEAN :
	-@erase "$(INTDIR)\D3d_err.obj"
	-@erase "$(INTDIR)\D3d_fx.obj"
	-@erase "$(INTDIR)\D3d_main.obj"
	-@erase "$(INTDIR)\D3dcache.obj"
	-@erase "$(INTDIR)\D3ddrv.obj"
	-@erase "$(INTDIR)\DDMemMgr.obj"
	-@erase "$(INTDIR)\Gspan.obj"
	-@erase "$(INTDIR)\Pcache.obj"
	-@erase "$(INTDIR)\Render.obj"
	-@erase "$(INTDIR)\Scene.obj"
	-@erase "$(INTDIR)\THandle.obj"
	-@erase "$(INTDIR)\tpage.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\D3DDrv.dll"
	-@erase "$(OUTDIR)\D3DDrv.exp"
	-@erase "$(OUTDIR)\D3DDrv.ilk"
	-@erase "$(OUTDIR)\D3DDrv.lib"
	-@erase "$(OUTDIR)\D3DDrv.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /GX /Zi /Od /X /I "..\..\.." /I "..\..\..\Math" /I "..\\" /I "..\..\..\..\Sdk\Dx6Sdk\Include" /I "..\D3DDrv" /I "..\..\..\Support" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\Bitmap" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "D3DDRV_EXPORTS" /Fp"$(INTDIR)\D3DDrv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\D3DDrv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:yes /pdb:"$(OUTDIR)\D3DDrv.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\D3DDrv.dll" /implib:"$(OUTDIR)\D3DDrv.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\D3d_err.obj" \
	"$(INTDIR)\D3d_fx.obj" \
	"$(INTDIR)\D3d_main.obj" \
	"$(INTDIR)\D3dcache.obj" \
	"$(INTDIR)\D3ddrv.obj" \
	"$(INTDIR)\DDMemMgr.obj" \
	"$(INTDIR)\Gspan.obj" \
	"$(INTDIR)\Pcache.obj" \
	"$(INTDIR)\Render.obj" \
	"$(INTDIR)\Scene.obj" \
	"$(INTDIR)\THandle.obj" \
	"$(INTDIR)\tpage.obj" \
	"..\..\..\..\MSDev60\lib\Wininet.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Libcmtd.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\dxguid.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\ddraw.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\d3dim.lib"

"$(OUTDIR)\D3DDrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("D3DDrv.dep")
!INCLUDE "D3DDrv.dep"
!ELSE 
!MESSAGE Warning: cannot find "D3DDrv.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "D3DDrv - Win32 Release" || "$(CFG)" == "D3DDrv - Win32 Debug"
SOURCE=.\D3d_err.cpp

"$(INTDIR)\D3d_err.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\D3d_fx.cpp

"$(INTDIR)\D3d_fx.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\D3d_main.cpp

"$(INTDIR)\D3d_main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\D3dcache.cpp

"$(INTDIR)\D3dcache.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\D3ddrv.cpp

"$(INTDIR)\D3ddrv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DDMemMgr.c

"$(INTDIR)\DDMemMgr.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Gspan.cpp

"$(INTDIR)\Gspan.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Pcache.cpp

"$(INTDIR)\Pcache.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Render.cpp

"$(INTDIR)\Render.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Scene.cpp

"$(INTDIR)\Scene.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\THandle.cpp

"$(INTDIR)\THandle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tpage.cpp

"$(INTDIR)\tpage.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

