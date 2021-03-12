echo off
rem
rem Nzipchop.bat, By ROBERT NEWMAN, JULY 22,1991  VER 1.0.0
rem
rem NZIPCHOP.BAT handles a non zipped file and chops it into object set.
rem PUMS.exe calls this bat file if the control file CONTROL.TXT has 
rem ZIPIND SET TO Y
rem calling sequence: Nzipchop App-ID CSCScscs SINn chop_size
REM                    lines chars  FILENAME
REM
rem App-id    - 4 character identifier of the application directory
rem CSCScscs  - 4 char CSCS or 8 char HEX string for CSCScscs
rem SINn      - 4 char SINn of this package
rem chop_size - block size to chop zipfile into
rem lines     - Lines In Screen of Description
rem chars     - Chars In line of Description
rem FILE NAME - UP TO  12 CHARACTERS XXXXXXXX.YYY 
rem
rem sample activation: nzipchop vibb 0b23defa 0012 1000 9 38 TEST.TXT
rem
rem will chop up PRODLZIP.ZIP into equal size parts and make them
rem      an object set
rem will chop up DESCRIBE.TXT into screen size parts and make them
rem      an object set
rem
rem
rem change into APID\APIDSINn

cd %1\%1%3
echo NOT ZIPping DownLoad package file.
COPY %7  %1%3.ZIP
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
