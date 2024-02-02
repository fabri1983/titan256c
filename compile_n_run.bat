@ECHO OFF

SET "EMU_PATH=C:\Games\blastem-win64-0.6.3-pre-cde4ea2b4929\blastem.exe"

IF NOT "%1" == "--no-emu" IF NOT EXIST %EMU_PATH% (
    ECHO Please set blastem path or use option --no-emu
    PAUSE
    GOTO :END
)

IF "%GDK_WIN%" == "" (
    ECHO Please set GDK_WIN env path
    PAUSE
    GOTO :END
)

%GDK_WIN%\bin\make -f %GDK_WIN%\makefile.gen && IF NOT "%1" == "--no-emu" %EMU_PATH% out\rom.bin

:END
EXIT /B