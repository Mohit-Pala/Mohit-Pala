@echo off
rem Builds Release (build.bat) and packs a zip Definitive Mod Manager can import:
rem   dist\TraversalMaster-DMM.zip  ->  bin64\TraversalMaster.asi + bin64\TraversalMaster.ini
setlocal
set "HERE=%~dp0"
call "%HERE%build.bat" || exit /b 1

set "STAGE=%HERE%dist\package"
if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%\bin64"
copy /y "%HERE%dist\TraversalMaster.asi" "%STAGE%\bin64\" >nul || exit /b 1
copy /y "%HERE%TraversalMaster.ini" "%STAGE%\bin64\" >nul || exit /b 1

if exist "%HERE%dist\TraversalMaster-DMM.zip" del "%HERE%dist\TraversalMaster-DMM.zip"
powershell -NoProfile -Command "Compress-Archive -Path '%STAGE%\*' -DestinationPath '%HERE%dist\TraversalMaster-DMM.zip'" || exit /b 1
echo.
echo -> %HERE%dist\TraversalMaster-DMM.zip
endlocal
