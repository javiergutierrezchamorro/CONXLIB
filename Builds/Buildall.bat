@ECHO OFF
copy /y d:\proyectos\coniox\coniox.c
copy /y d:\proyectos\coniox\coniox.h
goto oc20

:vc2017
rem *** VISUAL C++ 2017 ***
SET MSC="D:\Archiv~1\Visual Studio\VC\Tools\MSVC\14.14.26428\"
SET OLDPATH=%PATH%
SET PATH=%MSC%\BIN\Hostx64\x64;%PATH%
SET INCLUDE=%MSC%\INCLUDE;
SET LIB=%MSC%\LIB;
DEL coniox_ms32w.exe
REM cl /O2 /Og /Ob2 /Oi /Ot /Oy /GT /G6 /GA /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MEMTRACE=5" /GF /FD /ML /Zp4 /Gy /Fo"Release/" /Fd"Release/vc70.pdb" /W4 /nologo /c /Gr /TP ..\coniodemo.c ..\coniox.c
ren coniodemo.exe coniox_ms32w.exe
SET MSC=
SET PATH=%OLDPATH%
SET INCLUDE=
SET LIB=

:bc502
SET BORLAND=D:\FICHEROS\BCW\
SET OLDPATH=%PATH%
SET PATH=%BORLAND%\BIN;%PATH%
SET INCLUDE=%BORLAND%\INCLUDE;..\
SET LIB=%BORLAND%\LIB;
DEL coniox_bc16d.exe
bcc -1- -a2 -d -ff -k- -ms -po -RT- -O2 -I%INCLUDE% -L%LIB% ..\coniodemo.c ..\coniox.c
ren coniodemo.exe coniox_bc16d.exe
SET BORLAND=
SET PATH=%OLDPATH%
SET INCLUDE=
SET LIB=


:dmc857
rem *** DIGITALMARS C++ 8.57 ***
SET ZORTECH=D:\FICHEROS\DM
SET OLDPATH=%PATH%
SET PATH=%ZORTECH%\BIN;%PATH%
SET INCLUDE=%ZORTECH%\INCLUDE;..\
SET LIB=%ZORTECH%\LIB;
DEL coniox_zc16d.exe
sc ..\coniodemo.c ..\coniox.c -mt -o+time -ff -0 -a4 -Nc
ren coniodemo.exe coniox_zc16d.exe
DEL coniox_zc32d.exe
sc ..\coniodemo.c ..\coniox.c -mx -o+time -ff -3 -a8 -Nc
ren coniodemo.exe coniox_zc32d.exe
DEL coniox_zc32w.exe
sc ..\coniodemo.c ..\coniox.c -mn -o+time -ff -6 -a8 -Nc
ren coniodemo.exe coniox_zc32w.exe
SET ZORTECH=
SET PATH=%OLDPATH%
SET INCLUDE=
SET LIB=


:oc20
rem *** OPENWATCOM C++ 2.0 *** 
SET WATCOM=D:\FICHEROS\OC
SET OLDPATH=%PATH%
SET PATH=%WATCOM%\BINNT;%WATCOM%\BINW;%WATCOM%\BINP;%PATH%
SET EDPATH=%WATCOM%\EDDAT
SET INCLUDE=%WATCOM%\H;%WATCOM%\MFC\INCLUDE;%WATCOM%\H\NT;INCLUDE;
SET LIB=LIB;
rem *** Win32 ***
rem DEL coniox_ocw32.exe
rem WCL386 -w3 -oneatx -ohirbk -ol -ol+ -oi -ei -zp16 -6r  -fpi87 -fp6 -mf -s -ri -zm /bt=nt /l=nt /fhwe /"OPTION ELIMINATE" /"OPTION VFREMOVAL" ..\coniodemo.c ..\coniox.c ..\conxlib.c
REN coniodemo.exe coniox_ocw32.exe
rem *** DOS16 ***
DEL conxdemo_ocd16.com
WCL -w3 -oneatx -ohirbk -ol -ol+ -oi -ei -zp4 -0 -mt -s -ri -zm /bt=dos /l=com /fhwe /"OPTION ELIMINATE" /"OPTION VFREMOVAL" ..\conxdemo.c ..\coniox.c ..\conxlib.c
REN coniodemo.com coniox_ocd16.com
rem *** DOS32 ***
DEL conxdemo_ocd32.exe
WCL386 -w3 -oneatx -ohirbk -ol -ol+ -oi -ei -zp16 -6r -fpi87 -fp6 -mf -s -ri -zm /bt=dos /l=pmodew /fhwe /"OPTION ELIMINATE" /"OPTION VFREMOVAL" ..\conxdemo.c ..\coniox.c ..\conxlib.c
REN coniodemo.exe coniox_ocd32.exe

SET WATCOM=
SET PATH=%OLDPATH%
SET EDPATH=
SET INCLUDE=
SET LIB=

:FIN
DEL *.obj
DEL *.pch
DEL *.tds
DEL *.bak
DEL *.err
DEL *.map
DEL *.sym