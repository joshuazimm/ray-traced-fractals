@echo off

:: Define build directory paths
set BUILD_DIR=build

:: Check if build directory exists, otherwise create it
if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
    echo Created build directory: %BUILD_DIR%
) else (
    echo Cleaning build directory...
    rmdir /s /q %BUILD_DIR%
    mkdir %BUILD_DIR%
    echo Build directory cleaned.
)

:: Recreate necessary directories
mkdir build

:: Regenerate CMake build files for Windows using Visual Studio generator
echo Generating build files...
cd build
cmake ..

:: Build all targets
echo Building project...
cmake --build .

echo Build completed.