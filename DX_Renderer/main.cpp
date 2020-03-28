#include <iostream>
#include <Windows.h>
#include <thread>
#include "Tooling/Log.hpp"
#include "Core/Components/Command Queue/GraphicsCommandQueue.hpp"
#include "Core/Windows Abstractions/Window.hpp"
#include "Core/Components/GraphicsDevice.hpp"
#include "Core/Components/Fence.hpp"
#include "Core/Components/Command List/GraphicsCommandList.hpp"
#include "Core/Components/Swapchain.hpp"
#include "Core/Components/Vertices/VertexBuffer.hpp"
#include "Core/Components/Vertices/IndexBuffer.hpp"
#include "Core/Components/Pipeline/PipelineStateObject.hpp"
#include "Core/Components/Shader/VertexShader.hpp"
#include "Core/Components/Shader/PixelShader.hpp"
#include "Core/Components/Resource/GPU Buffers/ConstantBuffer.hpp"
#include "Core/Components/Resource/Texture/Texture.hpp"
#include "Interface/GUI.hpp"
#include "Core//Components/RayTracing/AccelerationStructure.hpp"

void MainDirectXThread(DXR::Window& window)
{
	SUCCESS_LOG(L"Main DirectX12 Thread Started");

	DXR::GraphicsDevice device;

	DXR::VertexShader vs = DXR::VertexShader::CompileShaderFromFile(L"./DX_Renderer/Resources/Shaders/VertexShader.hlsl", L"VSMain");
	DXR::PixelShader ps = DXR::PixelShader::CompileShaderFromFile(L"./DX_Renderer/Resources/Shaders/VertexShader.hlsl", L"PSMain");

	DXR::RootSignature root_signature;
	//DXR::DescriptorTableRootParameter desc_table;
	//desc_table.AddCBVEntry(1);
	DXR::DescriptorTableRootParameter srv_desc_table;
	srv_desc_table.AddSRVEntry(1);
	
	DXR::DescriptorTableRootParameter sampler_desc_table;
	sampler_desc_table.AddSamplerEntry(1);
	
	//root_signature.AddDescriptorTableRootParameter(desc_table);
	root_signature.AddDescriptorTableRootParameter(srv_desc_table);
	root_signature.AddDescriptorTableRootParameter(sampler_desc_table);
	
	DXR::DescriptorRootParameter rp(DXR::RootParameterDescriptorType::CBV,0);
	root_signature.AddDescriptorRootParameter(rp);
	
	root_signature.CreateRootSignature(device);

	DXR::PipelineStateObject pso = {
		device,
		vs.GetShaderBytecode(),
		ps.GetShaderBytecode(),
		root_signature,
		DXR::Vertex::GetInputLayout(),
		DXR::Swapchain::m_backbuffer_format,
		DXR::DepthStencilBuffer::DepthStencilBufferFormat };

	DXR::Fence fence = device.CreateFence(0);
	DXR::GraphicsCommandList commandList = device.CreateGraphicsCommandList();

	commandList.FullReset(pso);
	commandList.SetName(L"Main Command List");

	DXR::Swapchain swapchain = device.CreateSwapchain(window, 60, commandList);
	DXR::VertexBuffer<DXR::Vertex> vertex_buffer(device, commandList,
		{
			{{-1.0f, -1.0f,  0.0f},	{0.0f,1.0f}},
			{{1.0f, -1.0f,  0.0f},	{1.0f,1.0f}},
			{{-1.0f, 1.0f,  0.0f},	{0.0f,0.0f}},
			{{1.0f, 1.0f,  0.0f},	{1.0f,0.0f}},
		});
	DXR::IndexBuffer index_buffer(device, commandList,
		{ 0,2,1,
			1,3,2,
		});

	auto texture = DXR::Texture(L"./DX_Renderer/Resources/Textures/star.jpg", device, commandList);

	commandList->Close();

	device.GetGraphicsCommandQueue().ExecuteCommandList(commandList);
	device.GetGraphicsCommandQueue().Flush(fence);

	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, 1280.0f / 720.0f, 0.1f, 1000.0f);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH({ 0.0f,0.0f,-10.0f,1.0f }, { 0.0f,0.0f,0.0f,1.0f }, { 0.0f,1.0f,0.0f,0.0f });
	DirectX::XMMATRIX model = DirectX::XMMatrixScaling(1, 1, 1);

	DirectX::XMMATRIX mvp = model * view * projection;
	DXR::ConstantBuffer<DirectX::XMMATRIX> constant_buffer(device, { mvp });

	commandList.FullReset(pso);

	FLOAT color[4] = { 0.4f, 0.6f, 0.9f, 1.0f };

	float x_rotation_angle = 0;
	float y_rotation_angle = 1;
	float z_rotation_angle = 0;
	float scale = 1.0f;

	DXR::GUI gui(device, window, swapchain);

	DXR::BLAS blas(vertex_buffer);

	while (window.ShouldContinue)
	{

		// Start the Dear ImGui frame
		gui.StartFrame();

		ImGui::Begin("Window");
		ImGui::SliderAngle("X Rotation", &x_rotation_angle);
		ImGui::SliderAngle("Y Rotation", &y_rotation_angle);
		ImGui::SliderAngle("Z Rotation", &z_rotation_angle);
		ImGui::SliderFloat("Model Scale",&scale,0,10);
		ImGui::End();

		{
			model  = DirectX::XMMatrixRotationAxis({ 1.0f,0.0f,0.0f }, x_rotation_angle);
			model *= DirectX::XMMatrixRotationAxis({ 0.0f,1.0f,0.0f }, y_rotation_angle);
			model *= DirectX::XMMatrixRotationAxis({ 0.0f,0.0f,1.0f }, z_rotation_angle);
			model *= DirectX::XMMatrixScaling(scale,scale,scale);
			mvp = model * view * projection;
			constant_buffer.UpdateData({ mvp });
		}

		commandList.SetGraphicsRootSignature(root_signature);
		swapchain.Prepare(commandList);

		swapchain.GetCurrentBackBuffer().Clear(commandList, color);
		swapchain.GetDepthStencilBuffer().Clear(commandList);
		commandList.SetDisplayRenderTarget(swapchain.GetCurrentBackBuffer(), swapchain.GetDepthStencilBuffer());

		commandList.BindDescriptorHeaps({texture.GetSRVHeap(),texture.GetSamplerHeap()});

		commandList.BindVertexBuffer(vertex_buffer);
		commandList.BindIndexBuffer(index_buffer);
		commandList.BindConstantBuffer(constant_buffer, 0);
		commandList.BindTexture(texture, 1);

		commandList.SendDrawCall();

		gui.Render(commandList);

		swapchain.PrepareBackbufferForPresentation(commandList);

		commandList.Close();
		device.GetGraphicsCommandQueue().ExecuteCommandList(commandList);

		swapchain.Present();

		device.GetGraphicsCommandQueue().Flush(fence);

		commandList.FullReset(pso);
	}

}

int WINAPI CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	DXR::Window window{ hInstance,nCmdShow,{1280,720},"DX Renderer" };

	std::thread main_dx12_thread(MainDirectXThread, std::ref(window));

	while (window.ShouldContinue)
	{
		window.UpdateWindow();
	}

	main_dx12_thread.join();
	return 0;
}