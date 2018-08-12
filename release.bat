@echo off
mkdir adf
copy of adf
xcopy data adf\data /e /i /h
xcopy precalc adf\precalc /e /i /h
mkdir adf\s
echo of > adf\s\startup-sequence
exe2adf -l "OpenFire" -a "openfire.adf" -d adf
rmdir adf /s /q