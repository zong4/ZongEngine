#include "pch.h"
#include "VulkanImGuiLayer.h"

#include "imgui.h"
#include "Engine/ImGui/ImGuizmo.h"
#include "Engine/ImGui/ImGuiFonts.h"
#ifndef IMGUI_IMPL_API
#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "Engine/Core/Application.h"
#include <GLFW/glfw3.h>

#include "Engine/Editor/FontAwesome.h"

#include "Engine/Renderer/Renderer.h"

#include "Engine/Platform/Vulkan/VulkanContext.h"

namespace Engine {

	static std::vector<VkCommandBuffer> s_ImGuiCommandBuffers;

	VulkanImGuiLayer::VulkanImGuiLayer()
	{
	}

	VulkanImGuiLayer::VulkanImGuiLayer(const std::string& name)
	{

	}

	VulkanImGuiLayer::~VulkanImGuiLayer()
	{
	}

	void VulkanImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		// Configure Fonts
		{
			UI::FontConfiguration robotoBold;
			robotoBold.FontName = "Bold";
			robotoBold.FilePath = "Resources/Fonts/Roboto/Roboto-Bold.ttf";
			robotoBold.Size = 18.0f;
			UI::Fonts::Add(robotoBold);

			UI::FontConfiguration robotoLarge;
			robotoLarge.FontName = "Large";
			robotoLarge.FilePath = "Resources/Fonts/Roboto/Roboto-Regular.ttf";
			robotoLarge.Size = 24.0f;
			UI::Fonts::Add(robotoLarge);

			UI::FontConfiguration robotoDefault;
			robotoDefault.FontName = "Default";
			robotoDefault.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
			robotoDefault.Size = 15.0f;
			UI::Fonts::Add(robotoDefault, true);

			static const ImWchar s_FontAwesomeRanges[] = { ZONG_ICON_MIN, ZONG_ICON_MAX, 0 };
			UI::FontConfiguration fontAwesome;
			fontAwesome.FontName = "FontAwesome";
			fontAwesome.FilePath = "Resources/Fonts/FontAwesome/fontawesome-webfont.ttf";
			fontAwesome.Size = 16.0f;
			fontAwesome.GlyphRanges = s_FontAwesomeRanges;
			fontAwesome.MergeWithLast = true;
			UI::Fonts::Add(fontAwesome);

			UI::FontConfiguration robotoMedium;
			robotoMedium.FontName = "Medium";
			robotoMedium.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
			robotoMedium.Size = 18.0f;
			UI::Fonts::Add(robotoMedium);

			UI::FontConfiguration robotoSmall;
			robotoSmall.FontName = "Small";
			robotoSmall.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
			robotoSmall.Size = 12.0f;
			UI::Fonts::Add(robotoSmall);

			UI::FontConfiguration robotoExtraSmall;
			robotoExtraSmall.FontName = "ExtraSmall";
			robotoExtraSmall.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
			robotoExtraSmall.Size = 10.0f;
			UI::Fonts::Add(robotoExtraSmall);

			UI::FontConfiguration robotoBoldTitle;
			robotoBoldTitle.FontName = "BoldTitle";
			robotoBoldTitle.FilePath = "Resources/Fonts/Roboto/Roboto-Bold.ttf";
			robotoBoldTitle.Size = 16.0f;
			UI::Fonts::Add(robotoBoldTitle);
		}

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		SetDarkThemeV2Colors();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

		VulkanImGuiLayer* instance = this;
		Renderer::Submit([instance]()
		{
			Application& app = Application::Get();
			GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

			auto vulkanContext = VulkanContext::Get();
			auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			VkDescriptorPool descriptorPool;

			// Create Descriptor Pool
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 100 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool));

			// Setup Platform/Renderer bindings
			ImGui_ImplGlfw_InitForVulkan(window, true);
			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = VulkanContext::GetInstance();
			init_info.PhysicalDevice = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetVulkanPhysicalDevice();
			init_info.Device = device;
			init_info.QueueFamily = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
			init_info.Queue = VulkanContext::GetCurrentDevice()->GetGraphicsQueue();
			init_info.PipelineCache = nullptr;
			init_info.DescriptorPool = descriptorPool;
			init_info.Allocator = nullptr;
			init_info.MinImageCount = 2;
			VulkanSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
			init_info.ImageCount = swapChain.GetImageCount();
			init_info.CheckVkResultFn = Utils::VulkanCheckResult;
			ImGui_ImplVulkan_Init(&init_info, swapChain.GetRenderPass());

			// Upload Fonts
			{
				// Use any command queue

				VkCommandBuffer commandBuffer = vulkanContext->GetCurrentDevice()->GetCommandBuffer(true);
				ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
				vulkanContext->GetCurrentDevice()->FlushCommandBuffer(commandBuffer);

				VK_CHECK_RESULT(vkDeviceWaitIdle(device));
				ImGui_ImplVulkan_DestroyFontUploadObjects();
			}

			uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
			s_ImGuiCommandBuffers.resize(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; i++)
				s_ImGuiCommandBuffers[i] = VulkanContext::GetCurrentDevice()->CreateSecondaryCommandBuffer("ImGuiSecondaryCoommandBuffer");
		});
	}

	void VulkanImGuiLayer::OnDetach()
	{
		Renderer::Submit([]()
		{
			auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			VK_CHECK_RESULT(vkDeviceWaitIdle(device));
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		});
	}

	void VulkanImGuiLayer::Begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void VulkanImGuiLayer::End()
	{
		ImGui::Render();

		VulkanSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();

		VkClearValue clearValues[2];
		clearValues[0].color = { {0.1f, 0.1f,0.1f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		uint32_t width = swapChain.GetWidth();
		uint32_t height = swapChain.GetHeight();

		uint32_t commandBufferIndex = swapChain.GetCurrentBufferIndex();

		VkCommandBufferBeginInfo drawCmdBufInfo = {};
		drawCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		drawCmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		drawCmdBufInfo.pNext = nullptr;

		VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &drawCmdBufInfo));

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2; // Color + depth
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

		vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = swapChain.GetRenderPass();
		inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

		VK_CHECK_RESULT(vkBeginCommandBuffer(s_ImGuiCommandBuffers[commandBufferIndex], &cmdBufInfo));

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = (float)height;
		viewport.height = -(float)height;
		viewport.width = (float)width;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(s_ImGuiCommandBuffers[commandBufferIndex], 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(s_ImGuiCommandBuffers[commandBufferIndex], 0, 1, &scissor);

		ImDrawData* main_draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, s_ImGuiCommandBuffers[commandBufferIndex]);

		VK_CHECK_RESULT(vkEndCommandBuffer(s_ImGuiCommandBuffers[commandBufferIndex]));

		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.push_back(s_ImGuiCommandBuffers[commandBufferIndex]);

		vkCmdExecuteCommands(drawCommandBuffer, uint32_t(commandBuffers.size()), commandBuffers.data());

		vkCmdEndRenderPass(drawCommandBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void VulkanImGuiLayer::OnImGuiRender()
	{
	}

}
