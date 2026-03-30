#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <map>
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

	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		pickPhysicalDevice();
		createLogicalDevice();
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
		auto requiredExtensions	 = getRequiredInstanceExtensions();
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

	/**
	 * 필수 인스턴스 확장 목록을 반환하는 함수
	 */
	std::vector<const char*> getRequiredInstanceExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions			= glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers)
		{
			extensions.push_back(vk::EXTDebugUtilsExtensionName);
		}

		return extensions;
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers)
			return;

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
			.messageSeverity = severityFlags, .messageType = messageTypeFlags, .pfnUserCallback = &debugCallback};

		debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	/**
	 * @vk::DebugUtilsMessageSeverityFlagBitsEXT
	 * vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose : Vulkan 구성요소에서 보내는 진단 메세지
	 * vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo : 리소스 생성과 같은 정보성 메세지
	 * vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning : 오류는 아니지만 애플리케이션의 버그일 가능성이 매우 높은
	 * 동작에 대한 메시지 vk::DebugUtilsMessageSeverityFlagBitsEXT::eError : 유효하지않으며 충돌을 유발할 수 있는 동작에
	 * 대한 메시지
	 * @vk::DebugUtilsMessageTypeFlagBitsEXT
	 * vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral : 사양이나 성능과는 무관한 어떤사건이 발생
	 * vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation : 사양을 위반하는 상황이 발생했거나 오류 가능성을 시사하는
	 * 상황이 발생 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance : Vulkan의 최적화되지 않은 사용 가능성
	 * @vk::DeubgUtilsMessengerCallbackDataEXT
	 * vk::DeubgUtilsMessengerCallbackDataEXT::pMessage : 디버그 메시지는 null로 끝나는 문자열
	 * vk::DeubgUtilsMessengerCallbackDataEXT::pObjects : 메시지와 관련된 Vulkan 객체 핸들 배열
	 * vk::DeubgUtilsMessengerCallbackDataEXT::objectCount : 배열에 있는 객체의 개수
	 */
	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
		const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		const char* color = "\033[0m";

		switch (severity)
		{
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
			color = "\033[90m"; // 회색
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
			color = "\033[37m"; // 흰색
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
			color = "\033[33m"; // 노란색
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
			color = "\033[31m"; // 빨간색
			break;
		}
		std::cerr << color << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage
				  << "\033[0m" << std::endl;

		return vk::False;
	}

	void pickPhysicalDevice()
	{
		std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		const auto devIter									  = std::ranges::find_if(
			   physicalDevices, [&](const auto& physicalDevice) { return isDeviceSuitable(physicalDevice); });

		if (devIter == physicalDevices.end())
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}

		physicalDevice = *devIter;
	}

	bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice)
	{
		// 장치가 Vulkan1.3 API버전을 지원하는지 확인
		bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

		// 큐 패밀리중 그래픽작업을 지원하는 것이 있는지 확인
		auto queueFamilies	  = physicalDevice.getQueueFamilyProperties();
		bool supportsGraphics = std::ranges::any_of(
			queueFamilies, [](const auto& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

		// 필요한 모든 장치 확장기능이 사용가능한지 확인
		auto availableDeviceExtensions	   = physicalDevice.enumerateDeviceExtensionProperties();
		bool supportsAllRequiredExtensions = std::ranges::all_of(
			requiredDeviceExtension, [&availableDeviceExtensions](const auto& requiredDeviceExtension) {
				return std::ranges::any_of(
					availableDeviceExtensions, [requiredDeviceExtension](const auto& availableDeviceExtension) {
						return std::strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
					});
			});

		// 장치가 필요한 기능을 지원하는지 확인
		auto features = physicalDevice.template getFeatures2<
			vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
		bool supportsRequiredFeatures =
			features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
			features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

		// 모두 가능하면 true
		return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
	}

	void createLogicalDevice()
	{
		// 그래픽을 지원하는 첫번쨰 큐 패밀리의 인덱스를 찾기
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		// 그래픽을 지원하는 queueFamilyPropertise의 첫 번쨰 인덱스 가져오기
		auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](const auto& qfp) {
			return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
		});
		auto graphicsIndex =
			static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

		// Vulkan 1.3 기능 쿼리 (기능 구조체 체인 생성)
		vk::StructureChain<
			vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
			featureChain = {{}, {.dynamicRendering = true}, {.extendedDynamicState = true}};

		// 큐 정보 생성
		float queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
			.queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};

		// 논리 장치 정보 생성
		vk::DeviceCreateInfo deviceCreateInfo{
			.pNext					 = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount	 = 1,
			.pQueueCreateInfos		 = &deviceQueueCreateInfo,
			.enabledExtensionCount	 = static_cast<uint32_t>(requiredDeviceExtension.size()),
			.ppEnabledExtensionNames = requiredDeviceExtension.data()};

		// 큐, 논리장치 생성
		device		  = vk::raii::Device(physicalDevice, deviceCreateInfo);
		graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
	}

private:
	GLFWwindow* window;

	// RAII Context 저장변수
	vk::raii::Context context;

	// Instance Handle
	vk::raii::Instance instance = nullptr;

	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

	// GPU 장치
	vk::raii::PhysicalDevice physicalDevice = nullptr;

	std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};

	vk::raii::Device device		  = nullptr;
	vk::raii::Queue graphicsQueue = nullptr;
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