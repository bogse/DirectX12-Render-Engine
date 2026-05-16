# Define the library project framework name and specify C++ language support.
project(Demo LANGUAGES CXX)

# Create the primary application executable target (WIN32 flag hides the command prompt window).
add_executable(Demo WIN32)

# C++ Standard.
target_compile_features(Demo PRIVATE cxx_std_20)

# Individual Shader File Paths (Single point of definition)
set(VERTEX_SHADER_SRC "../../data/Shaders/VertexShader.hlsl")
set(PIXEL_SHADER_SRC  "../../data/Shaders/PixelShader.hlsl")

# List combining all shader tracking assets
set(SHADER_SOURCES
    ${VERTEX_SHADER_SRC}
    ${PIXEL_SHADER_SRC}
)

# Explicitly list all source and header files for perfect Visual Studio tracking.
target_sources(Demo PRIVATE
    inc/Demo.h
    src/main.cpp
    src/Demo.cpp
    ${SHADER_SOURCES}
)

# Include Directories.
target_include_directories(Demo PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Link Libraries.
# DX12Lib automatically brings in ImGui and custom DirectX::Headers definitions!
target_link_libraries(Demo
    PRIVATE
        DX12Lib
        shlwapi
        d3d12
)

# Global Debugger Settings (Ensures your app finds its runtime assets instantly).
set_target_properties(Demo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Demo>"
)

# Native Visual Studio HLSL Shader Integration (Shader Model 5.1+).
# Configure common pipeline compilation properties across all shaders.
set_source_files_properties(${SHADER_SOURCES} PROPERTIES
    VS_SHADER_OBJECT_FILE_NAME "$(OutDir)shaders/%(Filename).cso"
    VS_SHADER_MODEL "5.1"
    VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
    VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

# Explicitly flag target pipeline stages for individual assets.
set_source_files_properties(${VERTEX_SHADER_SRC} PROPERTIES VS_SHADER_TYPE Vertex)
set_source_files_properties(${PIXEL_SHADER_SRC} PROPERTIES VS_SHADER_TYPE Pixel)


# Visual Studio Solution Explorer Layout Matching.
source_group("inc" FILES inc/Demo.h)
source_group("src" FILES src/main.cpp src/Demo.cpp)
source_group("shaders" FILES ${SHADER_SOURCES})

# Execute an automated post-build safety sequence to verify physical generation directory paths exist.
add_custom_command(TARGET Demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Demo>/shaders"
    COMMENT "Verifying physical shader asset destination directories..."
)