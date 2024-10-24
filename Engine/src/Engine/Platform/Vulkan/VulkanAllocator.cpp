#include "pch.h"
#include "VulkanAllocator.h"

#include "VulkanContext.h"

#include "Engine/Utilities/StringUtils.h"

#if ZONG_LOG_RENDERER_ALLOCATIONS
#define ZONG_ALLOCATOR_LOG(...) ZONG_CORE_TRACE(__VA_ARGS__)
#else
#define ZONG_ALLOCATOR_LOG(...)
#endif

#define ZONG_GPU_TRACK_MEMORY_ALLOCATION 1

namespace Engine {

	struct VulkanAllocatorData
	{
		VmaAllocator Allocator;
		uint64_t TotalAllocatedBytes = 0;
		
		uint64_t MemoryUsage = 0; // all heaps
	};

	enum class AllocationType : uint8_t
	{
		None = 0, Buffer = 1, Image = 2
	};

	static VulkanAllocatorData* s_Data = nullptr;
	struct AllocInfo
	{
		uint64_t AllocatedSize = 0;
		AllocationType Type = AllocationType::None;
	};
	static std::map<VmaAllocation, AllocInfo> s_AllocationMap;

	VulkanAllocator::VulkanAllocator(const std::string& tag)
		: m_Tag(tag)
	{
	}

	VulkanAllocator::~VulkanAllocator()
	{
	}

#if 0
	void VulkanAllocator::Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
	{
		ZONG_CORE_ASSERT(m_Device);

		// TODO: Tracking
		ZONG_CORE_TRACE("VulkanAllocator ({0}): allocating {1}", m_Tag, Utils::BytesToString(requirements.size));

		{
			static uint64_t totalAllocatedBytes = 0;
			totalAllocatedBytes += requirements.size;
			ZONG_CORE_TRACE("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(totalAllocatedBytes));
		}

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = m_Device->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetVulkanDevice(), &memAlloc, nullptr, dest));
	}
#endif

	VmaAllocation VulkanAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer)
	{
		ZONG_CORE_VERIFY(bufferCreateInfo.size > 0);

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_Data->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);
		if (allocation == nullptr)
		{
			ZONG_CORE_ERROR_TAG("Renderer", "Failed to allocate GPU buffer!");
			ZONG_CORE_ERROR_TAG("Renderer", "  Requested size: {}", Utils::BytesToString(bufferCreateInfo.size));
			auto stats = GetStats();
			ZONG_CORE_ERROR_TAG("Renderer", "  GPU mem usage: {}/{}", Utils::BytesToString(stats.Used), Utils::BytesToString(stats.TotalAvailable));
		}

		// TODO: Tracking
		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		ZONG_ALLOCATOR_LOG("VulkanAllocator ({0}): allocating buffer; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			ZONG_ALLOCATOR_LOG("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}

#if ZONG_GPU_TRACK_MEMORY_ALLOCATION
		auto& allocTrack = s_AllocationMap[allocation];
		allocTrack.AllocatedSize = allocInfo.size;
		allocTrack.Type = AllocationType::Buffer;
		s_Data->MemoryUsage += allocInfo.size;
#endif

		return allocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage, VkDeviceSize* allocatedSize)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(s_Data->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);
		if (allocation == nullptr)
		{
			ZONG_CORE_ERROR_TAG("Renderer", "Failed to allocate GPU image!");
			ZONG_CORE_ERROR_TAG("Renderer", "  Requested size: {}x{}x{}", imageCreateInfo.extent.width, imageCreateInfo.extent.height, imageCreateInfo.extent.depth);
			ZONG_CORE_ERROR_TAG("Renderer", "  Mips: {}", imageCreateInfo.mipLevels);
			ZONG_CORE_ERROR_TAG("Renderer", "  Layers: {}", imageCreateInfo.arrayLayers);
			auto stats = GetStats();
			ZONG_CORE_ERROR_TAG("Renderer", "  GPU mem usage: {}/{}", Utils::BytesToString(stats.Used), Utils::BytesToString(stats.TotalAvailable));
		}

		// TODO: Tracking
		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		if (allocatedSize)
			*allocatedSize = allocInfo.size;
		ZONG_ALLOCATOR_LOG("VulkanAllocator ({0}): allocating image; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			ZONG_ALLOCATOR_LOG("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}

#if ZONG_GPU_TRACK_MEMORY_ALLOCATION
		auto& allocTrack = s_AllocationMap[allocation];
		allocTrack.AllocatedSize = allocInfo.size;
		allocTrack.Type = AllocationType::Image;
		s_Data->MemoryUsage += allocInfo.size;
#endif

		return allocation;
	}

	void VulkanAllocator::Free(VmaAllocation allocation)
	{
		vmaFreeMemory(s_Data->Allocator, allocation);

#if ZONG_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_Data->MemoryUsage -= it->second.AllocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			ZONG_CORE_ERROR("Could not find GPU memory allocation: {}", (void*)allocation);
		}
#endif
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		ZONG_CORE_ASSERT(image);
		ZONG_CORE_ASSERT(allocation);
		vmaDestroyImage(s_Data->Allocator, image, allocation);

#if ZONG_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_Data->MemoryUsage -= it->second.AllocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			ZONG_CORE_ERROR("Could not find GPU memory allocation: {}", (void*)allocation);
		}
#endif
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		ZONG_CORE_ASSERT(buffer);
		ZONG_CORE_ASSERT(allocation);
		vmaDestroyBuffer(s_Data->Allocator, buffer, allocation);

#if ZONG_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_Data->MemoryUsage -= it->second.AllocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			ZONG_CORE_ERROR("Could not find GPU memory allocation: {}", (void*)allocation);
		}
#endif
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocator::DumpStats()
	{
		const auto& memoryProps = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryHeapCount);
		vmaGetBudget(s_Data->Allocator, budgets.data());

		ZONG_CORE_WARN("-----------------------------------");
		for (VmaBudget& b : budgets)
		{
			ZONG_CORE_WARN("VmaBudget.allocationBytes = {0}", Utils::BytesToString(b.allocationBytes));
			ZONG_CORE_WARN("VmaBudget.blockBytes = {0}", Utils::BytesToString(b.blockBytes));
			ZONG_CORE_WARN("VmaBudget.usage = {0}", Utils::BytesToString(b.usage));
			ZONG_CORE_WARN("VmaBudget.budget = {0}", Utils::BytesToString(b.budget));
		}
		ZONG_CORE_WARN("-----------------------------------");
	}

	GPUMemoryStats VulkanAllocator::GetStats()
	{
		const auto& memoryProps = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryHeapCount);
		vmaGetBudget(s_Data->Allocator, budgets.data());

		uint64_t budget = 0;
		for (VmaBudget& b : budgets)
			budget += b.budget;

		GPUMemoryStats result;
		for (const auto& [k, v] : s_AllocationMap)
		{
			if (v.Type == AllocationType::Buffer)
			{
				result.BufferAllocationCount++;
				result.BufferAllocationSize += v.AllocatedSize;
			}
			else if (v.Type == AllocationType::Image)
			{
				result.ImageAllocationCount++;
				result.ImageAllocationSize += v.AllocatedSize;
			}
		}

		result.AllocationCount = s_AllocationMap.size();
		result.Used = s_Data->MemoryUsage;
		result.TotalAvailable = budget;
		return result;
#if 0
		VmaStats stats;
		vmaCalculateStats(s_Data->Allocator, &stats);

		uint64_t usedMemory = stats.total.usedBytes;
		uint64_t freeMemory = stats.total.unusedBytes;

		return { usedMemory, freeMemory };
#endif
	}

	void VulkanAllocator::Init(Ref<VulkanDevice> device)
	{
		s_Data = hnew VulkanAllocatorData();

		// Initialize VulkanMemoryAllocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		allocatorInfo.device = device->GetVulkanDevice();
		allocatorInfo.instance = VulkanContext::GetInstance();

		vmaCreateAllocator(&allocatorInfo, &s_Data->Allocator);
	}

	void VulkanAllocator::Shutdown()
	{
		vmaDestroyAllocator(s_Data->Allocator);

		delete s_Data;
		s_Data = nullptr;
	}

	VmaAllocator& VulkanAllocator::GetVMAAllocator()
	{
		return s_Data->Allocator;
	}

} 
