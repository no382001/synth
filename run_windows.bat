REM temporary way to make the .exe access the SDL .dlls
del .\res\SDL2\bin\synth.exe
copy .\synth.exe .\res\SDL2\bin\
.\res\SDL2\bin\synth.exe