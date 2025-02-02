#pragma once

#include <wrl.h>
#include <d3d12.h>

namespace DXR
{
	struct Fence;
	using namespace Microsoft;
    
	enum class CommandQueueType
	{
		None,
		Direct,
		Bundle,
		Compute,
        VideoEncode
	};
    
	struct CommandQueue
	{
        public:
		const CommandQueueType Type;
        protected:
		D3D12_COMMAND_LIST_TYPE m_command_queue_type;
		WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
        public:
		virtual ~CommandQueue(){};
		ID3D12CommandQueue* operator->();
		ID3D12CommandQueue* GetCommandQueueRawPtr();
		void Flush(Fence& fence);
        protected:
		CommandQueue(CommandQueueType type);
		
	};
}
