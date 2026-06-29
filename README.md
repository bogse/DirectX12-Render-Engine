# DirectX12-Render-Engine

This is a DirectX 12 rendering engine built in C++ for learning and experimentation with low-level graphics programming.

## Features: 
- Win32 window creation and message loop
- DirectX 12 initialization
- Command list abstraction
- Descriptor heap management
- Resource management and state tracking
- Upload buffer system
- Basic rendering pipeline setup
- Shader support (Vertex and Pixel shaders)
- Multiple Render Target (MRT) and Depth Buffer architecture
- Mesh class implementation for geometry handling and rendering
- Texture support via DirectXTex standalone pipeline (DDS, PNG, JPG, BMP, TGA, and HDR formats)
   - 1D and 2D mipmap generation via custom Compute Shader
- Lighting & Materials:
   - Blinn-Phong lighting system with Emissive glow, Ambient, Lambertian Diffuse, and Specular highlight layers
   - Modular Material preset framework
- Input system
- Dear ImGui integration for real-time debugging and tooling

## Requirements
- Windows 10/11
- DirectX 12 compatible GPU
- CMake (v4.3.2+ recommended)
- Visual Studio 2022 recommended (can be changed from GenerateSolution.bat)
- Windows SDK 10.0+
- MSVC v143 build tools

## Generating and Building the Project
Ensure you have CMake and Visual Studio installed. Run the following commands from the project root directory using your terminal (Command Prompt, PowerShell, or VS Developer Command Prompt):

```bash
# 1. Configure the project and generate the build files
cmake -B build

# 2. Compile the engine (Release mode)
cmake --build build --config Release
```

*Note: To compile the project for debugging, change `--config Release` to `--config Debug`.*