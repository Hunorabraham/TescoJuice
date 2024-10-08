D:\msys64\usr\bin\env MSYSTEM=UCRT64 /usr/bin/bash -lc D:\\repos\\TJGNU\\buildAndRunInternal.bat
cd %~dp0\build
@ECHO OFF
:start
SET /p choice=run the game? [Y/N]: 
IF NOT '%choice%'=='' SET choice=%choice:~0,1%
IF '%choice%'=='Y' GOTO yes
IF '%choice%'=='y' GOTO yes
IF '%choice%'=='N' GOTO no
IF '%choice%'=='n' GOTO no
IF '%choice%'=='' GOTO yes
ECHO "%choice%" is not valid
ECHO.
GOTO start

:no
EXIT

:yes
out
EXIT
