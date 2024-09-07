@echo off
SET compiler=-nologo -Zi -W4 -wd4005 -wd4100 -wd4201 -wd4244 -wd4305 -wd4312 -wd4457 -wd4459 -wd4505 -wd4530 -wd4838
SET linker=/IGNORE:4098 /IGNORE:4099 /IGNORE:4286 /OUT:8086.exe 
SET definitions=/D _MBCS /D _CRT_SECURE_NO_WARNINGS
SET libraries=user32.lib 

cd ..
IF NOT EXIST p:/8086/build mkdir build
cd build
cl %compiler% %definitions% p:/8086/source/8086.cpp %libraries% /I p:/8086/source /link %linker%
cd ..



