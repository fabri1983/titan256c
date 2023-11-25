@ECHO OFF

SET "EMU_PATH=C:\Games\blastem-win64-0.6.3-pre-9e6cb50d0639\blastem.exe"
IF NOT EXIST %EMU_PATH% (
    ECHO Please set blastem path
    PAUSE
    GOTO :END
)

IF "%GDK_WIN%" == "" (
    ECHO Please set GDK_WIN env path
    PAUSE
    GOTO :END
)

%GDK_WIN%\bin\make -f %GDK_WIN%\makefile.gen && %EMU_PATH% out\rom.bin

:END
EXIT /B