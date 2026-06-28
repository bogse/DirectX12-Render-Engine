# Latest version of CMake when this file was created.
cmake_minimum_required(VERSION 4.3.2)

# Define the library project framework name and specify C++ language support.
project(DX12Lib LANGUAGES CXX)

# Halt generation processes immediately if the host compilation platform is not Windows.
if(NOT WIN32)
    message(FATAL_ERROR "DX12Lib only supports Windows.")
endif()

# Explicitly list all source and header files.
# Headers files.
set(DX12_HEADERS
    inc/DX12LibPCH.h
    inc/Application.h
    inc/Buffer.h
	inc/Camera.h
	inc/CameraController.h
    inc/CommandList.h
    inc/CommandQueue.h
    inc/DescriptorAllocation.h
    inc/DescriptorAllocator.h
    inc/DescriptorAllocatorPage.h
    inc/DynamicDescriptorHeap.h
    inc/Events.h
	inc/GenerateMipsPSO.h
    inc/GUISystem.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/IndexBuffer.h
	inc/Input.h
    inc/KeyCodes.h
	inc/Material.h
    inc/Mesh.h
    inc/RenderApp.h
	inc/RenderTarget.h
    inc/Resource.h
    inc/ResourceStateTracker.h
    inc/RootSignature.h
	inc/Texture.h
    inc/UploadBuffer.h
    inc/VertexBuffer.h
    inc/Window.h
)

# Source files.
set(DX12_SOURCES
    src/DX12LibPCH.cpp
    src/Application.cpp
    src/Buffer.cpp
	src/Camera.cpp
	src/CameraController.cpp
    src/CommandList.cpp
    src/CommandQueue.cpp
    src/DescriptorAllocation.cpp
    src/DescriptorAllocator.cpp
    src/DescriptorAllocatorPage.cpp
    src/DynamicDescriptorHeap.cpp
	src/GenerateMipsPSO.cpp
    src/GUISystem.cpp
    src/HighResolutionClock.cpp
    src/IndexBuffer.cpp
	src/Material.cpp
    src/Mesh.cpp
    src/RenderApp.cpp
	src/RenderTarget.cpp
    src/Resource.cpp
    src/ResourceStateTracker.cpp
    src/RootSignature.cpp
	src/Texture.cpp
    src/UploadBuffer.cpp
    src/VertexBuffer.cpp
    src/Window.cpp
)

# Define the shader file relative to Dx12Lib's directory
set(GENERATE_MIPS_CS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders/GenerateMips_CS.hlsl")

# Static Library Target.
add_library(DX12Lib STATIC
    ${DX12_HEADERS}
    ${DX12_SOURCES}
    ${GENERATE_MIPS_CS_SRC}
)

# C++ Standard.
target_compile_features(DX12Lib PUBLIC cxx_std_20)

# Configure MSBuild to build the C++ header file automatically
set_source_files_properties(${GENERATE_MIPS_CS_SRC} PROPERTIES
    VS_SHADER_TYPE Compute
    VS_SHADER_MODEL "5.1"
    VS_SHADER_OBJECT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/shaders/%(Filename).cso"
    VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/inc/GenerateMips_CS.h"
    VS_SHADER_VARIABLE_NAME g_GenerateMips_CS
    VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
    VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

# Include Directories.
target_include_directories(DX12Lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
		${CMAKE_CURRENT_SOURCE_DIR}/../DirectXTex
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/generated/inc
)

# Link Libraries.
target_link_libraries(DX12Lib
    PUBLIC
        d3d12
        dxgi
        dxguid
        ImGui::ImGui
		DirectXTexDX12
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
source_group("inc" FILES ${DX12_HEADERS})
source_group("src" FILES ${DX12_SOURCES})
source_group("shaders" FILES ${GENERATE_MIPS_CS_SRC})
