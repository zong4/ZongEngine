#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/RenderCommandBuffer.h"

#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanAllocator.h"

#include <vector>

struct GLFWwindow;

namespace Hazel {

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain() = default;

		void Init(VkInstance instance, const Ref<VulkanDevice>& device);
		void InitSurface(GLFWwindow* windowHandle);
		void Create(uint32_t* width, uint32_t* height, bool vsync);
		void Destroy();

		void OnResize(uint32_t width, uint32_t height);

		void BeginFrame();
		void Present();

		uint32_t GetImageCount() const { return m_ImageCount; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		VkRenderPass GetRenderPass() { return m_RenderPass; }

		VkFramebuffer GetCurrentFramebuffer() { return GetFramebuffer(m_CurrentImageIndex); }
		VkCommandBuffer GetCurrentDrawCommandBuffer() { return GetDrawCommandBuffer(m_CurrentFrameIndex); }

		VkFormat GetColorFormat() { return m_ColorFormat; }

		uint32_t GetCurrentBufferIndex() const { return m_CurrentFrameIndex; }

		VkFramebuffer GetFramebuffer(uint32_t index)
		{
			HZ_CORE_ASSERT(index < m_Framebuffers.size());
			return m_Framebuffers[index];
		}

		VkCommandBuffer GetDrawCommandBuffer(uint32_t index)
		{
			HZ_CORE_ASSERT(index < m_CommandBuffers.size());
			return m_CommandBuffers[index].CommandBuffer;
		}

		void SetVSync(const bool enabled) { m_VSync = enabled; }

	private:
		uint32_t AcquireNextImage();

		void FindImageFormatAndColorSpace();
	private:
		VkInstance m_Instance = nullptr;
		Ref<VulkanDevice> m_Device;
		bool m_VSync = false;

		VkFormat m_ColorFormat;
		VkColorSpaceKHR m_ColorSpace;

		VkSwapchainKHR m_SwapChain = nullptr;
		uint32_t m_ImageCount = 0;
		std::vector<VkImage> m_VulkanImages;

		struct SwapchainImage
		{
			VkImage Image = nullptr;
			VkImageView ImageView = nullptr;
		};
		std::vector<SwapchainImage> m_Images;

		struct
		{
			VkImage Image = nullptr;
			VmaAllocation MemoryAlloc = nullptr;
			VkImageView ImageView = nullptr;
		} m_DepthStencil;

		std::vector<VkFramebuffer> m_Framebuffers;
		
		struct SwapchainCommandBuffer
		{
			VkCommandPool CommandPool = nullptr;
			VkCommandBuffer CommandBuffer = nullptr;
		};
		std::vector<SwapchainCommandBuffer> m_CommandBuffers;

		// Semaphores to signal that images are available for rendering and that rendering has finished (one pair for each frame in flight)
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;

		// Fences to signal that command buffers are ready to be reused (one for each frame in flight)
		std::vector<VkFence> m_WaitFences;

		VkRenderPass m_RenderPass = nullptr;
		uint32_t m_CurrentFrameIndex = 0;    // Index of the frame we are currently working on, up to max frames in flight
		uint32_t m_CurrentImageIndex = 0;    // Index of the current swapchain image.  Can be different from frame index

		uint32_t m_QueueNodeIndex = UINT32_MAX;
		uint32_t m_Width = 0, m_Height = 0;

		VkSurfaceKHR m_Surface;

		friend class VulkanContext;
	};
}
