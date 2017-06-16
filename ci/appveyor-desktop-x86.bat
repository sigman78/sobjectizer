call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x86 || exit /b

mkdir build && cd build || exit /b
cmake .. -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 15 2017"

cmake --build . || exit /b
cmake --build . --target install || exit /b

call cmake\run_tests.bat || exit /b