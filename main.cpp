#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <map>
#include <optional>
#include <set>

#include <cstdint>
#include <limits>
#include <algorithm> 

namespace Core
{
	class VulkanTriangle
	{
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool isComplete()
			{
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

	private:
		GLFWwindow* window = nullptr;

		const uint32_t WIDTH = 800;
		const uint32_t HEIGHT = 600;

		VkInstance instance = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger = nullptr;
		VkPhysicalDevice physicalDevice = nullptr;
		VkDevice device = nullptr;
		VkSurfaceKHR surface = nullptr;
		VkQueue presentQueue = nullptr;

		VkSwapchainKHR swapChain = nullptr; 
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkImageView> swapChainImageViews;

		const std::vector<const char*> validationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};
		const std::vector<const char*> deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		void createSurface()
		{
			VkWin32SurfaceCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hwnd = glfwGetWin32Window(window);
			createInfo.hinstance = GetModuleHandle(nullptr);

			if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create window surface!");
			}
		}

		void CreateSwapChain()
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

			VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
			VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

			uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

			if (swapChainSupport.capabilities.maxImageCount > 0 && 
				imageCount > swapChainSupport.capabilities.maxImageCount)
			{
				imageCount = swapChainSupport.capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
			uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

			if (indices.graphicsFamily != indices.presentFamily)
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0; // Optional
				createInfo.pQueueFamilyIndices = nullptr; // Optional
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create swap chain!");
			}

			
			vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
			swapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

			swapChainImageFormat = surfaceFormat.format;
			swapChainExtent = extent;
		}

		void CreateImageViews()
		{
			swapChainImageViews.resize(swapChainImages.size());

			for (size_t i = 0; i < swapChainImages.size(); i++)
			{
				VkImageViewCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = swapChainImages[i];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = swapChainImageFormat;
				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;

				if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) 
				{
					throw std::runtime_error("failed to create image views!");
				}
			}
		}


	public:
		void run() {
			initWindow();
			initVulkan();
			mainLoop();
			cleanup();
		}

	private:
		void initWindow()
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, false);
			glfwWindowHint(GLFW_RESIZABLE, false);

			window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulkan", nullptr, nullptr);
		}

		void initVulkan()
		{
			createInstance();
			setupDebugMessenger();
			createSurface();
			pickPhysicalDevice();
			createLogicalDevice();
			CreateSwapChain();
			CreateImageViews();
		}

		void createInstance()
		{
			if (enableValidationLayers && !checkValidationLayerSupport())
			{
				throw std::runtime_error("validation layers requested, but not available!");
			}

			VkApplicationInfo appInfo{};

			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName = "Hello Vulkan";
			appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
			appInfo.pEngineName = "Empty Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;

			auto requiredExt = getRequiredExtensions();

			VkInstanceCreateInfo createInfo{};

			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExt.size());
			createInfo.ppEnabledExtensionNames = requiredExt.data();

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			if (enableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();

				populateDebugMessengerCreateInfo(debugCreateInfo);
				createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
			}
			else
			{
				createInfo.enabledLayerCount = 0;

				createInfo.pNext = nullptr;
			}

			if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create instance!");
			}
		}

		void setupDebugMessenger()
		{
			if (!enableValidationLayers) return;

			VkDebugUtilsMessengerCreateInfoEXT createInfo{};
			populateDebugMessengerCreateInfo(createInfo);

			if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to set up debug messenger!");
			}
		}

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debugMessenger, pAllocator);
			}
		}

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
		{
			createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;
		}

		bool checkValidationLayerSupport()
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : validationLayers)
			{
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
				{
					return false;
				}
			}

			return true;
		}

		std::vector<const char*> getRequiredExtensions()
		{
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

			if (enableValidationLayers)
			{
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			return extensions;
		}

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
		{
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}

		void mainLoop()
		{
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
			}
		}

		void pickPhysicalDevice()
		{
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0)
			{
				throw std::runtime_error("failed to find GPUs with Vulkan support!");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			std::multimap<int, VkPhysicalDevice> candidates;


			for (const auto& device : devices)
			{
				if (!isDeviceSuitable(device))
				{
					continue;
				}

				int score = rateDeviceSuitability(device);
				candidates.insert(std::make_pair(score, device));
			}

			if (candidates.rbegin()->first > 0)
			{
				physicalDevice = candidates.rbegin()->second;
			}
			else
			{
				throw std::runtime_error("failed to find a suitable GPU!");
			}

			if (physicalDevice == nullptr)
			{
				throw std::runtime_error("failed to find a suitable GPU!");
			}
		}

		void createLogicalDevice()
		{
			QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

			float queuePriority = 1.0f;
			for (uint32_t queueFamily : uniqueQueueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures{};

			VkDeviceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.pEnabledFeatures = &deviceFeatures;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			if (enableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create logical device!");
			}

			VkQueue graphicsQueue;
			vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
			vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
		}

		int rateDeviceSuitability(const VkPhysicalDevice& device)
		{
			int score = 0;

			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;

			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			// Discrete GPUs have a significant performance advantage
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				score += 1000;
			}

			// Maximum possible size of textures affects graphics quality
			score += deviceProperties.limits.maxImageDimension2D;

			if (!deviceFeatures.geometryShader)
			{
				return 0;
			}

			return score;
		}

		bool isDeviceSuitable(VkPhysicalDevice device)
		{
			QueueFamilyIndices indices = FindQueueFamilies(device);

			bool extensionsSupported = CheckDeviceExtensionSupport(device);

			bool swapChainAdequate = false;

			if (extensionsSupported)
			{
				SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
				swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
			}

			return indices.isComplete() && extensionsSupported && swapChainAdequate;
		}

		bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}

			return requiredExtensions.empty();
		}

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
		{
			QueueFamilyIndices indices;

			VkBool32 presentSupport = false;


			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			int i = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

				if (presentSupport)
				{
					indices.presentFamily = i;
				}

				if (queueFamily.queueFlags &
					VK_QUEUE_COMPUTE_BIT)
				{
					indices.graphicsFamily = i;
					break;
				}

				i++;
			}

			return indices;
		}

		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
		{
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0)
			{
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			for(const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
					availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
		{
			for (const auto& availablePresentMode : availablePresentModes) 
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
				{
					return availablePresentMode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
		{
			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				return capabilities.currentExtent;
			}
			else 
			{
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);

				VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}
		}

		void cleanup()
		{
			for (auto imageView : swapChainImageViews) {
				vkDestroyImageView(device, imageView, nullptr);
			}

			vkDestroySwapchainKHR(device, swapChain, nullptr);
			vkDestroyDevice(device, nullptr);

			if (enableValidationLayers) 
			{
				DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
			}

			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroyInstance(instance, nullptr);

			glfwDestroyWindow(window);

			glfwTerminate();
		}
	};
}

int main()
{
	Core::VulkanTriangle app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}