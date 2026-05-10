# Latest version of CMake when this file was created.
cmake_minimum_required(VERSION 4.3.2)

# Define the library project framework name and specify C++ language support.
project(ImGui VERSION 1.0 LANGUAGES CXX)

# Static Library Target.
add_library(ImGui STATIC)

# C++ Standard
target_compile_features(ImGui PUBLIC cxx_std_17)

# Explicitly list all source and header files for perfect Visual Studio tracking.
target_sources(ImGui PRIVATE
    src/imgui.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_tables.cpp
    src/imgui_widgets.cpp
    src/imgui_impl_dx12.cpp
    src/imgui_impl_win32.cpp
    
    inc/imgui.h
    inc/imgui_internal.h
    inc/imstb_rectpack.h
    inc/imstb_textedit.h
    inc/imstb_truetype.h
    inc/imgui_impl_dx12.h
    inc/imgui_impl_win32.h
)

# Include Directories.
target_include_directories(ImGui
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Libraries.
target_link_libraries(ImGui PRIVATE d3d12 dxgi)

# Create a scoped namespace alias for external usage
add_library(ImGui::ImGui ALIAS ImGui)

# Visual Studio Organization.
source_group("inc" FILES 
    inc/imgui.h inc/imgui_internal.h inc/imstb_rectpack.h 
    inc/imstb_textedit.h inc/imstb_truetype.h 
    inc/imgui_impl_dx12.h inc/imgui_impl_win32.h
)

source_group("src" FILES 
    src/imgui.cpp src/imgui_demo.cpp src/imgui_draw.cpp 
    src/imgui_tables.cpp src/imgui_widgets.cpp 
    src/imgui_impl_dx12.cpp src/imgui_impl_win32.cpp
)