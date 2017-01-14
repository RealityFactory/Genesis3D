# Microsoft Developer Studio Generated NMAKE File, Based on GlideDrv.dsp
!IF "$(CFG)" == ""
CFG=GlideDrv - Win32 Debug
!MESSAGE No configuration specified. Defaulting to GlideDrv - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "GlideDrv - Win32 Release" && "$(CFG)" != "GlideDrv - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GlideDrv.mak" CFG="GlideDrv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GlideDrv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GlideDrv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "GlideDrv - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\GlideDrv.dll"


CLEAN :
	-@erase "$(INTDIR)\Bmp.obj"
	-@erase "$(INTDIR)\GCache.obj"
	-@erase "$(INTDIR)\GlideDrv.obj"
	-@erase "$(INTDIR)\GMain.obj"
	-@erase "$(INTDIR)\GMemMgr.obj"
	-@erase "$(INTDIR)\GSpan.obj"
	-@erase "$(INTDIR)\GThandle.obj"
	-@erase "$(INTDIR)\Render.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\GlideDrv.dll"
	-@erase "$(OUTDIR)\GlideDrv.exp"
	-@erase "$(OUTDIR)\GlideDrv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\..\..\Support" /I "..\\" /I "..\..\..\Math" /I "..\..\..\Bitmap" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\..\Sdk\Glide\Include" /I "..\..\..\\" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLIDEDRV_EXPORTS" /D "__MSC__" /Fp"$(INTDIR)\GlideDrv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GlideDrv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:no /pdb:"$(OUTDIR)\GlideDrv.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\GlideDrv.dll" /implib:"$(OUTDIR)\GlideDrv.lib" 
LINK32_OBJS= \
	"$(INTDIR)\Bmp.obj" \
	"$(INTDIR)\GCache.obj" \
	"$(INTDIR)\GlideDrv.obj" \
	"$(INTDIR)\GMain.obj" \
	"$(INTDIR)\GMemMgr.obj" \
	"$(INTDIR)\GSpan.obj" \
	"$(INTDIR)\GThandle.obj" \
	"$(INTDIR)\Render.obj" \
	"..\..\..\..\Sdk\Glide\Lib\glide2x.lib" \
	"..\..\..\..\MSDev60\lib\Libcmt.lib" \
	"..\..\..\..\MSDev60\lib\Winspool.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib"

"$(OUTDIR)\GlideDrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "GlideDrv - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\GlideDrv.dll"


CLEAN :
	-@erase "$(INTDIR)\Bmp.obj"
	-@erase "$(INTDIR)\GCache.obj"
	-@erase "$(INTDIR)\GlideDrv.obj"
	-@erase "$(INTDIR)\GMain.obj"
	-@erase "$(INTDIR)\GMemMgr.obj"
	-@erase "$(INTDIR)\GSpan.obj"
	-@erase "$(INTDIR)\GThandle.obj"
	-@erase "$(INTDIR)\Render.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\GlideDrv.dll"
	-@erase "$(OUTDIR)\GlideDrv.exp"
	-@erase "$(OUTDIR)\GlideDrv.ilk"
	-@erase "$(OUTDIR)\GlideDrv.lib"
	-@erase "$(OUTDIR)\GlideDrv.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\Support" /I "..\\" /I "..\..\..\Math" /I "..\..\..\Bitmap" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\..\Sdk\Glide\Include" /I "..\..\..\\" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLIDEDRV_EXPORTS" /D "__MSC__" /Fp"$(INTDIR)\GlideDrv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GlideDrv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:yes /pdb:"$(OUTDIR)\GlideDrv.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\GlideDrv.dll" /implib:"$(OUTDIR)\GlideDrv.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Bmp.obj" \
	"$(INTDIR)\GCache.obj" \
	"$(INTDIR)\GlideDrv.obj" \
	"$(INTDIR)\GMain.obj" \
	"$(INTDIR)\GMemMgr.obj" \
	"$(INTDIR)\GSpan.obj" \
	"$(INTDIR)\GThandle.obj" \
	"$(INTDIR)\Render.obj" \
	"..\..\..\..\Sdk\Glide\Lib\glide2x.lib" \
	"..\..\..\..\MSDev60\lib\Libcmtd.lib" \
	"..\..\..\..\MSDev60\lib\Winspool.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib"

"$(OUTDIR)\GlideDrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("GlideDrv.dep")
!INCLUDE "GlideDrv.dep"
!ELSE 
!MESSAGE Warning: cannot find "GlideDrv.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "GlideDrv - Win32 Release" || "$(CFG)" == "GlideDrv - Win32 Debug"
SOURCE=..\Bmp.c

"$(INTDIR)\Bmp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\GCache.c

"$(INTDIR)\GCache.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\GlideDrv.c

"$(INTDIR)\GlideDrv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\GMain.c

"$(INTDIR)\GMain.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\GMemMgr.c

"$(INTDIR)\GMemMgr.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\GSpan.cpp

"$(INTDIR)\GSpan.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\GThandle.c

"$(INTDIR)\GThandle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Render.c

"$(INTDIR)\Render.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

