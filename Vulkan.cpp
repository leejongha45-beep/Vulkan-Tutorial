#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

constexpr uint32_t WIDTH  = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<char const*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class HelloTriangleApplication
{
public:
	void run()
	{
		initWindow();

		initVulkan();

		mainLoop();

		cleanup();
	}

private:
	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() { createInstance(); }

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		glfwDestroyWindow(window);

		glfwTerminate();
	}

private:
	void createInstance()
	{
		constexpr vk::ApplicationInfo appInfo{
			.pApplicationName	= "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName		= "No Engine",
			.engineVersion		= VK_MAKE_VERSION(1, 0, 0),
			.apiVersion			= vk::ApiVersion14};

		// 지원되는 레이어 계층인지 검사
		std::vector<const char*> requiredLayers;

		if (enableValidationLayers)
		{
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());
		}

		auto layerProperties	= context.enumerateInstanceLayerProperties();
		auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](const auto& requiredLayer) {
			return std::ranges::none_of(layerProperties, [requiredLayer](const auto& layerProperty) {
				return std::strcmp(layerProperty.layerName, requiredLayer) == 0;
			});
		});

		if (unsupportedLayerIt != requiredLayers.end())
		{
			throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
		}

		// 지원되는 확장인지 검사
		auto requiredExtensions = getRequiredInstanceExtensions();

		auto extensionProperties = context.enumerateInstanceExtensionProperties();

		auto unsupportedExtensionIt =
			std::ranges::find_if(requiredExtensions, [&extensionProperties](const auto& requiredExtension) {
				return std::ranges::none_of(extensionProperties, [requiredExtension](const auto& extensionProperty) {
					return std::strcmp(extensionProperty.extensionName, requiredExtension) == 0;
				});
			});

		if (unsupportedExtensionIt != requiredExtensions.end())
		{
			throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedExtensionIt));
		}

#ifdef __APPLE__
		requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

		// 인스턴스 정보 생성
		vk::InstanceCreateInfo createInfo{
#ifdef __APPLE__
			.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif
			.pApplicationInfo		 = &appInfo,
			.enabledLayerCount		 = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames	 = requiredLayers.data(),
			.enabledExtensionCount	 = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data(),

		};

		// Vulkan 인스턴스 생성 및 핸들 저장
		instance = vk::raii::Instance(context, createInfo);
	}

	std::vector<const char*> getRequiredInstanceExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions			= glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		return extensions;
	}

private:
	GLFWwindow* window;

	// RAII Context 저장변수
	vk::raii::Context context;

	// Instance Handle
	vk::raii::Instance instance = nullptr;
};

int main()
{
	try
	{
		HelloTriangleApplication app;
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}