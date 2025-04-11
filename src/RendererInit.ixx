module;
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <cassert>
#include <print>
#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <deque>
#include <functional>
#include <vma/vk_mem_alloc.h>

export module renderer_init;
export using u8  = uint8_t;
export using u16 = uint16_t;
export using u32 = uint32_t;
export using u64 = uint64_t;

export using i8  = int8_t;
export using i16 = int16_t;
export using i32 = int32_t;
export using i64 = int64_t;

export using f32 = float;
export using f64 = double;

export namespace fpt
{

constexpr void VkCheck(VkResult result)
{
  if (result != VK_SUCCESS) {
    throw std::runtime_error(std::format("Detected Vulkan error: {}", static_cast<i64>(result)));
  }
}

template<typename T>
constexpr void VkNullCheck(T *handle)
{
  if (handle == VK_NULL_HANDLE) {
    throw std::runtime_error("Null handle detected");
  }
}

struct DeletionQueue
{
  std::deque<std::function<void()>> deleters;

  void PushFunction(std::function<void()> &&function)
  {
    deleters.push_front(function);
  }

  void Flush()
  {
    for (auto &deleter : deleters) {
      deleter();
    }
    deleters.clear();
  }
};

struct AllocatedImage
{
  VkImage       image;
  VkImageView   imageView;
  VmaAllocation allocation;
  VkExtent3D    imageExtent;
  VkFormat      imageFormat;
};

struct VulkanApi
{
  vk::Extent2D               windowExtent{};
  vk::Instance               instance{};
  vk::DebugUtilsMessengerEXT debugMessenger{};
  vk::PhysicalDevice         chosenGPU{};
  vk::Device                 device{};
  vk::SurfaceKHR             surface{};

  vk::SwapchainKHR swapchain{};
  vk::Format       swapchainImageFormat{};

  std::vector<vk::Image>     swapchainImages{};
  std::vector<vk::ImageView> swapchainImageViews{};
  vk::Extent2D               swapchainExtent{};

  vk::Queue graphicsQueue{};
  u32       graphicsQueueFamily{};

  VmaAllocator allocator{};

  AllocatedImage drawImage{};
  vk::Extent2D   drawExtent{};

  using VkDeleteFunc = std::function<void()>;
  // std::deque<VkDeleteFunc> deleteList;
  DeletionQueue deleteQueue;
};
void InitVulkan(
  SDL_Window                          *sdlWindow,
  VulkanApi                           &vk,
  PFN_vkDebugUtilsMessengerCallbackEXT debugCallback,
  void                                *userData)
{
  vkb::InstanceBuilder builder;

  if (debugCallback != nullptr) {
    builder.set_debug_callback(debugCallback);
    builder.set_debug_callback_user_data_pointer(userData);
  } else {
    builder.use_default_debug_messenger();
  }

  auto result = builder.set_app_name("Example vulkan_application")
                  .request_validation_layers(true)
                  .require_api_version(1, 3, 0)
                  .build();
  vkb::Instance vkbInstance = result.value();
  vk.instance               = vkbInstance.instance;
  vk.debugMessenger         = vkbInstance.debug_messenger;

  if (!SDL_Vulkan_CreateSurface(
        sdlWindow,
        vk.instance,
        reinterpret_cast<VkSurfaceKHR *>(&vk.surface))) {
    std::println("couldn't create surface");
  }

  VkPhysicalDeviceVulkan13Features features{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  VkPhysicalDeviceVulkan12Features features12{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing  = true;

  VkPhysicalDeviceVulkan11Features features11{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
  features11.variablePointers              = true;
  features11.variablePointersStorageBuffer = true;

  vkb::PhysicalDeviceSelector selector{vkbInstance};

  vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                         .set_required_features_13(features)
                                         .set_required_features_12(features12)
                                         .set_required_features_11(features11)
                                         .set_surface(vk.surface)
                                         .select()
                                         .value();

  vkb::DeviceBuilder deviceBuilder{physicalDevice};
  vkb::Device        vkbDevice = deviceBuilder.build().value();

  vk::Device d{vk.device};

  vk.device    = vkbDevice.device;
  vk.chosenGPU = physicalDevice.physical_device;

  vk.graphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  vk.graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice         = vk.chosenGPU;
  allocatorInfo.device                 = vk.device;
  allocatorInfo.instance               = vk.instance;
  allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocatorInfo, &vk.allocator);

  // vk.deleteList.push_front([&]() { vmaDestroyAllocator(vk.allocator); });
  vk.deleteQueue.PushFunction([=]() { vmaDestroyAllocator(vk.allocator); });
}

void DestroySwapchain(const VulkanApi &vk)
{
  vkDestroySwapchainKHR(vk.device, vk.swapchain, nullptr);

  for (const auto &view : vk.swapchainImageViews) {
    vkDestroyImageView(vk.device, view, nullptr);
  }
}

struct Window
{
  SDL_Window *sdlWindow;
  u32         width;
  u32         height;
};

void InitSDL()
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(std::format("Couldn't init video: {}", SDL_GetError()));
  }
}

Window OpenWindow(const u32 width, const u32 height, const char *title)
{
  const SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;

  Window window{
    .sdlWindow = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      static_cast<i32>(width),
      static_cast<i32>(height),
      windowFlags),
    .width  = width,
    .height = height,
  };
  if (window.sdlWindow == nullptr) {
    throw std::runtime_error(std::format("Couldn't open window: {}", SDL_GetError()));
  }
  return window;
}

VulkanApi InitVulkanApi(
  const Window                        &window,
  PFN_vkDebugUtilsMessengerCallbackEXT debugCallback = nullptr,
  void                                *userData      = nullptr)
{
  VulkanApi vk{};
  vk.windowExtent.setWidth(window.width);
  vk.windowExtent.setHeight(window.height);

  InitVulkan(window.sdlWindow, vk, debugCallback, userData);

  const VkExtent3D drawImageExtent = {
    vk.windowExtent.width,
    vk.windowExtent.height,
    1,
  };

  vk.drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  vk.drawImage.imageExtent = drawImageExtent;

  constexpr vk::ImageUsageFlags drawImageUsages =
    vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
    | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment;

  VkImageCreateInfo rimgInfo = {};

  rimgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  rimgInfo.pNext = nullptr;

  rimgInfo.imageType = VK_IMAGE_TYPE_2D;

  rimgInfo.format = vk.drawImage.imageFormat;
  rimgInfo.extent = drawImageExtent;

  rimgInfo.mipLevels   = 1;
  rimgInfo.arrayLayers = 1;

  // for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
  rimgInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  // optimal tiling, which means the image is stored on the best gpu format
  rimgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  rimgInfo.usage  = static_cast<VkImageUsageFlags>(drawImageUsages);


  // Allocate the image from gpu local memory
  VmaAllocationCreateInfo rimgAllocInfo = {};
  rimgAllocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
  rimgAllocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  vmaCreateImage(
    vk.allocator,
    &rimgInfo,
    &rimgAllocInfo,
    &vk.drawImage.image,
    &vk.drawImage.allocation,
    nullptr);

  VkImageViewCreateInfo rViewInfo = {};

  rViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  rViewInfo.pNext = nullptr;

  rViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  rViewInfo.image                           = vk.drawImage.image;
  rViewInfo.format                          = vk.drawImage.imageFormat;
  rViewInfo.subresourceRange.baseMipLevel   = 0;
  rViewInfo.subresourceRange.levelCount     = 1;
  rViewInfo.subresourceRange.baseArrayLayer = 0;
  rViewInfo.subresourceRange.layerCount     = 1;
  rViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

  VkCheck(vkCreateImageView(vk.device, &rViewInfo, nullptr, &vk.drawImage.imageView));

  vk.deleteQueue.PushFunction([=]() {
    vkDestroyImageView(vk.device, vk.drawImage.imageView, nullptr);
    vmaDestroyImage(vk.allocator, vk.drawImage.image, vk.drawImage.allocation);
  });

  return vk;
}

void CreateSwapchain(VulkanApi &vk, u32 width, u32 height)
{
  vkb::SwapchainBuilder swapchainBuilder{vk.chosenGPU, vk.device, vk.surface};
  vk.swapchainImageFormat = vk::Format::eB8G8R8A8Unorm;
  vk::SurfaceFormatKHR surfaceFormat{vk.swapchainImageFormat, vk::ColorSpaceKHR::eSrgbNonlinear};

  vkb::Swapchain vkbSwapchain = swapchainBuilder
                                  .set_desired_format(
                                    surfaceFormat)
                                  .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                  .set_desired_extent(width, height)
                                  .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                  .build()
                                  .value();
  vk.swapchainExtent = vkbSwapchain.extent;
  vk.swapchain       = vkbSwapchain.swapchain;
  for (auto &image : vkbSwapchain.get_images().value()) {
    vk.swapchainImages.emplace_back(image);
  }
  for (auto &imageView: vkbSwapchain.get_image_views().value()) {
   vk.swapchainImageViews.emplace_back(imageView);
  }
}

void Destroy(VulkanApi &vk)
{
  vkDeviceWaitIdle(vk.device);
  vk.deleteQueue.Flush();

  DestroySwapchain(vk);
  vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);
  vkDestroyDevice(vk.device, nullptr);
  vkb::destroy_debug_utils_messenger(vk.instance, vk.debugMessenger);
  vkDestroyInstance(vk.instance, nullptr);
}

void DestroyWindow(Window &window)
{
  SDL_DestroyWindow(window.sdlWindow);
  window.sdlWindow = nullptr;
}

void ShutdownSDL()
{
  SDL_Quit();
}

} // namespace fpt
