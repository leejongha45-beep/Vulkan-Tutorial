#include "Vulkan.h"
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <cstdlib>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef  NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif //  NDEBUG


class HelloTriangleApplication
{
public:
	void run()
	{
		initVulkan();

		mainLoop();

		cleanup();
	}

private:
	void initVulkan()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

		createInstance();
	}

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

	void createInstance()
	{
		// 애플리케이션에 대한 정보를 구조체에 입력
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hell Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// (전역)VKInstance 생성 정보 구조체 설정
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// 창과 상호작용을 해야하므로 GLFW로 VulkanInstance에 생성정보 넣기
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

		/**
		 * &createInfo : 생성 정보가 포함된 구조체에 대한 포인터
		 * nullptr : nullptr이 튜토리얼에서는 항상 사용자 지정 할당자 콜백에 대한 포인터를 사용합니다. (커스텀 메모리 할당자 :  nullptr = 커스텀 사용안함)
		 * &instance : 새 객체의 핸들을 저장하는 변수에 대한 포인터 
		 */
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}

		/**
		 * vkEnumerateInstanceExtensionProperties : 인스턴스를 생성하기 전에 지원되는 확장 프로그램 목록을 가져오는 함수 
		 * 확장 프로그램 갯수(extensionCount)를 저장하는 변수에 대한 포인터와 VkExtensionProperties(확장 프로그램 세부정보) 배열을 인수로 받음
		 */
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "availabe extensions: \n";

		for (const auto& extension : extensions)
		{
			std::cout << '\t' << extension.extensionName << '\n';
		}

		std::cout << "===================================================" << std::endl;

		for (int i = 0; i < glfwExtensionCount; i++)
		{
			std::cout << '\t' << glfwExtensions[i] << '\n';
		}

		std::cout << "===================================================" << std::endl;

		for (int i = 0; i < glfwExtensionCount; i++)
		{
			const char* tempGlfw = glfwExtensions[i];

			for (const auto& extension : extensions)
			{
				const char* tempVK = extension.extensionName;
				const int rt = std::strcmp(tempGlfw, tempVK);

				if (rt == 0)
				{
					std::cout << tempGlfw << " supported" << std::endl;
				}
			}
		}
	}

	// 요청된 레이어가 모두 사용가능한지 확인하는 함수
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
				const char* tempLayerName = layerProperties.layerName;
				const int rt = std::strcmp(layerName, tempLayerName);

				if (rt == 0)
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

private:
	GLFWwindow* window;

	VkInstance instance;
};

int main()
{
	HelloTriangleApplication app;

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
