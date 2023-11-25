@ECHO OFF
SET EMU_PATH=C:\Games\blastem-win64-0.6.3-pre-9e6cb50d0639\blastem.exe
%GDK_WIN%\bin\make -f %GDK_WIN%\makefile.gen && %EMU_PATH% out\rom.bin
EXIT /B
