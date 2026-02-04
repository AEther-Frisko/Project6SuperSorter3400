# Project VI Webserver - Super Sorter 3400
CROW-based webserver that runs a simple puzzle game. This will run in a Docker container later, but for now it just runs on your device directly.

## How to Build Locally
1. Clone the repo
2. Install [CMake](https://cmake.org/download/) if you do not already have it
3. Create a build folder in the repo:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
If you want to rebuild the webserver after making changes, it's best to delete the build folder first (ensures a clean build). This can be done using `rm -r build`, or manually through your file explorer.

4. Run the created `server.exe` file from your project's root directory, for example:
```
./build/Debug/server.exe
```
This should be located within the build folder. In my case, it created a Visual Studio solution, so the .exe was located in the Debug directory of the build folder.

**If you don't run the .exe from the project root, it can't locate files!**

5. Open your browser at `http://localhost:18080`

If all is working correctly, you should see the home page in your browser.