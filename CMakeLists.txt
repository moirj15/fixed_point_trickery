cmake_minimum_required(VERSION 3.30)

project(fixed_point_trickery LANGUAGES CXX)

message("prefix ${CMAKE_PREFIX_PATH}")
include_directories(${CMAKE_SOURCE_DIR}/src)

find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

find_program(SLANGC slangc HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(SHADER_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/shaders/colored_triangle.slang
)

foreach (SHADER ${SHADER_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${SHADER} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
  message(STATUS ${SHADER})
  message(STATUS COMMAND ${SLANGC} ${SHADER} -target spirv -fvk-use-entrypoint-name -emit-spirv-directly -o ${SPIRV})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${SLANGC} ${SHADER} -target spirv -o ${SPIRV}
    DEPENDS ${SHADER})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (SHADER)

add_custom_target(
  Shaders ALL
  DEPENDS ${SPIRV_BINARY_FILES}
)

add_executable(fixed_point_trickery
  src/main.cpp
)

target_sources(fixed_point_trickery PUBLIC FILE_SET CXX_MODULES FILES
  src/RendererInit.ixx
)

target_compile_features(fixed_point_trickery PRIVATE cxx_std_23)
target_link_libraries(fixed_point_trickery PRIVATE
  SDL2::SDL2
  SDL2::SDL2main
  imgui::imgui
  assimp::assimp
  vk-bootstrap::vk-bootstrap
  vk-bootstrap::vk-bootstrap-compiler-warnings
  Vulkan::Headers
  GPUOpen::VulkanMemoryAllocator

)