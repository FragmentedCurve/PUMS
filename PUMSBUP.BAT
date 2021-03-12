@echo off
rem pumsbup.bat, By Israel Baumel, November 1989
rem
rem calling sequence: pumsbup source target
rem
rem target will be "tape" for future implementation. Otherwise it is
rem a floppy drive letter.
rem
rem source is a directory name.
rem
cls
if %2 == tape goto tape
backup %1 %2 /S /F
goto exit

:tape
echo Tape Backup not Implemented.

:exit
