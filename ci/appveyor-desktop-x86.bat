call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x86 || exit /b

mkdir build && cd build || exit /b
cmake .. -DBUILD_TESTS=ON -G Ninja

cmake --build . || exit /b
cmake --build . --target install || exit /b

cd ..
:: call cmake\run_tests.bat || exit /b