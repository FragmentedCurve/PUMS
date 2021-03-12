@echo off
rem mnumaker.bat, By Israel Baumel, November 1989
rem
rem calling sequence: mnumaker Applic_Id Entries_Screen CSCScscs
rem
rem mnumaker.bat will
rem              create a fixed format raw menu file by scanning all
rem                   application id sub directories
rem              sort the raw menu file (APID0000.MNU)
rem                   into different sort sequences
rem              read the sort output to create a selection menu object set
rem                   named APID000X.D01-D99 (X is sort identification)
rem

cls
echo Creating the Selection Menu Object Files. Please Wait...
makemenu %1
echo.

rem Sort by SINn (reversed)
sort /R /+21 < %1\%10000.MNU > %1\%1000S.MNU
chopmenu %1 S %2 %3

rem Sort by Title(1)
sort /+42 < %1\%10000.MNU > %1\%1000T.MNU
chopmenu %1 T %2 %3

rem Sort by Version Date
sort /+3 < %1\%10000.MNU > %1\%1000D.MNU
chopmenu %1 D %2 %3

rem Sort by Zipfile Name
sort /+90 < %1\%10000.MNU > %1\%1000Z.MNU
chopmenu %1 Z %2 %3

rem Sort by Version
sort /+12 < %1\%10000.MNU > %1\%1000V.MNU
chopmenu %1 V %2 %3
@echo on
