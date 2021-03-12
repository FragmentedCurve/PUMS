echo off
rem
rem zipchop.bat, By Israel Baumel, November 1989
rem
rem ZIPCHOP.BAT pkzip download package into self extract file and chop
rem into object set.
rem
rem calling sequence: zipchop App-ID CSCScscs SINn chop_size lines chars
rem
rem App-id    - 4 character identifier of the application directory
rem CSCScscs  - 4 char CSCS or 8 char HEX string for CSCScscs
rem SINn      - 4 char SINn of this package
rem chop_size - block size to chop zipfile into
rem lines     - Lines In Screen of Description
rem chars     - Chars In line of Description
rem
rem sample activation: zipchop vibb 0b23defa 0012 1000 9 38
rem
rem uses the PRODLZIP.LST file prepared by PUMS as file list
rem will create a Self Extract ZIP file by the fix name: PRODLZIP.EXE
rem will chop up PRODLZIP.EXE into equal size parts and make them
rem      an object set
rem will chop up DESCRIBE.TXT into screen size parts and make them
rem      an object set
rem
rem
rem change into APID\APIDSINn

cd %1\%1%3
echo ZIPping DownLoad package files. Please Wait...
pkzip -ei prodlzip.zip @prodlzip.lst > NUL:
zip2exe -j prodlzip.zip > NUL:
ren prodlzip.exe %1%3.ZIP
del prodlzip.zip
del prodlzip.lst
echo.
chop %1%3.ZIP %4 %1%3 %2 Y
rem dont need the ZIP file any more
del %1%3.zip
echo.
echo.
echo Chop DESCRIBE.TXT into %5 lines, %6 chars Object Screens, CSCS=%2
chopdes DESCRIBE.TXT %5 %6 %1%3 %2
cd ..\..
@echo on
