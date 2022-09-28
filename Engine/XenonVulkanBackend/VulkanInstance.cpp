// Copyright 2022 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "VulkanInstance.hpp"
#include "VulkanMacros.hpp"

#include <sstream>

namespace /* anonymous */
{
	/**
	 * Check if the requested validation layers are available.
	 *
	 * @param layers The validation layers to check.
	 * @return Whether or not the validation layers are supported.
	 */
	bool CheckValidationLayerSupport(const std::vector<const char*>& layers)
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// Iterate through the layers and check if our layer exists.
		for (const char* layerName : layers)
		{
			bool layerFound = false;
			for (const VkLayerProperties& layerProperties : availableLayers)
			{
				// If the layer exists, we can mark the boolean as true.
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}

	/**
	 * Get all the required instance extensions.
	 *
	 * @return The instance extensions.
	 */
	std::vector<const char*> GetRequiredInstanceExtensions()
	{
		std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME , VK_KHR_DISPLAY_EXTENSION_NAME };

#if defined(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);

#elif defined(VK_FUCHSIA_IMAGEPIPE_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_FUCHSIA_IMAGEPIPE_SURFACE_EXTENSION_NAME);

#elif defined(VK_FUCHSIA_IMAGEPIPE_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_FUCHSIA_IMAGEPIPE_SURFACE_EXTENSION_NAME);

#elif defined(VK_MVK_IOS_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);

#elif defined(VK_MVK_MACOS_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);

#elif defined(VK_EXT_METAL_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);

#elif defined(VK_NN_VI_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_NN_VI_SURFACE_EXTENSION_NAME);

#elif defined(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);

#elif defined(FLINT_PLATFORM_WINDOWS)
		extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

#elif defined(VK_KHR_XCB_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);

#elif defined(VK_KHR_XLIB_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);

#elif defined(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME);

#elif defined(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME)
		extensions.emplace_back(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME);

#elif defined(VK_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME);

#elif defined(VK_QNX_SCREEN_SURFACE_EXTENSION_NAME)
		extensions.emplace_back(VK_QNX_SCREEN_SURFACE_EXTENSION_NAME);

#endif

#ifdef XENON_DEBUG
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#endif // XENON_DEBUG

		return extensions;
	}

	/**
	 * Vulkan debug callback.
	 * This function is used by Vulkan to report any internal message to the user.
	 *
	 * @param messageSeverity The severity of the message.
	 * @param messageType The type of the message.
	 * @param pCallbackData The data passed by the API.
	 * @param The user data submitted to Vulkan before this call.
	 * @return The status return.
	 */
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::stringstream messageStream;
		messageStream << "Vulkan Validation Layer : ";

		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			messageStream << "GENERAL | ";

		else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			messageStream << "VALIDATION | ";

		else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			messageStream << "PERFORMANCE | ";

		messageStream << pCallbackData->pMessage;

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			XENON_LOG_WARNING(messageStream.str());
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			XENON_LOG_ERROR(messageStream.str());
			break;

		default:
			XENON_LOG_INFORMATION(messageStream.str());
			break;
		}

		return VK_FALSE;
	}

	/**
	 * Create the default debug messenger create info structure.
	 *
	 * @return The created structure.
	 */
	VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.pNext = VK_NULL_HANDLE;
		createInfo.pUserData = VK_NULL_HANDLE;
		createInfo.flags = 0;
		createInfo.pfnUserCallback = DebugCallback;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		return createInfo;
	}

	/**
	 * Static initializer struct.
	 * These structs are used to initialize data that are to be initialized just once in the application.
	 */
	struct StaticInitializer
	{
		/**
		 * Default constructor.
		 */
		StaticInitializer()
		{
			// Initialize volk.
			XENON_VK_ASSERT(volkInitialize(), "Failed to initialize volk!");
		}
	};
}

namespace Xenon
{
	namespace Backend
	{
		VulkanInstance::VulkanInstance(const std::string& applicationName, uint32_t applicationVersion)
			: Instance(applicationName, applicationVersion)
		{
			// Setup the static initializer to initialize volk.
			static StaticInitializer initializer;

			// Create the instance.
			createInstance(applicationName, applicationVersion);

#ifdef XENON_DEBUG
			// Create the debugger.
			const auto debugMessengerCreateInfo = CreateDebugMessengerCreateInfo();
			const auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
			XENON_VK_ASSERT(vkCreateDebugUtilsMessengerEXT(m_Instance, &debugMessengerCreateInfo, nullptr, &m_DebugMessenger), "Failed to create the debug messenger.");

#endif // XENON_DEBUG
		}

		VulkanInstance::~VulkanInstance()
		{
#ifdef XENON_DEBUG
			// Destroy the debugger.
			const auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
			vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

#endif // XENON_DEBUG

			vkDestroyInstance(m_Instance, nullptr);
		}

		void VulkanInstance::createInstance(const std::string& applicationName, uint32_t applicationVersion)
		{
			// Setup the application information.
			VkApplicationInfo applicationInfo = {};
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.pNext = nullptr;
			applicationInfo.applicationVersion = applicationVersion;
			applicationInfo.pApplicationName = applicationName.data();
			applicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
			applicationInfo.pEngineName = "Xenon";
			applicationInfo.apiVersion = VulkanVersion;

			// Setup the instance create info.
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.pApplicationInfo = &applicationInfo;
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;

			const auto requiredExtensions = GetRequiredInstanceExtensions();
			createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
			createInfo.ppEnabledExtensionNames = requiredExtensions.data();

#ifdef XENON_DEBUG
			// Emplace the required validation layer.
			m_pValidationLayers.emplace_back("VK_LAYER_KHRONOS_validation");

			// Create the debug messenger create info structure.
			const auto debugCreateInfo = CreateDebugMessengerCreateInfo();

			// Submit it to the instance.
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_pValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_pValidationLayers.data();
			createInfo.pNext = &debugCreateInfo;

#endif // XENON_DEBUG

			// Create the instance.
			XENON_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_Instance), "Failed to create the instance!");

			// Load the instance functions.
			volkLoadInstance(m_Instance);
		}

	}
}