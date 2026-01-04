#--------------------------------------------------------------------
# Minimal D3D11 Win32 App – CMakeLists.txt
#--------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10)

# Project name & language
project(MinimalD3D11App LANGUAGES CXX)

#--------------------------------------------------------------------
# 1. Add the executable
#--------------------------------------------------------------------
#  * WIN32 tells CMake to build a GUI (no console) application.
#  * main.cpp is the single source file from the previous answer.
add_executable(${PROJECT_NAME} WIN32 main.cpp)

#--------------------------------------------------------------------
# 2. C++ standard & Unicode definitions
#--------------------------------------------------------------------
# Use at least C++17 (feel free to bump to C++20/23 if you wish).
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Define UNICODE macros so the Windows headers use the Unicode API.
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

#--------------------------------------------------------------------
# 3. Link the DirectX libraries
#--------------------------------------------------------------------
# These libraries are part of the Windows SDK and are found automatically
# by the linker on a standard Windows installation.
target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 d3dcompiler)

#--------------------------------------------------------------------
# 4. Optional: set output directories & debugging settings
#--------------------------------------------------------------------
# Put the built executable in a dedicated folder (e.g. <build>/bin).
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# When debugging from VS, start the program in the project folder.
# (Only affects the VS generator; harmless for other generators.)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

#--------------------------------------------------------------------
# 5. (Optional) Add a custom command to copy any runtime files
#--------------------------------------------------------------------
# For this minimal sample we don't need any external assets, so
# this section is left empty. Add your own post‑build commands here
# if you later ship textures, shaders, etc.

#--------------------------------------------------------------------
# End of CMakeLists.txt
#--------------------------------------------------------------------