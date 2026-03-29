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

		// GLFW 필요 확장 목록 가져오기
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions			= glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		// GLFW 필요 확장의 Vulkan 지원 여부 확인
		auto extensionProperties = context.enumerateInstanceExtensionProperties();
		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		{
			if (std::ranges::none_of(
					extensionProperties, [glfwExtension = glfwExtensions[i]](auto const& extensionProperty) {
						return std::strcmp(extensionProperty.extensionName, glfwExtension) == 0;
					}))
			{
				throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
			}
		}

		// 확장 목록 구성
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef __APPLE__
		extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

		// 인스턴스 정보 생성
		vk::InstanceCreateInfo createInfo{
#ifdef __APPLE__
			.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif
			.pApplicationInfo		 = &appInfo,
			.enabledExtensionCount	 = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()};

		// Vulkan 인스턴스 생성 및 핸들 저장
		instance = vk::raii::Instance(context, createInfo);

		// 지원되는 확장 모듈 출력
		auto availableExtensions = context.enumerateInstanceExtensionProperties();
		std::cout << "available extensions:\n";
		for (const auto& availableExtension : availableExtensions)
		{
			std::cout << "\t" << availableExtension.extensionName << std::endl;
		}

		// 실제 사용가능한 확장 출력
		std::cout << "==========================================" << std::endl;

		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		{
			if (std::ranges::any_of(
					availableExtensions, [glfwExtension = glfwExtensions[i]](const auto& availableExtension) {
						return std::strcmp(availableExtension.extensionName, glfwExtension) == 0;
					}))
			{
				std::cout << availableExtensions[i].extensionName << std::endl;
			}
		}
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