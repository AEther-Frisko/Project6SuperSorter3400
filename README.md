# Project VI Webserver - Super Sorter 3400
CROW-based webserver that runs a simple puzzle game. This will run in a Docker container later, but for now it just runs on your device directly.

## How to Build Locally
1. Clone the repo
2. Install CMake if you do not already have it
3. Create a build folder in the repo:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
4. Run the created `server.exe` file
    - This should be located within the build folder. In my case, it created a Visual Studio solution, so the .exe was located in the Debug directory.
5. Open your browser at `http://localhost:1800`

If all is working correctly, you should see a plain "Hello world" message in the browser tab.