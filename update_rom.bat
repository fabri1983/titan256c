@ECHO OFF

SET DEST_ROM=titan256c_rom.bin

SET "OUT_ROM_PATH=out\rom.bin"
IF NOT EXIST %OUT_ROM_PATH% (
    ECHO Please compile first to create %OUT_ROM_PATH%
    PAUSE
    GOTO :END
)

DEL /F /Q %DEST_ROM% >NUL 2>&1
COPY %OUT_ROM_PATH% .\%DEST_ROM%

:END
EXIT /B