#pragma once
#include "../Resource.hpp"
#include <memory>

namespace DXR
{
	struct GPUDefaultBuffer;
	struct GraphicsCommandList;
	struct GraphicsDevice;

	struct GPUUploadBuffer :public Resource
	{
	public:
	protected:
		UINT64 m_element_count = 0;
		UINT64 m_element_size = 0;
	public:
		GPUUploadBuffer() = default;//null constructor
		GPUUploadBuffer(GraphicsDevice& device, UINT64 elementCount, UINT64 elementSize, void* Data);
		GPUUploadBuffer(GraphicsDevice& device, UINT64 elementCount, UINT64 elementSize, void* Data, D3D12_RESOURCE_DESC ResourceDescription);
		void CopyDataToGPUBuffer(GraphicsCommandList& commandList, GPUDefaultBuffer& buffer);
		void UploadDataFromCPUBuffer(void* Data) const;
		std::unique_ptr<BYTE> GetData();
		void Evict(GraphicsDevice& device);
	protected:
		void CreateResource(GraphicsDevice& device);
		D3D12_RESOURCE_DESC CreateResourceDescription(D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE) override;
		D3D12_CLEAR_VALUE CreateOptimizedClearValue() override;
		D3D12_HEAP_PROPERTIES CreateResourceHeapDescription() override;
	};
}
