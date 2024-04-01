## Building on Windows:
- install cmake `https://cmake.org/download/`
- save and extract mingw `https://github.com/skeeto/w64devkit/releases/tag/v1.21.0` to `C:\w64devkit`
- maybe run as admin?
```powershell
SET PATH=%PATH%;C:\\w64devkit\\bin
```
(or put this in you PATH `C:\\w64devkit\\bin`)
- run `.\build_windows.bat`

## Building on Linux
 - `cmake . && make`