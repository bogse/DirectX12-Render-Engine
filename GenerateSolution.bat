@echo off

if not exist build (
    mkdir build
)

cd build

:: Change the generator (-G) or architecture (-A) for different Visual Studio versions
cmake .. -G "Visual Studio 17 2022" -A x64

cd ..