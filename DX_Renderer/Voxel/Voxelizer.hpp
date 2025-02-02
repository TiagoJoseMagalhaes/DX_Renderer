#pragma once

#include "../Core/Components/Shader/PixelShader.hpp"
#include "../Core/Components/Shader/VertexShader.hpp"
#include "../Core/Components/Pipeline/PipelineStateObject.hpp"
#include "../Core/Components/RayTracing/AccelerationStructure.hpp"
#include "../Core/Components/Resource/GPU Buffers/ConstantBuffer.hpp"
#include "VoxelMap.hpp"
#include "../Core/Components/Vertices/VertexBuffer.hpp"
#include "../Core/Components/Vertices/IndexBuffer.hpp"
#include "../Model Loader/ModelLoader.hpp"
#include "../Core/Components/Vertices/Vertex.hpp"

#include <directxmath.h>
#include <memory>

namespace DXR
{
    
    using namespace DirectX;
    
    struct Camera;
    struct GraphicsDevice;
    struct GraphicsCommandList;
    struct Model;
    struct Swapchain;
    struct RootSignature;//TODO(Tiago): Build root signature specifically for voxelization?
    
    struct Voxelization_CBuffer
    {
        XMMATRIX clip_space_transformation_matrix;
        XMMATRIX voxel_space_transformation_matrix;
    };
    
#define VOXEL_RES 128
#define VOXEL_WIDTH VOXEL_RES
#define VOXEL_HEIGHT VOXEL_RES
#define VOXEL_DEPTH VOXEL_RES
    
    struct Voxelizer
    {
        VertexShader voxelization_vertex_shader;
        VertexShader no_tex_voxelization_vertex_shader;
        PixelShader voxelization_pixel_shader;
        PixelShader no_tex_voxelization_pixel_shader;
        VoxelMap ocupancy_map;
        VoxelMap albedo_map;
        VoxelMap diffuse_map;
        VoxelMap specular_map;
        VoxelMap exponent_map;
        VoxelMap normal_map;
        Model* model;
        VertexBuffer<OBJVertex> model_vertex_buffer;
        IndexBuffer model_index_buffer;
        PipelineStateObject pso;
        PipelineStateObject no_tex_pso;
        TLAS acceleration_structure;
        RootSignature voxelization_root_signature;
        std::unique_ptr<BLAS> voxel_cube_blas = nullptr;
        std::unique_ptr<ConstantBuffer<Voxelization_CBuffer>> voxelization_cbuffer_x = nullptr;
        std::unique_ptr<ConstantBuffer<Voxelization_CBuffer>> voxelization_cbuffer_y = nullptr;
        std::unique_ptr<ConstantBuffer<Voxelization_CBuffer>> voxelization_cbuffer_z = nullptr;
        std::unique_ptr<VertexBuffer<Vertex>> voxel_cube_vertex_buffer = nullptr;
        std::unique_ptr<IndexBuffer> voxel_cube_index_buffer = nullptr;
        //NOTE(Tiago):Voxelization matrices support data
        XMFLOAT3 extent;
        XMFLOAT3 center;
        XMFLOAT3 voxel_space;
        //NOTE(Tiago):Voxelization matrices
        XMMATRIX voxel_space_conversion_matrix;
        XMMATRIX clip_space_conversion_matrix;
        
        Voxelizer() = default;
        Voxelizer(GraphicsDevice& device,
                  GraphicsCommandList& command_list,
                  RootSignature& root_signature,
                  Model& model,
                  XMMATRIX mvp);
        void Voxelize(GraphicsCommandList& command_list,RootSignature& root_signature);
        void BuildAccelerationStructure(GraphicsDevice& device, GraphicsCommandList& command_list, Fence& fence);
        private:
        void CreateVoxelMaps(GraphicsDevice& device);
        void VoxelizeSubmesh(Submesh& submesh, GraphicsCommandList& command_list);
        void CreateVoxelizationRootSignature(GraphicsDevice& device);
        void CreateVoxelizationConstantBuffers(GraphicsDevice& device, GraphicsCommandList& command_list);
        void XAxisVoxelizationCall(GraphicsCommandList& command_list, UINT constant_buffer_slot);
        void YAxisVoxelizationCall(GraphicsCommandList& command_list, UINT constant_buffer_slot);
        void ZAxisVoxelizationCall(GraphicsCommandList& command_list, UINT constant_buffer_slot);
        void CalculateVoxelizationSupportData();
        void UpdateVoxelizationMatrices(XMMATRIX& model_matrix);
        void UpdateVoxelizationCBufferX();
        void UpdateVoxelizationCBufferY();
        void UpdateVoxelizationCBufferZ();
        void CreateVoxelizationShaders();
        void SetViewport(GraphicsCommandList& command_list);
        void CreateVoxelCube(GraphicsDevice& device, GraphicsCommandList& command_list);
    };
    
}
