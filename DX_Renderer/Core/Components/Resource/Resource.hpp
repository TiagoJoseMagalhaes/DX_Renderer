#pragma once

#include <d3d12.h>
#include <wrl.h>

namespace DXR
{
	using namespace Microsoft;

	struct DescriptorHeap;

	struct Resource
	{
	public:
	protected:
		WRL::ComPtr<ID3D12Resource> m_resource;
		size_t m_heap_index;
		DescriptorHeap* m_descriptor_heap = nullptr;
		D3D12_RESOURCE_DESC m_resource_description = {};
		D3D12_CLEAR_VALUE m_optimized_clear_value = {};
		D3D12_HEAP_PROPERTIES m_resource_heap_description = {};
	public:
		ID3D12Resource* operator->() const;
		ID3D12Resource* GetResource() const;
		virtual ~Resource() = default;
		DescriptorHeap* GetDescriptorHeap();
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const;
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
	protected:
		Resource(DescriptorHeap& heap, size_t HeapIndex);
		Resource();
		virtual D3D12_RESOURCE_DESC CreateResourceDescription(D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE) = 0;
		virtual D3D12_CLEAR_VALUE CreateOptimizedClearValue() = 0;
		virtual D3D12_HEAP_PROPERTIES CreateResourceHeapDescription() = 0;
	};

};