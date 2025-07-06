cd build
cmake . 
cmake --build . --target shader_binaries
cmake --build . --target shader_drafter
cd ../bin/Debug
shader_drafter.exe
pause