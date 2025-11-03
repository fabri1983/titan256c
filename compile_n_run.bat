@ECHO OFF

:: Eg:
:: compile_n_run.bat release --no-emu

SET "target=%1"
IF NOT "%target%" == "release" IF NOT "%target%" == "debug" IF NOT "%target%" == "clean" IF NOT "%target%" == "asm" (
	ECHO Please provide a valid target: release, debug, clean, asm
    PAUSE
    GOTO :END
)

:: Edit accordingly
SET "EMU_PATH=D:\Games\retro\blastem-win64-0.6.3-pre-9c105d94ad6f\blastem.exe"

IF NOT "%2" == "--no-emu" IF NOT EXIST %EMU_PATH% (
    ECHO Please set blastem path or use option --no-emu
    PAUSE
    GOTO :END
)

IF "%GDK_WIN%" == "" (
    ECHO Please set GDK_WIN env path
    PAUSE
    GOTO :END
)

%GDK_WIN%\bin\make -f %GDK_WIN%\makefile.gen %target% && IF NOT "%2" == "--no-emu" START "" /B %EMU_PATH% out\rom.bin

:END
EXIT /B