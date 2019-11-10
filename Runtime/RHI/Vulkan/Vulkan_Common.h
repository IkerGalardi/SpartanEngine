/*
Copyright(c) 2016-2019 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= IMPLEMENTATION ===============
#include "../RHI_Implementation.h"
#ifdef API_GRAPHICS_VULKAN
//================================

//= INCLUDES =================
#include <optional>
#include <set>
#include "../../Logging/Log.h"
#include "../RHI_Device.h"
#include "../../Math/Vector4.h"
#include "../RHI_Texture.h"
//============================

namespace Spartan::Vulkan_Common
{
    namespace error
    {
        inline const char* to_string(const VkResult result)
        {
            switch (result)
            {
                case VK_SUCCESS:                                            return "VK_SUCCESS";
                case VK_NOT_READY:											return "VK_NOT_READY";
                case VK_TIMEOUT:											return "VK_TIMEOUT";
                case VK_EVENT_SET:											return "VK_EVENT_SET";
                case VK_EVENT_RESET:										return "VK_EVENT_RESET";
                case VK_INCOMPLETE:											return "VK_INCOMPLETE";
                case VK_ERROR_OUT_OF_HOST_MEMORY:							return "VK_ERROR_OUT_OF_HOST_MEMORY";
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:							return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                case VK_ERROR_INITIALIZATION_FAILED:						return "VK_ERROR_INITIALIZATION_FAILED";
                case VK_ERROR_DEVICE_LOST:									return "VK_ERROR_DEVICE_LOST";
                case VK_ERROR_MEMORY_MAP_FAILED:							return "VK_ERROR_MEMORY_MAP_FAILED";
                case VK_ERROR_LAYER_NOT_PRESENT:							return "VK_ERROR_LAYER_NOT_PRESENT";
                case VK_ERROR_EXTENSION_NOT_PRESENT:						return "VK_ERROR_EXTENSION_NOT_PRESENT";
                case VK_ERROR_FEATURE_NOT_PRESENT:							return "VK_ERROR_FEATURE_NOT_PRESENT";
                case VK_ERROR_INCOMPATIBLE_DRIVER:							return "VK_ERROR_INCOMPATIBLE_DRIVER";
                case VK_ERROR_TOO_MANY_OBJECTS:								return "VK_ERROR_TOO_MANY_OBJECTS";
                case VK_ERROR_FORMAT_NOT_SUPPORTED:							return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                case VK_ERROR_FRAGMENTED_POOL:								return "VK_ERROR_FRAGMENTED_POOL";
                case VK_ERROR_OUT_OF_POOL_MEMORY:							return "VK_ERROR_OUT_OF_POOL_MEMORY";
                case VK_ERROR_INVALID_EXTERNAL_HANDLE:						return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
                case VK_ERROR_SURFACE_LOST_KHR:								return "VK_ERROR_SURFACE_LOST_KHR";
                case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:						return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
                case VK_SUBOPTIMAL_KHR:										return "VK_SUBOPTIMAL_KHR";
                case VK_ERROR_OUT_OF_DATE_KHR:								return "VK_ERROR_OUT_OF_DATE_KHR";
                case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:						return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
                case VK_ERROR_VALIDATION_FAILED_EXT:						return "VK_ERROR_VALIDATION_FAILED_EXT";
                case VK_ERROR_INVALID_SHADER_NV:							return "VK_ERROR_INVALID_SHADER_NV";
                case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
                case VK_ERROR_FRAGMENTATION_EXT:							return "VK_ERROR_FRAGMENTATION_EXT";
                case VK_ERROR_NOT_PERMITTED_EXT:							return "VK_ERROR_NOT_PERMITTED_EXT";
                case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:					return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
                case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
            }

            return "Unknown error code";
        }

        inline bool check_result(VkResult result)
        {
            if (result == VK_SUCCESS)
                return true;

            LOG_ERROR("%s", to_string(result));
            return false;
        }

        inline void assert_result(VkResult result)
        {
            SPARTAN_ASSERT(result == VK_SUCCESS);
        }
    }

	namespace memory
	{
		inline uint32_t get_type(const VkPhysicalDevice device, const VkMemoryPropertyFlags properties, const uint32_t type_bits)
		{
			VkPhysicalDeviceMemoryProperties prop;
			vkGetPhysicalDeviceMemoryProperties(device, &prop);
			for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
				if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
					return i;

			return 0xFFFFFFFF; // Unable to find memoryType
		}

		inline void free(const std::shared_ptr<RHI_Device>& rhi_device, void*& device_memory)
		{
			if (!device_memory)
				return;

			vkFreeMemory(rhi_device->GetContextRhi()->device, static_cast<VkDeviceMemory>(device_memory), nullptr);
			device_memory = nullptr;
		}
	}

	namespace command
	{
        inline bool create_pool(const std::shared_ptr<RHI_Device>& rhi_device, VkCommandPool& command_pool)
        {
            VkCommandPoolCreateInfo cmd_pool_info   = {};
            cmd_pool_info.sType                     = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmd_pool_info.queueFamilyIndex          = rhi_device->GetContextRhi()->indices.graphics_family.value();
            cmd_pool_info.flags                     = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            return error::check_result(vkCreateCommandPool(rhi_device->GetContextRhi()->device, &cmd_pool_info, nullptr, &command_pool));
        }

		inline bool create_buffer(const std::shared_ptr<RHI_Device>& rhi_device, VkCommandPool& cmd_pool, VkCommandBuffer& command_buffer, const VkCommandBufferLevel level)
		{
			VkCommandBufferAllocateInfo allocate_info	= {};
			allocate_info.sType							= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocate_info.commandPool					= cmd_pool;
			allocate_info.level							= level;
            allocate_info.commandBufferCount            = 1;

            return error::check_result(vkAllocateCommandBuffers(rhi_device->GetContextRhi()->device, &allocate_info, &command_buffer));
		}

        inline bool begin(const std::shared_ptr<RHI_Device>& rhi_device, VkCommandPool& command_pool, VkCommandBuffer& command_buffer)
        {
            // Create command pool
            if (!create_pool(rhi_device, command_pool))
                return false;

            // Create command buffer
            if (!create_buffer(rhi_device, command_pool, command_buffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY))
                return false;

            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            return error::check_result(vkBeginCommandBuffer(command_buffer, &begin_info));
        }

        inline bool end(const std::shared_ptr<RHI_Device>& rhi_device, VkCommandPool& command_pool, VkCommandBuffer& command_buffer)
        {
            if (!error::check_result(vkEndCommandBuffer(command_buffer)))
                return false;

            VkSubmitInfo submit_info = {};
            submit_info.sType               = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount  = 1;
            submit_info.pCommandBuffers     = &command_buffer;

            auto queue = rhi_device->GetContextRhi()->queue_copy;

            if (!error::check_result(vkQueueSubmit(queue, 1, &submit_info, nullptr)))
                return false;

            if (!error::check_result(vkQueueWaitIdle(queue)))
                return false;

            vkFreeCommandBuffers(rhi_device->GetContextRhi()->device, command_pool, 1, &command_buffer);
            return true;
        }
	}

	namespace semaphore
	{
		inline void* create(const std::shared_ptr<RHI_Device>& rhi_device)
		{
			VkSemaphoreCreateInfo semaphore_info	= {};
			semaphore_info.sType					= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkSemaphore semaphore_out;
			SPARTAN_ASSERT(vkCreateSemaphore(rhi_device->GetContextRhi()->device, &semaphore_info, nullptr, &semaphore_out) == VK_SUCCESS);

			return static_cast<void*>(semaphore_out);
		}

		inline void destroy(const std::shared_ptr<RHI_Device>& rhi_device, void*& semaphore_in)
		{
			if (!semaphore_in)
				return;

			vkDestroySemaphore(rhi_device->GetContextRhi()->device, static_cast<VkSemaphore>(semaphore_in), nullptr);
			semaphore_in = nullptr;
		}
	}

	namespace fence
	{
		inline void* create(const std::shared_ptr<RHI_Device>& rhi_device)
		{
			VkFenceCreateInfo fence_info	= {};
			fence_info.sType				= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
			VkFence fence_out;
			SPARTAN_ASSERT(vkCreateFence(rhi_device->GetContextRhi()->device, &fence_info, nullptr, &fence_out) == VK_SUCCESS);

			return static_cast<void*>(fence_out);
		}

		inline void destroy(const std::shared_ptr<RHI_Device>& rhi_device, void*& fence_in)
		{
			if (!fence_in)
				return;

			vkDestroyFence(rhi_device->GetContextRhi()->device, static_cast<VkFence>(fence_in), nullptr);
			fence_in = nullptr;
		}

		inline void wait(const std::shared_ptr<RHI_Device>&rhi_device, void*& fence_in)
		{
            const auto fence_temp = reinterpret_cast<VkFence*>(&fence_in);
			SPARTAN_ASSERT(vkWaitForFences(rhi_device->GetContextRhi()->device, 1, fence_temp, true, 0xFFFFFFFFFFFFFFFF) == VK_SUCCESS);
		}

		inline void reset(const std::shared_ptr<RHI_Device>& rhi_device, void*& fence_in)
		{
            const auto fence_temp = reinterpret_cast<VkFence*>(&fence_in);
			SPARTAN_ASSERT(vkResetFences(rhi_device->GetContextRhi()->device, 1, fence_temp) == VK_SUCCESS);
		}

		inline void wait_reset(const std::shared_ptr<RHI_Device>& rhi_device, void*& fence_in)
		{
			const auto fence_temp = reinterpret_cast<VkFence*>(&fence_in);
			SPARTAN_ASSERT(vkWaitForFences(rhi_device->GetContextRhi()->device, 1, fence_temp, true, 0xFFFFFFFFFFFFFFFF) == VK_SUCCESS);
			SPARTAN_ASSERT(vkResetFences(rhi_device->GetContextRhi()->device, 1, fence_temp) == VK_SUCCESS);
		}
	}

	namespace buffer
	{
		inline bool create(const std::shared_ptr<RHI_Device>& rhi_device, VkBuffer& _buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& size, VkBufferUsageFlags usage)
		{
			VkBufferCreateInfo buffer_info	= {};
			buffer_info.sType				= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_info.size				= size;
			buffer_info.usage				= usage;
			buffer_info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;

			if (!error::check_result(vkCreateBuffer(rhi_device->GetContextRhi()->device, &buffer_info, nullptr, &_buffer)))
				return false;

			VkMemoryRequirements memory_requirements;
			vkGetBufferMemoryRequirements(rhi_device->GetContextRhi()->device, _buffer, &memory_requirements);

			VkMemoryAllocateInfo alloc_info = {};			
			alloc_info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize		= memory_requirements.size;
			alloc_info.memoryTypeIndex		= memory::get_type(rhi_device->GetContextRhi()->device_physical, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memory_requirements.memoryTypeBits);

            if (!error::check_result(vkAllocateMemory(rhi_device->GetContextRhi()->device, &alloc_info, nullptr, &buffer_memory)))
                return false;

            if (!error::check_result(vkBindBufferMemory(rhi_device->GetContextRhi()->device, _buffer, buffer_memory, 0)))
                return false;

			return true;
		}

		inline void destroy(const std::shared_ptr<RHI_Device>& rhi_device, void*& _buffer)
		{
			if (!_buffer)
				return;

			vkDestroyBuffer(rhi_device->GetContextRhi()->device, static_cast<VkBuffer>(_buffer), nullptr);
			_buffer = nullptr;
		}
	}

    namespace image
    {
        inline VkImageAspectFlags bind_flags_to_aspect_mask(const uint16_t bind_flags)
        {
            // Resolve aspect mask
            VkImageAspectFlags aspect_mask = 0;
            if (bind_flags & RHI_Texture_DepthStencil)
            {
                // Depth-only image formats can have only the VK_IMAGE_ASPECT_DEPTH_BIT set
                aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            else
            {
                aspect_mask |= (bind_flags & RHI_Texture_Sampled)       ? VK_IMAGE_ASPECT_COLOR_BIT : 0;
                aspect_mask |= (bind_flags & RHI_Texture_RenderTarget)  ? VK_IMAGE_ASPECT_COLOR_BIT : 0;
            }

            return aspect_mask;
        }

        inline bool create_view(const std::shared_ptr<RHI_Device>& rhi_device, VkImage& image, VkImageView* image_view, const VkFormat format, const VkImageAspectFlags aspect_flags)
        {
            VkImageViewCreateInfo create_info = {};
            create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image                           = image;
            create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format                          = format;
            create_info.subresourceRange.aspectMask     = aspect_flags;
            create_info.subresourceRange.baseMipLevel   = 0;
            create_info.subresourceRange.levelCount     = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount     = 1;
            create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

            return error::check_result(vkCreateImageView(rhi_device->GetContextRhi()->device, &create_info, nullptr, image_view));
        }

        inline bool create_frame_buffer(const std::shared_ptr<RHI_Device>& rhi_device, const VkRenderPass& render_pass, const std::vector<VkImageView> attachments, const uint32_t width, const uint32_t height, VkFramebuffer* frame_buffer)
        {
            VkFramebufferCreateInfo create_info = {};
            create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.renderPass      = render_pass;
            create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            create_info.pAttachments    = attachments.data();
            create_info.width           = width;
            create_info.height          = height;
            create_info.layers          = 1;

            return error::check_result(vkCreateFramebuffer(rhi_device->GetContextRhi()->device, &create_info, nullptr, frame_buffer));
        }
    }

    namespace extension
    {
        inline bool is_present(const char* extension_name)
        {
            uint32_t layer_count;
            vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

            std::vector<VkLayerProperties> available_layers(layer_count);
            vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

            for (const auto& layer_properties : available_layers)
            {
                if (strcmp(extension_name, layer_properties.layerName) == 0)
                    return true;
            }

            return false;
        }
    }

    namespace debug
    {
        static VKAPI_ATTR VkBool32 VKAPI_CALL callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
            VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
            void* p_user_data
        ) {
            if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            {
                LOG_INFO(p_callback_data->pMessage);
            }
            else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            {
                LOG_INFO(p_callback_data->pMessage);
            }
            else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                LOG_WARNING(p_callback_data->pMessage);
            }
            else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                LOG_ERROR(p_callback_data->pMessage);
            }

            return VK_FALSE;
        }

        inline VkResult create(RHI_Device* rhi_device, const VkDebugUtilsMessengerCreateInfoEXT* create_info)
        {
            if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(rhi_device->GetContextRhi()->instance, "vkCreateDebugUtilsMessengerEXT")))
                return func(rhi_device->GetContextRhi()->instance, create_info, nullptr, &rhi_device->GetContextRhi()->callback_handle);

            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        inline void destroy(RHI_Context* context)
        {
            if (!context->validation_enabled)
                return;

            if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT")))
            {
                func(context->instance, context->callback_handle, nullptr);
            }
        }
    }

    namespace debug_marker
    {
        static bool active = false;

        // The debug marker extension is not part of the core, so function pointers need to be loaded manually
        static PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin  = VK_NULL_HANDLE;
        static PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd      = VK_NULL_HANDLE;

        inline void setup(VkDevice device)
        {
            bool is_extension_present = extension::is_present(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);

            if (is_extension_present)
            {
                pfnCmdDebugMarkerBegin  = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT"));
                pfnCmdDebugMarkerEnd    = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT"));
            }
            else
            {
                LOG_WARNING("Extension \"%s\" not present, debug markers are disabled.", VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
                LOG_INFO("Try running from inside a Vulkan graphics debugger (e.g. RenderDoc)");
            }

            active = (pfnCmdDebugMarkerBegin != VK_NULL_HANDLE) && (pfnCmdDebugMarkerEnd != VK_NULL_HANDLE);
        }

        inline void begin(VkCommandBuffer cmd_buffer, const char* name, const Math::Vector4& color)
        {
            if (!active)
                return;

            VkDebugMarkerMarkerInfoEXT marker_info = {};
            marker_info.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
            memcpy(marker_info.color, color.Data(), sizeof(float) * 4);
            marker_info.pMarkerName = name;
            pfnCmdDebugMarkerBegin(cmd_buffer, &marker_info);
        }

        inline void end(VkCommandBuffer cmdBuffer)
        {
            if (!active)
                return;

            pfnCmdDebugMarkerEnd(cmdBuffer);
        }
    };
}

#endif
