# Latest version of CMake when this file was created.
cmake_minimum_required(VERSION 4.3.2)

# Define the library project framework name and specify C++ language support.
project(DX12Lib LANGUAGES CXX)

# Halt generation processes immediately if the host compilation platform is not Windows.
if(NOT WIN32)
    message(FATAL_ERROR "DX12Lib only supports Windows.")
endif()

# Static Library Target.
add_library(DX12Lib STATIC)

# C++ Standard.
target_compile_features(DX12Lib PUBLIC cxx_std_20)

# Explicitly list all source and header files for perfect Visual Studio tracking.
target_sources(DX12Lib PRIVATE

    # Headers.
    inc/DX12LibPCH.h
    inc/Application.h
    inc/Buffer.h
    inc/CommandList.h
    inc/CommandQueue.h
    inc/DescriptorAllocation.h
    inc/DescriptorAllocator.h
    inc/DescriptorAllocatorPage.h
    inc/DynamicDescriptorHeap.h
    inc/Events.h
    inc/GUISystem.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/IndexBuffer.h
    inc/KeyCodes.h
    inc/Mesh.h
    inc/RenderApp.h
    inc/Resource.h
    inc/ResourceStateTracker.h
    inc/RootSignature.h
    inc/UploadBuffer.h
    inc/VertexBuffer.h
    inc/Window.h

    # Source Files.
    src/DX12LibPCH.cpp
    src/Application.cpp
    src/Buffer.cpp
    src/CommandList.cpp
    src/CommandQueue.cpp
    src/DescriptorAllocation.cpp
    src/DescriptorAllocator.cpp
    src/DescriptorAllocatorPage.cpp
    src/DynamicDescriptorHeap.cpp
    src/GUISystem.cpp
    src/HighResolutionClock.cpp
    src/IndexBuffer.cpp
    src/Mesh.cpp
    src/RenderApp.cpp
    src/Resource.cpp
    src/ResourceStateTracker.cpp
    src/RootSignature.cpp
    src/UploadBuffer.cpp
    src/VertexBuffer.cpp
    src/Window.cpp
)

# Include Directories.
target_include_directories(DX12Lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Link Libraries.
target_link_libraries(DX12Lib
    PUBLIC
        d3d12
        dxgi
        dxguid
        ImGui::ImGui
        DirectX::Headers
)

# Instruct the MSVC compiler compilation phase to use the precompiled header file
set_source_files_properties(${PCH_TARGET_SOURCES} PROPERTIES 
    COMPILE_FLAGS "/Yu\"DX12LibPCH.h\""
)

# Instruct the MSVC compiler compilation phase to generate the binary precompiled block layout
set_source_files_properties(src/DX12LibPCH.cpp PROPERTIES
    COMPILE_FLAGS "/Yc\"DX12LibPCH.h\""
)

# Visual Studio Organization.
source_group("inc" FILES
    inc/DX12LibPCH.h
    inc/Application.h
    inc/Buffer.h
    inc/CommandList.h
    inc/CommandQueue.h
    inc/DescriptorAllocation.h
    inc/DescriptorAllocator.h
    inc/DescriptorAllocatorPage.h
    inc/DynamicDescriptorHeap.h
    inc/Events.h
    inc/GUISystem.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/IndexBuffer.h
    inc/KeyCodes.h
    inc/Mesh.h
    inc/RenderApp.h
    inc/Resource.h
    inc/ResourceStateTracker.h
    inc/RootSignature.h
    inc/UploadBuffer.h
    inc/VertexBuffer.h
    inc/Window.h
)

source_group("src" FILES
    src/DX12LibPCH.cpp
    src/Application.cpp
    src/Buffer.cpp
    src/CommandList.cpp
    src/CommandQueue.cpp
    src/DescriptorAllocation.cpp
    src/DescriptorAllocator.cpp
    src/DescriptorAllocatorPage.cpp
    src/DynamicDescriptorHeap.cpp
    src/GUISystem.cpp
    src/HighResolutionClock.cpp
    src/IndexBuffer.cpp
    src/Mesh.cpp
    src/RenderApp.cpp
    src/Resource.cpp
    src/ResourceStateTracker.cpp
    src/RootSignature.cpp
    src/UploadBuffer.cpp
    src/VertexBuffer.cpp
    src/Window.cpp
)