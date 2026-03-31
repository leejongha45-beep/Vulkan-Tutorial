#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
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
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createGraphicsPipeline();
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

	void createSurface()
	{
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
		{
			throw std::runtime_error("failed to create window surface!");
		}
		surface = vk::raii::SurfaceKHR(instance, _surface);
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
		uint32_t queueIndex											 = ~0;
		for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
		{
			if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
				physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
			{
				queueIndex = qfpIndex;
				break;
			}
		}
		if (queueIndex == ~0)
		{
			throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
		}

		// Vulkan 1.3 기능 쿼리 (기능 구조체 체인 생성)
		vk::StructureChain<
			vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
			featureChain = {
				{},								// vk::PhysicalDeviceFeatures2
				{.shaderDrawParameters = true}, // vk::PhysicalDeviceVulkan11Features
				{.dynamicRendering = true},		// vk::PhysicalDeviceVulkan13Features
				{.extendedDynamicState = true}	// vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
			};

		// 큐 정보 생성
		float queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
			.queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};

		// 논리 장치 정보 생성
		vk::DeviceCreateInfo deviceCreateInfo{
			.pNext					 = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount	 = 1,
			.pQueueCreateInfos		 = &deviceQueueCreateInfo,
			.enabledExtensionCount	 = static_cast<uint32_t>(requiredDeviceExtension.size()),
			.ppEnabledExtensionNames = requiredDeviceExtension.data()};

		// 큐, 논리장치 생성
		device		  = vk::raii::Device(physicalDevice, deviceCreateInfo);
		graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
	}

	void createSwapChain()
	{
		// 기본 표면 기능 조회 (스왑체인의 최소/최대 이미지 수, 이미지의 최소/최대 너비 및 높이)
		vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
		swapChainExtent								   = chooseSwapExtent(surfaceCapabilities);
		uint32_t minImageCount						   = chooseSwapMinImageCount(surfaceCapabilities);

		// 지원되는 surface 형식 조회 (픽셀 형식, 색 공간)
		std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
		swapChainSurfaceFormat							   = chooseSwapSurfaceFormat(availableFormats);

		// 프레전테이션 모드 조회
		std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
		vk::PresentModeKHR presentMode						  = choosseSwapPresentMode(availablePresentModes);

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
			.surface		  = *surface,
			.minImageCount	  = minImageCount,
			.imageFormat	  = swapChainSurfaceFormat.format,
			.imageColorSpace  = swapChainSurfaceFormat.colorSpace,
			.imageExtent	  = swapChainExtent,
			.imageArrayLayers = 1, // 각 이미지가 구성하는 레이어 수 지정
			.imageUsage =
				vk::ImageUsageFlagBits::eColorAttachment,	 // 스왑 체인에 있는 이미지를 어떤 작업에 사용할지 지정
			.imageSharingMode = vk::SharingMode::eExclusive, // 여러 큐 패밀리에서 사용될 수 있는 스왑 체인 이미지를
															 // 처리하는 방법을 지정
			.preTransform =
				surfaceCapabilities.currentTransform, // 스왑 체인의 이미지에 특정 변환을 적용하도록 지정 할수 있게함
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque, // 창 시스템 내의 다른창과 블랜딩할 떄 알파 채널을
																	  // 사용할지 여부를 지정
			.presentMode = presentMode,
			.clipped	 = true};

		swapChainCreateInfo.oldSwapchain = nullptr;

		swapChain		= vk::raii::SwapchainKHR(device, swapChainCreateInfo);
		swapChainImages = swapChain.getImages();
	}
	// 표면형식 (색심도)
	static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
	{
		const auto formatIt = std::ranges::find_if(availableFormats, [](const auto format) {
			return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});

		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	// 프레젠테이션 모드 (화면에 이미지를 교체하는 조건)
	static vk::PresentModeKHR choosseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
	{
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
			return presentMode == vk::PresentModeKHR::eFifo;
		}));

		return std::ranges::any_of(
				   availablePresentModes,
				   [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; })
				   ? vk::PresentModeKHR::eMailbox
				   : vk::PresentModeKHR::eFifo;
	}

	// 스왑 범위 (스왑체인 내 이미지 해상도)
	vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
	{
		// 윈도우 계열 (스왑체인 이미지 크기 == 창크기)
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		// 맥북 레티나 계열 (스왑체인 이미지크기 != 창크기)
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
	}

	// 스왑체인에 포함될 이미지의 갯수가 최대상한을 초과하지않게 제한
	static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilites)
	{
		auto minImageCount = std::max(3u, surfaceCapabilites.minImageCount);
		if ((0 < surfaceCapabilites.maxImageCount) && (surfaceCapabilites.maxImageCount < minImageCount))
		{
			minImageCount = surfaceCapabilites.maxImageCount;
		}
		return minImageCount;
	}

	void createImageViews()
	{
		assert(swapChainImageViews.empty());

		// 이미지 뷰 정보 생성
		vk::ImageViewCreateInfo imageViewCreateInfo{
			.viewType		  = vk::ImageViewType::e2D,
			.format			  = swapChainSurfaceFormat.format,
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

		for (auto& image : swapChainImages)
		{
			imageViewCreateInfo.image = image;
			swapChainImageViews.emplace_back(device, imageViewCreateInfo);
		}
	}

	void createGraphicsPipeline()
	{
		vk::raii::ShaderModule shaderModule = createShaderModule(
			readFile("C:/Users/leejo/00.workspace/Git/Vulkan/Practice/2026.03.28/Vulkan/shader/slang.spv"));

		// 셰이더 정보 입력
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
			.stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
		vk::PipelineShaderStageCreateInfo fragShaerStageInfo{
			.stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaerStageInfo};

		// 버텍스 정보 입력
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		// 입력어셈블리 정보 입력
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
		// 뷰포트와 가위 정보 입력 (정적)
		vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

		// 레스터라이징 정보 입력
		vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable		 = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode			 = vk::PolygonMode::eFill,
			.cullMode				 = vk::CullModeFlagBits::eBack,
			.frontFace				 = vk::FrontFace::eClockwise,
			.lineWidth				 = 1.0f};

		// 멀티샘플링 정보 입력 (안티앨리어싱)
		vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};

		// 연결된 프레임버퍼별 구성 설정
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable	= vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
							  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		// 색상 혼합 정보 설정
		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable	 = vk::False,
			.logicOp		 = vk::LogicOp::eCopy,
			.attachmentCount = 1,
			.pAttachments	 = &colorBlendAttachment};

		std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamicState{
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0, .pushConstantRangeCount = 0};
		pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		// 파이프라인 렌더링 정보생성 (정적)
		//vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
		//	.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format};

		//vk::GraphicsPipelineCreateInfo pipelineInfo{
		//	.pNext				 = &pipelineRenderingCreateInfo,
		//	.stageCount			 = 2,
		//	.pStages			 = shaderStages,
		//	.pVertexInputState	 = &vertexInputInfo,
		//	.pInputAssemblyState = &inputAssembly,
		//	.pViewportState		 = &viewportState,
		//	.pRasterizationState = &rasterizer,
		//	.pMultisampleState	 = &multisampling,
		//	.pColorBlendState	 = &colorBlending,
		//	.pDynamicState		 = &dynamicState,
		//	.layout				 = pipelineLayout,
		//	.renderPass			 = nullptr};

		//pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		//pipelineInfo.basePipelineIndex	= -1;

		// 파이프라인 렌더링 정보 생성 (동적)
		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
			{.stageCount		  = 2,
			 .pStages			  = shaderStages,
			 .pVertexInputState	  = &vertexInputInfo,
			 .pInputAssemblyState = &inputAssembly,
			 .pViewportState	  = &viewportState,
			 .pRasterizationState = &rasterizer,
			 .pMultisampleState	  = &multisampling,
			 .pColorBlendState	  = &colorBlending,
			 .pDynamicState		  = &dynamicState,
			 .layout			  = pipelineLayout,
			 .renderPass		  = nullptr},
			{.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}};

		graphicsPipeline =
			vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

	static std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		std::vector<char> buffer(file.tellg());

		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

		file.close();

		return buffer;
	}

	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const
	{
		vk::ShaderModuleCreateInfo createInfo{
			.codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data())};

		vk::raii::ShaderModule shaderModule{device, createInfo};

		return shaderModule;
	}

private:
	GLFWwindow* window;

	// RAII Context 저장변수
	vk::raii::Context context;

	// Instance Handle
	vk::raii::Instance instance = nullptr;

	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

	vk::raii::SurfaceKHR surface = nullptr;

	// GPU 장치
	vk::raii::PhysicalDevice physicalDevice = nullptr;

	std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};

	vk::raii::Device device		  = nullptr;
	vk::raii::Queue graphicsQueue = nullptr;

	vk::raii::SwapchainKHR swapChain = nullptr;
	std::vector<vk::Image> swapChainImages;
	vk::SurfaceFormatKHR swapChainSurfaceFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline		= nullptr;
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