cmake_minimum_required(VERSION 3.28)

project(fixed_point_trickery LANGUAGES CXX)

message("prefix ${CMAKE_PREFIX_PATH}")
include_directories(${CMAKE_SOURCE_DIR}/src)

find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

find_program(SLANGC slangc HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(VERTEX_SOURCES
  ${PROJECT_SOURCE_DIR}/shaders/colored_triangle.slang
  ${PROJECT_SOURCE_DIR}/shaders/colored_normals.slang
)

set(PIXEL_SOURCES
  ${PROJECT_SOURCE_DIR}/shaders/colored_triangle.slang
  ${PROJECT_SOURCE_DIR}/shaders/colored_normals.slang
)

#TODO: get shaders compiling again
#foreach (SHADER ${VERTEX_SOURCES})
#  message(STATUS "BUILDING SHADER")
#  get_filename_component(FILE_NAME ${SHADER} NAME)
#  set(DXBC "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.vert.dxbc")
#  set(ARGS ${SHADER} -profile sm_5_0 -target dxbc -entry "VSMain" -o ${DXBC})
#  message(STATUS ${SHADER})
#  message(STATUS COMMAND "${SLANGC} ${ARGS}")
#  add_custom_command(
#    OUTPUT ${DXBC}
#    COMMAND ${SLANGC} ${ARGS}
#    DEPENDS ${SHADER} ${PROJECT_SOURCE_DIR}/shaders)
#
#  list(APPEND DXBC_BINARY_FILES ${DXBC})
#endforeach (SHADER)
#
#foreach (SHADER ${PIXEL_SOURCES})
#  message(STATUS "BUILDING SHADER")
#  get_filename_component(FILE_NAME ${SHADER} NAME)
#  set(DXBC "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.pix.dxbc")
#  set(ARGS ${SHADER} -profile sm_5_0 -target dxbc -entry "PSMain" -o ${DXBC})
#  message(STATUS ${SHADER})
#  message(STATUS COMMAND "${SLANGC} ${ARGS}")
#  add_custom_command(
#    OUTPUT ${DXBC}
#    COMMAND ${SLANGC} ${ARGS}
#    DEPENDS ${SHADER} ${PROJECT_SOURCE_DIR}/shaders)
#  list(APPEND DXBC_BINARY_FILES ${DXBC})
#endforeach (SHADER)
#
#
#add_custom_target(
#  Shaders ALL
#  DEPENDS ${DXBC_BINARY_FILES}
#)


add_executable(fixed_point_trickery
  src/main.cpp
)

include(cmake/CompilerWarnings.cmake)
myproject_set_project_warnings(
    fixed_point_trickery 
    "TRUE"
    ""
    ""
    ""
    "")

target_compile_features(fixed_point_trickery PRIVATE cxx_std_23)
target_link_libraries(fixed_point_trickery PRIVATE
  SDL2::SDL2
  SDL2::SDL2main
  imgui::imgui
  assimp::assimp
  Microsoft::DirectXTK
  gdi32.lib
  user32.lib
  kernel32.lib
  msvcrtd.lib
  winmm.lib
  imm32.lib
  version.lib
  libcmt.lib
  dxgi.lib
  dxguid.lib
  winmm.lib
  gdi32
  Setupapi.lib
  d3d11.lib
  d3dcompiler.lib
)