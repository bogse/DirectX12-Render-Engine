# Latest version of CMake when this file was created.
cmake_minimum_required(VERSION 3.21)

# Define the root workspace name and specify C++ language support.
project(DX12RenderEngine LANGUAGES CXX)

# Enable folder nesting within the Visual Studio Solution Explorer tree view.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Enable multithreaded compilation (MSVC only).
if(MSVC)
    add_compile_options(/MP)
endif()

# Base output directory used as a common root for all build artifacts.
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

# Output directories.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Output directory for static lib (.LIB).
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for shared lib (.DLL).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Putput directory for executables (.EXE).

# Integrate external vendor targets.
add_subdirectory(thirdparty/DirectXTex)
add_subdirectory(thirdparty/DirectXHeaders)
add_subdirectory(thirdparty/ImGui)

# Integrate internal framework components.
add_subdirectory(code/DX12Lib)

# Integrate client execution sandboxes.
add_subdirectory(code/Demo) 

target_compile_definitions(Demo PRIVATE
    ASSET_DIR="${CMAKE_SOURCE_DIR}/data"
)

# Set the startup project (Visual Studio only).
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Demo)