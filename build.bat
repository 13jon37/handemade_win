@echo off
if not exist build mkdir build
pushd build
g++ -c -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 ..\source\win32_handmade.cpp -g -Wall -m64 && g++ *.o -o main.exe -lmingw32 -lgdi32 
popd