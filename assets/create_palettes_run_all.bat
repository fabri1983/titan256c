:: create_palettes_run_all.bat titan_*.png <true\false>
:: Kill them all: open new cmd and run: taskkill /f /fi "imagename eq aseprite.exe"
@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET "IMAGES_PATTERN=%1"
SET "TARGET_PATH=%~p1"
IF "%TARGET_PATH%" == "" SET TARGET_PATH=.
:: true if you want to execute single instance process per image
SET "SINGLE_INSTANCE=%2"

ECHO 111111111111111111111111111111111111111111111
ECHO Creating images with RGB palette ...
ECHO 111111111111111111111111111111111111111111111
ECHO:

:: Get every image matching the pattern and run the LUA script with ID_PROC 0 to get the number of permutations per proc
:: If SINGLE_INSTANCE = true then it will directly generate the RGB palettes, no proc ids will be created
FOR %%f IN (%IMAGES_PATTERN%) DO (
  SET "filename=%%~nf"
  SET "extension=%%~xf"
  :: Extract the part of the filename after the first underscore
  SET "suffix=!filename:*_=!"
  REM Check if the suffix contains only numbers (with optional underscore). Valid ones: 0, 12, 0_0, 0_25, 12_1
  REM IMPORTANT: the pipe connector | must be free of any blank spaces around it
  REM IMPORTANT: findstr regular expression only accept * as repetition meta character
  ECHO !suffix!|findstr /x /r "^[0-9][0-9]*[_0-9]*$" >NUL && (
    aseprite -b %%f -script-param ID_PROC=0 -script-param SINGLE_INSTANCE=%SINGLE_INSTANCE% --script create_palettes_for_md.lua
  )
)

:: Get every file matching the pattern for permutations and run the executor script
IF NOT "%SINGLE_INSTANCE%" == "true" (
  FOR %%f IN (%IMAGES_PATTERN%) DO (
    SET "filename=%%~nf"
    SET "procs_file=%TARGET_PATH%\!filename!.procs"
    IF NOT EXIST !procs_file! (
      ECHO Not found: !procs_file!
    ) ELSE (
      SET /p INSTANCES=<"!procs_file!" >NUL 2>&1
      :: Eg: create_palettes_executor.bat titan_0.png 22
      CALL create_palettes_executor.bat %%f !INSTANCES! 2>&1
      :: wait 7 ms
      ping -n 1 -w 7 127.0.0.1 >NUL
      DEL /F /Q !procs_file! >NUL 2>&1
    )
  )
)

ECHO:
ECHO 111111111111111111111111111111111111111111111
ECHO Finished palette creation process.
ECHO 111111111111111111111111111111111111111111111
ECHO:
ECHO 111111111111111111111111111111111111111111111
ECHO Copying failed images into failed folder ...
ECHO 111111111111111111111111111111111111111111111
ECHO:

:: Prepare failed images folder
SET FAILED_FOLDER=%CD%\failed
RMDIR /S /Q %FAILED_FOLDER% 2>NUL
MD %FAILED_FOLDER%

:: collect failed images into faild folder
FOR %%f IN (%IMAGES_PATTERN%) DO (
  SET "filename=%%~nf"
  SET "extension=%%~xf"
  SET "folder=%%~pf"
  :: Extract the part of the filename after the underscore
  SET "suffix=!filename:*_=!"
  :: Check if the suffix contains numbers and not RGB. IMPORTANT: the pipe connector | must be free of any blank spaces around it
  ECHO !suffix!|findstr /xr "[1-9][0-9]* 0"|findstr /V "_RGB" >NUL && (
    SET "rgbFile=!filename!_RGB!extension!"
    IF NOT EXIST "!folder!\!rgbFile!" (
      COPY /Y "%%f" "!FAILED_FOLDER!\" >NUL
    )
  )
)

ECHO:
ECHO 111111111111111111111111111111111111111111111
ECHO Finished. Failed images copied into %FAILED_FOLDER%
ECHO 111111111111111111111111111111111111111111111
ECHO:
ECHO 111111111111111111111111111111111111111111111
ECHO Copying RGB images into rgb folder ...
ECHO 111111111111111111111111111111111111111111111
ECHO:

:: Prepare RGBs folder
SET RGB_FOLDER=%CD%\res\rgb
MD %RGB_FOLDER% 2>NUL

:: collect RGB images into RGB_FOLDER folder
FOR %%f IN (%IMAGES_PATTERN%) DO (
  SET "filename=%%~nf"
  SET "extension=%%~xf"
  SET "folder=%%~pf"
  :: Extract the part of the filename after the underscore
  SET "suffix=!filename:*_=!"
  :: Check if the suffix contains numbers and RGB. IMPORTANT: the pipe connector | must be free of any blank spaces around it
  ECHO !suffix!|findstr /xr "[1-9][0-9]*_RGB 0" >NUL && (
    SET "rgbFile=!filename!_RGB!extension!"
    IF NOT EXIST "!folder!\!rgbFile!" (
      COPY /Y "%%f" "!RGB_FOLDER!\" >NUL
    )
  )
)

ECHO:
ECHO 111111111111111111111111111111111111111111111
ECHO Finished. RGB images copied into %RGB_FOLDER%
ECHO 111111111111111111111111111111111111111111111
ECHO:

EXIT /B 0