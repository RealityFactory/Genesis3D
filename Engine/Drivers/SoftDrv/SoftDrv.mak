# Microsoft Developer Studio Generated NMAKE File, Based on SoftDrv.dsp
!IF "$(CFG)" == ""
CFG=SoftDrv - Win32 Debug
!MESSAGE No configuration specified. Defaulting to SoftDrv - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "SoftDrv - Win32 Release" && "$(CFG)" != "SoftDrv - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SoftDrv.mak" CFG="SoftDrv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SoftDrv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SoftDrv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "SoftDrv - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\SoftDrv.dll"


CLEAN :
	-@erase "$(INTDIR)\dmodes.obj"
	-@erase "$(INTDIR)\drawspan.obj"
	-@erase "$(INTDIR)\register.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\scene.obj"
	-@erase "$(INTDIR)\softdrv.obj"
	-@erase "$(INTDIR)\span.obj"
	-@erase "$(INTDIR)\system.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\Vec3d.obj"
	-@erase "$(INTDIR)\W32sal.obj"
	-@erase "$(INTDIR)\x86span555.obj"
	-@erase "$(INTDIR)\x86span565.obj"
	-@erase "$(INTDIR)\Xform3d.obj"
	-@erase "$(OUTDIR)\SoftDrv.dll"
	-@erase "$(OUTDIR)\SoftDrv.exp"
	-@erase "$(OUTDIR)\SoftDrv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\..\.." /I "..\..\..\Support" /I "..\\" /I "..\..\..\Math" /I "..\..\..\Bitmap" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\..\Sdk\Dx6SDK\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV_EXPORTS" /Fp"$(INTDIR)\SoftDrv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SoftDrv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:no /pdb:"$(OUTDIR)\SoftDrv.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\SoftDrv.dll" /implib:"$(OUTDIR)\SoftDrv.lib" 
LINK32_OBJS= \
	"$(INTDIR)\dmodes.obj" \
	"$(INTDIR)\drawspan.obj" \
	"$(INTDIR)\register.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\scene.obj" \
	"$(INTDIR)\softdrv.obj" \
	"$(INTDIR)\span.obj" \
	"$(INTDIR)\system.obj" \
	"$(INTDIR)\Vec3d.obj" \
	"$(INTDIR)\W32sal.obj" \
	"$(INTDIR)\x86span555.obj" \
	"$(INTDIR)\x86span565.obj" \
	"$(INTDIR)\Xform3d.obj" \
	"..\..\..\..\MSDev60\lib\Winspool.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Libcmt.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\..\..\MSDev60\lib\Winmm.lib" \
	".\amdspan.obj"

"$(OUTDIR)\SoftDrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "SoftDrv - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\SoftDrv.dll"


CLEAN :
	-@erase "$(INTDIR)\dmodes.obj"
	-@erase "$(INTDIR)\drawspan.obj"
	-@erase "$(INTDIR)\register.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\scene.obj"
	-@erase "$(INTDIR)\softdrv.obj"
	-@erase "$(INTDIR)\span.obj"
	-@erase "$(INTDIR)\system.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\Vec3d.obj"
	-@erase "$(INTDIR)\W32sal.obj"
	-@erase "$(INTDIR)\x86span555.obj"
	-@erase "$(INTDIR)\x86span565.obj"
	-@erase "$(INTDIR)\Xform3d.obj"
	-@erase "$(OUTDIR)\SoftDrv.dll"
	-@erase "$(OUTDIR)\SoftDrv.exp"
	-@erase "$(OUTDIR)\SoftDrv.ilk"
	-@erase "$(OUTDIR)\SoftDrv.lib"
	-@erase "$(OUTDIR)\SoftDrv.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\..\.." /I "..\..\..\Support" /I "..\\" /I "..\..\..\Math" /I "..\..\..\Bitmap" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\..\Sdk\Dx6SDK\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV_EXPORTS" /Fp"$(INTDIR)\SoftDrv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SoftDrv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:yes /pdb:"$(OUTDIR)\SoftDrv.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\SoftDrv.dll" /implib:"$(OUTDIR)\SoftDrv.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\dmodes.obj" \
	"$(INTDIR)\drawspan.obj" \
	"$(INTDIR)\register.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\scene.obj" \
	"$(INTDIR)\softdrv.obj" \
	"$(INTDIR)\span.obj" \
	"$(INTDIR)\system.obj" \
	"$(INTDIR)\Vec3d.obj" \
	"$(INTDIR)\W32sal.obj" \
	"$(INTDIR)\x86span555.obj" \
	"$(INTDIR)\x86span565.obj" \
	"$(INTDIR)\Xform3d.obj" \
	"..\..\..\..\MSDev60\lib\Winspool.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Libcmtd.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\..\..\MSDev60\lib\Winmm.lib" \
	".\amdspan.obj"

"$(OUTDIR)\SoftDrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("SoftDrv.dep")
!INCLUDE "SoftDrv.dep"
!ELSE 
!MESSAGE Warning: cannot find "SoftDrv.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "SoftDrv - Win32 Release" || "$(CFG)" == "SoftDrv - Win32 Debug"
SOURCE=.\amdspan.asm

!IF  "$(CFG)" == "SoftDrv - Win32 Release"

InputPath=.\amdspan.asm

".\amdspan.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	..\..\..\..\masm\bin\ml -c -I ..\..\..\..\masm\include -coff amdspan.asm
<< 
	

!ELSEIF  "$(CFG)" == "SoftDrv - Win32 Debug"

InputPath=.\amdspan.asm

".\amdspan.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	..\..\..\..\masm\bin\ml -c -I ..\..\..\..\masm\include -coff amdspan.asm
<< 
	

!ENDIF 

SOURCE=.\dmodes.c

"$(INTDIR)\dmodes.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\drawspan.c

"$(INTDIR)\drawspan.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\register.c

"$(INTDIR)\register.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\render.c

"$(INTDIR)\render.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\scene.c

"$(INTDIR)\scene.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\softdrv.c

"$(INTDIR)\softdrv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\span.c

"$(INTDIR)\span.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\system.c

"$(INTDIR)\system.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\..\Math\Vec3d.c

"$(INTDIR)\Vec3d.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\W32sal.cpp

"$(INTDIR)\W32sal.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\x86span555.c

"$(INTDIR)\x86span555.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\x86span565.c

"$(INTDIR)\x86span565.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\..\Math\Xform3d.c

"$(INTDIR)\Xform3d.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

