# UE4NetworkTool
This repository is the **UE4 Network Tool** initiative. A tool that provides support to launch Unreal Dedicated server from the `.uproject` and analyze network related data (aka.: sequence of frame snapshots).

## Setting-up for Development
This project uses CMake as it's buil system generator and Conan as a dependency package manager. Some dependencies are header-only and thus already included in the repo. Both CMake and Conan are required for the code to build:

CMake: https://cmake.org/
Conan: https://conan.io/
After installing both, you can open the CMake GUI and configure the CMake project. The configure step will invoke Conan, so make sure it is mapped in the PATH and is accessible from command line. Conan will then download any existing precompiled binaries or build them from source for the specified compiler-target profile.

This setup works great with Ninja as a Generator for either VSCode or CLion. It will also work if you generate a Visual Studio Solution. Compiling with Clang in Windows is a little trickier, you must force CMake to use Clang compiler through environmental variables (at least for the first configure run). If you don't, the program's code will try to compile with Clang but all dependencies downloaded with Conan will use MSVC, thus break the CMake configure step with errors saying the compilers missmatch.
