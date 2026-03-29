# CMakeList.txt: Vulkan에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
cmake_minimum_required (VERSION 3.28)

# 지원되는 경우 MSVC 컴파일러에 대해 핫 다시 로드 사용하도록 설정합니다.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Vulkan")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")
option(ENABLE_CPP20_MODULE "Enable Vulkan C++20 Module" OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Vulkan C++20 모듈 설정
if (ENABLE_CPP20_MODULE)
  add_library(VulkanCppModule)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)

  target_compile_definitions(VulkanCppModule PUBLIC
      VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
      VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
  )

  target_include_directories(VulkanCppModule PRIVATE "${Vulkan_INCLUDE_DIR}")
  target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)
  set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

  target_sources(VulkanCppModule
      PUBLIC
      FILE_SET cxx_modules TYPE CXX_MODULES
      BASE_DIRS "${Vulkan_INCLUDE_DIR}"
      FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
  )
endif()

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (Vulkan "Vulkan.cpp" "Vulkan.h")

target_include_directories(Vulkan PRIVATE ${Stb_INCLUDE_DIR})

if (ENABLE_CPP20_MODULE)
  target_compile_definitions(Vulkan PRIVATE USE_CPP20_MODULES)
  target_link_libraries(Vulkan PRIVATE
      Vulkan::cppm
      glfw
      glm::glm
      tinyobjloader::tinyobjloader
  )
else()
  target_compile_definitions(Vulkan PRIVATE
      VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
  )
  target_link_libraries(Vulkan PRIVATE
      Vulkan::Vulkan
      glfw
      glm::glm
      tinyobjloader::tinyobjloader
  )
endif()

set_property(TARGET Vulkan PROPERTY CXX_STANDARD 20)

# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.
