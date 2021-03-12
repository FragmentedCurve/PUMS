echo on
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
rem will chop up DESCRIBE.TXT into screen size parts and make them
rem      an object set
rem
rem
rem change into APID\APIDSINn

cd %1\%1%3
echo Chop DESCRIBE.TXT into %5 lines, %6 chars Object Screens, CSCS=%2
chopdes DESCRIBE.TXT %5 %6 %1%3 %2
cd ..\..
@echo on
