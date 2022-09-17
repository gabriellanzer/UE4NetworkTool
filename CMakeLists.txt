cmake_minimum_required(VERSION 3.1.0)

# Project Setup
project(UE4ServerBootstrap VERSION 0.0.2)

# Configure Conan
include(${CMAKE_SOURCE_DIR}/conan.cmake)

conan_cmake_run(REQUIRES
				glfw/3.3.8
				fmt/7.1.3
                BASIC_SETUP NO_OUTPUT_DIRS
				IMPORTS "., license* -> ../../licenses @ folder=True, ignore_case=True"
				IMPORTS "include, * -> ../../include @ ignore_case=True"
				CMAKE_TARGETS BUILD missing)

# Configure Includes
include_directories(${CMAKE_SOURCE_DIR}/include)

# Disable Manifest Generation
set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")

# Configure Executable
include_directories(${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.c*" "${CMAKE_SOURCE_DIR}/src/*.h*")
add_executable(ue4_server_bootstrap ${SRC_FILES})

# Target for C++ STD Version 2017
target_compile_features(ue4_server_bootstrap PRIVATE cxx_std_17)
target_compile_options(ue4_server_bootstrap PRIVATE -I -pthread)
target_link_options(ue4_server_bootstrap PRIVATE -mwindows)

target_link_libraries(ue4_server_bootstrap CONAN_PKG::glfw)
target_link_libraries(ue4_server_bootstrap CONAN_PKG::fmt)

set_target_properties(
    ue4_server_bootstrap
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}/"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}/"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    OUTPUT_NAME "UE4ServerBootstrap ${PROJECT_VERSION}"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})